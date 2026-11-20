#!/usr/bin/env python3
"""Replay a JSONL trace through a replaceable target and report field-level divergence."""

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

TRACE_VERSION = 1
KINDS = ("point", "alarm", "command")
BASE_DIR = Path(__file__).resolve().parent
DEFAULT_REPORT = BASE_DIR / "reports" / "replay.report.json"
EDGEC_TARGET_BIN = BASE_DIR / "edgec_target" / "edgec_modbus_target"


class TraceError(Exception):
    pass


class TargetError(Exception):
    pass


class MockTarget:
    """A target consumes one expected event and returns the events it emits.

    The mock echoes the event so the replay and diff plumbing can be checked
    before a real adapter exists.
    """

    name = "mock"

    def apply(self, record):
        return [record]


class EdgecModbusTarget:
    """The real EdgeC Modbus RTU codec, driven over a pipe.

    The C harness maps each key to a holding register, encodes a request with
    the EdgeC codec, decodes the simulated meter reply and prints one trace
    record per event. This target only moves JSON lines across the pipe.
    """

    name = "edgec-modbus"

    def __init__(self, corrupt_key=None):
        if not EDGEC_TARGET_BIN.is_file():
            raise TargetError(
                f"{EDGEC_TARGET_BIN} is missing; build it with the cc command in README.md section 1.1"
            )
        argv = [str(EDGEC_TARGET_BIN)]
        if corrupt_key is not None:
            argv += ["--corrupt", corrupt_key]
        self._proc = subprocess.Popen(
            argv,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
        )

    def apply(self, record):
        payload = {
            field: record[field]
            for field in ("v", "t_ms", "source", "kind", "key", "value")
        }
        self._proc.stdin.write(json.dumps(payload, ensure_ascii=False) + "\n")
        self._proc.stdin.flush()
        reply = self._proc.stdout.readline()
        if not reply:
            raise TargetError(
                f"edgec-modbus harness exited while handling {record['kind']} {record['key']};"
                " run it directly to see stderr"
            )
        try:
            return [json.loads(reply)]
        except ValueError as exc:
            raise TargetError(f"edgec-modbus harness emitted invalid JSON: {reply!r}") from exc

    def close(self):
        proc = getattr(self, "_proc", None)
        if proc is None:
            return
        self._proc = None
        try:
            proc.stdin.close()
            proc.wait(timeout=10)
        except (OSError, subprocess.TimeoutExpired):
            proc.kill()


TARGETS = {"mock": MockTarget, "edgec-modbus": EdgecModbusTarget}


def _reject_constant(name):
    raise ValueError(f"non-finite number {name}")


def load_trace(path):
    records = []
    last_t_ms = -1
    with open(path, encoding="utf-8") as stream:
        for lineno, line in enumerate(stream, 1):
            if not line.strip():
                continue
            try:
                raw = json.loads(line, parse_constant=_reject_constant)
            except ValueError as exc:
                raise TraceError(f"{path}:{lineno}: invalid JSON: {exc}") from exc
            record = _parse_record(raw, path, lineno, last_t_ms)
            records.append(record)
            last_t_ms = record["t_ms"]
    return records


def _parse_record(raw, path, lineno, last_t_ms):
    where = f"{path}:{lineno}"
    if not isinstance(raw, dict):
        raise TraceError(f"{where}: record must be a JSON object")
    for field in ("v", "t_ms", "source", "kind", "key", "value"):
        if field not in raw:
            raise TraceError(f"{where}: missing field {field!r}")
    if type(raw["v"]) is not int or raw["v"] != TRACE_VERSION:
        raise TraceError(
            f"{where}: unsupported v {raw['v']!r}, this tool reads v={TRACE_VERSION}"
        )
    t_ms = raw["t_ms"]
    if type(t_ms) is not int or t_ms < 0:
        raise TraceError(f"{where}: t_ms must be a non-negative integer")
    if t_ms < last_t_ms:
        raise TraceError(
            f"{where}: t_ms {t_ms} is before the previous record ({last_t_ms}), sort the trace first"
        )
    if not isinstance(raw["source"], str) or not raw["source"]:
        raise TraceError(f"{where}: source must be a non-empty string")
    if raw["kind"] not in KINDS:
        raise TraceError(f"{where}: kind must be one of {', '.join(KINDS)}")
    if not isinstance(raw["key"], str) or not raw["key"]:
        raise TraceError(f"{where}: key must be a non-empty string")
    # Unknown fields are dropped so a v1 reader can ignore additions from later writers.
    return {
        "v": TRACE_VERSION,
        "t_ms": t_ms,
        "source": raw["source"],
        "kind": raw["kind"],
        "key": raw["key"],
        "value": raw["value"],
    }


