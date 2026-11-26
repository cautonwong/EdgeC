#!/usr/bin/env python3
"""P4-01a: extract the IEC104 CA/point/control IOA table from v3 sources.

Read-only and offline. The input config is the flat ``pts[].pointInfo`` tuple
list (``[name, ioa, type, changeReport, coefficient]``). That flat shape does
not carry the RemotePoint/RemoteMeas/RemoteCtrl/RemoteRegu grouping of the
runtime map, so a point's kind comes from the protocol start address that
covers its IOA, parsed from ``src/automatic_north/iec104_create.c``.

Exit codes: 0 ok, 2 data or snapshot mismatch, 3 usage or environment.
"""

from __future__ import annotations

import argparse
import contextlib
import io
import json
import re
import shutil
import sys
import tempfile
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path

TELEMETRY = "telemetry"
CONTROL = "control"
UNKNOWN = "unknown"

# 遥信/遥测 are telemetry, 遥控/遥调 are control (iec104_create.h:40-43).
RANGE_CONSTANTS = (
    ("start_RemotePoint", TELEMETRY),
    ("start_RemoteMeas", TELEMETRY),
    ("start_RemoteCtrl", CONTROL),
    ("start_RemoteRegu", CONTROL),
)
SOURCE_REL = Path("src/automatic_north/iec104_create.c")
REAL_SOURCE_ROOT = Path("/home/edge/iems_v3")
REAL_CONFIG = REAL_SOURCE_ROOT / "config" / "iec104_config.json"
GOLDEN_DIR = Path(__file__).resolve().parent / "golden" / "iec104_points"
SYNTHETIC_DIR = GOLDEN_DIR / "synthetic"

RANGE_RE = re.compile(
    r"\.(start_RemotePoint|start_RemoteMeas|start_RemoteCtrl|start_RemoteRegu)"
    r"\s*=\s*(\d+)"
)
POINT_RE = re.compile(
    r'^\s*\[\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*[^,]+,\s*[^\]]+\]'
)


class UsageError(Exception):
    """Bad invocation or missing source file (exit 3)."""


class DataError(Exception):
    """Malformed config data (exit 2)."""


@dataclass(frozen=True)
class IoaRange:
    const: str
    start: int
    kind: str
    line: int


@dataclass(frozen=True)
class Point:
    ca: int
    name: str
    ioa: int
    data_type: str
    scale: str
    kind: str
    evidence: str


@dataclass
class Table:
    points: list[Point]
    ranges: list[IoaRange]
    config_rel: str
    warnings: list[str]


def load_ranges(source_root: Path) -> list[IoaRange]:
    path = Path(source_root) / SOURCE_REL
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise UsageError(f"cannot read {path}: {exc}") from exc

    found: dict[str, tuple[int, int]] = {}
    for lineno, line in enumerate(text.splitlines(), 1):
        for match in RANGE_RE.finditer(line):
            found.setdefault(match.group(1), (int(match.group(2)), lineno))
    missing = [const for const, _ in RANGE_CONSTANTS if const not in found]
    if missing:
        raise UsageError(f"{path}: missing {', '.join(missing)}")

    ranges = sorted(
        (IoaRange(const, found[const][0], kind, found[const][1])
         for const, kind in RANGE_CONSTANTS),
        key=lambda item: item.start,
    )
    if len({item.start for item in ranges}) != len(ranges):
        raise UsageError(f"{path}: duplicate start addresses across range constants")
    return ranges


def kind_for(ioa: int, ranges: list[IoaRange]) -> IoaRange | None:
    if ioa < ranges[0].start:
        return None
    hit = None
    for item in ranges:
        if ioa >= item.start:
            hit = item
        else:
            break
    return hit


def relative_path(path: Path, root: Path) -> str:
    try:
        return Path(path).resolve().relative_to(Path(root).resolve()).as_posix()
    except ValueError:
        return Path(path).as_posix()


def number_text(value: object) -> str:
    if isinstance(value, float):
        return repr(value)
    return str(value)


def locate_point(scanned: list[tuple[int, str, str, str]], cursor: int,
                 name: str, ioa: int, data_type: str) -> tuple[int | None, int]:
    for index in range(cursor, len(scanned)):
        line, s_name, s_ioa, s_type = scanned[index]
        if s_name != name or s_type != data_type:
            continue
        try:
            if int(s_ioa) == ioa:
                return line, index + 1
        except ValueError:
            continue
    return None, cursor


