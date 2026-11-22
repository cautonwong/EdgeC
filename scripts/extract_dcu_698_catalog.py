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
from typing import NamedTuple

ROOT = Path(__file__).resolve().parent.parent


def catalog_paths(root: Path) -> tuple[Path, Path, Path, Path]:
    return (
        root / "docs" / "698_parsed.md",
        root / "docs" / "design" / "DCU_698_OI_CATALOG.md",
        root / "docs" / "design" / "DCU_698_OI_CATALOG.json",
        root / "tests" / "fixtures" / "dcu_698_catalog_expected.h",
    )


def reconciliation_path(root: Path) -> Path:
    return root / "docs" / "design" / "DCU_698_CLASS_RECONCILIATION.md"


CLASS_HEAD = re.compile(r"7\.3\.(\d+)\s+(.*?)（class_id=(\d+)）")
ATTR = re.compile(r"^(\d+)[．.]\s*(.+?)\s*[（(](static|dyn\.?)[）)]\s*(.*)$")
METHOD = re.compile(r"^(\d+)[．.]\s*(.+?)(?:\s{2,}(必选|可选))?\s*$")
OI_ROW = re.compile(r"^([0-9A-Fa-f]{4})\s+(\d{1,2})\s+(.+)$")
METHOD_IN_ROW = re.compile(r"方法\s*(\d+)\s*[：:（(]")

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

# DCU-D1-01: rows where the range heuristic or the OCR text layer disagrees
# with the standard. `parsed` is the printed IC the decision was checked
# against; `model` is what the catalog uses. A `deferred` row keeps the
# product class until the object table is corrected.
#
# 201E..202C print 8, but 201C/2026..202E print 6 and 7.3.6 defines these event
# values as data variables. 4022 prints 6 between 8s. Every other decision
# follows the Appendix A IC column: collections are class 11, units class 8.
class ClassDecision(NamedTuple):
    parsed: int    # IC as printed in docs/698_parsed.md when the decision was made
    model: int     # class the catalog/object model uses
    source: str    # Appendix A table and line the decision cites
    deferred: bool = False