def _canon(value):
    # Canonical JSON keeps 1 and 1.0 distinct, which is what strict parity needs.
    return json.dumps(value, sort_keys=True, ensure_ascii=False, allow_nan=False)


def _group(records):
    groups = {}
    for record in records:
        groups.setdefault((record["kind"], record["key"]), []).append(record)
    return groups


def compare(expected, actual):
    expected_groups = _group(expected)
    actual_groups = _group(actual)
    keys = sorted(set(expected_groups) | set(actual_groups))
    groups = []
    missing = 0
    extra = 0
    mismatches = 0
    first_candidates = []

    for kind, key in keys:
        want = expected_groups.get((kind, key), [])
        got = actual_groups.get((kind, key), [])
        paired = min(len(want), len(got))
        group_mismatches = 0
        group_first = None

        for index in range(paired):
            fields = {}
            if _canon(want[index]["value"]) != _canon(got[index]["value"]):
                fields["value"] = {
                    "expected": want[index]["value"],
                    "actual": got[index]["value"],
                }
            if want[index]["t_ms"] != got[index]["t_ms"]:
                fields["t_ms"] = {
                    "expected": want[index]["t_ms"],
                    "actual": got[index]["t_ms"],
                }
            if fields:
                group_mismatches += 1
                if group_first is None:
                    group_first = {
                        "t_ms": want[index]["t_ms"],
                        "actual_t_ms": got[index]["t_ms"],
                        "fields": fields,
                    }

        group_missing = max(0, len(want) - paired)
        group_extra = max(0, len(got) - paired)
        missing += group_missing
        extra += group_extra
        mismatches += group_mismatches

        if group_first is None and group_missing:
            group_first = {
                "t_ms": want[paired]["t_ms"],
                "actual_t_ms": None,
                "fields": {
                    "record": {"expected": "present", "actual": "missing"}
                },
            }
        if group_first is None and group_extra:
            group_first = {
                "t_ms": got[paired]["t_ms"],
                "actual_t_ms": got[paired]["t_ms"],
                "fields": {
                    "record": {"expected": "missing", "actual": "present"}
                },
            }

        if group_first is not None:
            groups.append(
                {
                    "kind": kind,
                    "key": key,
                    "expected_records": len(want),
                    "actual_records": len(got),
                    "missing": group_missing,
                    "extra": group_extra,
                    "mismatches": group_mismatches,
                    "first_divergence": group_first,
                }
            )
            first_candidates.append((group_first["t_ms"], kind, key, group_first))

    first = None
    if first_candidates:
        t_ms, kind, key, detail = min(first_candidates)
        first = {"kind": kind, "key": key, **detail}

    return {
        "v": TRACE_VERSION,
        "expected_records": len(expected),
        "actual_records": len(actual),
        "missing": missing,
        "extra": extra,
        "mismatches": mismatches,
        "ok": not (missing or extra or mismatches),
        "first_divergence": first,
        "groups": groups,
    }


def run_replay(trace_path, target):
    expected = load_trace(trace_path)
    actual = []
    try:
        for record in expected:
            actual.extend(target.apply(record))
    finally:
        close = getattr(target, "close", None)
        if close is not None:
            close()
    report = compare(expected, actual)
    report["trace"] = str(trace_path)
    report["trace_sha256"] = hashlib.sha256(Path(trace_path).read_bytes()).hexdigest()
    report["target"] = target.name
    return report