def extract(config_path: Path, source_root: Path) -> Table:
    source_root = Path(source_root)
    ranges = load_ranges(source_root)

    config_path = Path(config_path)
    try:
        text = config_path.read_text(encoding="utf-8")
    except OSError as exc:
        raise UsageError(f"cannot read {config_path}: {exc}") from exc
    try:
        doc = json.loads(text)
    except json.JSONDecodeError as exc:
        raise DataError(f"{config_path}: invalid JSON: {exc}") from exc
    if not isinstance(doc, dict) or not isinstance(doc.get("pts"), list):
        raise DataError(f"{config_path}: no pts array")

    scanned: list[tuple[int, str, str, str]] = []
    for lineno, line in enumerate(text.splitlines(), 1):
        match = POINT_RE.match(line)
        if match:
            scanned.append((lineno, match.group(1), match.group(2), match.group(3)))

    config_rel = relative_path(config_path, source_root)
    points: list[Point] = []
    cursor = 0
    for ca_index, block in enumerate(doc["pts"]):
        if not isinstance(block, dict):
            raise DataError(f"pts[{ca_index}] is not an object")
        try:
            ca = int(str(block.get("commonAddr")))
        except ValueError as exc:
            raise DataError(f"pts[{ca_index}].commonAddr is not an integer") from exc
        info = block.get("pointInfo")
        if not isinstance(info, list):
            raise DataError(f"CA {ca}: pointInfo is not an array")
        for point_index, item in enumerate(info):
            if not isinstance(item, list) or len(item) < 5:
                raise DataError(f"CA {ca}: pointInfo[{point_index}] is not a 5-field array")
            name, ioa_value, data_type, _change_report, scale_value = item[:5]
            if not isinstance(name, str) or not isinstance(data_type, str):
                raise DataError(f"CA {ca}: pointInfo[{point_index}] name/type is not a string")
            try:
                ioa = int(str(ioa_value))
            except ValueError as exc:
                raise DataError(f"CA {ca}: pointInfo[{point_index}] IOA is not an integer") from exc

            line_no, cursor = locate_point(scanned, cursor, name, ioa, data_type)
            config_ev = f"{config_rel}:{line_no if line_no is not None else '?'}"

            hit = kind_for(ioa, ranges)
            if hit is None:
                kind = UNKNOWN
                range_ev = f"no kind range for IOA {ioa} (first start {ranges[0].start})"
            else:
                kind = hit.kind
                range_ev = f"{SOURCE_REL.as_posix()}:{hit.line} {hit.const}={hit.start}"
            points.append(Point(
                ca=ca,
                name=name,
                ioa=ioa,
                data_type=data_type,
                scale=number_text(scale_value),
                kind=kind,
                evidence=f"{config_ev}; {range_ev}",
            ))

    points.sort(key=lambda point: (point.ca, point.ioa, point.name))
    duplicates = Counter((point.ca, point.ioa) for point in points)
    warnings = [
        f"CA {ca}: IOA {ioa} used by {count} points"
        for (ca, ioa), count in sorted(duplicates.items())
        if count > 1
    ]
    return Table(points=points, ranges=ranges, config_rel=config_rel, warnings=warnings)


def render_tsv(table: Table) -> str:
    rows = ["\t".join(("ca", "name", "ioa", "type", "scale", "kind", "evidence"))]
    rows.extend("\t".join((
        str(point.ca), point.name, str(point.ioa), point.data_type,
        point.scale, point.kind, point.evidence,
    )) for point in table.points)
    return "\n".join(rows) + "\n"


def md_cell(value: object) -> str:
    return str(value).replace("|", "\\|")


