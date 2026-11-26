#!/usr/bin/env python3
"""Architecture graph guard: layer edges, component edges, cycles, parse gaps.

Two first-party graphs are built from the tree:

* CMake target graph: `add_library` / `add_executable` targets and
  `target_link_libraries` edges. Variables set with `set()` / `list(APPEND)`
  in the same directory are expanded, ALIAS targets resolve to their real
  target, and `include(...)` files are parsed recursively. Imported targets
  stay external.
* Include graph: `#include` sites folded to the component that owns the header.
  Ownership follows `check_plane_isolation.py`: `modules/<m>/include/**` is
  `<m>`, `adapters/<a>/include/**` is `<a>` (io_drivers/linux_platform alias to
  edge_io), `foundation/include/**` is FOUNDATION and root `include/**` is ROOT.
  C comments are stripped before the scan, a quoted include that no path
  resolves falls back to a unique first-party basename owner, and a macro
  include (`#include NAME`) stays a registered parse gap.

  The text scan is an approximation. With `--compile-commands <file>` (gate
  mode) every first-party translation unit is preprocessed with `-M -MG` and
  the real dependency closure decides component edges and forbidden headers, so
  shims, umbrella headers, `.inc` payloads, angle-bracket first-party headers,
  `..` paths and symlinked headers cannot hide an edge. Text-only edges (code in
  an uncompiled branch) are still kept and judged, so conditionals fail closed.
  Residuals of the text scan (C++ raw strings, macro-generated includes) are
  recorded in the evidence directory; the structural residuals are named in the
  matrix description.

The policy lives in `ci/baseline/architecture_graph.json`: allowed layer edges,
allowed cross-module edges, forbidden edges, forbidden headers, composition
roots, and the known parse gaps. Matrix entries are schema-checked: unknown
keys, missing sections, entries without a reason, and forbidden-header files
that no longer exist all fail. Allowed component edges carry `kinds` (include
and/or link), so a link permission never silently widens the include graph
beyond `check_plane_isolation`. A new edge that is not in the matrix fails the
gate with its source site. A parse gap that is not in `known_skips` fails too,
and a stale `known_skips` entry fails instead of warning.

Shared surfaces (`root/ROOT`, `foundation/FOUNDATION`) are not cycle nodes.
They are dependency surfaces by design: the public `include/` tree aggregates
module headers and also provides headers those modules include. Their mutual
edges are governed by the layer matrix, not by the component cycle check.
Cycle nodes are (layer, component) pairs, so `modules/edge_io` and
`adapters/edge_io` never merge, and a module named `ROOT` is not a shared
surface. Cycles among implementation components (modules, adapters, cmd, tests,
other) and among CMake targets are hard failures.
"""

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
from collections import defaultdict, deque

PRUNE_TOP_DIRS = {
    "build",
    "third-party",
    "node_modules",
    "docs",
    "web",
    "docker",
    "formal",
}
PRUNE_REL_PREFIX = "tests/guards/fixtures"
SHARED_SURFACE_NODES = {("root", "ROOT"), ("foundation", "FOUNDATION")}
ADAPTER_ALIAS = {"io_drivers": "edge_io", "linux_platform": "edge_io"}
LAYERS = {"foundation", "modules", "adapters", "root", "cmd", "tests", "other"}
SOURCE_LAYERS = ("foundation", "modules", "adapters", "cmd", "tests", "include")
OTHER_LAYERS = ("examples", "tools", "fuzz")
LINK_KEYWORDS = {
    "PUBLIC",
    "PRIVATE",
    "INTERFACE",
    "LINK_PUBLIC",
    "LINK_PRIVATE",
    "LINK_INTERFACE_LIBRARIES",
}
INC_QUOTED_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*"([^"\n]+)"')
INC_ANGLE_RE = re.compile(r"^[ \t]*#[ \t]*include[ \t]*<([^>\n]+)>")
INC_MACRO_RE = re.compile(r"^[ \t]*#[ \t]*include[ \t]+(.+?)[ \t]*$")
IDENT_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
VAR_RE = re.compile(r"\$\{([A-Za-z0-9_]+)\}")
BRACKET_OPEN_RE = re.compile(r"\[(=*)\[")
MAX_CYCLE_NODES = 1000
FIRST_PARTY_TU_PREFIXES = (
    "foundation/",
    "modules/",
    "adapters/",
    "cmd/",
    "tests/",
    "include/",
)
NON_TU_PREFIXES = ("tests/guards/fixtures/", "third-party/", "build/")

REQUIRED_SECTIONS = (
    "allowed_layer_edges",
    "allowed_component_edges",
    "forbidden_edges",
    "forbidden_headers",
    "known_skips",
    "composition_roots",
)
TOP_LEVEL_KEYS = {
    "version",
    "description",
    "shared_surfaces",
    "conditional_handling",
    "composition_roots",
} | set(REQUIRED_SECTIONS)
ENTRY_KEYS = {
    "allowed_layer_edges": {"from_layer", "to_layer", "reason"},
    "allowed_component_edges": {
        "from_layer",
        "to_layer",
        "from_component",
        "to_component",
        "kinds",
        "reason",
        "sources",
        "origin",
    },
    "forbidden_edges": {
        "from_layer",
        "to_layer",
        "from_component",
        "to_component",
        "reason",
    },
    "forbidden_headers": {
        "component",
        "header",
        "files",
        "file",
        "reason",
        "sources",
    },
    "composition_roots": {"path", "reason"},
    "known_skips": {"kind", "file", "detail", "reason"},
}
VALID_SOURCES = {"plane_isolation", "component_boundary", "preprocessor_closure"}
ENTRY_SIGNATURES = {
    "allowed_layer_edges": ("from_layer", "to_layer"),
    "allowed_component_edges": (
        "from_layer",
        "to_layer",
        "from_component",
        "to_component",
        "kinds",
    ),
    "forbidden_edges": ("from_layer", "to_layer", "from_component", "to_component"),
    "forbidden_headers": ("component", "files", "file", "header"),
    "composition_roots": ("path",),
    "known_skips": ("kind", "file", "detail"),
}


def matrix_fail(message):
    raise SystemExit(f"❌ [ARCHITECTURE_GRAPH] MATRIX_SCHEMA {message}")


def classify_path(rel):
    parts = rel.split("/")
    top = parts[0] if parts else ""
    if top == "foundation":
        return "foundation", "FOUNDATION"
    if top == "modules" and len(parts) > 1:
        return "modules", parts[1]
    if top == "adapters" and len(parts) > 1:
        return "adapters", ADAPTER_ALIAS.get(parts[1], parts[1])
    if top == "cmd":
        return "cmd", "cmd"
    if top == "tests":
        return "tests", "tests"
    if top == "include":
        return "root", "ROOT"
    if rel == "CMakeLists.txt":
        return "root", "ROOT"
    if top in OTHER_LAYERS:
        return "other", top
    return None, None


