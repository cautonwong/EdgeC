#!/usr/bin/env python3
"""Extract the DL/T 698.45 interface-class model and OI catalog from docs/698_parsed.md.

docs/698.pdf is the authoritative standard. docs/698_parsed.md is a derived text
extraction, and its OCR column shifts are the only source of class-assignment
conflicts. Fix the parsed text against the PDF, then rerun this script.

The catalog is the single source for cmd/dcu's object model. Regenerate it when the
parsed standard changes, and let --check fail when the checked-in catalog is stale.

Usage:
    python3 scripts/extract_dcu_698_catalog.py
    python3 scripts/extract_dcu_698_catalog.py --check
    python3 scripts/extract_dcu_698_catalog.py --check --root <dir>

--root 让门禁能对临时目录里的真实文档副本运行；默认是仓库根。
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def catalog_paths(root: Path) -> tuple[Path, Path, Path]:
    return (
        root / "docs" / "698_parsed.md",
        root / "docs" / "design" / "DCU_698_OI_CATALOG.md",
        root / "docs" / "design" / "DCU_698_OI_CATALOG.json",
    )

CLASS_HEAD = re.compile(r"7\.3\.(\d+)\s+(.*?)（class_id=(\d+)）")
ATTR = re.compile(r"^(\d+)[．.]\s*(.+?)\s*[（(](static|dyn\.?)[）)]\s*(.*)$")
METHOD = re.compile(r"^(\d+)[．.]\s*(.+?)(?:\s{2,}(必选|可选))?\s*$")
OI_ROW = re.compile(r"^([0-9A-Fa-f]{4})\s+(\d{1,2})\s+(.+)$")

# OIs whose class is not the obvious range owner. Values come from the class
# definitions in 7.3 and the object tables in 7.4 and Appendix A.
OI_CLASS_OVERRIDES = {
    0x4300: 19, 0x4307: 19, 0x4308: 19, 0x4309: 19,
    0x4400: 20,
    0x4500: 25, 0x4501: 25,
    0x4510: 26, 0x4511: 26, 0x4512: 26, 0x4513: 26,
    0x4514: 26, 0x4515: 26, 0x4516: 26, 0x4517: 26,
    0x3000: 24, 0x3001: 24, 0x3002: 24, 0x3003: 24, 0x3004: 24, 0x3005: 24,
    0x300B: 24,
    0x3300: 8, 0x3301: 8, 0x3302: 8,
}


def infer_class(oi: int) -> int:
    if oi in OI_CLASS_OVERRIDES:
        return OI_CLASS_OVERRIDES[oi]
    if oi < 0x1000:
        return 1
    if oi < 0x2000:
        return 2
    if oi <= 0x2003 or oi in (0x200B, 0x200C):
        return 3
    if 0x2004 <= oi <= 0x200A:
        return 4
    if oi in (0x200D, 0x200E):
        return 5
    if 0x200F <= oi <= 0x20FF:
        return 6
    if oi in (0x2140, 0x2141):
        return 2
    if 0x2100 <= oi <= 0x2104:
        return 14
    if 0x2110 <= oi <= 0x2114:
        return 15
    if 0x2120 <= oi <= 0x2124:
        return 16
    if 0x2301 <= oi <= 0x2308:
        return 23
    if 0x2401 <= oi <= 0x2408:
        return 12
    if 0x2105 <= oi <= 0x25FF:
        return 6
    if 0x3000 <= oi <= 0x3FFF:
        return 7
    if 0x4000 <= oi <= 0x4FFF:
        return 8
    if 0x5000 <= oi <= 0x5FFF:
        return 9
    if 0x6000 <= oi <= 0x6FFF:
        return 10
    if 0x7000 <= oi <= 0x7FFF:
        return 11
    if 0x8000 <= oi <= 0x8FFF:
        return 13
    if 0xF000 <= oi <= 0xF0FF:
        return 18
    if 0xF100 <= oi <= 0xF1FF:
        return 21
    if 0xF200 <= oi <= 0xF2FF:
        return 22
    if 0xF300 <= oi <= 0xF3FF:
        return 17
    return 0


def parse_classes(lines: list[str]) -> list[dict]:
    heads = []
    for i, line in enumerate(lines):
        m = CLASS_HEAD.search(line)
        if m:
            heads.append((i, int(m.group(1)), int(m.group(3)), m.group(2).strip()))

    classes = []
    for n, (start, seq, cid, name) in enumerate(heads):
        end = heads[n + 1][0] if n + 1 < len(heads) else min(len(lines), start + 320)
        section = lines[start:end]

        def_index = None
        attr_stop = None
        for j, line in enumerate(section):
            if "接口类定义" in line and def_index is None:
                def_index = j
            if "属性说明" in line and def_index is not None and j > def_index:
                attr_stop = j
                break
        block = section[(def_index or 0):(attr_stop or min(len(section), 110))]

        attrs: list[dict] = []
        methods: list[dict] = []
        mode = "attr"
        for line in block:
            text = line.strip()
            if text.startswith("方法"):
                mode = "meth"
                continue
            if text.startswith("属性") and "数据类型" in text:
                mode = "attr"
                continue
            m = ATTR.match(text)
            if m:
                attrs.append({
                    "no": int(m.group(1)),
                    "name": m.group(2).strip(),
                    "kind": m.group(3).rstrip("."),
                    "type": m.group(4).strip(),
                })
                continue
            m = METHOD.match(text)
            if m and mode == "meth" and not text.startswith("表"):
                methods.append({
                    "no": int(m.group(1)),
                    "name": m.group(2).strip(),
                    "requirement": m.group(3) or "",
                })

        classes.append({
            "sequence": seq,
            "class_id": cid,
            "name": name,
            "attributes": attrs,
            "methods": methods,
        })
    return classes


def parse_ois(lines: list[str], class_ids: set[int]) -> list[dict]:
    rows = []
    split_oi = re.compile(r"^([0-9A-Fa-f]{2})\s+([0-9A-Fa-f]{2})\s+(\d{1,2})\s+(.+)$")
    for i, line in enumerate(lines):
        if i < 6298 or i > 9450:
            continue
        text = line.strip()
        m = OI_ROW.match(text)
        if m:
            oi = int(m.group(1), 16)
            name = m.group(3).strip()
            parsed_class = int(m.group(2))
        else:
            m = split_oi.match(text)
            if not m:
                continue
            oi = int(m.group(1) + m.group(2), 16)
            name = m.group(4).strip()
            parsed_class = int(m.group(3))
        # The class column is the section a row belongs to. A row whose IC is
        # not one of the parsed 7.3 classes is OCR debris, not an object.
        if parsed_class not in class_ids or not name:
            continue
        rows.append({
            "oi": f"{oi:04X}",
            "name": name,
            "parsed_class": parsed_class,
            "inferred_class": infer_class(oi),
            "source_line": i + 1,
        })
    return rows


def render(classes: list[dict], ois: list[dict]) -> str:
    by_class: dict[int, list[dict]] = {}
    for row in ois:
        by_class.setdefault(row["inferred_class"], []).append(row)

    out = []
    out.append("# DL/T 698.45 OI catalog")
    out.append("")
    out.append("> Generated by `scripts/extract_dcu_698_catalog.py` from `docs/698_parsed.md`.")
    out.append("> Do not edit by hand. Regenerate after the parsed standard changes.")
    out.append("")
    conflicts = [r for r in ois if r["parsed_class"] != r["inferred_class"]]
    out.append(f"Classes: {len(classes)}. OI rows: {len(ois)}. "
               f"Rows where the parsed IC and the range mapping disagree: {len(conflicts)}.")
    out.append("")
    out.append("`parsed_class` is the IC column as printed in the parsed table. "
               "`inferred_class` is the range mapping used by the DCU object model. "
               "Where they differ, the task DCU-D1-01 reconciles the row against 7.3 and "
               "Appendix A before the row enters code. The parsed table has known OCR "
               "column shifts in the 3xxx and 6xxx ranges, so neither column is taken as "
               "final without reconciliation.")
    out.append("")
    out.append("## Class summary")
    out.append("")
    out.append("| Class | Name | Attributes | Methods | OI rows | OI range |")
    out.append("| --- | --- | --- | --- | --- | --- |")
    for cls in classes:
        cid = cls["class_id"]
        rows = by_class.get(cid, [])
        if rows:
            first, last = rows[0]["oi"], rows[-1]["oi"]
            rng = first if first == last else f"{first}..{last}"
        else:
            rng = "-"
        out.append(f"| {cid} | {cls['name']} | {len(cls['attributes'])} | "
                   f"{len(cls['methods'])} | {len(rows)} | {rng} |")
    out.append("")
    for cls in classes:
        cid = cls["class_id"]
        out.append(f"## Class {cid} {cls['name']}")
        out.append("")
        out.append("| # | Attribute | Kind | Type |")
        out.append("| --- | --- | --- | --- |")
        for attr in cls["attributes"]:
            out.append(f"| {attr['no']} | {attr['name']} | {attr['kind']} | {attr['type']} |")
        out.append("")
        out.append("| # | Method | Requirement |")
        out.append("| --- | --- | --- |")
        for method in cls["methods"]:
            out.append(f"| {method['no']} | {method['name']} | {method['requirement'] or '-'} |")
        out.append("")
        rows = by_class.get(cid, [])
        if rows:
            out.append("| OI | Name | parsed_class | inferred_class | Source |")
            out.append("| --- | --- | --- | --- | --- |")
            for row in rows:
                out.append(f"| {row['oi']} | {row['name']} | {row['parsed_class']} | "
                           f"{row['inferred_class']} | {row['source_line']} |")
            out.append("")
    return "\n".join(out) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--root", default=None,
                        help="repository root holding docs/; defaults to the script's repo")
    args = parser.parse_args()

    root = Path(args.root).resolve() if args.root else ROOT
    source, out_md, out_json = catalog_paths(root)

    if not source.exists():
        print(f"missing source: {source}", file=sys.stderr)
        return 1

    lines = source.read_text(encoding="utf-8").splitlines()
    classes = parse_classes(lines)
    ois = parse_ois(lines, {cls["class_id"] for cls in classes})

    if len(classes) != 26:
        print(f"expected 26 classes, parsed {len(classes)}", file=sys.stderr)
        return 1
    if len(ois) < 400:
        print(f"expected at least 400 OI rows, parsed {len(ois)}", file=sys.stderr)
        return 1
    if not any(row["oi"] == "F100" and row["parsed_class"] == 21 for row in ois):
        print("expected F100 in class 21 (ESAM); it is missing or misfiled", file=sys.stderr)
        return 1

    markdown = render(classes, ois)
    payload = json.dumps({"classes": classes, "ois": ois}, ensure_ascii=False, indent=1) + "\n"

    if args.check:
        stale = []
        if not out_md.exists() or out_md.read_text(encoding="utf-8") != markdown:
            stale.append(str(out_md))
        if not out_json.exists() or out_json.read_text(encoding="utf-8") != payload:
            stale.append(str(out_json))
        for path in stale:
            print(f"stale: {path}", file=sys.stderr)
        return 1 if stale else 0

    out_md.parent.mkdir(parents=True, exist_ok=True)
    out_md.write_text(markdown, encoding="utf-8")
    out_json.write_text(payload, encoding="utf-8")
    print(f"wrote {out_md.relative_to(root)} ({len(classes)} classes, {len(ois)} OIs)")
    print(f"wrote {out_json.relative_to(root)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