def render_md(table: Table) -> str:
    ranges = "; ".join(
        f"`{SOURCE_REL.as_posix()}:{item.line}` {item.const}={item.start} ({item.kind})"
        for item in table.ranges
    )
    counts: dict[int, Counter[str]] = defaultdict(Counter)
    for point in table.points:
        counts[point.ca][point.kind] += 1
    kinds = (TELEMETRY, CONTROL, UNKNOWN)

    lines = [
        "# IEC104 CA point table",
        "",
        f"- config: `{table.config_rel}`",
        f"- kind ranges: {ranges}",
        f"- points: {len(table.points)} across {len(counts)} CAs",
        "",
        "| CA | points | telemetry | control | unknown |",
        "| ---: | ---: | ---: | ---: | ---: |",
    ]
    for ca in sorted(counts):
        row = [str(ca), str(sum(counts[ca].values()))]
        row.extend(str(counts[ca][kind]) for kind in kinds)
        lines.append("| " + " | ".join(row) + " |")
    total = [str(len(table.points))]
    total.extend(str(sum(counts[ca][kind] for ca in counts)) for kind in kinds)
    lines.append("| total | " + " | ".join(total) + " |")

    lines += [
        "",
        "## Points",
        "",
        "| CA | name | IOA | type | scale | kind | evidence |",
        "| ---: | --- | ---: | --- | ---: | --- | --- |",
    ]
    for point in table.points:
        lines.append("| " + " | ".join((
            str(point.ca), md_cell(point.name), str(point.ioa),
            md_cell(point.data_type), md_cell(point.scale),
            point.kind, md_cell(point.evidence),
        )) + " |")

    lines += ["", "## Anomalies", ""]
    anomalies: list[str] = []
    by_ioa: dict[tuple[int, int], list[str]] = defaultdict(list)
    for point in table.points:
        by_ioa[(point.ca, point.ioa)].append(point.name)
    for (ca, ioa), names in sorted(by_ioa.items()):
        if len(names) > 1:
            anomalies.append(
                f"- CA {ca}: IOA {ioa} used by {len(names)} points: " + ", ".join(names))
    for point in table.points:
        if point.kind == UNKNOWN:
            anomalies.append(f"- CA {point.ca}: IOA {point.ioa} ({point.name}) has no kind range")
    lines.extend(anomalies or ["- none"])
    return "\n".join(lines) + "\n"


def write_outputs(out_dir: Path, stem: str, rendered: dict[str, str]) -> int:
    out_dir.mkdir(parents=True, exist_ok=True)
    for extension, text in rendered.items():
        (out_dir / f"{stem}.{extension}").write_text(text, encoding="utf-8")
    print(f"wrote {len(rendered)} file(s) to {out_dir}")
    return 0


def check_outputs(directory: Path, stem: str, rendered: dict[str, str]) -> int:
    failed = False
    for extension, text in rendered.items():
        path = Path(directory) / f"{stem}.{extension}"
        if not path.is_file():
            print(f"FAIL missing {path}", file=sys.stderr)
            return 3
        expected = path.read_text(encoding="utf-8")
        if expected == text:
            print(f"OK {path}")
            continue
        failed = True
        print(f"FAIL {path} differs from generated output", file=sys.stderr)
        for index, (want, got) in enumerate(
                zip(expected.splitlines(), text.splitlines()), 1):
            if want != got:
                print(f"  line {index}: snapshot={want!r} generated={got!r}", file=sys.stderr)
                break
        else:
            print(f"  lines snapshot={len(expected.splitlines())} "
                  f"generated={len(text.splitlines())}", file=sys.stderr)
    return 2 if failed else 0