def prune_dir(rel_dir, name):
    if name.startswith("."):
        return True
    if rel_dir == "." and name in PRUNE_TOP_DIRS:
        return True
    child = name if rel_dir == "." else f"{rel_dir}/{name}"
    return child == PRUNE_REL_PREFIX or child.startswith(PRUNE_REL_PREFIX + "/")


def walk_files(root, bases, suffixes, cmake=False):
    for base in bases:
        start = os.path.join(root, base)
        if not os.path.isdir(start):
            continue
        for dirpath, dirnames, filenames in os.walk(start):
            rel_dir = os.path.relpath(dirpath, root).replace("\\", "/")
            dirnames[:] = [d for d in dirnames if not prune_dir(rel_dir, d)]
            for name in filenames:
                if cmake:
                    if name != "CMakeLists.txt":
                        continue
                elif not name.endswith(suffixes):
                    continue
                yield os.path.join(dirpath, name)


def read_text(path):
    with open(path, encoding="utf-8", errors="ignore") as handle:
        return handle.read()


def bracket_span(text, start):
    """(content_start, content_end, span_end) for a CMake bracket argument."""
    match = BRACKET_OPEN_RE.match(text, start)
    if not match:
        return None
    close = "]" + match.group(1) + "]"
    end = text.find(close, match.end())
    if end < 0:
        return match.end(), len(text), len(text)
    return match.end(), end, end + len(close)


def normalize_c_source(text):
    """C preprocessing phase 2: drop the BOM, splice backslash-newline, expand %:."""
    if text.startswith("﻿"):
        text = text[1:]
    text = re.sub(r"\\\r?\n", "", text)
    return re.sub(r"(?m)^([ \t]*)%:([ \t]*)include\b", r"\1#\2include", text)


def strip_c_comments(text):
    """Replace C comments with spaces, keeping line numbers stable."""
    out = []
    i = 0
    size = len(text)
    quote = None
    while i < size:
        char = text[i]
        if quote:
            out.append(char)
            if char == "\\" and i + 1 < size:
                out.append(text[i + 1])
                i += 2
                continue
            if char == quote:
                quote = None
            i += 1
            continue
        if char in "\"'":
            quote = char
            out.append(char)
            i += 1
            continue
        if text.startswith("//", i):
            while i < size and text[i] != "\n":
                out.append(" ")
                i += 1
            continue
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = size if end < 0 else end + 2
            out.extend("\n" if ch == "\n" else " " for ch in text[i:end])
            i = end
            continue
        out.append(char)
        i += 1
    return "".join(out)


def strip_cmake_comments(text):
    """Replace CMake comments with spaces, keeping line numbers stable.

    Handles both `# ...` line comments and `#[[ ... ]]` bracket comments, and
    leaves `[[ ... ]]` bracket arguments untouched so a `#` inside one is not
    mistaken for a comment.
    """
    out = []
    i = 0
    size = len(text)
    quote = None
    while i < size:
        char = text[i]
        if quote:
            out.append(char)
            if char == "\\" and i + 1 < size:
                out.append(text[i + 1])
                i += 2
                continue
            if char == quote:
                quote = None
            i += 1
            continue
        if char in "\"'":
            quote = char
            out.append(char)
            i += 1
            continue
        if char == "\\" and i + 1 < size:
            out.append(char)
            out.append(text[i + 1])
            i += 2
            continue
        if char == "[":
            span = bracket_span(text, i)
            if span:
                out.append(text[i:span[2]])
                i = span[2]
                continue
        if char == "#":
            if i + 1 < size and text[i + 1] == "[":
                span = bracket_span(text, i + 1)
                if span:
                    end = span[2]
                    out.extend("\n" if ch == "\n" else " " for ch in text[i:end])
                    i = end
                    continue
            while i < size and text[i] != "\n":
                out.append(" ")
                i += 1
            continue
        out.append(char)
        i += 1
    return "".join(out)


def cmake_commands(text):
    """Yield (lowercase_name, body, line) with balanced parentheses."""
    text = strip_cmake_comments(text)
    i = 0
    size = len(text)
    while i < size:
        match = IDENT_RE.search(text, i)
        if not match:
            return
        name = match.group(0)
        j = match.end()
        while j < size and text[j] in " \t\r\n":
            j += 1
        if j >= size or text[j] != "(":
            i = match.end()
            continue
        depth = 1
        k = j + 1
        quote = None
        while k < size and depth:
            char = text[k]
            if quote:
                if char == "\\" and k + 1 < size:
                    k += 2
                    continue
                if char == quote:
                    quote = None
                k += 1
                continue
            if char in "\"'":
                quote = char
                k += 1
                continue
            if char == "\\" and k + 1 < size:
                k += 2
                continue
            if char == "[":
                span = bracket_span(text, k)
                if span:
                    k = span[2]
                    continue
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
            k += 1
        yield name.lower(), text[j + 1 : k - 1], text.count("\n", 0, match.start()) + 1
        i = k


def cmake_tokens(body):
    tokens = []
    i = 0
    size = len(body)
    while i < size:
        while i < size and body[i].isspace():
            i += 1
        if i >= size:
            break
        if body[i] == "[":
            span = bracket_span(body, i)
            if span:
                tokens.append(body[span[0] : span[1]])
                i = span[2]
                continue
        token = []
        quote = None
        depth = 0
        while i < size:
            char = body[i]
            if quote:
                if char == "\\" and i + 1 < size:
                    if body[i + 1] != "\n":
                        token.append(body[i + 1])
                    i += 2
                    continue
                if char == quote:
                    quote = None
                else:
                    token.append(char)
                i += 1
                continue
            if char in "\"'":
                quote = char
                i += 1
                continue
            if char == "[":
                span = bracket_span(body, i)
                if span:
                    token.append(body[span[0] : span[1]])
                    i = span[2]
                    continue
            if char == "\\" and i + 1 < size:
                if body[i + 1] != "\n":
                    token.append(body[i + 1])
                i += 2
                continue
            if char.isspace() and depth == 0:
                break
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
            token.append(char)
            i += 1
        tokens.append("".join(token))
    return tokens


def expand_token(token, variables):
    if "$<" in token:
        return None
    missing = []

    def replace(match):
        name = match.group(1)
        if name not in variables:
            missing.append(name)
            return ""
        return ";".join(variables[name])

    expanded = VAR_RE.sub(replace, token)
    if missing or "$" in expanded:
        return None
    return [part for part in expanded.split(";") if part]


def expanded_values(token, variables):
    parts = expand_token(token, variables)
    return parts if parts is not None else [token]


def add_skip(skips, kind, file, detail, line):
    key = (kind, file, detail)
    entry = skips.get(key)
    if entry is None:
        skips[key] = {"kind": kind, "file": file, "detail": detail, "line": line, "sites": 1}
    else:
        entry["sites"] += 1


CMAKE_BUILTIN_MODULES = None