OI_CLASS_DECISIONS: dict[int, ClassDecision] = {
    0x201E: ClassDecision(8, 6, "表A.3 @6693（OCR 列印 8；相邻 201C/2026-202E 为 6，7.3.6）", False),
    0x2020: ClassDecision(8, 6, "表A.3 @6694（同上）", False),
    0x2021: ClassDecision(8, 6, "表A.3 @6695（同上）", False),
    0x2022: ClassDecision(8, 6, "表A.3 @6696（同上）", False),
    0x2023: ClassDecision(8, 6, "表A.3 @6697（同上）", False),
    0x2024: ClassDecision(8, 6, "表A.3 @6701（同上）", False),
    0x2025: ClassDecision(8, 6, "表A.3 @6702（同上）", False),
    0x202A: ClassDecision(8, 6, "表A.3 @6715（同上）", False),
    0x202C: ClassDecision(8, 6, "表A.3 @6716（同上）", False),
    0x3006: ClassDecision(24, 24, "表A.4 @6906", False),
    0x3007: ClassDecision(24, 24, "表A.4 @6912", False),
    0x3008: ClassDecision(24, 24, "表A.4 @6917", False),
    0x3303: ClassDecision(8, 8, "表A.4 @7421", False),
    0x3304: ClassDecision(8, 8, "表A.4 @7430", False),
    0x3305: ClassDecision(8, 8, "表A.4 @7444", False),
    0x3306: ClassDecision(8, 8, "表A.4 @7460", False),
    0x3307: ClassDecision(8, 8, "表A.4 @7480", False),
    0x3308: ClassDecision(8, 8, "表A.4 @7493", False),
    0x3309: ClassDecision(8, 8, "表A.4 @7508", False),
    0x330A: ClassDecision(8, 8, "表A.4 @7526", False),
    0x330B: ClassDecision(8, 8, "表A.4 @7538", False),
    0x330C: ClassDecision(8, 8, "表A.4 @7553", False),
    0x330D: ClassDecision(8, 8, "表A.4 @7562", False),
    0x330E: ClassDecision(8, 8, "表A.4 @7572", False),
    0x330F: ClassDecision(8, 8, "表A.4 @7585", False),
    0x3310: ClassDecision(8, 8, "表A.4 @7600", False),
    0x3311: ClassDecision(8, 8, "表A.4 @7615", False),
    0x3312: ClassDecision(8, 8, "表A.4 @7627", False),
    0x3313: ClassDecision(8, 8, "表A.4 @7639", False),
    0x3314: ClassDecision(8, 8, "表A.4 @7655", False),
    0x3315: ClassDecision(8, 8, "表A.4 @7675", False),
    0x3320: ClassDecision(8, 8, "表A.4 @7695", False),
    0x4022: ClassDecision(6, 8, "表A.5 @7900（相邻 4021/4024 为 8）", False),
    0x6000: ClassDecision(11, 10, "表A.7 @8128", True),
    0x6001: ClassDecision(8, 8, "表A.7 @8165", False),
    0x6002: ClassDecision(11, 10, "表A.7 @8219", True),
    0x6003: ClassDecision(8, 10, "表A.7 @8255", True),
    0x6004: ClassDecision(8, 8, "表A.7 @8280", False),
    0x6013: ClassDecision(8, 8, "表A.7 @8312", False),
    0x6014: ClassDecision(11, 11, "表A.7 @8354", False),
    0x6015: ClassDecision(8, 8, "表A.7 @8372", False),
    0x6016: ClassDecision(11, 11, "表A.7 @8409", False),
    0x6017: ClassDecision(8, 8, "表A.7 @8423", False),
    0x6018: ClassDecision(11, 11, "表A.7 @8440", False),
    0x6019: ClassDecision(8, 8, "表A.7 @8467", False),
    0x601A: ClassDecision(11, 11, "表A.7 @8503", False),
    0x601B: ClassDecision(8, 8, "表A.7 @8505", False),
    0x601C: ClassDecision(11, 11, "表A.7 @8518", False),
    0x601D: ClassDecision(8, 8, "表A.7 @8526", False),
    0x601E: ClassDecision(11, 11, "表A.7 @8551", False),
    0x601F: ClassDecision(8, 8, "表A.7 @8559", False),
    0x6032: ClassDecision(11, 11, "表A.7 @8582", False),
    0x6033: ClassDecision(8, 8, "表A.7 @8583", False),
    0x6034: ClassDecision(11, 11, "表A.7 @8597", False),
    0x6035: ClassDecision(8, 8, "表A.7 @8598", False),
    0x6040: ClassDecision(8, 8, "表A.7 @8616", False),
    0x6041: ClassDecision(8, 8, "表A.7 @8618", False),
    0x6042: ClassDecision(8, 8, "表A.7 @8620", False),
    0x7001: ClassDecision(8, 8, "表A.8 @8648", False),
    0x7011: ClassDecision(8, 8, "表A.8 @8679", False),
    0x7013: ClassDecision(8, 8, "表A.8 @8691", False),
    0x8000: ClassDecision(8, 13, "表A.9 @8716", True),
    0x8001: ClassDecision(8, 8, "表A.9 @8756", False),
    0x8002: ClassDecision(8, 8, "表A.9 @8778", False),
    0x8003: ClassDecision(11, 11, "表A.9 @8793", False),
    0x8004: ClassDecision(11, 11, "表A.9 @8810", False),
    0x8100: ClassDecision(8, 8, "表A.9 @8821", False),
    0x8101: ClassDecision(8, 8, "表A.9 @8822", False),
    0x8102: ClassDecision(8, 8, "表A.9 @8824", False),
    0x8109: ClassDecision(8, 8, "表A.9 @8866", False),
    0x810A: ClassDecision(8, 8, "表A.9 @8894", False),
    0x810B: ClassDecision(8, 8, "表A.9 @8903", False),
    0x810C: ClassDecision(8, 8, "表A.9 @8910", False),
    0x810D: ClassDecision(8, 8, "表A.9 @8924", False),
    0x810E: ClassDecision(8, 8, "表A.9 @8931", False),
    0x810F: ClassDecision(8, 8, "表A.9 @8932", False),
    0x8110: ClassDecision(8, 8, "表A.9 @8933", False),
    0xF101: ClassDecision(8, 8, "表A.11 @9083", False),
    0xF210: ClassDecision(8, 8, "表A.12 @9300", False),
}

def infer_class(oi: int) -> int:
    decision = OI_CLASS_DECISIONS.get(oi)
    if decision:
        return decision.model
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


