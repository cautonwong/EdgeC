#!/usr/bin/env python3
"""Inventory iems_v3 JSON config snapshots.

Scans a config root, records each JSON file's relative path, SHA256, byte size,
top-level keys, and any parse error. `--check` compares a committed manifest
against the current snapshot; any missing file, extra file, hash change, or
unparseable JSON exits 1.

The manifest has no values and no secrets, so it is safe to commit. The actual
configuration files stay in the iems_v3 repository.
"""

import argparse
import hashlib
import json
import os
import sys

DEFAULT_ROOT = "/home/edge/iems_v3/config"
REPO_MANIFEST = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             "config_inventory.tsv")
HEADER = ["path", "sha256", "bytes", "top_keys", "parse_error"]


def parse_error_digest(exc):
    text = f"{type(exc).__name__}: {exc}"
    text = text.replace("\t", " ").replace("\r", " ").replace("\n", " ")
    return text[:300]


def scan(root):
    rows = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        for name in sorted(filenames):
            if not name.endswith(".json"):
                continue
            absolute = os.path.join(dirpath, name)
            rel = os.path.relpath(absolute, root).replace(os.sep, "/")
            try:
                with open(absolute, "rb") as handle:
                    payload = handle.read()
            except OSError as exc:
                rows.append({
                    "path": rel,
                    "sha256": "",
                    "bytes": "",
                    "top_keys": "[]",
                    "parse_error": parse_error_digest(exc),
                })
                continue
            digest = hashlib.sha256(payload).hexdigest()
            top_keys = "[]"
            parse_error = ""
            try:
                document = json.loads(payload.decode("utf-8"))
            except (UnicodeDecodeError, json.JSONDecodeError) as exc:
                parse_error = parse_error_digest(exc)
            else:
                if isinstance(document, dict):
                    top_keys = json.dumps(sorted(document.keys()), ensure_ascii=True)
            rows.append({
                "path": rel,
                "sha256": digest,
                "bytes": str(len(payload)),
                "top_keys": top_keys,
                "parse_error": parse_error,
            })
    rows.sort(key=lambda row: row["path"])
    return rows


def resolve_manifest(args, root):
    if args.manifest:
        return args.manifest
    if os.path.realpath(root) == os.path.realpath(DEFAULT_ROOT):
        return REPO_MANIFEST
    return os.path.join(root, "config_inventory.tsv")


def write_manifest(manifest, rows):
    parent = os.path.dirname(os.path.abspath(manifest))
    if parent:
        os.makedirs(parent, exist_ok=True)
    lines = ["\t".join(HEADER)]
    for row in rows:
        fields = [row[column] for column in HEADER]
        if any("\t" in field or "\n" in field for field in fields):
            raise ValueError(f"field contains a tab or newline: {row['path']}")
        lines.append("\t".join(fields))
    with open(manifest, "w", encoding="utf-8", newline="") as handle:
        handle.write("\n".join(lines) + "\n")
    errors = [row for row in rows if row["parse_error"]]
    print(f"WROTE {len(rows)} file(s) -> {manifest}")
    for row in errors:
        print(f"WARN parse error: {row['path']}: {row['parse_error']}")
    return 0


def read_manifest(manifest):
    with open(manifest, "r", encoding="utf-8", newline="") as handle:
        lines = handle.read().splitlines()
    if not lines or lines[0].split("\t") != HEADER:
        raise ValueError(
            f"manifest header is {lines[0] if lines else ''!r}, expected {HEADER!r}")
    rows = {}
    for number, line in enumerate(lines[1:], 2):
        fields = line.split("\t")
        if len(fields) != len(HEADER):
            raise ValueError(f"manifest line {number} has {len(fields)} fields")
        entry = dict(zip(HEADER, fields))
        path = entry["path"]
        if path in rows:
            raise ValueError(f"duplicate manifest row: {path}")
        rows[path] = entry
    return rows


def check_manifest(manifest, rows):
    failures = 0
    try:
        recorded = read_manifest(manifest)
    except FileNotFoundError:
        print(f"FAIL manifest not found: {manifest}")
        return 1
    except (OSError, ValueError) as exc:
        print(f"FAIL manifest not readable: {manifest}: {exc}")
        return 1

    scanned = {row["path"]: row for row in rows}
    for path in sorted(set(recorded) - set(scanned)):
        print(f"FAIL missing file (in manifest, not on disk): {path}")
        failures += 1
    for path in sorted(set(scanned) - set(recorded)):
        print(f"FAIL unexpected file (on disk, not in manifest): {path}")
        failures += 1

    for path in sorted(set(recorded) & set(scanned)):
        expected = recorded[path]
        actual = scanned[path]
        if expected["sha256"] != actual["sha256"]:
            print(f"FAIL sha256 mismatch: {path}: manifest {expected['sha256']} "
                  f"!= disk {actual['sha256']}")
            failures += 1
        if actual["parse_error"]:
            print(f"FAIL parse error: {path}: {actual['parse_error']}")
            failures += 1
        elif expected["parse_error"]:
            print(f"FAIL parse error recorded in manifest: {path}: "
                  f"{expected['parse_error']}")
            failures += 1

    if failures:
        print(f"FAIL {failures} problem(s) against {manifest}")
        return 1
    print(f"OK {len(scanned)} file(s) match {manifest}")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", default=DEFAULT_ROOT,
                        help=f"config root to scan (default: {DEFAULT_ROOT})")
    parser.add_argument("--manifest", default=None,
                        help="manifest path (default: repo manifest for the "
                             "default root, <root>/config_inventory.tsv otherwise)")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--write", action="store_true",
                      help="write the manifest from the current snapshot")
    mode.add_argument("--check", action="store_true",
                      help="verify the manifest against the current snapshot "
                           "(default)")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        print(f"FAIL config root is not a directory: {root}")
        return 1
    manifest = resolve_manifest(args, root)
    rows = scan(root)
    if args.write:
        return write_manifest(manifest, rows)
    return check_manifest(manifest, rows)


if __name__ == "__main__":
    sys.exit(main())