def cmake_builtin_modules():
    global CMAKE_BUILTIN_MODULES
    if CMAKE_BUILTIN_MODULES is None:
        try:
            proc = subprocess.run(
                ["cmake", "--help-module-list"],
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                text=True,
            )
            CMAKE_BUILTIN_MODULES = (
                set(proc.stdout.split()) if proc.returncode == 0 else set()
            )
        except OSError:
            CMAKE_BUILTIN_MODULES = set()
    return CMAKE_BUILTIN_MODULES


def path_variables(root, source_dir):
    """CMake path variables the parser can resolve without a configure run."""
    base = root if source_dir in ("", ".") else os.path.join(root, source_dir)
    return {
        "CMAKE_SOURCE_DIR": [root],
        "PROJECT_SOURCE_DIR": [root],
        "CMAKE_CURRENT_SOURCE_DIR": [base],
    }


def resolve_cmake_include(argument, source_dir, root, module_path):
    """(rel, status) honoring CMAKE_CURRENT_SOURCE_DIR and CMAKE_MODULE_PATH."""
    if os.path.isabs(argument):
        candidates = [argument]
    elif "/" in argument or argument.endswith(".cmake"):
        candidates = [os.path.join(root, source_dir, argument)]
    else:
        candidates = []
        for entry in module_path:
            base = entry if os.path.isabs(entry) else os.path.join(root, entry)
            candidates.append(os.path.join(base, argument + ".cmake"))
        for candidate in candidates:
            if os.path.isfile(candidate):
                return os.path.relpath(candidate, root).replace("\\", "/"), "found"
        if argument in cmake_builtin_modules():
            return None, "builtin"
        return None, "missing"
    for candidate in candidates:
        if os.path.isfile(candidate):
            return os.path.relpath(candidate, root).replace("\\", "/"), "found"
    return None, "missing"


def track_module_path(name, args, rel, line, root, source_dir, module_path, skips):
    variables = path_variables(root, source_dir)
    if name == "set" and args and args[0] == "CMAKE_MODULE_PATH":
        values = []
        for argument in args[1:]:
            if argument in ("CACHE", "PARENT_SCOPE"):
                break
            expanded = expand_token(argument, variables)
            if expanded is None:
                add_skip(skips, "cmake-module-path", rel, argument, line)
                continue
            values.extend(expanded)
        module_path[:] = values
    elif (
        name == "list"
        and len(args) > 1
        and args[0] == "APPEND"
        and args[1] == "CMAKE_MODULE_PATH"
    ):
        for argument in args[2:]:
            expanded = expand_token(argument, variables)
            if expanded is None:
                add_skip(skips, "cmake-module-path", rel, argument, line)
                continue
            module_path.extend(expanded)


def collect_cmake(
    path, rel, root, source_dir, commands, macros, declared, skips, stack, counters, module_path
):
    absolute = os.path.abspath(path)
    if absolute in stack:
        add_skip(skips, "cmake-include-cycle", rel, os.path.relpath(path, root), counters["line"])
        return
    stack.append(absolute)
    parsed = [
        (name, cmake_tokens(body), line)
        for name, body, line in cmake_commands(read_text(path))
    ]
    active = None
    for name, args, line in parsed:
        counters["line"] = line
        if name in ("macro", "function") and args:
            active = {
                "name": args[0],
                "line": line,
                "file": rel,
                "has_targets": False,
                "calls": 0,
            }
            continue
        if name in ("endmacro", "endfunction"):
            if active is not None:
                macros[active["name"]] = active
                active = None
            continue
        if active is not None:
            if name in ("add_library", "add_executable") and args:
                active["has_targets"] = True
                declared.add(args[0])
            continue
        track_module_path(name, args, rel, line, root, source_dir, module_path, skips)
        if name in ("add_library", "add_executable") and args:
            declared.add(args[0])
        if name == "include" and args:
            argument = args[0]
            if "$" in argument:
                add_skip(skips, "cmake-include-var", rel, argument, line)
                continue
            target, status = resolve_cmake_include(
                argument, source_dir, root, module_path
            )
            if target is None:
                if status == "builtin":
                    counters["external_cmake_modules"] += 1
                elif "/" in argument or argument.endswith(".cmake"):
                    add_skip(skips, "cmake-include", rel, argument, line)
                else:
                    add_skip(skips, "cmake-module", rel, argument, line)
                continue
            collect_cmake(
                os.path.join(root, target),
                target,
                root,
                source_dir,
                commands,
                macros,
                declared,
                skips,
                stack,
                counters,
                module_path,
            )
            continue
        commands.append((name, args, rel, line))
    stack.pop()


def register_target(name, args, env, rel, line, targets, aliases, imported, skips):
    expanded = expand_token(args[0], env)
    if expanded is None or len(expanded) != 1:
        add_skip(skips, "cmake-template-target", rel, args[0], line)
        return
    target = expanded[0]
    kinds = set(args[1:])
    if "IMPORTED" in kinds:
        imported.add(target)
        return
    if "ALIAS" in kinds:
        real = expand_token(args[2], env) if len(args) > 2 else None
        if real is None or len(real) != 1:
            add_skip(skips, "cmake-alias", rel, args[0], line)
            return
        aliases[target] = real[0]
        imported.add(target)
        return
    layer, component = classify_path(rel)
    targets.setdefault(
        target,
        {"layer": layer, "component": component, "file": rel, "line": line},
    )


def resolve_alias(name, aliases):
    seen = set()
    while name in aliases and name not in seen:
        seen.add(name)
        name = aliases[name]
    return name


def update_variables(name, args, env, file_rel, line, skips):
    """Apply set() / list() to the same-directory variable scope."""
    if not args:
        return
    if name == "unset":
        env.pop(args[0], None)
        return
    if name == "set":
        variable = args[0]
        if "PARENT_SCOPE" in args[1:]:
            add_skip(skips, "cmake-parent-scope", file_rel, variable, line)
            return
        values = []
        for argument in args[1:]:
            if argument == "CACHE":
                break
            values.extend(expanded_values(argument, env))
        env[variable] = values
        return
    if name != "list":
        return
    if len(args) < 2:
        add_skip(skips, "cmake-list", file_rel, "list", line)
        return
    subcommand = args[0]
    variable = args[1]
    if subcommand == "APPEND":
        current = list(env.get(variable, []))
        current.extend(
            value for argument in args[2:] for value in expanded_values(argument, env)
        )
        env[variable] = current
    elif subcommand == "INSERT":
        if len(args) < 4:
            add_skip(skips, "cmake-list", file_rel, "INSERT", line)
            return
        try:
            index = int(args[2])
        except ValueError:
            add_skip(skips, "cmake-list", file_rel, "INSERT index", line)
            return
        current = list(env.get(variable, []))
        if index < 0:
            index = max(0, len(current) + index + 1)
        current[index:index] = [
            value for argument in args[3:] for value in expanded_values(argument, env)
        ]
        env[variable] = current
    elif subcommand == "REMOVE_ITEM":
        removed = {
            value for argument in args[2:] for value in expanded_values(argument, env)
        }
        env[variable] = [
            value for value in env.get(variable, []) if value not in removed
        ]
    else:
        add_skip(skips, "cmake-list", file_rel, subcommand, line)


