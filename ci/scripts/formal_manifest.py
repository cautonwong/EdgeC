#!/usr/bin/env python3
"""Single source of truth for formal/manifest/*.tsv.

check_formal.sh, check_formal_negative.sh and check_formal_sync.py all read the
schema, owners, modes, path rules and negative-entry grammar from here. The
runner consumes parsed rows through `--rows`; nothing else may copy the field
list.

The 14 historical columns keep their order; the per-run columns are appended so
existing positions stay stable. Every field is mandatory; `n/a` marks a field
the row's mode does not use.
"""

import argparse
import os
import posixpath
import re
import sys

FIELDS = (
    "artifact",
    "property",
    "covers",
    "tool",
    "bounds",
    "unwind",
    "max_len",
    "negative",
    "sync",
    "owner",
    "runtime",
    "property_count",
    "state_count",
    "jar_sha256",
    "mode",
    "cfg",
    "function",
    "assertions",
)
OWNERS = {"iems", "dcu"}
MODES = {"proof", "cover", "function", "tla"}
CBMC_MODES = {"proof", "cover", "function"}
CBMC_TOOL_MARKER = "CBMC 5.12"
TLC_TOOL_MARKER = "TLC"
NONE = "n/a"
TLC_VERSION_RE = re.compile(r"TLC (\d{4}\.\d{2}\.\d{2}\.\d{6})")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
SYNC_RE = re.compile(r"^[0-9a-f]{7,40}$")
POSITIVE_INT_RE = re.compile(r"^[1-9][0-9]*$")
IDENTIFIER_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
SCRIPT_SUFFIXES = (".sh", ".py")
FORMAL_PREFIX = ("formal/",)
SCRIPT_PREFIXES = ("ci/scripts/", "formal/")


class ManifestError(Exception):
    pass


class Row:
    __slots__ = ("source", "line_no", "_values")

    def __init__(self, source, line_no, values):
        self.source = source
        self.line_no = line_no
        self._values = values

    def __getitem__(self, field):
        return self._values[field]

    @property
    def where(self):
        return f"{self.source}:{self.line_no}"


def checked_path(root, rel, prefixes, where, label):
    """Return rel after rejecting absolute, escaping, unnormalized paths."""
    if not rel or rel.startswith("/") or "\\" in rel:
        raise ManifestError(f"{where}: {label} must be a relative POSIX path: {rel!r}")
    if posixpath.normpath(rel) != rel or rel in (".", ".."):
        raise ManifestError(f"{where}: {label} is not normalized: {rel!r}")
    if not any(rel.startswith(prefix) for prefix in prefixes):
        raise ManifestError(
            f"{where}: {label} must live under {', '.join(prefixes)}: {rel!r}"
        )
    full = os.path.realpath(os.path.join(root, rel))
    for prefix in prefixes:
        prefix_root = os.path.realpath(os.path.join(root, prefix))
        if full == prefix_root or full.startswith(prefix_root + os.sep):
            break
    else:
        raise ManifestError(f"{where}: {label} escapes {', '.join(prefixes)}: {rel!r}")
    if not os.path.isfile(full):
        raise ManifestError(f"{where}: {label} missing: {rel}")
    return rel


def _positive_int(row, field):
    value = row[field]
    if not POSITIVE_INT_RE.match(value):
        raise ManifestError(f"{row.where}: {field} is not a positive integer: {value!r}")
    return int(value)


def assertion_patterns(row):
    raw = row["assertions"]
    if raw == NONE:
        return []
    patterns = []
    for part in raw.split(";"):
        if not part:
            raise ManifestError(f"{row.where}: empty assertion pattern")
        try:
            re.compile(part)
        except re.error as error:
            raise ManifestError(
                f"{row.where}: invalid assertion regex {part!r}: {error}"
            )
        patterns.append(part)
    return patterns


