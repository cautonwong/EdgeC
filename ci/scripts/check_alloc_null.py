#!/usr/bin/env python3
"""Require constructors to check allocation results before dereferencing them.

OOM on an embedded Linux box is not hypothetical. A `*_create`/`*_init` that
does `p = calloc(...); p->field = ...` crashes instead of returning NULL, and
the caller has no way to recover. This guard scans function bodies rather than
a fixed line window: a check anywhere before the first `->`/`[` on the
allocation result is enough, and returning the pointer to the caller transfers
ownership.

Scope is constructors only (`*_create*`, `*_init*`, `*_new*`, `*_alloc*`).
Execution paths allocate deliberately and are covered by the zero-heap guard.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import guard_baseline

GUARD_NAME = "ALLOC_NULL_GUARD"
from c_source_scan import SHARED_PRUNE_DIRS, rel_scan_parts

SKIP_DIRS = {".git", "build", "third-party", "node_modules", ".local", "web", ".venv"} | SHARED_PRUNE_DIRS
FUNC_RE = re.compile(r"^[A-Za-z_][\w \*]*\b(\w+)\s*\([^;]*$")
CTOR_RE = re.compile(r"_(?:create|init|new|alloc)\w*$")
ALLOC_RE = re.compile(r"([\w.>\-\[\]]+)\s*=\s*(?:calloc|malloc|realloc|strdup)\s*\(")
CHECK_TMPL = r"(?:if\s*\(\s*!\s*{v}\b|if\s*\(\s*{v}\s*(?:==\s*NULL|!=\s*NULL|\)|&&|\?)|\b{v}\s*\?\s)"


def functions(lines):
    """Yield (name, start_line_index, end_line_index) for C function bodies."""
    index = 0
    total = len(lines)
    while index < total:
        match = FUNC_RE.match(lines[index])
        if not match or lines[index].lstrip().startswith(("if", "while", "for", "switch", "return")):
            index += 1
            continue
        brace = None
        depth = 0
        began = False
        for probe in range(index, min(index + 16, total)):
            if "{" in lines[probe]:
                brace = probe
                break
            if ";" in lines[probe]:
                break
        if brace is None:
            index += 1
            continue
        for probe in range(brace, total):
            depth += lines[probe].count("{") - lines[probe].count("}")
            if "{" in lines[probe]:
                began = True
            if began and depth <= 0:
                yield match.group(1), index, probe
                index = probe + 1
                break
        else:
            return


def scan(root):
    violations = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS and not d.startswith("build")]
        for name in filenames:
            if not name.endswith(".c"):
                continue
            path = os.path.join(dirpath, name)
            if "fixtures" in rel_scan_parts(dirpath, root):
                continue
            try:
                lines = open(path, "r", encoding="utf-8", errors="replace").read().splitlines()
            except OSError:
                continue
            for fname, start, end in functions(lines):
                if not CTOR_RE.search(fname):
                    continue
                body = "\n".join(lines[start:end + 1])
                for match in ALLOC_RE.finditer(body):
                    var = match.group(1)
                    tail = body[match.end():]
                    deref = re.search(re.escape(var) + r"\s*(?:->|\[)", tail)
                    if not deref:
                        continue
                    before = tail[:deref.start()]
                    if re.search(CHECK_TMPL.format(v=re.escape(var)), before):
                        continue
                    if re.search(r"\breturn\s+" + re.escape(var) + r"\s*;", before):
                        continue
                    line_no = start + body.count("\n", 0, match.start()) + 1
                    rel = os.path.relpath(path, root).replace(os.sep, "/")
                    violations.append(
                        {
                            "fingerprint": guard_baseline.fingerprint(
                                rel, "ALLOC_UNCHECKED", f"{fname}:{var}"
                            ),
                            "location": f"{rel}:{line_no}",
                            "detail": f"{fname} dereferences '{var}' without a NULL check",
                            "rule_id": "ALLOC_UNCHECKED",
                        }
                    )
    return violations


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    parser.add_argument("--update-baseline", action="store_true")
    args = parser.parse_args()
    violations = scan(os.path.abspath(args.root))
    baseline_path = guard_baseline.baseline_path_for(__file__)
    return guard_baseline.apply(violations, baseline_path, args.update_baseline, GUARD_NAME)


if __name__ == "__main__":
    sys.exit(main())