def parse_cmake(root):
    cmake_files = sorted(walk_files(root, ("",), (), cmake=True))
    commands_by_file = {}
    macros = {}
    declared = set()
    skips = {}
    counters = {"external_cmake_modules": 0, "line": 0}

    for path in cmake_files:
        rel = os.path.relpath(path, root).replace("\\", "/")
        commands = []
        collect_cmake(
            path,
            rel,
            root,
            os.path.dirname(rel) or ".",
            commands,
            macros,
            declared,
            skips,
            [],
            counters,
            [],
        )
        commands_by_file[rel] = commands

    macro_targets = {}
    for commands in commands_by_file.values():
        for name, args, _rel, _line in commands:
            macro = macros.get(name)
            if macro is None or not macro["has_targets"] or not args:
                continue
            macro_targets.setdefault(args[0], macro["name"])
            macro["calls"] += 1
    for macro in macros.values():
        if macro["has_targets"] and macro["calls"]:
            add_skip(skips, "cmake-macro", macro["file"], macro["name"], macro["line"])

    targets = {}
    aliases = {}
    imported = set()
    for commands in commands_by_file.values():
        env = {}
        for name, args, file_rel, line in commands:
            if name in ("set", "list", "unset"):
                update_variables(name, args, env, file_rel, line, skips)
            elif name in ("add_library", "add_executable") and args:
                register_target(
                    name, args, env, file_rel, line, targets, aliases, imported, skips
                )

    edges = []
    external_links = 0
    for commands in commands_by_file.values():
        env = {}
        for name, args, file_rel, line in commands:
            if name in ("set", "list", "unset"):
                update_variables(name, args, env, file_rel, line, skips)
                continue
            if name != "target_link_libraries" or not args:
                continue
            owners = expand_token(args[0], env)
            if owners is None or len(owners) != 1:
                add_skip(skips, "cmake-template-link", file_rel, args[0], line)
                continue
            target = owners[0]
            if target not in targets:
                macro = macro_targets.get(target)
                if macro:
                    add_skip(skips, "cmake-macro-target", file_rel, macro, line)
                else:
                    add_skip(skips, "cmake-unknown-target", file_rel, target, line)
                continue
            if targets[target]["layer"] is None:
                add_skip(skips, "cmake-unowned-target", file_rel, target, line)
                continue
            for token in args[1:]:
                if token in LINK_KEYWORDS:
                    continue
                values = expand_token(token, env)
                if values is None:
                    add_skip(skips, "cmake-genex", file_rel, token, line)
                    continue
                for value in values:
                    if value.startswith("-"):
                        continue
                    resolved = resolve_alias(value, aliases)
                    if resolved in targets:
                        if targets[resolved]["layer"] is None:
                            add_skip(skips, "cmake-unowned-target", file_rel, resolved, line)
                            continue
                        edges.append(
                            {
                                "src_target": target,
                                "dst_target": resolved,
                                "src": targets[target],
                                "dst": targets[resolved],
                                "site": f"{file_rel}:{line}",
                            }
                        )
                    elif resolved in imported or value in imported:
                        external_links += 1
                    elif value in macro_targets:
                        add_skip(skips, "cmake-macro-target", file_rel, macro_targets[value], line)
                    elif value in declared:
                        add_skip(skips, "cmake-declared-target", file_rel, value, line)
                    else:
                        external_links += 1
    return (
        targets,
        edges,
        skips,
        external_links,
        counters["external_cmake_modules"],
        macros,
    )


def build_include_roots(root):
    roots = [
        ("include", ("root", "ROOT")),
        ("foundation/include", ("foundation", "FOUNDATION")),
    ]
    for base, layer in (("modules", "modules"), ("adapters", "adapters")):
        directory = os.path.join(root, base)
        if not os.path.isdir(directory):
            continue
        for name in sorted(os.listdir(directory)):
            rel = f"{base}/{name}/include"
            if os.path.isdir(os.path.join(root, rel)):
                component = (
                    ADAPTER_ALIAS.get(name, name) if base == "adapters" else name
                )
                roots.append((rel, (layer, component)))
    return roots


def build_header_owners(root):
    owners = defaultdict(set)
    for path in walk_files(root, SOURCE_LAYERS + OTHER_LAYERS, (".h", ".hpp")):
        rel = os.path.relpath(path, root).replace("\\", "/")
        layer, component = classify_path(rel)
        if layer is not None:
            owners[os.path.basename(rel)].add((layer, component))
    return owners


def own_include_roots(layer, component, source_rel):
    paths = []
    if layer == "modules":
        paths.append(f"modules/{component}/include")
    elif layer == "adapters":
        parts = source_rel.split("/")
        if len(parts) > 1:
            paths.append(f"adapters/{parts[1]}/include")
        paths.append(f"modules/{component}/include")
    elif layer == "foundation":
        paths.append("foundation/include")
    elif layer == "root":
        paths.append("include")
    return [path for path in paths if os.path.isdir(os.path.join(ROOT, path))]


def owner_of_include(include, source_rel, layer, component, quote):
    include_path = include.replace("\\", "/")
    if quote == '"':
        local = os.path.normpath(
            os.path.join(ROOT, os.path.dirname(source_rel), include_path)
        )
        if os.path.isfile(local) and not os.path.relpath(local, ROOT).startswith(".."):
            rel = os.path.relpath(local, ROOT).replace("\\", "/")
            target = classify_path(rel)
            if target[0] is not None:
                return target
    for prefix in ("include/", "foundation/", "modules/", "adapters/"):
        if include_path.startswith(prefix):
            return classify_path(include_path)
    for include_root in own_include_roots(layer, component, source_rel):
        candidate = os.path.join(include_root, include_path).replace("\\", "/")
        if os.path.isfile(os.path.join(ROOT, candidate)):
            return classify_path(candidate)
    if quote != '"':
        return None
    hits = []
    for include_root, owner in INCLUDE_ROOTS:
        if os.path.isfile(os.path.join(ROOT, include_root, include_path)):
            hits.append(owner)
    distinct = sorted(set(hits))
    if len(distinct) == 1:
        return distinct[0]
    if len(distinct) > 1:
        return ("ambiguous", "/".join(owner[1] for owner in distinct))
    owners = HEADER_OWNERS.get(os.path.basename(include_path), ())
    if len(owners) == 1:
        return next(iter(owners))
    if len(owners) > 1:
        return ("ambiguous", "/".join(owner[1] for owner in sorted(owners)))
    return None


def forbidden_rule_files(rule):
    if "files" in rule:
        return rule["files"]
    if "file" in rule:
        return [rule["file"]]
    return []