def parse_negative_entries(root, row):
    """Parse the negative field into ('cfg', path, invariant) / ('script', path, None)."""
    raw = row["negative"]
    if raw == NONE:
        return []
    entries = []
    for part in raw.split(";"):
        if not part or part.strip() != part:
            raise ManifestError(f"{row.where}: malformed negative entry {part!r}")
        if "#" in part:
            rel, invariant = part.split("#", 1)
            if not IDENTIFIER_RE.match(invariant):
                raise ManifestError(
                    f"{row.where}: negative invariant is not an identifier: {invariant!r}"
                )
            if row["mode"] != "tla":
                raise ManifestError(
                    f"{row.where}: cfg#invariant negatives are only valid on tla rows"
                )
            checked_path(root, rel, FORMAL_PREFIX, row.where, "negative cfg")
            with open(os.path.join(root, rel), encoding="utf-8", errors="replace") as handle:
                lines = {line.strip() for line in handle}
            if f"INVARIANT {invariant}" not in lines:
                raise ManifestError(
                    f"{row.where}: {rel} has no 'INVARIANT {invariant}' line"
                )
            entries.append(("cfg", rel, invariant))
        else:
            if not part.endswith(SCRIPT_SUFFIXES):
                raise ManifestError(
                    f"{row.where}: negative entry is neither <cfg>#<invariant> "
                    f"nor a .sh/.py script: {part!r}"
                )
            checked_path(root, part, SCRIPT_PREFIXES, row.where, "negative script")
            entries.append(("script", part, None))
    return entries


def _validate_cbmc_row(root, row):
    where = row.where
    mode = row["mode"]
    if mode not in CBMC_MODES:
        raise ManifestError(
            f"{where}: CBMC row mode must be one of {sorted(CBMC_MODES)}, got {mode!r}"
        )
    checked_path(root, row["artifact"], FORMAL_PREFIX, where, "artifact")
    unwind = _positive_int(row, "unwind")
    max_len = _positive_int(row, "max_len")
    if unwind < max_len + 1:
        raise ManifestError(f"{where}: unwind {unwind} must exceed max_len {max_len}")
    if "--unwind" not in row["bounds"]:
        raise ManifestError(f"{where}: bounds must record --unwind, got {row['bounds']!r}")
    for field in ("cfg", "state_count", "jar_sha256"):
        if row[field] != NONE:
            raise ManifestError(f"{where}: CBMC row {field} must be {NONE!r}")
    patterns = assertion_patterns(row)
    if mode == "proof":
        _positive_int(row, "property_count")
        if row["function"] != NONE:
            raise ManifestError(f"{where}: proof row function must be {NONE!r}")
        if patterns:
            raise ManifestError(f"{where}: proof rows do not run assertions; use {NONE!r}")
    else:
        if row["property_count"] != NONE:
            raise ManifestError(f"{where}: {mode} row property_count must be {NONE!r}")
        if not patterns:
            raise ManifestError(f"{where}: {mode} rows require assertions")
        if mode == "function":
            if not IDENTIFIER_RE.match(row["function"]):
                raise ManifestError(
                    f"{where}: function must be a C identifier, got {row['function']!r}"
                )
        elif row["function"] != NONE:
            raise ManifestError(f"{where}: cover row function must be {NONE!r}")


def _validate_tla_row(root, row):
    where = row.where
    if row["mode"] != "tla":
        raise ManifestError(f"{where}: TLC row mode must be 'tla', got {row['mode']!r}")
    if not row["artifact"].endswith(".tla"):
        raise ManifestError(f"{where}: TLA artifact must end with .tla")
    checked_path(root, row["artifact"], FORMAL_PREFIX, where, "artifact")
    checked_path(root, row["cfg"], FORMAL_PREFIX, where, "cfg")
    _positive_int(row, "state_count")
    if not SHA256_RE.match(row["jar_sha256"]):
        raise ManifestError(f"{where}: jar_sha256 is not a SHA256: {row['jar_sha256']!r}")
    if not TLC_VERSION_RE.search(row["tool"]):
        raise ManifestError(f"{where}: tool must name a TLC version, got {row['tool']!r}")
    for field in ("unwind", "max_len", "property_count", "function"):
        if row[field] != NONE:
            raise ManifestError(f"{where}: TLA row {field} must be {NONE!r}")
    if assertion_patterns(row):
        raise ManifestError(f"{where}: TLA rows do not run assertions; use {NONE!r}")


