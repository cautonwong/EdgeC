#!/usr/bin/env python3
"""Formal manifest guard: schema, paths, covers, sync ancestry, registration.

formal_manifest.py owns the schema and path/negative rules; this guard keeps the
checked-in claims honest: covered symbols exist, sync commits are ancestors,
covered files that changed since sync warn, and every tracked file under
formal/ is either declared by a row or on the small allowlist
(manifest shards, README, vendored tools).
"""

import argparse
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import c_source_scan
import formal_manifest


def git(root, *args):
    return subprocess.run(
        ["git", "-C", root, *args], check=True, capture_output=True, text=True
    ).stdout.strip()


def check_row(root, row):
    where = row.where
    errors = []
    covered_files = []
    for cover in row["covers"].split(";"):
        cover = cover.strip()
        if ":" not in cover:
            errors.append(f"{where}: covers entry lacks path:symbol: {cover!r}")
            continue
        rel_path, symbol = cover.rsplit(":", 1)
        source = os.path.join(root, rel_path)
        if not os.path.isfile(source):
            errors.append(f"{where}: covered file missing: {rel_path}")
            continue
        text = c_source_scan.strip_comments(
            open(source, encoding="utf-8", errors="replace").read()
        )
        if not re.search(rf"\b{re.escape(symbol)}\s*\(", text):
            errors.append(f"{where}: symbol {symbol} not found in {rel_path}")
        covered_files.append(rel_path)

    try:
        git(root, "rev-parse", "--verify", "--quiet", row["sync"] + "^{commit}")
    except subprocess.CalledProcessError:
        errors.append(f"{where}: sync commit {row['sync']} does not exist")
        return errors, []
    try:
        git(root, "merge-base", "--is-ancestor", row["sync"], "HEAD")
    except subprocess.CalledProcessError:
        errors.append(f"{where}: sync commit {row['sync']} is not an ancestor of HEAD")

    warnings = []
    changed = set(git(root, "diff", "--name-only", f"{row['sync']}..HEAD").splitlines())
    for rel_path in covered_files:
        if rel_path in changed:
            warnings.append(
                f"{where}: {rel_path} changed since sync {row['sync']}; "
                "update the harness and advance sync"
            )
    return errors, warnings


def unregistered_files(root, rows):
    """Tracked files under formal/ that no manifest row and no allowlist entry covers."""
    tracked = [path for path in git(root, "ls-files", "formal").splitlines() if path]
    allowed = {"formal/README.md"}
    allowed.update(
        path for path in tracked
        if path.startswith("formal/manifest/") and path.endswith(".tsv")
    )
    for row in rows:
        allowed.add(row["artifact"])
        if row["cfg"] != formal_manifest.NONE:
            allowed.add(row["cfg"])
        for kind, path, _invariant in formal_manifest.parse_negative_entries(root, row):
            if kind == "script" and path.startswith("formal/"):
                allowed.add(path)
            elif kind == "cfg":
                allowed.add(path)
    return [
        path for path in tracked
        if path not in allowed and not path.startswith("formal/tools/")
    ]


def main():
    parser = argparse.ArgumentParser(
        description="Check formal manifest metadata and sync."
    )
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)

    try:
        rows = formal_manifest.load_manifests(root)
    except formal_manifest.ManifestError as error:
        print(f"❌ [FORMAL-SYNC] {error}")
        return 1

    errors = []
    warnings = []
    for row in rows:
        row_errors, row_warnings = check_row(root, row)
        errors.extend(row_errors)
        warnings.extend(row_warnings)

    try:
        for path in unregistered_files(root, rows):
            errors.append(f"tracked formal file is not registered: {path}")
    except subprocess.CalledProcessError as error:
        print(f"❌ [FORMAL-SYNC] git ls-files formal failed: {error}")
        return 1

    warnings = list(dict.fromkeys(warnings))
    for warning in warnings:
        print(f"⚠️  [FORMAL-SYNC] {warning}")
    if errors:
        for error in errors:
            print(f"❌ [FORMAL-SYNC] {error}")
        return 1
    print(
        f"✅ [FORMAL-SYNC] {len(rows)} row(s) in "
        f"{len({row.source for row in rows})} shard(s), {len(warnings)} warning(s)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
