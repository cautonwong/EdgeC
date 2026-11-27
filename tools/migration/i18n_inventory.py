#!/usr/bin/env python3
"""Inventory iems_v3 gettext resources and EdgeC UI copy.

`--check` reads both input trees read-only, validates the pinned P6-05a counts,
classifies all 615 po messages, extracts EdgeC static UI copy, computes the
exact whitespace-stripped containment baseline, and writes a report directory.
Any input drift or count mismatch exits 2; nothing is skipped silently.
"""

import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

TOOL_DIR = Path(__file__).resolve().parent
REPO_ROOT = TOOL_DIR.parents[1]
IEMS_ROOT = Path("/home/edge/iems_v3")
IEMS_SHA = "3b5c73ff55a532fb715104d03fbe4e09db807173"
PO_REL = "translation/zh_CN.po"
PO_PATH = IEMS_ROOT / PO_REL
EMPTY_PO = "files/zh_CN.po"
UPDATE_PO = "src/update_po.sh"
OLD_BINARY = "tools/creat_patch/up_files_patch/up_files/opt/lnxall_app/bin/proto_forward"
MO_FIRMWARE = "firmware/ipk_ems_core/data/opt/lvt/app/global/language/zh_CN/LC_MESSAGES/language.mo"
MO_PATCH = "tools/creat_patch/up_files_patch/up_files/opt/lnxall_app/app/global/language/zh_CN/LC_MESSAGES/language.mo"
ALARM_ZH = "alarminfo/zh_CN/alarm_def.json"
ALARM_EN = "alarminfo/en_US/alarm_def.json"
REPORTS_DIR = TOOL_DIR / "i18n" / "reports"

EXPECTED_PO_ENTRIES = 616
EXPECTED_MESSAGES = 615
EXPECTED_C_FORMAT = 6
EXPECTED_MULTILINE = 2
EXPECTED_EQUAL = 2
EXPECTED_CLASSIFICATION = {
    "C_SOURCE": 472,
    "OTHER_TEXT": 2,
    "BINARY_ONLY": 48,
    "MISSING": 93,
}
EXPECTED_REVERSE_GAPS = 19
EXPECTED_MO = {MO_FIRMWARE: 526, MO_PATCH: 426}
EXPECTED_ALARM = {
    "zh_vendor_models": 16,
    "en_vendor_models": 15,
    "zh_tags": 3821,
    "en_tags": 3705,
    "zh_only": 149,
    "en_only": 33,
}
EXPECTED_MSGSTR_HITS = {
    ("web/products/iems/product.yaml", 97): frozenset({"Advanced", "Advanced features"}),
    ("web/permissions/catalog.yaml", 117): frozenset({"Reboot the device"}),
    ("web/permissions/catalog.yaml", 126): frozenset({"Restart the service"}),
    ("web/openapi/core.yaml", 381): frozenset({"Reboot the device"}),
    ("web/openapi/core.yaml", 590): frozenset({"Read configuration"}),
    ("web/openapi/core.yaml", 652): frozenset({"Write configuration"}),
    ("web/core/src/modules/dcu/PowerQuality.vue", 278): frozenset({"PF"}),
}
EXPECTED_MSGID_HITS = {
    ("web/products/smart_box/src/components/WaveformScope100Hz.vue", 17): frozenset(
        {"Precision"}
    ),
}
EXPECTED_UI_CATEGORIES = {
    "entitlement_description": 2,
    "index_text": 2,
    "index_title": 3,
    "manifest_group": 6,
    "manifest_page": 28,
    "openapi_summary": 61,
    "permission_description": 73,
    "product_name": 2,
    "product_page": 3,
    "schema_hint": 5,
    "schema_title": 13,
    "schema_unit": 1,
    "ui_attr": 112,
    "ui_text": 988,
}
EDGEC_REQUIRED_FILES = (
    "web/core/modules.manifest.yaml",
    "web/permissions/catalog.yaml",
    "web/openapi/core.yaml",
    "web/schemas/network.firewall.json",
    "web/schemas/network.interfaces.json",
    "web/schemas/system.time.json",
    "web/products/iems/openapi.extension.yaml",
    "web/products/dcu/openapi.extension.yaml",
    "web/products/iems/index.html",
    "web/products/dcu/index.html",
    "web/products/smart_box/index.html",
    "web/products/iems/product.yaml",
    "web/products/dcu/product.yaml",
    "web/dist/index.html",
)
EDGEC_REQUIRED_DIRS = (
    "web/core/src",
    "web/products/iems",
    "web/products/dcu",
    "web/products/smart_box",
    "web/schemas",
)
MSGSTR_MIN = 4
MSGID_MIN = 8
CLASSIFICATIONS = tuple(EXPECTED_CLASSIFICATION)
OWNER_PREFIXES = (
    ("web/core/", "core"),
    ("web/products/iems/", "iems"),
    ("web/products/dcu/", "dcu"),
    ("web/products/smart_box/", "smart_box"),
    ("web/permissions/", "shared"),
    ("web/schemas/", "shared"),
    ("web/openapi/", "shared"),
)
PLACEHOLDER_RE = re.compile(
    r"%(?:\d+\$)?[-+ #0]*\d*(?:\.\d+)?(?:hh|h|ll|l|L|z|j|t)?[diouxXeEfFgGaAcspn%]"
)
LITERAL_RE = re.compile(r'_\s*\(\s*"((?:[^"\\]|\\.)*)"\s*\)')
TEXT_SUFFIXES_EXCLUDED = (".po", ".mo")