def validate_row(root, source, line_no, values):
    if len(values) != len(FIELDS):
        raise ManifestError(
            f"{source}:{line_no}: expected {len(FIELDS)} tab-separated fields, "
            f"got {len(values)}"
        )
    row = Row(source, line_no, dict(zip(FIELDS, values)))
    where = row.where
    for field in FIELDS:
        if row[field] == "":
            raise ManifestError(f"{where}: empty {field}")
    owner = row["owner"]
    if owner not in OWNERS:
        raise ManifestError(
            f"{where}: unknown owner {owner!r}; add it to formal_manifest.OWNERS"
        )
    if os.path.basename(source) != f"{owner}.tsv":
        raise ManifestError(f"{where}: shard file must be named {owner}.tsv")
    if row["mode"] not in MODES:
        raise ManifestError(f"{where}: unknown mode {row['mode']!r}")
    if not SYNC_RE.match(row["sync"]):
        raise ManifestError(f"{where}: sync is not a commit SHA: {row['sync']!r}")
    for cover in row["covers"].split(";"):
        cover = cover.strip()
        if not cover or ":" not in cover:
            raise ManifestError(f"{where}: covers entry must be path:symbol, got {cover!r}")
    tool = row["tool"]
    if CBMC_TOOL_MARKER in tool:
        _validate_cbmc_row(root, row)
    elif TLC_TOOL_MARKER in tool:
        _validate_tla_row(root, row)
    else:
        raise ManifestError(
            f"{where}: tool must name {CBMC_TOOL_MARKER!r} or TLC, got {tool!r}"
        )
    parse_negative_entries(root, row)
    return row


def parse_manifest_file(root, path):
    source = os.path.relpath(path, root)
    with open(path, encoding="utf-8") as handle:
        lines = [
            (line_no, line.rstrip("\n"))
            for line_no, line in enumerate(handle, 1)
            if line.strip()
        ]
    if not lines:
        raise ManifestError(f"{source}: empty manifest")
    header_no, header_line = lines[0]
    header = header_line.split("\t")
    if header != list(FIELDS):
        missing = [field for field in FIELDS if field not in header]
        extra = [field for field in header if field not in FIELDS]
        detail = []
        if missing:
            detail.append("missing columns " + ", ".join(missing))
        if extra:
            detail.append("unknown columns " + ", ".join(extra))
        if not detail:
            detail.append("columns are out of order")
        raise ManifestError(f"{source}:{header_no}: " + "; ".join(detail))
    rows = [
        validate_row(root, source, line_no, line.split("\t"))
        for line_no, line in lines[1:]
    ]
    if not rows:
        raise ManifestError(f"{source}: no data rows")
    hashes = {row["jar_sha256"] for row in rows if row["mode"] == "tla"}
    if len(hashes) > 1:
        raise ManifestError(f"{source}: TLA rows disagree on jar_sha256")
    return rows


def load_manifests(root):
    manifest_dir = os.path.join(root, "formal", "manifest")
    if not os.path.isdir(manifest_dir):
        raise ManifestError("formal/manifest/ is missing")
    shards = sorted(
        name for name in os.listdir(manifest_dir) if name.endswith(".tsv")
    )
    if not shards:
        raise ManifestError("formal/manifest/ has no *.tsv shard")
    rows = []
    for name in shards:
        rows.extend(parse_manifest_file(root, os.path.join(manifest_dir, name)))
    seen = {}
    for row in rows:
        key = (row["artifact"], row["mode"])
        if key in seen:
            raise ManifestError(
                f"{row.where}: duplicate (artifact, mode) "
                f"{key[0]} [{key[1]}], first at {seen[key]}"
            )
        seen[key] = row.where
    return rows


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Parse and validate formal/manifest/*.tsv."
    )
    parser.add_argument("--root", default=".", help="repository root")
    parser.add_argument(
        "--rows",
        action="store_true",
        help="print parsed rows as fixed-order tab-separated fields",
    )
    args = parser.parse_args(argv)
    root = os.path.abspath(args.root)
    try:
        rows = load_manifests(root)
    except ManifestError as error:
        print(f"❌ [FORMAL-MANIFEST] {error}", file=sys.stderr)
        return 1
    if args.rows:
        for row in rows:
            print("\t".join(row[field] for field in FIELDS))
    else:
        print(
            f"✅ [FORMAL-MANIFEST] {len(rows)} row(s) in "
            f"{len({row.source for row in rows})} shard(s)"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
