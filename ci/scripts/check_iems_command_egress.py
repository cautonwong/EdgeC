#!/usr/bin/env python3
"""IEMS command egress guard: bus writes and raw IO leave through the allowed doors.

The aggregate boundary guard's WRITE_PATH rule only scans cmd/ and only the
`edge_api_write_tag(` spelling, so an adapter or a new `_ex`/raw-io caller can
reach the bus without review (P1-08a c1). This guard counts every occurrence of
an egress symbol in git-tracked .c files after backslash-newline folding and
comment/string blanking. A macro alias, a function pointer, a `#if 0` body, an
identifier split by a line continuation, a symlinked source and an inline-asm
string are all real references. Allowance is the exact file+symbol+occurrence
count table below rather than a "this file defines the symbol" skip, so a fake
definition cannot blind the rest of a file. Pure extern prototype lines are
declarations, not egress, and are exempt.

--selftest replays six round-2 bypass cases in memory plus a real symlink:
`#if 0` fake definition, macro alias/function pointer, line continuation, second
call in a whitelisted file, symlinked source, and inline-asm string. The
baseline also covers the definition file with a nonstandard return type and
extern prototypes outside the whitelist.
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from c_source_scan import strip_comments

# path -> {symbol: allowed occurrence count}. Adding a reference means editing
# this table on purpose, with a review record in the ticket.
ALLOWED_COUNTS = {
    "modules/edge_api/src/edge_api.c": {
        # Definition plus the legacy wrapper's internal delegation.
        "edge_api_write_tag_ex": 2,
        "edge_api_write_tag": 1,
        "edge_api_raw_io": 1,
    },
    "cmd/iems/iems_execution_adapter.c": {"edge_api_write_tag_ex": 1},
    "adapters/rest_api/src/rest_server.c": {
        "edge_api_write_tag": 1,
        "edge_api_raw_io": 1,
    },
    "cmd/dcu/dcu_node.c": {"edge_api_raw_io": 1},
}

SYMBOLS = ("edge_api_raw_io", "edge_api_write_tag", "edge_api_write_tag_ex")

# Tests and fixtures are validated by their own suites; generated and vendored
# trees are out of scope (same policy as check_aggregate_boundary.py).
PRUNE_PARTS = {
    ".git",
    ".local",
    ".venv",
    "build",
    "fixtures",
    "formal",
    "node_modules",
    "tests",
    "third-party",
}

STRING_RE = re.compile(r'"(?:\\[\s\S]|[^"\\])*"|\'(?:\\[\s\S]|[^\'\\])*\'')

# One declaration statement, no braces and no initializer, is a prototype: it
# binds the name but is not an egress site.
EXTERN_DECL_RE = re.compile(r"^[ \t]*extern\b[^;{}=]*;[ \t]*$")
ASM_RE = re.compile(r"\b(?:__asm__|__asm|asm)\b")


def fold_continuations(text):
    """Join backslash-newline pairs; keep a folded-line -> original-line map."""
    folded = []
    origins = []
    pending = ""
    start = 1
    for lineno, line in enumerate(text.split("\n"), 1):
        if not pending:
            start = lineno
        if line.endswith("\\"):
            pending += line[:-1]
            continue
        folded.append(pending + line)
        origins.append(start)
        pending = ""
    if pending:
        folded.append(pending)
        origins.append(start)
    return "\n".join(folded), origins


def blank_literals(text):
    """Blank string/char literals so a symbol inside one is not a reference; newlines stay."""
    return STRING_RE.sub(lambda match: re.sub(r"[^\n]", " ", match.group(0)), text)


def allowed_paths(symbol):
    return sorted(path for path, counts in ALLOWED_COUNTS.items() if symbol in counts)


def symbol_hits(folded, origins):
    """Return (symbol, original line) for every egress reference in the file."""
    stripped = strip_comments(folded)
    blanked = blank_literals(stripped)
    hits = []
    for index, line in enumerate(blanked.split("\n")):
        if EXTERN_DECL_RE.match(line):
            continue
        for symbol in SYMBOLS:
            for _match in re.finditer(rf"\b{re.escape(symbol)}\b", line):
                hits.append((symbol, origins[index]))
    # The assembler sees inline-asm string text, so a symbol there is a real
    # reference even though ordinary strings are blanked above.
    for literal in STRING_RE.finditer(stripped):
        boundary = max(stripped.rfind(char, 0, literal.start()) for char in ";{}")
        if not ASM_RE.search(stripped, boundary + 1, literal.start()):
            continue
        line = origins[stripped.count("\n", 0, literal.start())]
        for symbol in SYMBOLS:
            for _match in re.finditer(rf"\b{re.escape(symbol)}\b", literal.group(0)):
                hits.append((symbol, line))
    return hits


def evaluate_sources(sources):
    """Return (pass lines, fail lines) for [(repository-relative path, text)]."""
    passes = []
    failures = []
    for rel, text in sources:
        folded, origins = fold_continuations(text)
        counts = {}
        for symbol, line in symbol_hits(folded, origins):
            counts[symbol] = counts.get(symbol, 0) + 1
            limit = ALLOWED_COUNTS.get(rel, {}).get(symbol, 0)
            if counts[symbol] <= limit:
                passes.append((symbol, rel, line))
            elif limit:
                failures.append(
                    (symbol, rel, line, f"expected at most {limit} occurrence(s) in this file")
                )
            else:
                failures.append(
                    (symbol, rel, line,
                     f"expected one of [{', '.join(allowed_paths(symbol))}]")
                )
    return passes, failures


def render(passes, failures, symlinks):
    lines = [f"PASS {symbol} {rel}:{line}" for symbol, rel, line in sorted(passes)]
    lines += [
        f"FAIL {symbol} {rel}:{line} {reason}"
        for symbol, rel, line, reason in sorted(failures)
    ]
    lines += [
        f"FAIL SYMLINK {rel}:1 egress source must not be a symbolic link"
        for rel in sorted(symlinks)
    ]
    return lines


def tracked_c_files(root):
    proc = subprocess.run(
        ["git", "-C", root, "ls-files", "-z", "--", "*.c"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if proc.returncode != 0:
        return None, proc.stderr.strip()
    return sorted(path for path in proc.stdout.split("\0") if path), None


def read_sources(root, rel_paths):
    """Read tracked sources; return (sources, symlinked paths). Symlinks fail closed."""
    sources = []
    symlinks = []
    for rel in rel_paths:
        if any(part in PRUNE_PARTS for part in rel.split("/")):
            continue
        path = os.path.join(root, rel)
        if os.path.islink(path):
            symlinks.append(rel)
            continue
        try:
            with open(path, encoding="utf-8", errors="replace") as handle:
                sources.append((rel, handle.read()))
        except OSError:
            continue
    return sources, symlinks


def print_rules():
    print("IEMS_COMMAND_EGRESS rules (file + symbol + allowed occurrences):")
    for rel in sorted(ALLOWED_COUNTS):
        pairs = ", ".join(
            f"{symbol}={count}" for symbol, count in sorted(ALLOWED_COUNTS[rel].items())
        )
        print(f"  {rel}: {pairs}")


# Round-2 positives: the definition file and extern prototypes are not calls.
SELFTEST_BASELINE = {
    "modules/edge_api/src/edge_api.c": (
        '__attribute__((visibility("default"))) int edge_api_write_tag_ex(int bus_id) {\n'
        "    return bus_id;\n"
        "}\n"
        "\n"
        "int edge_api_write_tag(int bus_id) {\n"
        "    return edge_api_write_tag_ex(bus_id);\n"
        "}\n"
        "\n"
        "__typeof__(int) edge_api_raw_io(int bus_id) {\n"
        "    return bus_id;\n"
        "}\n"
    ),
    "cmd/iems/iems_execution_adapter.c": (
        "void execute_write(void) {\n"
        '    edge_api_write_tag_ex(0, "ModbusRTU");\n'
        "}\n"
    ),
    "adapters/rest_api/src/rest_server.c": (
        "void rest_write(void) {\n"
        '    edge_api_write_tag(0, "ModbusRTU");\n'
        "}\n"
        "\n"
        "void rest_raw(void) {\n"
        "    edge_api_raw_io(0);\n"
        "}\n"
    ),
    "cmd/dcu/dcu_node.c": (
        "void dcu_raw(void) {\n"
        "    edge_api_raw_io(0);\n"
        "}\n"
    ),
    "cmd/iems/notes.c": (
        "/* edge_api_write_tag(0); */\n"
        'const char *note = "edge_api_raw_io(0);";\n'
    ),
    "cmd/iems/externs.c": (
        "extern int edge_api_write_tag_ex(int bus_id);\n"
        "extern int edge_api_raw_io(int bus_id);\n"
        "extern int edge_api_write_tag(int bus_id);\n"
    ),
}

SELFTEST_EXPECTED_PASSES = [
    ("edge_api_raw_io", "adapters/rest_api/src/rest_server.c", 6),
    ("edge_api_raw_io", "cmd/dcu/dcu_node.c", 2),
    ("edge_api_raw_io", "modules/edge_api/src/edge_api.c", 9),
    ("edge_api_write_tag", "adapters/rest_api/src/rest_server.c", 2),
    ("edge_api_write_tag", "modules/edge_api/src/edge_api.c", 5),
    ("edge_api_write_tag_ex", "cmd/iems/iems_execution_adapter.c", 2),
    ("edge_api_write_tag_ex", "modules/edge_api/src/edge_api.c", 1),
    ("edge_api_write_tag_ex", "modules/edge_api/src/edge_api.c", 6),
]

# Each case overlays SELFTEST_BASELINE and must be rejected (rc=1) with every
# expected FAIL line, so a rule deleted later cannot stay green for another fix.
SELFTEST_CASES = [
    (
        "if0_fake_definition",
        {
            "cmd/iems/blind.c": (
                "#if 0\n"
                "int edge_api_write_tag_ex(int bus_id) { return bus_id; }\n"
                "#endif\n"
                "int blind_call(void) { return edge_api_write_tag_ex(0); }\n"
            )
        },
        ["FAIL edge_api_write_tag_ex cmd/iems/blind.c:4"],
    ),
    (
        "macro_and_function_pointer",
        {
            "cmd/iems/alias.c": (
                "#define WRITE_TAG edge_api_write_tag\n"
                "static int (*const egress_fn)(int) = edge_api_raw_io;\n"
            )
        },
        [
            "FAIL edge_api_write_tag cmd/iems/alias.c:1",
            "FAIL edge_api_raw_io cmd/iems/alias.c:2",
        ],
    ),
    (
        "line_continuation",
        {
            "cmd/iems/continued.c": (
                "void continued(void) {\n"
                "    edge_api_write_\\\n"
                "tag(0);\n"
                "}\n"
            )
        },
        ["FAIL edge_api_write_tag cmd/iems/continued.c:2"],
    ),
    (
        "second_call_in_whitelisted_file",
        {
            "cmd/iems/iems_execution_adapter.c": (
                "void execute_write(void) {\n"
                '    edge_api_write_tag_ex(0, "ModbusRTU");\n'
                "}\n"
                "void execute_write_again(void) {\n"
                '    edge_api_write_tag_ex(0, "ModbusRTU");\n'
                "}\n"
            )
        },
        ["FAIL edge_api_write_tag_ex cmd/iems/iems_execution_adapter.c:5"],
    ),
    (
        "asm_string",
        {
            "cmd/iems/asm_escape.c": (
                "void asm_call(void) {\n"
                '    __asm__ volatile("call edge_api_raw_io");\n'
                "}\n"
            )
        },
        ["FAIL edge_api_raw_io cmd/iems/asm_escape.c:2"],
    ),
]

SELFTEST_SYMLINK = "cmd/iems/symlinked.c"


def run_selftest():
    failed = 0
    passes, failures = evaluate_sources(list(SELFTEST_BASELINE.items()))
    missing_passes = [entry for entry in SELFTEST_EXPECTED_PASSES if entry not in passes]
    if failures or missing_passes or len(passes) != len(SELFTEST_EXPECTED_PASSES):
        failed += 1
        print("SELFTEST FAIL baseline is not rc=0 with the expected allowed occurrences:")
        for line in render(passes, failures, []):
            print(f"  {line}")
        for entry in missing_passes:
            print(f"  missing PASS {entry[0]} {entry[1]}:{entry[2]}")
    else:
        print(f"SELFTEST PASS baseline rc=0 ({len(passes)} allowed occurrences)")

    for name, files, expected in SELFTEST_CASES:
        tree = dict(SELFTEST_BASELINE)
        tree.update(files)
        _passes, case_failures = evaluate_sources(list(tree.items()))
        case_lines = render([], case_failures, [])
        missing = [
            line for line in expected
            if not any(actual.startswith(line) for actual in case_lines)
        ]
        if missing:
            failed += 1
            print(f"SELFTEST FAIL case={name} missing {missing!r}, got {case_lines}")
        else:
            print(f"SELFTEST PASS case={name} rc=1 {expected[0]}")

    with tempfile.TemporaryDirectory() as tmp:
        target = os.path.join(tmp, "clean_target.c")
        with open(target, "w", encoding="utf-8") as handle:
            handle.write("int clean(void) { return 0; }\n")
        link_dir = os.path.join(tmp, "cmd", "iems")
        os.makedirs(link_dir)
        os.symlink("../../clean_target.c", os.path.join(link_dir, "symlinked.c"))
        _sources, symlinks = read_sources(tmp, [SELFTEST_SYMLINK])
        if symlinks == [SELFTEST_SYMLINK]:
            print(f"SELFTEST PASS case=symlinked_source rc=1 {SELFTEST_SYMLINK} refused")
        else:
            failed += 1
            print(f"SELFTEST FAIL case=symlinked_source expected [{SELFTEST_SYMLINK}] "
                  f"refused, got symlinks={symlinks}")

    if failed:
        print(f"SELFTEST FAIL {failed} baseline/negative check(s) did not hold")
        return 1
    print(
        f"SELFTEST PASS {len(SELFTEST_CASES) + 1}/{len(SELFTEST_CASES) + 1} "
        "negative cases rejected; baseline rc=0"
    )
    return 0


def main():
    parser = argparse.ArgumentParser(description="IEMS command egress guard.")
    parser.add_argument("--root", default=".", help="repository root")
    parser.add_argument("--selftest", action="store_true", help="replay the negative cases")
    args = parser.parse_args()

    if args.selftest:
        return run_selftest()

    root = os.path.abspath(args.root)
    rel_paths, error = tracked_c_files(root)
    if rel_paths is None:
        print(f"❌ [IEMS_COMMAND_EGRESS] git ls-files failed under {root}: {error}")
        return 1

    sources, symlinks = read_sources(root, rel_paths)
    passes, failures = evaluate_sources(sources)
    print_rules()
    for line in render(passes, failures, symlinks):
        print(line)
    if failures or symlinks:
        print(
            f"❌ [IEMS_COMMAND_EGRESS] {len(failures)} command egress violation(s), "
            f"{len(symlinks)} symlinked source(s)"
        )
        return 1
    print(f"✅ [IEMS_COMMAND_EGRESS] {len(passes)} allowed egress occurrence(s); no bypass")
    return 0


if __name__ == "__main__":
    sys.exit(main())