KEYS_HEADER = "msgid\tmsgstr\tplaceholders\tclassification\tevidence\treverse_gap"
UI_HEADER = "file\tline\towner\tcategory\ttext"
MATCH_HEADER = "msg_key\tui_file\tui_line\tlength\tside"


class ValidationError(Exception):
    pass


def sha256_file(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read_text(path):
    # Bytes decode keeps CR-only bytes from inflating line numbers on binary files.
    return Path(path).read_bytes().decode("utf-8", errors="replace")


def normalize(text):
    return "".join(text.split())


def placeholders(text):
    return PLACEHOLDER_RE.findall(text)


def escape_tsv(text):
    return (
        text.replace("\\", "\\\\")
        .replace("\t", "\\t")
        .replace("\n", "\\n")
        .replace("\r", "\\r")
    )


def _unquote(value):
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
        return value[1:-1]
    return value


def parse_po(text):
    """Parse Poedit output; messages use one physical line per value here."""
    entries = []
    block = []
    start = 0
    for number, line in enumerate(text.splitlines(), 1):
        if not line.strip():
            if block:
                entries.append(_parse_entry(block, start))
                block = []
            continue
        if not block:
            start = number
        block.append(line)
    if block:
        entries.append(_parse_entry(block, start))
    return entries


def _parse_entry(lines, start):
    values = {"msgid": [], "msgstr": []}
    flags = []
    current = None
    msgid_line = start
    multiline = False
    for offset, line in enumerate(lines):
        if line.startswith("#"):
            if line.startswith("#,"):
                flags.append(line[2:].strip())
            current = None
            continue
        match = re.match(r'(msgid|msgstr)\s+"(.*)"$', line)
        if match:
            current = match.group(1)
            if current == "msgid" and not values["msgid"]:
                msgid_line = start + offset
            if values[current]:
                multiline = multiline or current == "msgid"
            values[current].append(match.group(2))
        elif line.startswith('"') and current:
            values[current].append(line[1:-1] if line.endswith('"') else line[1:])
            if current == "msgid":
                multiline = True
        else:
            current = None
    return {
        "msgid": "".join(values["msgid"]),
        "msgstr": "".join(values["msgstr"]),
        "flags": flags,
        "line": msgid_line,
        "multiline": multiline,
    }


def messages_of(entries):
    # Every empty msgid is a po header; a second header is malformed and must not
    # pad the 615-message count.
    return [entry for entry in entries if entry["msgid"] != ""]


def validate_po(entries):
    errors = []
    messages = messages_of(entries)
    if len(entries) != EXPECTED_PO_ENTRIES:
        errors.append(f"entries {len(entries)} != {EXPECTED_PO_ENTRIES}")
    if len(messages) != EXPECTED_MESSAGES:
        errors.append(f"messages {len(messages)} != {EXPECTED_MESSAGES}")
    msgids = [message["msgid"] for message in messages]
    duplicates = sorted(msgid for msgid, count in Counter(msgids).items() if count > 1)
    if duplicates:
        errors.append(f"duplicate msgid: {duplicates[:3]}")
    empty = sum(1 for message in messages if not message["msgstr"])
    if empty != 0:
        errors.append(f"empty msgstr {empty} != 0")
    c_format = [message for message in messages if "c-format" in message["flags"]]
    if len(c_format) != EXPECTED_C_FORMAT:
        errors.append(f"c-format {len(c_format)} != {EXPECTED_C_FORMAT}")
    for message in c_format:
        if placeholders(message["msgid"]) != placeholders(message["msgstr"]):
            errors.append(f"c-format placeholder mismatch: {message['msgid']!r}")
    multiline = sum(1 for message in messages if message["multiline"])
    if multiline != EXPECTED_MULTILINE:
        errors.append(f"multiline {multiline} != {EXPECTED_MULTILINE}")
    equal = sum(1 for message in messages if message["msgid"] == message["msgstr"])
    if equal != EXPECTED_EQUAL:
        errors.append(f"msgid==msgstr {equal} != {EXPECTED_EQUAL}")
    if errors:
        raise ValidationError("; ".join(errors))
    return {
        "empty_msgstr": empty,
        "c_format": len(c_format),
        "multiline": multiline,
        "msgid_eq_msgstr": equal,
    }


def _relative_files(root, suffixes=None):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = sorted(name for name in dirnames if name != ".git")
        for name in sorted(filenames):
            path = Path(dirpath) / name
            if suffixes and path.suffix not in suffixes:
                continue
            yield path


def _read_text_corpus(root):
    c_files = []
    other_files = []
    for path in _relative_files(root):
        if path.suffix in TEXT_SUFFIXES_EXCLUDED:
            continue
        data = path.read_bytes()
        if b"\x00" in data:
            continue
        rel = path.relative_to(root).as_posix()
        pair = (rel, data.decode("utf-8", errors="replace"))
        if path.suffix in (".c", ".h"):
            c_files.append(pair)
        else:
            other_files.append(pair)
    return c_files, other_files


def _line_of(text, index):
    return text.count("\n", 0, index) + 1


def _text_line(text, match):
    value = match.group(1)
    return _line_of(text, match.start(1) + len(value) - len(value.lstrip()))


def classify_messages(messages, root):
    c_files, other_files = _read_text_corpus(root)
    binary = read_text(root / OLD_BINARY)
    rows = []
    for message in messages:
        msgid = message["msgid"]
        classification = "MISSING"
        evidence = f"{PO_REL}:{message['line']}"
        for rel, text in c_files:
            index = text.find(msgid)
            if index >= 0:
                classification = "C_SOURCE"
                evidence = f"{rel}:{_line_of(text, index)}"
                break
        if classification == "MISSING":
            for rel, text in other_files:
                index = text.find(msgid)
                if index >= 0:
                    classification = "OTHER_TEXT"
                    evidence = f"{rel}:{_line_of(text, index)}"
                    break
        if classification == "MISSING":
            index = binary.find(msgid)
            if index >= 0:
                classification = "BINARY_ONLY"
                evidence = f"{OLD_BINARY}:{_line_of(binary, index)}"
        rows.append(
            {
                "msgid": msgid,
                "msgstr": message["msgstr"],
                "placeholders": ",".join(placeholders(msgid)),
                "classification": classification,
                "evidence": evidence,
                "reverse_gap": "no",
            }
        )
    return rows, dict(Counter(row["classification"] for row in rows))


def validate_evidence(rows):
    errors = []
    po_text = read_text(PO_PATH)
    po_lines = po_text.splitlines()
    po_line_of = {
        message["msgid"]: message["line"] for message in messages_of(parse_po(po_text))
    }
    binary = read_text(IEMS_ROOT / OLD_BINARY)
    texts = {}

    def text_of(rel):
        if rel not in texts:
            texts[rel] = read_text(IEMS_ROOT / rel)
        return texts[rel]

    for row in rows:
        path, _, line_text = row["evidence"].rpartition(":")
        line = int(line_text)
        if row["classification"] == "MISSING":
            if po_line_of.get(row["msgid"]) != line:
                errors.append(
                    f"MISSING evidence {row['evidence']} is not a po msgid line"
                )
            continue
        text = binary if row["classification"] == "BINARY_ONLY" else text_of(path)
        index = text.find(row["msgid"])
        if index < 0:
            errors.append(
                f"{row['classification']} evidence {row['evidence']} lacks {row['msgid']!r}"
            )
        elif _line_of(text, index) != line:
            errors.append(
                f"{row['classification']} evidence {row['evidence']} is line {_line_of(text, index)}"
            )
    if errors:
        raise ValidationError("; ".join(errors))


def validate_key_rows(rows):
    errors = []
    msgids = [row["msgid"] for row in rows]
    duplicates = sorted(
        msgid for msgid, count in Counter(msgids).items() if count > 1
    )
    if duplicates:
        errors.append(f"duplicate key rows: {duplicates[:3]}")
    counted = dict(Counter(row["classification"] for row in rows))
    if counted != EXPECTED_CLASSIFICATION:
        delta = {
            name: counted.get(name, 0) - EXPECTED_CLASSIFICATION.get(name, 0)
            for name in sorted(set(counted) | set(EXPECTED_CLASSIFICATION))
        }
        delta = {name: value for name, value in delta.items() if value}
        errors.append(
            f"classification counts {counted} != expected {EXPECTED_CLASSIFICATION} (delta {delta})"
        )
    valid_rows = []
    for row in rows:
        classification = row["classification"]
        match = re.match(r"^(.+):(\d+)$", row["evidence"])
        if not match:
            errors.append(f"evidence is not file:line: {row['evidence']!r}")
            continue
        path = match.group(1)
        if classification not in CLASSIFICATIONS:
            continue
        if classification == "C_SOURCE" and not path.endswith((".c", ".h")):
            errors.append(f"C_SOURCE evidence is not C source: {path}")
            continue
        if classification == "OTHER_TEXT" and (
            path.endswith((".c", ".h")) or path == OLD_BINARY
        ):
            errors.append(f"OTHER_TEXT evidence is C or binary: {path}")
            continue
        if classification == "BINARY_ONLY" and path != OLD_BINARY:
            errors.append(f"BINARY_ONLY evidence is not the old binary: {path}")
            continue
        if classification == "MISSING" and path != PO_REL:
            errors.append(f"MISSING evidence is not the po: {path}")
            continue
        valid_rows.append(row)
    if errors:
        raise ValidationError("; ".join(errors))
    validate_evidence(valid_rows)


def find_reverse_gaps(messages, root):
    known = {message["msgid"] for message in messages}
    first = {}
    for path in _relative_files(root / "src", (".c", ".h")):
        text = read_text(path)
        rel = path.relative_to(root).as_posix()
        for match in LITERAL_RE.finditer(text):
            literal = match.group(1)
            if literal in known or literal in first:
                continue
            first[literal] = (rel, _line_of(text, match.start(1)))
    gaps = [
        {"file": file, "line": line, "text": literal}
        for literal, (file, line) in first.items()
    ]
    gaps.sort(key=lambda gap: (gap["file"], gap["line"], gap["text"]))
    return gaps


def validate_reverse_gaps(gaps):
    if len(gaps) != EXPECTED_REVERSE_GAPS:
        raise ValidationError(f"reverse gaps {len(gaps)} != {EXPECTED_REVERSE_GAPS}")


def read_mo_count(path):
    data = Path(path).read_bytes()
    if len(data) < 16:
        raise ValidationError(f"{path} is too short for a GNU mo header")
    if data[:4] in (b"\xde\x12\x04\x95", b"\x95\x04\x12\xde"):
        endian = "<" if data[:4] == b"\xde\x12\x04\x95" else ">"
    else:
        raise ValidationError(f"{path} is not a GNU mo file")
    _, count, _ = struct.unpack(endian + "III", data[4:16])
    return count


def alarminfo_counts(root):
    def load(rel):
        data = json.loads(read_text(root / rel))
        keys = set()
        tags = 0
        for vendor_model in data["vendor_models"]:
            for tag in vendor_model["tags"]:
                tags += 1
                keys.add(
                    (
                        vendor_model["type"],
                        vendor_model["vendor"],
                        vendor_model["model"],
                        tag["tag"],
                        tag["alarmCode"],
                        tag["reasonCode"],
                    )
                )
        return data, keys, tags

    zh, zh_keys, zh_tags = load(ALARM_ZH)
    en, en_keys, en_tags = load(ALARM_EN)
    return {
        "zh_vendor_models": len(zh["vendor_models"]),
        "en_vendor_models": len(en["vendor_models"]),
        "zh_tags": zh_tags,
        "en_tags": en_tags,
        "zh_only": len(zh_keys - en_keys),
        "en_only": len(en_keys - zh_keys),
    }


def validate_alarm(alarm):
    if alarm != EXPECTED_ALARM:
        raise ValidationError(f"alarminfo {alarm} != {EXPECTED_ALARM}")


def _blank(match):
    return "\n" * match.group(0).count("\n")


def _strip_markup(text):
    text = re.sub(r"<!--.*?-->", _blank, text, flags=re.S)
    return re.sub(r"<(script|style)\b.*?</\1>", _blank, text, flags=re.S | re.I)


def _text_nodes(text):
    """Yield (start, end) spans outside tags; a '>' in text is literal."""
    index, size = 0, len(text)
    while index < size:
        start = text.find("<", index)
        if start < 0:
            yield index, size
            return
        if start > index:
            yield index, start
        if text.startswith("<!--", start):
            end = text.find("-->", start + 4)
            index = size if end < 0 else end + 3
            continue
        cursor = start + 1
        quote = None
        while cursor < size:
            char = text[cursor]
            if quote:
                if char == quote:
                    quote = None
            elif char in "\"'":
                quote = char
            elif char == ">":
                break
            cursor += 1
        index = cursor + 1


def _span_line(text, start, end):
    raw = text[start:end]
    return _line_of(text, start + len(raw) - len(raw.lstrip()))


def _add_ui(rows, rel, line, owner, category, text):
    text = " ".join(text.split())
    if text:
        rows.append(
            {"file": rel, "line": line, "owner": owner, "category": category, "text": text}
        )


def _extract_index(rows, root, product):
    rel = f"web/products/{product}/index.html"
    text = _strip_markup(read_text(root / rel))
    for match in re.finditer(r"<title>(.*?)</title>", text, flags=re.S | re.I):
        _add_ui(rows, rel, _text_line(text, match), product, "index_title", match.group(1))
        text = text[: match.start(1)] + " " * (match.end(1) - match.start(1)) + text[match.end(1):]
    for start, end in _text_nodes(text):
        _add_ui(
            rows,
            rel,
            _span_line(text, start, end),
            product,
            "index_text",
            text[start:end],
        )


def _extract_manifest(rows, root):
    rel = "web/core/modules.manifest.yaml"
    for line_number, line in enumerate(
        read_text(root / rel).splitlines(), 1
    ):
        group = re.match(r"^\s*-\s*\{\s*id:\s*[^,]+,\s*title:\s*([^},]+)\s*\}", line)
        if group:
            _add_ui(rows, rel, line_number, "core", "manifest_group", group.group(1))
        page = re.match(r"^ {8}title:\s*(.+?)\s*$", line)
        if page:
            _add_ui(rows, rel, line_number, "core", "manifest_page", page.group(1))


def _extract_permissions(rows, root):
    rel = "web/permissions/catalog.yaml"
    for line_number, line in enumerate(
        read_text(root / rel).splitlines(), 1
    ):
        match = re.match(r"^\s+description:\s*(.+?)\s*$", line)
        if match:
            _add_ui(rows, rel, line_number, "shared", "permission_description", match.group(1))


def _extract_product(rows, root, product):
    rel = f"web/products/{product}/product.yaml"
    section = None
    for line_number, line in enumerate(
        read_text(root / rel).splitlines(), 1
    ):
        if line and not line.startswith((" ", "#")):
            section = line.split(":", 1)[0]
        match = re.match(r"^name:\s*(.+?)\s*$", line)
        if match:
            _add_ui(rows, rel, line_number, product, "product_name", match.group(1))
        match = re.match(r"^ {4}title:\s*(.+?)\s*$", line)
        if match and section == "pages":
            _add_ui(rows, rel, line_number, product, "product_page", match.group(1))
        match = re.match(r"^\s+-\s*\{.*?\bdescription:\s*([^,}]+)", line)
        if match:
            _add_ui(
                rows, rel, line_number, product, "permission_description", match.group(1)
            )
        match = re.match(r"^ {4}description:\s*(.+?)\s*$", line)
        if match and section == "entitlements":
            _add_ui(
                rows, rel, line_number, product, "entitlement_description", match.group(1)
            )


def _extract_schemas(rows, root):
    paths = sorted((root / "web/schemas").glob("*.json"))
    if not paths:
        raise ValidationError(f"no schema json under {root / 'web/schemas'}")
    for path in paths:
        rel = path.relative_to(root).as_posix()
        raw = read_text(path)
        for key, category in (
            ("title", "schema_title"),
            ("hint", "schema_hint"),
            ("unit", "schema_unit"),
        ):
            pattern = re.compile(r'"%s"\s*:\s*"((?:[^"\\]|\\.)*)"' % key)
            for match in pattern.finditer(raw):
                value = json.loads('"' + match.group(1) + '"')
                _add_ui(rows, rel, _line_of(raw, match.start()), "shared", category, value)


def _extract_openapi(rows, root):
    sources = (
        ("web/openapi/core.yaml", "shared"),
        ("web/products/iems/openapi.extension.yaml", "iems"),
        ("web/products/dcu/openapi.extension.yaml", "dcu"),
    )
    for rel, owner in sources:
        path = root / rel
        if not path.is_file():
            raise ValidationError(f"missing OpenAPI source {rel}")
        for line_number, line in enumerate(read_text(path).splitlines(), 1):
            match = re.match(r"^\s*summary:\s*(.+?)\s*$", line)
            if match:
                _add_ui(
                    rows,
                    rel,
                    line_number,
                    owner,
                    "openapi_summary",
                    _unquote(match.group(1)),
                )


def _extract_vue(rows, root):
    attribute = re.compile(
        r'(?<![:\w-])(?:title|placeholder|label|alt|aria-label)="([^"]*)"'
    )
    roots = (
        ("web/core/src", "core"),
        ("web/products/iems", "iems"),
        ("web/products/dcu", "dcu"),
        ("web/products/smart_box", "smart_box"),
    )
    for base, owner in roots:
        for path in sorted((root / base).rglob("*.vue")):
            rel = path.relative_to(root).as_posix()
            if any(part in ("generated", "dist", "node_modules") for part in rel.split("/")):
                continue
            text = _strip_markup(read_text(path))
            for start, end in _text_nodes(text):
                value = text[start:end]
                if "{{" in value or "}}" in value:
                    continue
                _add_ui(rows, rel, _span_line(text, start, end), owner, "ui_text", value)
            for match in attribute.finditer(text):
                _add_ui(rows, rel, _text_line(text, match), owner, "ui_attr", match.group(1))


def extract_ui_strings(root):
    rows = []
    for product in ("iems", "dcu", "smart_box"):
        _extract_index(rows, root, product)
    _extract_manifest(rows, root)
    _extract_permissions(rows, root)
    for product in ("iems", "dcu"):
        _extract_product(rows, root, product)
    _extract_schemas(rows, root)
    _extract_openapi(rows, root)
    _extract_vue(rows, root)
    rows.sort(key=lambda row: (row["file"], row["line"], row["category"], row["text"]))
    validate_ui_rows(rows, root)
    return rows


def derive_owner(rel):
    for prefix, owner in OWNER_PREFIXES:
        if rel.startswith(prefix):
            return owner
    return None


def validate_ui_rows(rows, root):
    errors = []
    for row in rows:
        parts = row["file"].split("/")
        if any(part in ("dist", "generated", "node_modules") for part in parts):
            errors.append(f"excluded UI source path {row['file']}")
            continue
        owner = derive_owner(row["file"])
        if owner is None:
            errors.append(f"unexpected UI source path {row['file']}")
            continue
        if row["owner"] != owner:
            errors.append(
                f"owner {row['owner']!r} != path-derived {owner!r} for {row['file']}"
            )
        if row["line"] <= 0:
            errors.append(f"bad line {row['line']} in {row['file']}")
        if not (root / row["file"]).is_file():
            errors.append(f"missing source {row['file']}")
    counted = dict(Counter(row["category"] for row in rows))
    if counted != EXPECTED_UI_CATEGORIES:
        delta = {
            name: counted.get(name, 0) - EXPECTED_UI_CATEGORIES.get(name, 0)
            for name in sorted(set(counted) | set(EXPECTED_UI_CATEGORIES))
        }
        delta = {name: value for name, value in delta.items() if value}
        errors.append(
            f"UI category counts {counted} != expected {EXPECTED_UI_CATEGORIES} (delta {delta})"
        )
    stale = root / "web/dist/index.html"
    if not stale.is_file() or 'lang="en"' not in read_text(stale):
        errors.append(
            "web/dist/index.html is missing or no longer lang=en, revisit exclusion"
        )
    if errors:
        raise ValidationError("; ".join(errors))


def is_hit(haystack, needle, threshold):
    needle = normalize(needle)
    return len(needle) >= threshold and needle in normalize(haystack)


def match_ui(ui_rows, messages):
    matches = []
    key_hits = set()
    seen = set()
    for ui in ui_rows:
        for message in messages:
            for side, needle, threshold in (
                ("msgstr", message["msgstr"], MSGSTR_MIN),
                ("msgid", message["msgid"], MSGID_MIN),
            ):
                if not is_hit(ui["text"], needle, threshold):
                    continue
                if side == "msgstr":
                    key_hits.add((message["msgid"], ui["file"], ui["line"]))
                key = (ui["file"], ui["line"], side, normalize(needle))
                if key in seen:
                    continue
                seen.add(key)
                matches.append(
                    {
                        "msg_key": message["msgid"],
                        "ui_file": ui["file"],
                        "ui_line": ui["line"],
                        "length": len(normalize(needle)),
                        "side": side,
                    }
                )
    matches.sort(key=lambda row: (row["ui_file"], row["ui_line"], row["side"], row["msg_key"]))
    return matches, key_hits


def shared_msgstr_keys(key_hits):
    positions = {}
    for msgid, file, line in key_hits:
        positions.setdefault((file, line), set()).add(msgid)
    return [
        {"file": file, "line": line, "keys": sorted(keys)}
        for (file, line), keys in sorted(positions.items())
        if len(keys) > 1
    ]


def _position_keys(rows):
    positions = {}
    for file, line, msg_key in rows:
        positions.setdefault((file, line), set()).add(msg_key)
    return positions


def validate_matches(matches, key_hits):
    errors = []
    msgstr_hits = [match for match in matches if match["side"] == "msgstr"]
    msgid_hits = [match for match in matches if match["side"] == "msgid"]
    msgstr_positions = {(match["ui_file"], match["ui_line"]) for match in msgstr_hits}
    if msgstr_positions != set(EXPECTED_MSGSTR_HITS):
        errors.append(
            f"msgstr match positions {sorted(msgstr_positions)} != {sorted(EXPECTED_MSGSTR_HITS)}"
        )
    msgstr_keys = _position_keys(
        (file, line, msg_id) for msg_id, file, line in key_hits
    )
    for position in sorted(set(msgstr_keys) | set(EXPECTED_MSGSTR_HITS)):
        actual = msgstr_keys.get(position, set())
        expected = EXPECTED_MSGSTR_HITS.get(position, frozenset())
        if actual != expected:
            errors.append(
                f"msgstr keys at {position[0]}:{position[1]} "
                f"{sorted(actual)} != {sorted(expected)}"
            )
    if any(match["length"] >= 8 for match in msgstr_hits):
        errors.append("msgstr matches at length >= 8 must be 0")
    msgid_keys = _position_keys(
        (match["ui_file"], match["ui_line"], match["msg_key"])
        for match in msgid_hits
    )
    if set(msgid_keys) != set(EXPECTED_MSGID_HITS):
        errors.append(
            f"msgid match positions {sorted(msgid_keys)} != {sorted(EXPECTED_MSGID_HITS)}"
        )
    for position in sorted(set(msgid_keys) & set(EXPECTED_MSGID_HITS)):
        if msgid_keys[position] != EXPECTED_MSGID_HITS[position]:
            errors.append(
                f"msgid keys at {position[0]}:{position[1]} "
                f"{sorted(msgid_keys[position])} != {sorted(EXPECTED_MSGID_HITS[position])}"
            )
    if errors:
        raise ValidationError("; ".join(errors))
    return msgstr_hits, msgid_hits


def render_keys_tsv(key_rows, reverse_rows):
    lines = [KEYS_HEADER]
    for row in key_rows:
        lines.append(
            "\t".join(
                (
                    escape_tsv(row["msgid"]),
                    escape_tsv(row["msgstr"]),
                    escape_tsv(row["placeholders"]),
                    row["classification"],
                    row["evidence"],
                    row["reverse_gap"],
                )
            )
        )
    for gap in reverse_rows:
        lines.append(
            "\t".join(
                (
                    escape_tsv(gap["text"]),
                    "",
                    escape_tsv(",".join(placeholders(gap["text"]))),
                    "-",
                    f"{gap['file']}:{gap['line']}",
                    "yes",
                )
            )
        )
    return "\n".join(lines) + "\n"


def render_ui_tsv(ui_rows):
    lines = [UI_HEADER]
    for row in ui_rows:
        lines.append(
            "\t".join(
                (
                    row["file"],
                    str(row["line"]),
                    row["owner"],
                    row["category"],
                    escape_tsv(row["text"]),
                )
            )
        )
    return "\n".join(lines) + "\n"


def render_matches_tsv(matches):
    lines = [MATCH_HEADER]
    for row in matches:
        lines.append(
            "\t".join(
                (
                    escape_tsv(row["msg_key"]),
                    row["ui_file"],
                    str(row["ui_line"]),
                    str(row["length"]),
                    row["side"],
                )
            )
        )
    return "\n".join(lines) + "\n"


def _git(root, *args):
    return subprocess.run(
        ["git", "-C", str(root), *args],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()


def check_edgec_inputs(root):
    missing = [
        f"missing EdgeC input {rel}"
        for rel in EDGEC_REQUIRED_FILES
        if not (root / rel).is_file()
    ]
    missing += [
        f"missing EdgeC input dir {rel}"
        for rel in EDGEC_REQUIRED_DIRS
        if not (root / rel).is_dir()
    ]
    if missing:
        raise ValidationError("; ".join(missing))


def check_inputs():
    errors = []
    if not IEMS_ROOT.is_dir():
        raise ValidationError(f"missing input tree {IEMS_ROOT}")
    for rel in (PO_REL, EMPTY_PO, UPDATE_PO, OLD_BINARY, MO_FIRMWARE, MO_PATCH, ALARM_ZH, ALARM_EN):
        if not (IEMS_ROOT / rel).is_file():
            errors.append(f"missing {IEMS_ROOT / rel}")
    try:
        sha = _git(IEMS_ROOT, "rev-parse", "HEAD")
        status = _git(IEMS_ROOT, "status", "--porcelain")
        edgec_sha = _git(REPO_ROOT, "rev-parse", "HEAD")
        web_status = _git(REPO_ROOT, "status", "--porcelain", "--", "web")
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ValidationError(f"git input check failed: {exc}") from exc
    if sha != IEMS_SHA:
        errors.append(f"iems_v3 HEAD {sha} != pinned {IEMS_SHA}")
    if status:
        errors.append(f"iems_v3 is dirty: {status.splitlines()[:3]}")
    if web_status:
        errors.append(f"EdgeC web/ is dirty: {web_status.splitlines()[:3]}")
    if errors:
        raise ValidationError("; ".join(errors))
    check_edgec_inputs(REPO_ROOT)
    return {
        "iems_v3": {"path": str(IEMS_ROOT), "sha": sha, "clean": True},
        "edgec": {"path": str(REPO_ROOT), "sha": edgec_sha, "web_clean": True},
    }


def validate_auxiliary(empty_po_size, update_po_text, mo_counts):
    errors = []
    if empty_po_size != 0:
        errors.append(f"{EMPTY_PO} is not 0 bytes")
    if "PO_PATH=../files" not in update_po_text:
        errors.append(f"{UPDATE_PO} no longer points at ../files")
    if mo_counts != EXPECTED_MO:
        errors.append(f"mo headers {mo_counts} != {EXPECTED_MO}")
    if errors:
        raise ValidationError("; ".join(errors))
    return mo_counts


def validate_ancillary():
    return validate_auxiliary(
        (IEMS_ROOT / EMPTY_PO).stat().st_size,
        read_text(IEMS_ROOT / UPDATE_PO),
        {rel: read_mo_count(IEMS_ROOT / rel) for rel in (MO_FIRMWARE, MO_PATCH)},
    )


def collect_input_hashes(ui_rows):
    paths = [PO_REL, EMPTY_PO, UPDATE_PO, OLD_BINARY, MO_FIRMWARE, MO_PATCH, ALARM_ZH, ALARM_EN]
    paths += sorted({row["file"] for row in ui_rows})
    paths.append("web/dist/index.html")
    return {rel: sha256_file(REPO_ROOT / rel) if rel.startswith("web/") else sha256_file(IEMS_ROOT / rel) for rel in paths}


def render_report(summary, key_rows, reverse_rows, ui_rows, matches, shared):
    lines = [
        "# P6-05b b1-a i18n inventory",
        "",
        f"- run_id: {summary['run_id']}",
        f"- generated_at: {summary['generated_at']}",
        f"- iems_v3: {summary['inputs']['iems_v3']['sha']} (clean)",
        f"- EdgeC web HEAD: {summary['inputs']['edgec']['sha']} (web clean)",
        "",
        "## Counts",
        "",
        "| item | value |",
        "| --- | --- |",
        f"| po entries / messages | {summary['counts']['po_entries']} / {summary['counts']['po_messages']} |",
        f"| classification C / other / binary / missing | {summary['counts']['classification']['C_SOURCE']} / {summary['counts']['classification']['OTHER_TEXT']} / {summary['counts']['classification']['BINARY_ONLY']} / {summary['counts']['classification']['MISSING']} |",
        f"| reverse gaps | {summary['counts']['reverse_gaps']} |",
        f"| empty msgstr / c-format / multiline / msgid==msgstr | {summary['counts']['empty_msgstr']} / {summary['counts']['c_format']} / {summary['counts']['multiline']} / {summary['counts']['msgid_eq_msgstr']} |",
        f"| mo headers firmware / patch | {summary['counts']['mo_firmware']} / {summary['counts']['mo_patch']} |",
        f"| EdgeC UI strings | {summary['counts']['edgec_ui_strings']} |",
        f"| exact msgstr matches >= {MSGSTR_MIN} (positions / keys) | {summary['counts']['msgstr_matches_positions']} / {summary['counts']['msgstr_matches_keys']} |",
        f"| exact msgstr matches >= 8 | {summary['counts']['msgstr_matches_ge8']} |",
        f"| exact msgid matches >= {MSGID_MIN} | {summary['counts']['msgid_matches']} |",
        "",
        "## po checks",
        "",
        "All of: 616 entries / 615 messages, 0 empty msgstr, 6 c-format entries with identical placeholder order, 2 multiline msgids, 2 msgid==msgstr, mo headers 526/426, files/zh_CN.po 0 B, update_po.sh PO_PATH=../files.",
        "",
        "## Classification",
        "",
        "Evidence is the first occurrence in sorted path order. MISSING rows point at their own po line because no source occurrence exists; BINARY_ONLY rows point into the old precompiled proto_forward with a byte-derived line number.",
        "",
        "| classification | count |",
        "| --- | --- |",
    ]
    for name in CLASSIFICATIONS:
        lines.append(f"| {name} | {summary['counts']['classification'][name]} |")
    lines += [
        "",
        "## Reverse gaps (source `_()` literals not in po)",
        "",
        "`i18n_keys.tsv` appends these 19 rows after the 615 forward rows: forward rows carry `reverse_gap=no` and one of the four classifications; reverse rows carry `reverse_gap=yes`, classification `-`, an empty msgstr, and their source `file:line`. The 615-message uniqueness invariant counts forward rows only.",
        "",
        "| file:line | literal |",
        "| --- | --- |",
    ]
    for gap in reverse_rows:
        lines.append(f"| {gap['file']}:{gap['line']} | {escape_tsv(gap['text'])} |")
    lines += [
        "",
        "## EdgeC UI strings",
        "",
        "Sources: product index.html titles/text, manifest group/page titles, permission and entitlement descriptions, schema title/hint/unit, OpenAPI summaries, and .vue template text/static attributes under core/src and the three product trees.",
        "",
        "| owner | count |",
        "| --- | --- |",
    ]
    for owner, count in sorted(Counter(row["owner"] for row in ui_rows).items()):
        lines.append(f"| {owner} | {count} |")
    lines += ["", "| category | count |", "| --- | --- |"]
    for category, count in sorted(Counter(row["category"] for row in ui_rows).items()):
        lines.append(f"| {category} | {count} |")
    lines += [
        "",
        "Excluded: `web/dist/index.html` (stale, lang=en), `.ts`/`.js` scripts, comments, CSS, testid attributes, generated, dist, node_modules, and mock data. Page titles come from the manifest.",
        "",
        "## Containment matches",
        "",
        "Exact whitespace-stripped containment. Thresholds: msgstr >= 4 characters, msgid >= 8. No fuzzy or semantic merge.",
        "",
        f"msgstr containment: {summary['counts']['msgstr_matches_positions']} positions / {summary['counts']['msgstr_matches_keys']} keys. Position-level matching records one row per (UI line, side, normalized needle), so keys sharing a msgstr on one line collapse to the first po key. Key-level matching counts every po key separately.",
        "",
    ]
    if shared:
        lines.append(
            "Shared msgstr positions: "
            + "; ".join(
                f"{', '.join(group['keys'])} at `{group['file']}:{group['line']}`"
                for group in shared
            )
            + "."
        )
        lines.append("")
    lines += [
        "| msg_key | ui_file:line | length | side |",
        "| --- | --- | --- | --- |",
    ]
    for row in matches:
        lines.append(
            f"| {escape_tsv(row['msg_key'])} | {row['ui_file']}:{row['ui_line']} | {row['length']} | {row['side']} |"
        )
    lines += [
        "",
        "## Alarminfo",
        "",
        f"vendor/model zh/en: {summary['alarminfo']['zh_vendor_models']}/{summary['alarminfo']['en_vendor_models']}; tags: {summary['alarminfo']['zh_tags']}/{summary['alarminfo']['en_tags']}; single-language only: {summary['alarminfo']['zh_only']}/{summary['alarminfo']['en_only']}.",
        "",
        "## Outputs (sha256)",
        "",
    ]
    for name, digest in summary["outputs"].items():
        lines.append(f"- `{name}`: `{digest}`")
    lines += [
        "",
        "## Scope",
        "",
        "Read-only inventory. Key completeness is not a gate: the 615 messages are classified one by one with evidence. No vue-i18n install, no locale files, no `$t()` edits, no P6-05c decision, no page-by-page b2 conclusion.",
        "",
    ]
    return "\n".join(lines)


def run_check(out_dir, run_id=None):
    out_dir = Path(out_dir)
    if run_id is None:
        run_id = out_dir.name or datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    inputs = check_inputs()
    entries = parse_po(read_text(PO_PATH))
    po_stats = validate_po(entries)
    messages = messages_of(entries)
    key_rows, counts = classify_messages(messages, IEMS_ROOT)
    validate_key_rows(key_rows)
    reverse_rows = find_reverse_gaps(messages, IEMS_ROOT)
    validate_reverse_gaps(reverse_rows)
    mo_counts = validate_ancillary()
    alarm = alarminfo_counts(IEMS_ROOT)
    validate_alarm(alarm)
    ui_rows = extract_ui_strings(REPO_ROOT)
    matches, key_hits = match_ui(ui_rows, messages)
    msgstr_hits, msgid_hits = validate_matches(matches, key_hits)
    shared = shared_msgstr_keys(key_hits)

    contents = {
        "i18n_keys.tsv": render_keys_tsv(key_rows, reverse_rows),
        "edgec_ui_strings.tsv": render_ui_tsv(ui_rows),
        "matches.tsv": render_matches_tsv(matches),
    }
    out_dir.mkdir(parents=True, exist_ok=True)
    for name, text in contents.items():
        (out_dir / name).write_text(text, encoding="utf-8")
    outputs = {
        name: hashlib.sha256(text.encode("utf-8")).hexdigest()
        for name, text in contents.items()
    }
    summary = {
        "run_id": run_id,
        "generated_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "ok": True,
        "inputs": inputs,
        "input_sha256": collect_input_hashes(ui_rows),
        "counts": {
            "po_entries": len(entries),
            "po_messages": len(messages),
            "key_rows": len(key_rows),
            "table_rows": len(key_rows) + len(reverse_rows),
            "classification": counts,
            "reverse_gaps": len(reverse_rows),
            "empty_msgstr": po_stats["empty_msgstr"],
            "c_format": po_stats["c_format"],
            "multiline": po_stats["multiline"],
            "msgid_eq_msgstr": po_stats["msgid_eq_msgstr"],
            "mo_firmware": mo_counts[MO_FIRMWARE],
            "mo_patch": mo_counts[MO_PATCH],
            "edgec_ui_strings": len(ui_rows),
            "msgstr_matches_positions": len(msgstr_hits),
            "msgstr_matches_keys": len(key_hits),
            "msgstr_matches_ge8": sum(1 for match in msgstr_hits if match["length"] >= 8),
            "msgid_matches": len(msgid_hits),
        },
        "placeholders": {"c_format": po_stats["c_format"], "consistent": po_stats["c_format"]},
        "alarminfo": alarm,
        "excluded": [
            "web/dist/index.html (stale, lang=en)",
            ".ts/.js scripts, comments, CSS, testid attributes, generated, dist, node_modules, mock",
        ],
        "outputs": outputs,
    }
    (out_dir / "summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    (out_dir / "report.md").write_text(
        render_report(summary, key_rows, reverse_rows, ui_rows, matches, shared),
        encoding="utf-8",
    )
    print(
        "i18n_inventory: ok "
        f"keys={len(key_rows)} C={counts['C_SOURCE']} OTHER={counts['OTHER_TEXT']} "
        f"BIN={counts['BINARY_ONLY']} MISS={counts['MISSING']} reverse={len(reverse_rows)} "
        f"ui={len(ui_rows)} msgstr_positions={len(msgstr_hits)} "
        f"msgstr_keys={len(key_hits)} msgid_matches={len(msgid_hits)} out={out_dir}"
    )
    return summary


def run_selftest():
    result = subprocess.run(
        [sys.executable, str(TOOL_DIR / "test_i18n_inventory.py")]
    )
    return 0 if result.returncode == 0 else 1


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate inputs and write the inventory")
    parser.add_argument("--selftest", action="store_true", help="run the self-test")
    parser.add_argument("--out", help="output directory (default: tools/migration/i18n/reports/<run-id>)")
    args = parser.parse_args(argv)
    if args.selftest:
        return run_selftest()
    if not args.check:
        parser.error("one of --check or --selftest is required")
    if args.out:
        out_dir = Path(args.out)
        run_id = out_dir.name or None
    else:
        run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        out_dir = REPORTS_DIR / run_id
    try:
        run_check(out_dir, run_id=run_id)
    except ValidationError as exc:
        print(f"i18n_inventory: FAIL {exc}", file=sys.stderr)
        return 2
    except OSError as exc:
        print(f"i18n_inventory: FAIL {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
