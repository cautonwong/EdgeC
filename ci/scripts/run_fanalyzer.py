#!/usr/bin/env python3
"""Run GCC's -fanalyzer over a slice of the tree using the real build flags.

The analyzer is too slow for the default build, but a one-off manual run rots:
the flags drift and nobody can rerun it. This script pulls each translation
unit's command from `ninja -t commands`, swaps in -O0 -fanalyzer, and compares
findings against a checked-in ledger so CI fails only on new reports.

Ledger format, one per line: <repo-relative path>\t<option>\t<message>
"""
import argparse
import concurrent.futures
import os
import re
import shlex
import subprocess
import sys

DIAG_RE = re.compile(r"^(?P<path>/[^:]+):(?P<line>\d+):(?:\d+:)?\s+"
                     r"(?P<kind>warning|error):\s+(?P<msg>.*?)(?:\s+\[(?P<opt>-W[^\]]+)\])?$")
DEFAULT_PATHS = ("modules/edge_core/src,modules/edge_io/src,modules/edge_session/src,"
                 "modules/edge_stream/src,modules/edge_north,sdk,adapters,foundation/src,cmd")


def repo_root():
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def commands(build_dir, root, paths):
    try:
        out = subprocess.run(["ninja", "-C", build_dir, "-t", "commands"],
                             capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError) as exc:
        sys.exit(f"cannot read ninja commands from {build_dir}: {exc}")
    wanted = []
    for line in out.splitlines():
        parts = line.split()
        if "-c" not in parts:
            continue
        src = parts[parts.index("-c") + 1]
        rel = os.path.relpath(src, root)
        if rel.endswith(".c") and any(rel.startswith(p.rstrip("/") + "/") for p in paths):
            wanted.append((src, line))
    return wanted


def analyzer_command(line, compiler):
    parts = shlex.split(line)
    if parts and parts[0].endswith("ccache"):
        parts = parts[1:]
    parts = parts[1:]
    out, skip = [], False
    for token in parts:
        if skip:
            skip = False
        elif token in ("-MD", "--coverage", "-fanalyzer"):
            continue
        elif token in ("-MT", "-MF", "-o"):
            skip = True
        elif token.startswith("-O"):
            continue
        else:
            out.append(token)
    return [compiler] + out + ["-O0", "-fanalyzer", "-c", "-o", os.devnull]


def run_one(item, compiler):
    src, line = item
    try:
        proc = subprocess.run(analyzer_command(line, compiler), capture_output=True,
                              text=True, timeout=240)
        return src, proc.stderr
    except subprocess.TimeoutExpired:
        return src, f"{src}:0:0: error: analyzer timeout [-Wanalyzer-timeout]\n"


def findings(root, results):
    seen = {}
    for src, stderr in results:
        for text in stderr.splitlines():
            match = DIAG_RE.match(text.strip())
            if not match:
                continue
            rel = os.path.relpath(match.group("path"), root)
            key = (rel, match.group("opt") or "", match.group("msg").strip())
            seen.setdefault(key, int(match.group("line")))
    return [(path, line, opt, msg) for (path, opt, msg), line in sorted(seen.items())]


def read_ledger(path):
    entries = set()
    if not os.path.exists(path):
        return entries
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.rstrip("\n")
            if line and not line.startswith("#"):
                path_name, opt, msg = line.split("\t", 2)
                entries.add((path_name, opt, msg))
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--paths", default=DEFAULT_PATHS,
                        help="comma-separated repo-relative directories")
    parser.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 4))
    parser.add_argument("--ledger", default="ci/baseline/fanalyzer.txt")
    parser.add_argument("--update-ledger", action="store_true")
    parser.add_argument("--compiler", default="/usr/bin/cc")
    args = parser.parse_args()

    root = repo_root()
    build_dir = os.path.join(root, args.build_dir)
    ledger_path = os.path.join(root, args.ledger)
    paths = [p for p in args.paths.split(",") if p]
    items = commands(build_dir, root, paths)
    if not items:
        sys.exit(f"no translation units matched {args.paths} in {build_dir}")

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(lambda item: run_one(item, args.compiler), items))

    found = findings(root, results)
    ledger = read_ledger(ledger_path)

    if args.update_ledger:
        with open(ledger_path, "w", encoding="utf-8") as handle:
            handle.write("# path\toption\tmessage\n")
            for path, _line, opt, msg in found:
                handle.write(f"{path}\t{opt}\t{msg}\n")
        ledger = {(path, opt, msg) for path, _line, opt, msg in found}
        print(f"wrote {len(found)} entries to {args.ledger}")

    new = [f for f in found if (f[0], f[2], f[3]) not in ledger]
    stale = ledger - {(p, o, m) for p, _l, o, m in found}
    for path, line, opt, msg in new:
        print(f"{path}:{line}: {msg} [{opt}]")
    for path, opt, msg in sorted(stale):
        print(f"stale ledger entry: {path}: {msg} [{opt}]")

    if new:
        print(f"❌ [FANALYZER] {len(new)} finding(s) outside the ledger")
        return 1
    print(f"✅ [FANALYZER] {len(found)} known finding(s), no new reports")
    return 0


if __name__ == "__main__":
    sys.exit(main())