def parse_includes(root, matrix):
    edges = {}
    skips = {}
    external_quoted = 0
    external_angle = 0
    files = 0
    forbidden = []

    for path in walk_files(
        root, SOURCE_LAYERS + OTHER_LAYERS, (".c", ".h", ".cpp", ".hpp")
    ):
        rel = os.path.relpath(path, root).replace("\\", "/")
        layer, component = classify_path(rel)
        if layer is None:
            continue
        files += 1
        for line_no, line in enumerate(
            strip_c_comments(normalize_c_source(read_text(path))).splitlines(), 1
        ):
            match = INC_QUOTED_RE.match(line)
            quote = '"'
            if match is None:
                match = INC_ANGLE_RE.match(line)
                quote = "<"
            if match is None:
                ident = INC_MACRO_RE.match(line)
                if ident:
                    add_skip(
                        skips,
                        "include-macro",
                        rel,
                        ident.group(1).strip(),
                        line_no,
                    )
                continue
            include = match.group(1)
            target = owner_of_include(include, rel, layer, component, quote)
            header = os.path.basename(include.replace("\\", "/"))
            for rule in matrix.get("forbidden_headers", []):
                if header != rule["header"]:
                    continue
                rule_files = forbidden_rule_files(rule)
                if rel not in rule_files and rule.get("component") != component:
                    continue
                if target and target[0] != "ambiguous" and tuple(target) == (layer, component):
                    continue
                forbidden.append(
                    {
                        "rule_id": "FORBIDDEN_HEADER",
                        "location": f"{rel}:{line_no}",
                        "detail": rule.get("reason", rule["header"]),
                    }
                )
            if target is None:
                if quote == '"':
                    external_quoted += 1
                else:
                    external_angle += 1
                continue
            if target[0] == "ambiguous":
                add_skip(
                    skips,
                    "include-ambiguous",
                    rel,
                    f"{header}: {target[1]}",
                    line_no,
                )
                continue
            dst_layer, dst_component = target
            if (layer, component) == (dst_layer, dst_component):
                continue
            key = (layer, component, dst_layer, dst_component)
            edges.setdefault(
                key,
                {
                    "src_layer": layer,
                    "src": component,
                    "dst_layer": dst_layer,
                    "dst": dst_component,
                    "sites": [],
                    "headers": [],
                },
            )
            edges[key]["sites"].append(f"{rel}:{line_no}")
            edges[key]["headers"].append(include)
    return (
        list(edges.values()),
        skips,
        forbidden,
        files,
        external_quoted,
        external_angle,
    )


def load_compile_commands(path):
    try:
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
    except (OSError, ValueError) as error:
        raise SystemExit(
            f"❌ [ARCHITECTURE_GRAPH] cannot read compile commands {path}: {error}"
        )
    if not isinstance(data, list):
        raise SystemExit(
            f"❌ [ARCHITECTURE_GRAPH] compile commands {path} is not a JSON list"
        )
    return data


def compile_entry_argv(entry):
    arguments = entry.get("arguments")
    if isinstance(arguments, list):
        return [str(argument) for argument in arguments]
    command = entry.get("command")
    if not isinstance(command, str) or not command:
        raise SystemExit(
            "❌ [ARCHITECTURE_GRAPH] compile command entry has neither command nor arguments"
        )
    return shlex.split(command)


def preprocessor_argv(entry):
    argv = compile_entry_argv(entry)
    stripped = []
    index = 0
    while index < len(argv):
        token = argv[index]
        if token == "-c":
            index += 1
            continue
        if token == "-o":
            index += 2
            continue
        if token.startswith("-o") and len(token) > 2:
            index += 1
            continue
        if token in ("-MF", "-MT", "-MQ"):
            index += 2
            continue
        if token.startswith(("-MF", "-MT", "-MQ")) and len(token) > 3:
            index += 1
            continue
        if token in ("-MD", "-MMD"):
            index += 1
            continue
        stripped.append(token)
        index += 1
    return stripped + ["-M", "-MG"]


def parse_make_dependencies(output):
    text = output.replace("\\\r\n", "").replace("\\\n", "")
    colon = text.find(":")
    if colon < 0:
        return []
    body = text[colon + 1 :]
    tokens = []
    current = []
    index = 0
    while index < len(body):
        char = body[index]
        if char == "\\" and index + 1 < len(body):
            current.append(body[index + 1])
            index += 2
            continue
        if char == "$" and index + 1 < len(body) and body[index + 1] == "$":
            current.append("$")
            index += 2
            continue
        if char.isspace():
            if current:
                tokens.append("".join(current))
                current = []
            index += 1
            continue
        current.append(char)
        index += 1
    if current:
        tokens.append("".join(current))
    return tokens


def dependency_paths(tokens, directory, root):
    paths = []
    for token in tokens:
        absolute = token if os.path.isabs(token) else os.path.join(directory, token)
        if os.path.exists(absolute):
            absolute = os.path.realpath(absolute)
        else:
            absolute = os.path.normpath(absolute)
        rel = os.path.relpath(absolute, root).replace("\\", "/")
        if rel == ".." or rel.startswith("../"):
            continue
        paths.append(rel)
    return paths


def tu_rel_path(entry, directory, root):
    path = entry.get("file")
    if not isinstance(path, str) or not path:
        return None
    absolute = path if os.path.isabs(path) else os.path.join(directory, path)
    if os.path.exists(absolute):
        absolute = os.path.realpath(absolute)
    rel = os.path.relpath(os.path.normpath(absolute), root).replace("\\", "/")
    if rel.startswith("../"):
        return None
    return rel


def first_party_tu(rel):
    return rel.startswith(FIRST_PARTY_TU_PREFIXES) and not rel.startswith(
        NON_TU_PREFIXES
    )