def unexplained_conflicts(ois: list[dict]) -> tuple[list[dict], list[int]]:
    unexplained = []
    seen = set()
    for row in ois:
        oi = int(row["oi"], 16)
        decision = OI_CLASS_DECISIONS.get(oi)
        if decision:
            seen.add(oi)
            if not decision.source or decision.parsed != row["parsed_class"]:
                unexplained.append(row)
                continue
            if row["inferred_class"] != decision.model:
                unexplained.append(row)
                continue
        if row["parsed_class"] != row["inferred_class"] and not decision:
            unexplained.append(row)
    missing = sorted(set(OI_CLASS_DECISIONS) - seen)
    return unexplained, missing


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


def parse_oi_methods(lines: list[str]) -> dict[str, set[int]]:
    """Methods an OI defines in Appendix A, which the 7.3 class tables omit."""
    methods: dict[str, set[int]] = {}
    current: str | None = None
    split_oi = re.compile(r"^([0-9A-Fa-f]{2})\s+([0-9A-Fa-f]{2})\s+(\d{1,2})\s+(.+)$")
    for i, line in enumerate(lines):
        if i < 6298 or i > 9450:
            current = None
            continue
        text = line.strip()
        m = OI_ROW.match(text)
        if m:
            current = f"{int(m.group(1), 16):04X}"
        elif split_oi.match(text):
            s = split_oi.match(text)
            current = f"{int(s.group(1) + s.group(2), 16):04X}"
        if current is None:
            continue
        for mm in METHOD_IN_ROW.finditer(text):
            methods.setdefault(current, set()).add(int(mm.group(1)))
    return methods


def render_header(classes: list[dict], ois: list[dict],
                  oi_methods: dict[str, set[int]]) -> str:
    class_attr = {cls["class_id"]: max((a["no"] for a in cls["attributes"]), default=0)
                  for cls in classes}
    class_method = {cls["class_id"]: max((m["no"] for m in cls["methods"]), default=0)
                    for cls in classes}
    for row in ois:
        cid = row["inferred_class"]
        extra = max(oi_methods.get(row["oi"], {0}))
        class_method[cid] = max(class_method.get(cid, 0), extra)

    out = []
    out.append("/* Generated by scripts/extract_dcu_698_catalog.py from docs/698_parsed.md.")
    out.append(" * Do not edit by hand. Regenerate after the parsed standard changes. */")
    out.append("")
    out.append("#ifndef DCU_698_CATALOG_EXPECTED_H")
    out.append("#define DCU_698_CATALOG_EXPECTED_H")
    out.append("")
    out.append("#include <stdint.h>")
    out.append("")
    out.append("typedef struct {")
    out.append("    uint16_t oi;")
    out.append("    uint8_t class_id;")
    out.append("} dcu698_catalog_oi_t;")
    out.append("")
    out.append("static const dcu698_catalog_oi_t dcu698_catalog_ois[] = {")
    for row in sorted(ois, key=lambda r: int(r["oi"], 16)):
        out.append(f"    {{0x{row['oi']}, {row['inferred_class']}}},")
    out.append("};")
    out.append("")
    out.append("typedef struct {")
    out.append("    uint8_t class_id;")
    out.append("    uint8_t max_attr;")
    out.append("    uint8_t max_method;")
    out.append("} dcu698_catalog_class_t;")
    out.append("")
    out.append("static const dcu698_catalog_class_t dcu698_catalog_classes[] = {")
    for cls in sorted(classes, key=lambda c: c["class_id"]):
        cid = cls["class_id"]
        out.append(f"    {{{cid}, {class_attr[cid]}, {class_method[cid]}}},")
    out.append("};")
    out.append("")
    out.append("#endif /* DCU_698_CATALOG_EXPECTED_H */")
    return "\n".join(out) + "\n"


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
    deferred = sum(1 for decision in OI_CLASS_DECISIONS.values() if decision.deferred)
    out.append(f"Classes: {len(classes)}. OI rows: {len(ois)}. "
               f"Rows whose parsed IC differs from the chosen class: {len(conflicts)} "
               f"(OCR corrections and {deferred} deferred object-table rows).")
    out.append("")
    out.append("`parsed_class` is the IC column printed in the parsed table; "
               "`inferred_class` is the class the catalog uses. "
               "`DCU_698_CLASS_RECONCILIATION.md` cites the source for every row "
               "where the two differ.")
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


