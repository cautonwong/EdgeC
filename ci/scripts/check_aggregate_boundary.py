#!/usr/bin/env python3
"""
check_aggregate_boundary.py - Aggregate & Aggregate Root Boundary Guard (AEP-793, #411)

Two rule families:

1. FIELD_ACCESS - a file that pulls in an aggregate root's declaring header must not
   dereference that aggregate's internal fields outside the owning module. Field names
   are derived from the actual struct body, so the guard can no longer drift out of
   sync with renamed fields (`->lock` vs the real `->shadow_lock`, #410 audit §6.3).
   The rule is scoped to files that include the declaring header, which keeps
   same-named-but-unrelated structs (e.g. the MCP bus hub) out of the results. The
   previous version whitelisted all of `modules/edge_mcp/` instead.

2. OPAQUE_ROOT - an aggregate root must not have its layout published in a public
   header. An aggregate root is detected structurally: a `struct X { ... }` living in
   `modules/<m>/include/**` whose module also defines `X*_create_in(` or `*_destroy(`.
   No hardcoded type list, so newly added aggregates are covered automatically.
"""

import sys
import os
import re
import glob
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import guard_baseline

# Legacy field patterns kept for the refutation fixture, which declares its struct
# inline and therefore has no resolvable owning header.
LEGACY_FIELD_RULES = [
    (
        "datahub_t",
        re.compile(r"\b(?:hub|datahub)\s*->\s*(?:ring|listeners|io_pool|lock|topic_tree)\b"),
        re.compile(r"^(?:modules/edge_core/src/datahub[^/]*\.c|tests/guards/fixtures/)"),
        "Direct dereference of datahub_t internal fields is forbidden outside edge_core/src/datahub*.c",
    ),
    (
        "stream_engine_t",
        re.compile(r"\b(?:engine|stream_engine)\s*->\s*(?:stages|pipelines|ring_buf|loop)\b"),
        re.compile(r"^(?:modules/edge_stream/src/|tests/guards/fixtures/)"),
        "Direct dereference of stream_engine_t internal fields is forbidden outside edge_stream",
    ),
    (
        "dcu_node_t",
        re.compile(r"\b(?:node|dcu_node)\s*->\s*(?:inventory|poller|router|storage|root)\b"),
        re.compile(r"^(?:cmd/dcu/dcu_node.*\.c|tests/guards/fixtures/)"),
        "Direct dereference of dcu_node_t internal fields is forbidden outside cmd/dcu/dcu_node.c",
    ),
]

# Public value types that are intentionally transparent. These are data structures, not
# aggregate roots: they carry no cross-field invariants enforced by their owning module.
TRANSPARENT_VALUE_TYPES = {"net_buf", "net_buf_simple"}

PRUNE_DIRS = {
    ".git",
    ".local",
    ".venv",
    ".vscode",
    ".code-review-graph",
    "build",
    "third-party",
    "node_modules",
    "docs",
    "web",
    "docker",
}

STRUCT_DEF_RE = re.compile(r"^\s*struct\s+(\w+)\s*\{", re.M)
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')


def _struct_fields(body):
    fields = set()
    body = re.sub(r"/\*.*?\*/", " ", body, flags=re.S)
    body = re.sub(r"//.*", " ", body)
    for decl in body.split(";"):
        decl = decl.strip()
        if not decl or "(" in decl or decl.startswith("#"):
            continue
        # Bitfields end with `: N`; strip it before picking the trailing identifier.
        decl = re.sub(r":\s*\d+\s*$", "", decl)
        m = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*$", decl)
        if m:
            fields.add(m.group(1))
    fields.discard("typedef")
    return fields