def write_report(report, path):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )


def _format_fields(fields):
    parts = []
    for name, pair in fields.items():
        parts.append(
            f"{name}: expected {json.dumps(pair['expected'], ensure_ascii=False)}"
            f" actual {json.dumps(pair['actual'], ensure_ascii=False)}"
        )
    return ", ".join(parts)


def _describe_first(divergence):
    return (
        f"{divergence['kind']} {divergence['key']}"
        f" at t_ms={divergence['t_ms']}"
        f" ({_format_fields(divergence['fields'])})"
    )


def _check(condition, message):
    if not condition:
        print(f"SELFTEST FAIL: {message}", file=sys.stderr)
        raise SystemExit(1)


class _CorruptingTarget(MockTarget):
    name = "corrupt"

    def __init__(self, victim):
        self.victim = victim
        self.corrupted = False

    def apply(self, record):
        if not self.corrupted and all(
            record[field] == self.victim[field] for field in ("kind", "key", "t_ms")
        ):
            self.corrupted = True
            return [dict(record, value=record["value"] + 1)]
        return [record]


def _cmd_selftest():
    trace_path = BASE_DIR / "samples" / "sample_trace.jsonl"
    reports_dir = BASE_DIR / "reports"
    expected = load_trace(trace_path)

    mirror = run_replay(trace_path, MockTarget())
    mirror_path = reports_dir / "selftest_mirror.json"
    write_report(mirror, mirror_path)
    _check(mirror["ok"], "identical replay must report zero divergence")
    _check(mirror["missing"] == 0 and mirror["extra"] == 0, "identical replay must have no missing or extra records")
    _check(mirror["mismatches"] == 0, "identical replay must have no mismatched records")
    _check(mirror["first_divergence"] is None, "identical replay must have no first divergence")
    print(
        f"selftest 1/4 ok: identical replay, expected={mirror['expected_records']},"
        f" actual={mirror['actual_records']}, report={mirror_path}"
    )

    victim = None
    for record in reversed(expected):
        if record["kind"] == "point" and type(record["value"]) in (int, float):
            victim = record
            break
    _check(victim is not None, "sample trace needs a numeric point to corrupt")

    corrupt = run_replay(trace_path, _CorruptingTarget(victim))
    corrupt_path = reports_dir / "selftest_corrupt.json"
    write_report(corrupt, corrupt_path)
    _check(not corrupt["ok"], "corrupted replay must report a difference")
    _check(corrupt["missing"] == 0 and corrupt["extra"] == 0, "corrupted replay must keep record counts equal")
    _check(corrupt["mismatches"] == 1, "corrupted replay must report exactly one mismatched record")
    first = corrupt["first_divergence"]
    _check(first is not None, "corrupted replay must report a first divergence")
    _check(
        first["kind"] == victim["kind"] and first["key"] == victim["key"],
        f"corrupted report must name {victim['kind']} {victim['key']}",
    )
    _check(
        first["t_ms"] == victim["t_ms"],
        f"corrupted report must give first divergence t_ms={victim['t_ms']}",
    )
    _check(
        first["fields"].get("value", {}).get("expected") == victim["value"],
        "corrupted report must show the expected value",
    )
    _check(
        first["fields"].get("value", {}).get("actual") == victim["value"] + 1,
        "corrupted report must show the actual value",
    )

    TARGETS["corrupt"] = lambda: _CorruptingTarget(victim)
    try:
        exit_code = main(
            [
                "replay",
                "--trace",
                str(trace_path),
                "--target",
                "corrupt",
                "--report",
                str(reports_dir / "selftest_corrupt_cli.json"),
            ]
        )
    finally:
        del TARGETS["corrupt"]
    _check(exit_code == 1, "replay with a difference must exit 1")

    print(
        f"selftest 2/4 ok: corrupted {first['kind']} {first['key']},"
        f" first divergence t_ms={first['t_ms']}, exit={exit_code}, report={corrupt_path}"
    )

    try:
        edgec_target = EdgecModbusTarget()
    except TargetError as exc:
        _check(False, str(exc))
    edgec = run_replay(trace_path, edgec_target)
    edgec_path = reports_dir / "selftest_edgec_target.json"
    write_report(edgec, edgec_path)
    _check(edgec["ok"], "real EdgeC codec replay must report zero divergence")
    _check(
        edgec["missing"] == 0 and edgec["extra"] == 0 and edgec["mismatches"] == 0,
        "real EdgeC codec replay must have no missing, extra or mismatched records",
    )
    _check(
        edgec["expected_records"] == edgec["actual_records"] == 14,
        "real EdgeC codec replay must return all 14 sample records",
    )
    print(
        f"selftest 3/4 ok: real EdgeC Modbus RTU codec, expected=14, actual=14,"
        f" report={edgec_path}"
    )

    corrupt_key = "meter1.grid_power"
    try:
        corrupt_edgec = EdgecModbusTarget(corrupt_key=corrupt_key)
    except TargetError as exc:
        _check(False, str(exc))
    edgec_corrupt = run_replay(trace_path, corrupt_edgec)
    edgec_corrupt_path = reports_dir / "selftest_edgec_corrupt.json"
    write_report(edgec_corrupt, edgec_corrupt_path)
    _check(not edgec_corrupt["ok"], "corrupted EdgeC register must report a difference")
    edgec_first = edgec_corrupt["first_divergence"]
    _check(edgec_first is not None, "corrupted EdgeC register must report a first divergence")
    _check(
        edgec_first["kind"] == "point" and edgec_first["key"] == corrupt_key,
        f"corrupted EdgeC report must name point {corrupt_key}",
    )
    _check(
        edgec_first["t_ms"] == 3000,
        "corrupted EdgeC report must give the first divergence at t_ms=3000",
    )

    TARGETS["edgec-modbus-corrupt"] = lambda: EdgecModbusTarget(corrupt_key=corrupt_key)
    try:
        exit_code = main(
            [
                "replay",
                "--trace",
                str(trace_path),
                "--target",
                "edgec-modbus-corrupt",
                "--report",
                str(reports_dir / "selftest_edgec_corrupt_cli.json"),
            ]
        )
    finally:
        del TARGETS["edgec-modbus-corrupt"]
    _check(exit_code == 1, "replay with a corrupted EdgeC register must exit 1")

    print(
        f"selftest 4/4 ok: corrupted register {corrupt_key},"
        f" first divergence t_ms={edgec_first['t_ms']}, exit={exit_code},"
        f" report={edgec_corrupt_path}"
    )
    print("SELFTEST PASS")
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Replay a JSONL trace and report field-level divergence."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("selftest", help="run the built-in zero-diff and mutation checks")

    replay = subparsers.add_parser("replay", help="replay one trace through a target")
    replay.add_argument("--trace", required=True, type=Path)
    replay.add_argument("--target", choices=sorted(TARGETS), default="mock")
    replay.add_argument("--report", type=Path, default=DEFAULT_REPORT)

    args = parser.parse_args(argv)
    try:
        if args.command == "selftest":
            return _cmd_selftest()

        report = run_replay(args.trace, TARGETS[args.target]())
        write_report(report, args.report)
        print(
            f"trace={args.trace} target={args.target}"
            f" expected={report['expected_records']} actual={report['actual_records']}"
            f" missing={report['missing']} extra={report['extra']}"
            f" mismatches={report['mismatches']}"
        )
        if report["ok"]:
            print(f"OK: no divergence, report={args.report}")
            return 0
        print(f"DIFF: first divergence {_describe_first(report['first_divergence'])}, report={args.report}")
        return 1
    except (TraceError, TargetError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