def run_selftest() -> int:
    checks: list[tuple[str, bool, str]] = []

    def check(name: str, ok: bool, detail: str = "") -> None:
        checks.append((name, ok, detail))

    with tempfile.TemporaryDirectory(prefix="iec104_points_selftest_") as tmp:
        root = Path(tmp)
        shutil.copytree(SYNTHETIC_DIR, root, dirs_exist_ok=True)
        config = root / "config" / "iec104_config.json"
        table = extract(config, root)

        check("synthetic tsv snapshot",
              render_tsv(table) == (SYNTHETIC_DIR / "expected.tsv").read_text(encoding="utf-8"))
        check("synthetic md snapshot",
              render_md(table) == (SYNTHETIC_DIR / "expected.md").read_text(encoding="utf-8"))
        check("synthetic 7 points / 2 CAs",
              len(table.points) == 7 and {point.ca for point in table.points} == {1, 2})

        by_name = {point.name: point for point in table.points}
        check("kind telemetry",
              by_name["A.sig"].kind == TELEMETRY and by_name["A.meas"].kind == TELEMETRY)
        check("kind control",
              by_name["A.ctrl"].kind == CONTROL and by_name["A.regu"].kind == CONTROL)
        check("kind unknown", by_name["A.none"].kind == UNKNOWN)
        check("scale kept",
              [by_name[name].scale for name in ("A.sig", "A.meas", "A.regu")]
              == ["1.0", "0.1", "2.5"])
        check("control evidence cites config and range",
              "config/iec104_config.json:9" in by_name["A.ctrl"].evidence
              and "src/automatic_north/iec104_create.c:4" in by_name["A.ctrl"].evidence)
        check("unknown evidence explains gap",
              "no kind range" in by_name["A.none"].evidence)
        check("duplicate IOA warned",
              any("IOA 150" in warning for warning in table.warnings))

        out = root / "out"
        out.mkdir()
        rendered = {"tsv": render_tsv(table), "md": render_md(table)}
        with contextlib.redirect_stdout(io.StringIO()):
            write_outputs(out, "iec104_config", rendered)
        with contextlib.redirect_stderr(io.StringIO()), \
                contextlib.redirect_stdout(io.StringIO()):
            green = check_outputs(out, "iec104_config", rendered)
            (out / "iec104_config.tsv").write_text("drift\n", encoding="utf-8")
            drift = check_outputs(out, "iec104_config", {"tsv": rendered["tsv"]})
            (out / "iec104_config.tsv").unlink()
            missing = check_outputs(out, "iec104_config", {"tsv": rendered["tsv"]})
        check("check mode green", green == 0, f"rc={green}")
        check("check mode detects drift", drift == 2, f"rc={drift}")
        check("check mode reports missing file", missing == 3, f"rc={missing}")

        bad = root / "bad"
        (bad / "src" / "automatic_north").mkdir(parents=True)
        (bad / "src" / "automatic_north" / "iec104_create.c").write_text(
            ".start_RemotePoint = 1\n.start_RemoteMeas = 100\n"
            ".start_RemoteCtrl = 200\n", encoding="utf-8")
        try:
            extract(config, bad)
            missing_range = False
        except UsageError:
            missing_range = True
        check("missing range constant rejected", missing_range)

        if REAL_CONFIG.is_file():
            real = extract(REAL_CONFIG, REAL_SOURCE_ROOT)
            check("real config 2 CAs / 178 points",
                  len(real.points) == 178
                  and {point.ca for point in real.points} == {1, 2})
            check("real tsv matches snapshot",
                  render_tsv(real) == (GOLDEN_DIR / "iec104_config.tsv").read_text(encoding="utf-8"))
            check("real md matches snapshot",
                  render_md(real) == (GOLDEN_DIR / "iec104_config.md").read_text(encoding="utf-8"))
        else:
            check("real config snapshot", True, "skipped: /home/edge/iems_v3 not present")

    passed = sum(1 for _, ok, _ in checks if ok)
    for name, ok, detail in checks:
        suffix = f" ({detail})" if detail and not ok else ""
        print(f"{'PASS' if ok else 'FAIL'} {name}{suffix}")
    print(f"selftest {passed}/{len(checks)} ok")
    return 0 if passed == len(checks) else 1


class Parser(argparse.ArgumentParser):
    def error(self, message: str) -> None:
        self.print_usage(sys.stderr)
        print(f"FAIL {message}", file=sys.stderr)
        raise SystemExit(3)


def build_parser() -> argparse.ArgumentParser:
    parser = Parser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", type=Path, help="iec104_config.json to read")
    parser.add_argument("--source-root", type=Path, help="v3 source root for range constants")
    parser.add_argument("--format", choices=("tsv", "md", "both"), default="both")
    parser.add_argument("--out", type=Path, help="write <config stem>.tsv/.md here")
    parser.add_argument("--check", type=Path, metavar="DIR",
                        help="compare output with <config stem>.tsv/.md in DIR")
    parser.add_argument("--selftest", action="store_true", help="run the synthetic checks")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.selftest:
        if args.config or args.source_root or args.out or args.check:
            print("FAIL --selftest takes no other mode", file=sys.stderr)
            return 3
        return run_selftest()
    if args.config is None or args.source_root is None:
        print("FAIL --config and --source-root are required", file=sys.stderr)
        return 3
    try:
        table = extract(args.config, args.source_root)
    except UsageError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 3
    except DataError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 2
    for warning in table.warnings:
        print(f"WARN {warning}", file=sys.stderr)

    rendered: dict[str, str] = {}
    if args.format in ("tsv", "both"):
        rendered["tsv"] = render_tsv(table)
    if args.format in ("md", "both"):
        rendered["md"] = render_md(table)
    stem = Path(args.config).stem
    if args.check is not None and args.out is not None:
        print("FAIL --out and --check are mutually exclusive", file=sys.stderr)
        return 3
    if args.check is not None:
        return check_outputs(args.check, stem, rendered)
    if args.out is not None:
        return write_outputs(args.out, stem, rendered)
    for index, text in enumerate(rendered.values()):
        if index:
            sys.stdout.write("\n")
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