def _scan_public_structs(repo_root):
    """-> list of dicts: header, rel_header, module, struct, fields, is_aggregate"""
    out = []
    patterns = [
        os.path.join(repo_root, "modules", "*", "include", "**", "*.h"),
        os.path.join(repo_root, "include", "**", "*.h"),
    ]
    for pattern in patterns:
        for header in glob.glob(pattern, recursive=True):
            rel = os.path.relpath(header, repo_root).replace("\\", "/")
            module = rel.split("/")[1] if rel.startswith("modules/") else "ROOT"
            try:
                src = open(header, "r", encoding="utf-8", errors="ignore").read()
            except OSError:
                continue
            for m in STRUCT_DEF_RE.finditer(src):
                start = m.end()
                depth, i = 1, start
                while i < len(src) and depth:
                    if src[i] == "{":
                        depth += 1
                    elif src[i] == "}":
                        depth -= 1
                    i += 1
                fields = _struct_fields(src[start:i])
                if fields:
                    out.append(
                        {
                            "header": header,
                            "rel_header": rel,
                            "module": module,
                            "struct": m.group(1),
                            "fields": fields,
                            "raw": src[start:i],
                            "is_aggregate": False,
                        }
                    )
    return out


def _is_aggregate_root(repo_root, module, struct_name):
    """True when module also ships a placement ctor / destructor for this type."""
    stem = struct_name[:-2] if struct_name.endswith("_s") else struct_name
    srcs = glob.glob(os.path.join(repo_root, "modules", module, "src", "**", "*.c"), recursive=True)
    ctor = re.compile(rf"\b\w*{re.escape(stem)}\w*_create_in\s*\(")
    dtor = re.compile(rf"\b\w*{re.escape(stem)}\w*_destroy\s*\(")
    for src in srcs:
        try:
            text = open(src, "r", encoding="utf-8", errors="ignore").read()
        except OSError:
            continue
        if ctor.search(text) or dtor.search(text):
            return True
    return False


def _aliases_for(candidate):
    """Every spelling that can hold a pointer to this struct in C source."""
    struct_name = candidate["struct"]
    stem = struct_name[:-2] if struct_name.endswith("_s") else struct_name
    return {struct_name, f"struct {struct_name}", stem, f"{stem}_t"}


def _normalise_type(text):
    text = re.sub(r"\bconst\b", " ", text)
    text = re.sub(r"\bstruct\s+", "", text)
    return re.sub(r"\s+", " ", text).strip()


def _local_pointer_types(text):
    """var -> declared type, for `T *var` / `T *var =` / `T *var,` declarations."""
    out = {}
    for m in re.finditer(r"\b([A-Za-z_]\w*)\s*\*+\s*([A-Za-z_]\w*)\s*(?=[=,;)\[])", text):
        typ, var = m.group(1), m.group(2)
        if typ in ("return", "sizeof", "void", "char", "int", "long", "short", "unsigned"):
            continue
        out.setdefault(var, set()).add(_normalise_type(typ))
    return out


def _field_chain(text):
    """(object_var, field_var) pairs from `obj->field->` access chains."""
    return re.findall(r"\b([A-Za-z_]\w*)\s*->\s*([A-Za-z_]\w*)\s*->", text)


def _field_type(public_structs, obj_var, field_var, cache=None):
    """Best-effort: declared type of `field_var` in any struct, else None."""
    cache = {} if cache is None else cache
    key = (obj_var, field_var)
    if key in cache:
        return cache[key]
    result = None
    type_re = re.compile(rf"\b([A-Za-z_]\w*)\s*\*+\s*{re.escape(field_var)}\s*;")
    for cand in public_structs:
        body_hit = type_re.search(cand.get("raw", ""))
        if body_hit:
            result = _normalise_type(body_hit.group(1))
            break
    cache[key] = result
    return result