def render_reconciliation(ois: list[dict]) -> str:
    by_oi = {row["oi"]: row for row in ois}
    resolved = sum(1 for decision in OI_CLASS_DECISIONS.values() if not decision.deferred)
    deferred = len(OI_CLASS_DECISIONS) - resolved
    out = [
        "# DCU 698 class reconciliation",
        "",
        "> Generated by `scripts/extract_dcu_698_catalog.py` from `OI_CLASS_DECISIONS`.",
        "> Do not edit by hand. Regenerate after the decisions change.",
        "",
        "Appendix A and `docs/698.pdf` decide each conflict. `parsed` is the printed",
        "IC; `chosen` is the class the catalog uses. 201E..202C and 4022 are OCR",
        "misreads: their neighbouring rows and 7.3.6 win. Deferred rows keep the",
        "product class until the object table is corrected in a separate defect.",
        "",
        f"Decisions: {resolved} resolved, {deferred} deferred.",
        "",
        "| OI | parsed | chosen | source |",
        "| --- | --- | --- | --- |",
    ]
    for oi in sorted(OI_CLASS_DECISIONS):
        decision = OI_CLASS_DECISIONS[oi]
        row = by_oi[f"{oi:04X}"]
        if decision.deferred:
            out.append(f"| {row['oi']} | {decision.parsed} | {decision.model} | "
                       f"{decision.source}；对象表仍用 {decision.model}，另立缺陷 |")
        else:
            out.append(f"| {row['oi']} | {decision.parsed} | {decision.model} | "
                       f"{decision.source} |")
    return "\n".join(out) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--root", default=None,
                        help="repository root holding docs/; defaults to the script's repo")
    args = parser.parse_args()

    root = Path(args.root).resolve() if args.root else ROOT
    source, out_md, out_json, out_header = catalog_paths(root)

    if not source.exists():
        print(f"missing source: {source}", file=sys.stderr)
        return 1

    lines = source.read_text(encoding="utf-8").splitlines()
    classes = parse_classes(lines)
    ois = parse_ois(lines, {cls["class_id"] for cls in classes})
    oi_methods = parse_oi_methods(lines)

    if len(classes) != 26:
        print(f"expected 26 classes, parsed {len(classes)}", file=sys.stderr)
        return 1
    if len(ois) < 400:
        print(f"expected at least 400 OI rows, parsed {len(ois)}", file=sys.stderr)
        return 1
    if not any(row["oi"] == "F100" and row["parsed_class"] == 21 for row in ois):
        print("expected F100 in class 21 (ESAM); it is missing or misfiled", file=sys.stderr)
        return 1

    unexplained, missing = unexplained_conflicts(ois)
    if missing:
        print("class decisions for OIs absent from the parsed table:", file=sys.stderr)
        for oi in missing:
            print(f"  0x{oi:04X}", file=sys.stderr)
        return 1
    if unexplained:
        print("class conflicts without a recorded decision (DCU-D1-01):", file=sys.stderr)
        for row in unexplained:
            print(f"  {row['oi']} parsed={row['parsed_class']} "
                  f"inferred={row['inferred_class']} @{row['source_line']}", file=sys.stderr)
        return 1

    markdown = render(classes, ois)
    payload = json.dumps({"classes": classes, "ois": ois}, ensure_ascii=False, indent=1) + "\n"
    header = render_header(classes, ois, oi_methods)
    reconciliation = render_reconciliation(ois)
    out_recon = reconciliation_path(root)

    if args.check:
        stale = []
        if not out_md.exists() or out_md.read_text(encoding="utf-8") != markdown:
            stale.append(str(out_md))
        if not out_json.exists() or out_json.read_text(encoding="utf-8") != payload:
            stale.append(str(out_json))
        if not out_header.exists() or out_header.read_text(encoding="utf-8") != header:
            stale.append(str(out_header))
        if not out_recon.exists() or out_recon.read_text(encoding="utf-8") != reconciliation:
            stale.append(str(out_recon))
        for path in stale:
            print(f"stale: {path}", file=sys.stderr)
        return 1 if stale else 0

    out_md.parent.mkdir(parents=True, exist_ok=True)
    out_header.parent.mkdir(parents=True, exist_ok=True)
    out_md.write_text(markdown, encoding="utf-8")
    out_json.write_text(payload, encoding="utf-8")
    out_header.write_text(header, encoding="utf-8")
    out_recon.write_text(reconciliation, encoding="utf-8")
    print(f"wrote {out_md.relative_to(root)} ({len(classes)} classes, {len(ois)} OIs)")
    print(f"wrote {out_json.relative_to(root)}")
    print(f"wrote {out_header.relative_to(root)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