def analyze_compile_commands(path, root, matrix):
    """Edges and forbidden headers from the real preprocessor closure."""
    commands_dir = os.path.dirname(os.path.abspath(path))
    entries = load_compile_commands(path)
    edges = {}
    forbidden = []
    forbidden_seen = set()
    skips = {}
    tu_count = 0
    external_deps = 0
    for entry in entries:
        directory = entry.get("directory") or commands_dir
        if not os.path.isabs(directory):
            # Relative directories resolve against the database file; CMake writes absolute ones.
            directory = os.path.abspath(os.path.join(commands_dir, directory))
        rel = tu_rel_path(entry, directory, root)
        if rel is None or not first_party_tu(rel):
            continue
        layer, component = classify_path(rel)
        if layer is None:
            add_skip(skips, "include-unowned", rel, rel, 0)
            continue
        tu_count += 1
        proc = subprocess.run(
            preprocessor_argv(entry),
            cwd=directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        if proc.returncode != 0:
            tail = "\n".join(proc.stderr.strip().splitlines()[-10:])
            raise SystemExit(
                f"❌ [ARCHITECTURE_GRAPH] preprocess failed for {rel} "
                f"(rc={proc.returncode}):\n{tail}"
            )
        deps = dependency_paths(parse_make_dependencies(proc.stdout), directory, root)
        if rel not in deps:
            raise SystemExit(
                f"❌ [ARCHITECTURE_GRAPH] dependency closure for {rel} does not "
                "contain the TU source itself; check the compiler flags (a depfile "
                "or a no-op command would hide the closure)"
            )
        seen = set()
        for dep in deps:
            if dep == rel or dep in seen:
                continue
            seen.add(dep)
            target = classify_path(dep)
            if target[0] is None:
                if dep.startswith(FIRST_PARTY_TU_PREFIXES):
                    add_skip(skips, "include-unowned", rel, dep, 0)
                else:
                    external_deps += 1
                continue
            for rule in matrix.get("forbidden_headers", []):
                if os.path.basename(dep) != rule["header"]:
                    continue
                if rule.get("component") != component and rel not in forbidden_rule_files(rule):
                    continue
                if tuple(target) == (layer, component):
                    continue
                key = (rule["header"], rel, rule.get("reason", rule["header"]))
                if key in forbidden_seen:
                    continue
                forbidden_seen.add(key)
                forbidden.append(
                    {
                        "rule_id": "FORBIDDEN_HEADER",
                        "location": f"{rel} (dependency closure)",
                        "detail": rule.get("reason", rule["header"]),
                    }
                )
            if tuple(target) == (layer, component):
                continue
            key = (layer, component, target[0], target[1])
            item = edges.setdefault(
                key,
                {
                    "src_layer": layer,
                    "src": component,
                    "dst_layer": target[0],
                    "dst": target[1],
                    "sites": [],
                    "headers": [],
                },
            )
            if not item["sites"] or item["sites"][-1] != rel:
                item["sites"].append(f"{rel} (closure)")
            item["headers"].append(dep)
    return edges, forbidden, skips, tu_count, external_deps


def merge_include_edges(text_edges, strong_edges):
    merged = {}
    for edge in text_edges:
        key = (edge["src_layer"], edge["src"], edge["dst_layer"], edge["dst"])
        merged[key] = {
            **edge,
            "sites": list(edge["sites"]),
            "headers": list(edge["headers"]),
            "text": True,
            "strong": False,
        }
    for key, edge in strong_edges.items():
        if key in merged:
            merged[key]["strong"] = True
            merged[key]["sites"].extend(edge["sites"])
            merged[key]["headers"].extend(edge["headers"])
        else:
            merged[key] = {**edge, "text": False, "strong": True}
    text_only = sum(1 for edge in merged.values() if edge["text"] and not edge["strong"])
    strong_only = sum(1 for edge in merged.values() if edge["strong"] and not edge["text"])
    return list(merged.values()), text_only, strong_only


def entry_matches(entry, src_layer, src_comp, dst_layer, dst_comp):
    if "from_layer" in entry and entry["from_layer"] != src_layer:
        return False
    if "to_layer" in entry and entry["to_layer"] != dst_layer:
        return False
    if "from_component" in entry and entry["from_component"] != src_comp:
        return False
    if "to_component" in entry and entry["to_component"] != dst_comp:
        return False
    return True


def allowed_edge(matrix, kind, src_layer, src_comp, dst_layer, dst_comp):
    if (src_layer, src_comp) == (dst_layer, dst_comp):
        return True, ""
    for entry in matrix.get("forbidden_edges", []):
        if entry_matches(entry, src_layer, src_comp, dst_layer, dst_comp):
            return False, "forbidden: " + entry["reason"]
    for entry in matrix.get("allowed_layer_edges", []):
        if entry["from_layer"] != src_layer or entry["to_layer"] != dst_layer:
            continue
        if src_layer == "modules" and dst_layer == "modules":
            for pair in matrix.get("allowed_component_edges", []):
                if (
                    pair.get("from_layer", "modules") == "modules"
                    and pair.get("to_layer", "modules") == "modules"
                    and pair.get("from_component") == src_comp
                    and pair.get("to_component") == dst_comp
                    and kind in pair["kinds"]
                ):
                    return True, ""
            return False, "cross-module edge not declared"
        return True, ""
    for pair in matrix.get("allowed_component_edges", []):
        if kind in pair["kinds"] and entry_matches(
            pair, src_layer, src_comp, dst_layer, dst_comp
        ):
            return True, ""
    return False, "layer edge not declared"


def node_name(node):
    layer, component = node
    return f"{layer}/{component}"


def is_shared_node(layer, component):
    return (layer, component) in SHARED_SURFACE_NODES


def find_cycle(start, adjacency, nodes):
    parent = {start: None}
    queue = deque([start])
    while queue:
        node = queue.popleft()
        for nxt in sorted(adjacency.get(node, ())):
            if nxt not in nodes:
                continue
            if nxt == start:
                path = []
                current = node
                while current is not None:
                    path.append(current)
                    current = parent[current]
                path.reverse()
                return path + [start]
            if nxt not in parent:
                parent[nxt] = node
                queue.append(nxt)
    return None


def find_cycles(nodes, adjacency):
    """One full cycle path per strongly connected component that has one."""
    if len(nodes) > MAX_CYCLE_NODES:
        raise SystemExit(
            f"❌ [ARCHITECTURE_GRAPH] cycle detection is capped at "
            f"{MAX_CYCLE_NODES} nodes; graph has {len(nodes)}. Raise the cap "
            "with a measured runtime budget, do not skip the check."
        )
    index = {}
    low = {}
    on_stack = set()
    stack = []
    components = []
    counter = [0]

    def strong(node):
        index[node] = low[node] = counter[0]
        counter[0] += 1
        stack.append(node)
        on_stack.add(node)
        for nxt in sorted(adjacency.get(node, ())):
            if nxt not in index:
                strong(nxt)
                low[node] = min(low[node], low[nxt])
            elif nxt in on_stack:
                low[node] = min(low[node], index[nxt])
        if low[node] == index[node]:
            component = []
            while True:
                member = stack.pop()
                on_stack.discard(member)
                component.append(member)
                if member == node:
                    break
            components.append(component)

    for node in sorted(nodes):
        if node not in index:
            strong(node)

    cycles = []
    for component in components:
        if len(component) == 1 and component[0] not in adjacency.get(component[0], ()):
            continue
        members = set(component)
        cycle = find_cycle(component[0], adjacency, members)
        if cycle:
            cycles.append(cycle)
    return cycles


def validate_matrix(matrix, root):
    if not isinstance(matrix, dict):
        matrix_fail("matrix must be a JSON object")
    unknown = sorted(set(matrix) - TOP_LEVEL_KEYS)
    if unknown:
        matrix_fail(f"unknown top-level key(s): {', '.join(unknown)}")
    if matrix.get("version") != 1:
        matrix_fail("unsupported version (expected 1)")
    for section in REQUIRED_SECTIONS:
        if section not in matrix:
            matrix_fail(f"missing section {section}")
        if not isinstance(matrix[section], list):
            matrix_fail(f"section {section} must be a list")
    if "shared_surfaces" in matrix:
        surfaces = matrix["shared_surfaces"]
        if not isinstance(surfaces, dict) or set(surfaces) - {"reason"}:
            matrix_fail("shared_surfaces must be an object with only a reason")
        if not surfaces.get("reason"):
            matrix_fail("shared_surfaces has no reason")

    signed = set()
    for section in REQUIRED_SECTIONS:
        keys = ENTRY_SIGNATURES[section]
        for index, entry in enumerate(matrix[section]):
            if not isinstance(entry, dict):
                matrix_fail(f"{section}[{index}] must be an object")
            unknown = sorted(set(entry) - ENTRY_KEYS[section])
            if unknown:
                matrix_fail(
                    f"{section}[{index}] unknown key(s): {', '.join(unknown)}"
                )
            if not entry.get("reason"):
                matrix_fail(f"{section}[{index}] has no reason")
            signature = tuple(
                tuple(entry[key]) if isinstance(entry.get(key), list) else entry.get(key)
                for key in keys
            )
            if signature in signed:
                matrix_fail(f"{section}[{index}] duplicates an earlier entry")
            signed.add(signature)
            if "sources" in entry:
                sources = entry["sources"]
                if not isinstance(sources, list) or any(
                    not isinstance(source, str) for source in sources
                ):
                    matrix_fail(f"{section}[{index}] sources must be a list of strings")
                unknown_sources = set(sources) - VALID_SOURCES
                if unknown_sources:
                    matrix_fail(
                        f"{section}[{index}] unknown source(s): "
                        + ", ".join(sorted(unknown_sources))
                    )
                if len(set(sources)) != len(sources):
                    matrix_fail(f"{section}[{index}] duplicate sources entry")
            if section in ("allowed_layer_edges", "allowed_component_edges", "forbidden_edges"):
                for field in ("from_layer", "to_layer"):
                    if field not in entry:
                        matrix_fail(f"{section}[{index}] missing {field}")
                    if entry[field] not in LAYERS:
                        matrix_fail(
                            f"{section}[{index}] unknown layer {entry[field]!r}"
                        )
            if section == "allowed_component_edges":
                if "from_component" not in entry and "to_component" not in entry:
                    matrix_fail(
                        f"allowed_component_edges[{index}] needs from_component "
                        "and/or to_component"
                    )
                kinds = entry.get("kinds")
                if (
                    not isinstance(kinds, list)
                    or not kinds
                    or set(kinds) - {"include", "link"}
                ):
                    matrix_fail(
                        f"allowed_component_edges[{index}] needs non-empty kinds "
                        "drawn from include/link"
                    )
            if section == "forbidden_headers":
                if not entry.get("header"):
                    matrix_fail(f"forbidden_headers[{index}] missing header")
                paths = forbidden_rule_files(entry)
                if not paths and "component" not in entry:
                    matrix_fail(
                        f"forbidden_headers[{index}] needs component or files"
                    )
                for path in paths:
                    if (
                        not isinstance(path, str)
                        or os.path.isabs(path)
                        or os.path.normpath(path).replace("\\", "/") != path
                    ):
                        matrix_fail(
                            f"forbidden_headers[{index}] path is not normalized: {path!r}"
                        )
                    if not os.path.isfile(os.path.join(root, path)):
                        matrix_fail(
                            f"forbidden_headers[{index}] file does not exist: {path}"
                        )
            if section == "composition_roots":
                path = entry.get("path")
                if not isinstance(path, str) or not path:
                    matrix_fail(f"composition_roots[{index}] missing path")
                if (
                    os.path.isabs(path)
                    or os.path.normpath(path).replace("\\", "/") != path
                ):
                    matrix_fail(
                        f"composition_roots[{index}] path is not normalized: {path!r}"
                    )
                if not os.path.isfile(os.path.join(root, path)):
                    matrix_fail(
                        f"composition_roots[{index}] path does not exist: {path}"
                    )
            if section == "known_skips":
                for field in ("kind", "file", "detail"):
                    if not entry.get(field):
                        matrix_fail(f"known_skips[{index}] missing {field}")


def load_matrix(path, root):
    if not os.path.exists(path):
        raise SystemExit(f"❌ [ARCHITECTURE_GRAPH] matrix missing: {path}")
    try:
        with open(path, encoding="utf-8") as handle:
            matrix = json.load(handle)
    except (OSError, ValueError) as error:
        raise SystemExit(
            f"❌ [ARCHITECTURE_GRAPH] MATRIX_SCHEMA cannot read matrix {path}: {error}"
        )
    validate_matrix(matrix, root)
    return matrix


def skip_key(entry):
    return (entry["kind"], entry["file"], entry["detail"])


def main():
    global ROOT, INCLUDE_ROOTS, HEADER_OWNERS
    parser = argparse.ArgumentParser(description="Architecture graph guard")
    parser.add_argument("--root", default=".", help="repository root")
    parser.add_argument("--matrix", default=None, help="architecture matrix JSON")
    parser.add_argument(
        "--compile-commands",
        default=None,
        help="CMake compile_commands.json; enables the real-preprocessor gate mode",
    )
    args = parser.parse_args()

    ROOT = os.path.abspath(args.root)
    matrix_path = args.matrix or os.path.join(
        ROOT, "ci", "baseline", "architecture_graph.json"
    )
    matrix = load_matrix(matrix_path, ROOT)
    INCLUDE_ROOTS = build_include_roots(ROOT)
    HEADER_OWNERS = build_header_owners(ROOT)

    (
        targets,
        link_edges,
        cmake_skips,
        external_links,
        external_cmake_modules,
        macros,
    ) = parse_cmake(ROOT)
    include_edges, include_skips, forbidden, files, external_quoted, external_angle = (
        parse_includes(ROOT, matrix)
    )
    strong_info = None
    if args.compile_commands:
        strong_path = os.path.abspath(args.compile_commands)
        strong_edges, strong_forbidden, strong_skips, tu_count, external_deps = (
            analyze_compile_commands(strong_path, ROOT, matrix)
        )
        include_edges, text_only, strong_only = merge_include_edges(
            include_edges, strong_edges
        )
        forbidden.extend(strong_forbidden)
        include_skips.update(strong_skips)
        strong_info = (tu_count, text_only, strong_only, external_deps)
    link_edges.sort(key=lambda edge: (edge["site"], edge["src_target"], edge["dst_target"]))
    include_edges.sort(
        key=lambda edge: (edge["src_layer"], edge["src"], edge["dst_layer"], edge["dst"])
    )
    forbidden.sort(key=lambda item: item["location"])

    violations = []

    target_adj = defaultdict(set)
    for edge in link_edges:
        target_adj[edge["src_target"]].add(edge["dst_target"])
    for cycle in find_cycles(set(targets), target_adj):
        violations.append(
            {
                "rule_id": "TARGET_CYCLE",
                "location": "CMake target graph",
                "detail": " -> ".join(cycle),
            }
        )

    grouped = {}
    for edge in link_edges:
        src = edge["src"]
        dst = edge["dst"]
        ok, reason = allowed_edge(
            matrix, "link", src["layer"], src["component"], dst["layer"], dst["component"]
        )
        if ok:
            continue
        key = (
            src["layer"],
            src["component"],
            dst["layer"],
            dst["component"],
            reason,
        )
        item = grouped.setdefault(
            key,
            {
                "rule_id": "UNDECLARED_EDGE",
                "location": edge["site"],
                "detail": (
                    f"link {edge['src_target']} ({src['layer']}/{src['component']})"
                    f" -> {edge['dst_target']} ({dst['layer']}/{dst['component']})"
                    f"; {reason}; sites: "
                ),
                "sites": [],
            },
        )
        item["sites"].append(edge["site"])
    for item in grouped.values():
        item["detail"] += ", ".join(item["sites"][:8])
        if len(item["sites"]) > 8:
            item["detail"] += f", ... (+{len(item['sites']) - 8})"
        del item["sites"]
        violations.append(item)

    include_adj = defaultdict(set)
    for edge in include_edges:
        ok, reason = allowed_edge(
            matrix,
            "include",
            edge["src_layer"],
            edge["src"],
            edge["dst_layer"],
            edge["dst"],
        )
        if not ok:
            violations.append(
                {
                    "rule_id": "UNDECLARED_EDGE",
                    "location": edge["sites"][0],
                    "detail": (
                        f"include {edge['src']} ({edge['src_layer']})"
                        f" -> {edge['dst']} ({edge['dst_layer']})"
                        f" via {edge['headers'][0]}; {reason}; sites: "
                        + ", ".join(edge["sites"][:8])
                        + (
                            f", ... (+{len(edge['sites']) - 8})"
                            if len(edge["sites"]) > 8
                            else ""
                        )
                    ),
                }
            )
        if is_shared_node(edge["src_layer"], edge["src"]) or is_shared_node(
            edge["dst_layer"], edge["dst"]
        ):
            continue
        src_node = (edge["src_layer"], edge["src"])
        dst_node = (edge["dst_layer"], edge["dst"])
        if src_node == dst_node:
            continue
        include_adj[src_node].add(dst_node)
    include_nodes = set()
    for edge in include_edges:
        if not is_shared_node(edge["src_layer"], edge["src"]):
            include_nodes.add((edge["src_layer"], edge["src"]))
        if not is_shared_node(edge["dst_layer"], edge["dst"]):
            include_nodes.add((edge["dst_layer"], edge["dst"]))
    for cycle in find_cycles(include_nodes, include_adj):
        evidence = []
        for src, dst in zip(cycle, cycle[1:]):
            site = next(
                (
                    edge["sites"][0] + f" ({edge['headers'][0]})"
                    for edge in include_edges
                    if (edge["src_layer"], edge["src"]) == src
                    and (edge["dst_layer"], edge["dst"]) == dst
                ),
                "?",
            )
            evidence.append(f"{node_name(src)} -> {node_name(dst)}: {site}")
        violations.append(
            {
                "rule_id": "INCLUDE_CYCLE",
                "location": "component include graph",
                "detail": " -> ".join(node_name(node) for node in cycle)
                + " | "
                + "; ".join(evidence),
            }
        )

    violations.extend(forbidden)

    combined_skips = dict(cmake_skips)
    combined_skips.update(include_skips)
    known = {skip_key(entry): entry for entry in matrix.get("known_skips", [])}
    for key, entry in combined_skips.items():
        if key not in known:
            violations.append(
                {
                    "rule_id": "UNREGISTERED_PARSE_GAP",
                    "location": f"{entry['file']}:{entry['line']}",
                    "detail": (
                        f"{entry['kind']} {entry['detail']} at {entry['sites']} site(s); "
                        "register it in known_skips with a reason"
                    ),
                }
            )
    for key in known:
        if key not in combined_skips:
            violations.append(
                {
                    "rule_id": "STALE_KNOWN_SKIP",
                    "location": f"{key[1]}",
                    "detail": (
                        f"{key[0]} {key[2]} no longer reproduces; "
                        "remove it from known_skips"
                    ),
                }
            )

    if violations:
        print(f"❌ [ARCHITECTURE_GRAPH] {len(violations)} violation(s):")
        for item in violations:
            print(f"  - [{item['rule_id']}] {item['location']}: {item['detail']}")
        return 1

    include_edge_count = len(include_edges)
    include_pairs = {
        ((edge["src_layer"], edge["src"]), (edge["dst_layer"], edge["dst"]))
        for edge in include_edges
    }
    mutual = sorted(
        {
            tuple(sorted((src, dst)))
            for src, dst in include_pairs
            if (dst, src) in include_pairs
            and (is_shared_node(*src) or is_shared_node(*dst))
        }
    )
    folded_macro_targets = sum(
        macro["calls"] for macro in macros.values() if macro["has_targets"]
    )
    print(
        f"✅ [ARCHITECTURE_GRAPH] clean parsed subgraph: "
        f"{len(targets)} CMake target(s), {len(link_edges)} first-party link edge(s), "
        f"0 target cycle(s); "
        f"{files} include file(s), {include_edge_count} component edge(s), "
        f"0 component cycle(s); "
        f"{len(combined_skips)} known parse gap(s), {external_links} external/imported "
        f"link(s) ignored, {external_cmake_modules} external CMake module include(s), "
        f"{external_quoted} quoted and {external_angle} angle include(s) outside the "
        f"owner map; matrix {os.path.relpath(matrix_path, ROOT)}"
    )
    if strong_info is not None:
        tu_count, text_only, strong_only, external_deps = strong_info
        print(
            "ℹ️  [ARCHITECTURE_GRAPH] strong mode: "
            f"{tu_count} first-party TU(s) preprocessed, 0 failure(s); "
            f"{strong_only} preprocessor-only edge(s), {text_only} text-only edge(s) "
            f"(non-TU files and uncompiled branches, kept fail-closed); "
            f"{external_deps} dependency(ies) outside the owner map ignored."
        )
    print(
        f"ℹ️  [ARCHITECTURE_GRAPH] folded out of the parsed subgraph: "
        f"{folded_macro_targets} macro-generated target call(s) (add_edge_test etc.) "
        "and cmake/ helper macros; File API reports the full configure-time graph."
    )
    if mutual:
        print(
            "ℹ️  [ARCHITECTURE_GRAPH] shared-surface mutual include(s) outside cycle "
            "nodes by design: "
            + ", ".join(f"{node_name(a)} <-> {node_name(b)}" for a, b in mutual)
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