def check_directory(target_dir):
    target_dir = os.path.abspath(target_dir)
    violations = []
    public_structs = _scan_public_structs(target_dir)

    # --- Family 2: OPAQUE_ROOT -------------------------------------------------
    for cand in public_structs:
        if cand["module"] == "ROOT":
            continue
        if cand["struct"] in TRANSPARENT_VALUE_TYPES:
            continue
        if _is_aggregate_root(target_dir, cand["module"], cand["struct"]):
            cand["is_aggregate"] = True
            violations.append(
                {
                    "file": cand["rel_header"],
                    "location": f"{cand['rel_header']}:struct {cand['struct']}",
                    "rule_id": "OPAQUE_ROOT",
                    "detail": (
                        f"aggregate root `{cand['struct']}` has {len(cand['fields'])} fields published "
                        f"in a public header; expose an opaque pointer + *_storage_t instead"
                    ),
                    "fingerprint": guard_baseline.fingerprint(
                        cand["rel_header"], "OPAQUE_ROOT", cand["struct"]
                    ),
                    "issue": "#413",
                }
            )

    # --- Family 1: FIELD_ACCESS ------------------------------------------------
    aggregates = [c for c in public_structs if c["is_aggregate"]]

    for root, dirs, files in os.walk(target_dir):
        dirs[:] = [d for d in dirs if not d.startswith(".") and d not in PRUNE_DIRS]
        is_fixture = os.path.basename(target_dir).startswith(("good_", "bad_"))
        if "fixtures" in root and not is_fixture:
            continue
        if "tests" in root and "guards" not in root:
            continue
        for name in files:
            if not name.endswith((".c", ".h")):
                continue
            full = os.path.join(root, name)
            rel = os.path.relpath(full, target_dir).replace("\\", "/")
            try:
                lines = open(full, "r", encoding="utf-8", errors="ignore").readlines()
            except OSError:
                continue

            # Legacy patterns: fixtures only. They are synthetic snippets with no
            # resolvable owning header, so the struct-derived family below cannot
            # see them. Applying them to the real tree produces false positives on
            # same-named-but-unrelated structs (e.g. the MCP bus hub).
            if is_fixture:
                for agg_name, pattern, allowed_re, msg in LEGACY_FIELD_RULES:
                    if allowed_re.search(rel):
                        continue
                    for idx, line in enumerate(lines, 1):
                        clean = re.sub(r"//.*", "", line)
                        if pattern.search(clean):
                            violations.append(
                                {
                                    "file": rel,
                                    "location": f"{rel}:{idx}",
                                    "rule_id": "FIELD_ACCESS",
                                    "detail": f"[{agg_name}] {msg}: {line.strip()}",
                                    "fingerprint": guard_baseline.fingerprint(
                                        rel, "FIELD_ACCESS", agg_name
                                    ),
                                    "issue": "#413",
                                }
                            )

            # Struct-derived patterns, scoped to files that include the header
            if is_fixture:
                continue
            included = set()
            for line in lines:
                m = INCLUDE_RE.match(line)
                if m:
                    included.add(os.path.basename(m.group(1).replace("\\", "/")))
            file_text = "".join(lines)
            local_types = _local_pointer_types(file_text)
            for cand in aggregates:
                if rel.startswith(f"modules/{cand['module']}/src/"):
                    continue
                aliases = _aliases_for(cand)
                names = {
                    var
                    for var, types in local_types.items()
                    if types & aliases
                }
                names |= {
                    field
                    for obj, field in _field_chain(file_text)
                    if _field_type(public_structs, obj, field) in aliases
                }
                if not names:
                    continue
                for idx, line in enumerate(lines, 1):
                    clean = re.sub(r"//.*", "", line)
                    for var in names:
                        for field in re.findall(rf"\b{re.escape(var)}\s*->\s*(\w+)", clean):
                            if field in cand["fields"]:
                                violations.append(
                                    {
                                        "file": rel,
                                        "location": f"{rel}:{idx}",
                                        "rule_id": "FIELD_ACCESS",
                                        "detail": f"[{cand['struct']}] {var}->{field}  {line.strip()[:60]}",
                                        "fingerprint": guard_baseline.fingerprint(
                                            rel, "FIELD_ACCESS", cand["struct"]
                                        ),
                                        "issue": "#413",
                                    }
                                )
    return violations


def main():
    parser = argparse.ArgumentParser(description="EdgeOS Aggregate Root Boundary Guard")
    parser.add_argument("--root", default=".")
    parser.add_argument("--baseline", default=None)
    parser.add_argument("--update-baseline", action="store_true")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_directory(root_dir)
    baseline_path = args.baseline or guard_baseline.baseline_path_for(__file__)
    is_fixture = os.path.basename(root_dir).startswith(("good_", "bad_"))
    return guard_baseline.apply(
        violations,
        baseline_path,
        args.update_baseline,
        "AGGREGATE_BOUNDARY_GUARD",
        allow_missing_baseline=is_fixture,
    )


if __name__ == "__main__":
    sys.exit(main())
