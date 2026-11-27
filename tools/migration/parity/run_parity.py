#!/usr/bin/env python3
"""Replay a JSONL trace through a replaceable target and report field-level divergence."""

import argparse
import contextlib
import hashlib
import io
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path

TRACE_VERSION = 1
KINDS = ("point", "alarm", "command")
BASE_DIR = Path(__file__).resolve().parent
DEFAULT_REPORT = BASE_DIR / "reports" / "replay.report.json"
EDGEC_TARGET_BIN = BASE_DIR / "edgec_target" / "edgec_modbus_target"
# Pinned golden hash of samples/sample_trace.jsonl; update only with a deliberate trace change.
SAMPLE_TRACE_SHA256 = "32de3467bb747e751954575e2d12dd56ae80540771335cad9eb09f688463fa29"
# Pinned golden hash of samples/iems_mqtt_trace.jsonl; update only with a deliberate trace change.
IEMS_MQTT_TRACE = BASE_DIR / "samples" / "iems_mqtt_trace.jsonl"
IEMS_MQTT_TRACE_SHA256 = "3e1256b3f21d8bb75087a167eb7faf82239710b800fcb20988609e4247a264e7"
SELFTEST_CASES = 22


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


IEMS_MQTT_SN = "E4B853F333"
IEMS_MQTT_ENDPOINTS = {
    # P4-02c section 1.1 sample config: module/dev_id/type.
    "pcs1": {"module": "ems", "dev_id": "pcs456", "type": "pcsdata"},
    "meter1": {"module": "env", "dev_id": "ems", "type": "emsctrl"},
}
# v3 config/bmser_ipc_bin.json: block id and 1-based sub index; all three are S32 scale 10.
IEMS_MQTT_POINTS = {
    "pcs1.active_power": {"endpoint": "pcs1", "block_id": 0x8021, "start_sub_id": 12, "scale": 10},
    "pcs1.reactive_power": {"endpoint": "pcs1", "block_id": 0x8021, "start_sub_id": 16, "scale": 10},
    "meter1.grid_power": {"endpoint": "meter1", "block_id": 0x8001, "start_sub_id": 2, "scale": 10},
}
IEMS_MQTT_POINTS_BY_BLOCK = {
    (point["block_id"], point["start_sub_id"]): key
    for key, point in IEMS_MQTT_POINTS.items()
}
IEMS_MQTT_VERB = "GetResp"  # v3 publishes device data as GetResp (mqtt_north.c:935).


def _iems_mqtt_topic(endpoint_name):
    endpoint = IEMS_MQTT_ENDPOINTS[endpoint_name]
    return (
        f"ipc/{IEMS_MQTT_SN}/{endpoint['module']}/{endpoint['dev_id']}"
        f"/{endpoint['type']}/{IEMS_MQTT_VERB}/binaryFlow"
    )


def _iems_mqtt_payload(record, corrupt_key):
    """binaryFlow payload section: block_num(1) + block(P4-02b section 1.2)."""
    if record["kind"] != "point":
        raise TargetError(f"iems-mqtt target only maps point records, got {record['kind']!r}")
    point = IEMS_MQTT_POINTS.get(record["key"])
    if point is None:
        raise TargetError(f"no iems-mqtt mapping for key {record['key']!r}")
    value = record["value"]
    if type(value) not in (int, float):
        raise TargetError(f"iems-mqtt point {record['key']!r} needs a numeric value")
    scaled = value * point["scale"]
    # A finite value times the scale can overflow to inf (1e308 * 10); range-check
    # before round() so it is rejected instead of raising OverflowError.
    raw = round(scaled) if -(2**31) <= scaled <= 2**31 - 1 else None
    if raw is None or abs(scaled - raw) > 1e-9:
        raise TargetError(
            f"iems-mqtt point {record['key']!r} value {value!r} is not representable"
            f" as S32 at scale {point['scale']}"
        )
    if corrupt_key == record["key"]:
        raw += 1
    data = raw.to_bytes(4, "big", signed=True)
    block = (
        point["block_id"].to_bytes(4, "big")
        + point["start_sub_id"].to_bytes(2, "big")
        + (1).to_bytes(2, "big")
        + len(data).to_bytes(2, "big")
        + data
    )
    return b"\x01" + block


def _decode_iems_mqtt_payload(payload):
    if len(payload) != 15 or payload[0] != 1:
        raise TargetError("iems-mqtt frame must carry exactly one block")
    block = payload[1:]
    block_id = int.from_bytes(block[0:4], "big")
    start_sub_id = int.from_bytes(block[4:6], "big")
    sub_num = int.from_bytes(block[6:8], "big")
    data_len = int.from_bytes(block[8:10], "big")
    data = block[10:]
    if sub_num != 1 or data_len != 4:
        raise TargetError("iems-mqtt frame block must carry exactly one S32")
    key = IEMS_MQTT_POINTS_BY_BLOCK.get((block_id, start_sub_id))
    if key is None:
        raise TargetError(
            f"iems-mqtt frame has no mapping for block {block_id:#06x} sub {start_sub_id}"
        )
    return key, int.from_bytes(data, "big", signed=True)


class IemsMqttTarget:
    """The EdgeC IEMS MQTT northbound mapping, mirrored for the parity bench.

    Maps one trace point to the v3 topic (P4-02c section 1.1) and binaryFlow
    payload section (P4-02b section 1.2), then decodes the frame back. The
    annotation sidecar carries the v3-derived topic and frame bytes; a replay
    is only green when both the round trip and every annotation match.

    shortcut: the bench doubles the mapping in Python instead of linking the
    EdgeC C codec; upgrade when the product target can link edge_codec directly.
    """

    name = "iems-mqtt"

    def __init__(self, corrupt_key=None):
        if corrupt_key is not None and corrupt_key not in IEMS_MQTT_POINTS:
            raise TargetError(f"iems-mqtt target has no mapping for corrupt key {corrupt_key!r}")
        self.corrupt_key = corrupt_key
        self._annotations = None

    def prepare(self, trace_path, records):
        annotations_path = _iems_annotations_path(trace_path)
        if not annotations_path.is_file():
            raise TargetError(f"{annotations_path}: missing annotations sidecar")
        try:
            raw = json.loads(
                annotations_path.read_text(encoding="utf-8"),
                parse_constant=_reject_constant,
                object_pairs_hook=_reject_duplicate_keys,
            )
        except ValueError as exc:
            raise TargetError(f"{annotations_path}: invalid annotations JSON: {exc}") from exc
        if not isinstance(raw, dict):
            raise TargetError(f"{annotations_path}: annotations must be an object keyed by t_ms")
        expected = {}
        for record in records:
            key = str(record["t_ms"])
            if key in expected:
                raise TargetError(
                    f"{annotations_path}: duplicate t_ms {key} in trace; annotations need unique keys"
                )
            expected[key] = record
        missing = sorted(set(expected) - set(raw))
        if missing:
            raise TargetError(f"{annotations_path}: missing annotation for t_ms {missing[0]}")
        extra = sorted(set(raw) - set(expected))
        if extra:
            raise TargetError(
                f"{annotations_path}: annotation for unknown t_ms {extra[0]!r} has no trace record"
            )
        annotations = {}
        for key, entry in raw.items():
            where = f"{annotations_path}: annotation t_ms {key}"
            if not isinstance(entry, dict):
                raise TargetError(f"{where} must be an object")
            unknown = sorted(set(entry) - {"topic", "payload_hex", "fields"})
            if unknown:
                raise TargetError(f"{where}: unknown annotation field {unknown[0]!r}")
            lacking = sorted({"topic", "payload_hex", "fields"} - set(entry))
            if lacking:
                raise TargetError(f"{where}: missing annotation field {lacking[0]!r}")
            topic = entry["topic"]
            if not isinstance(topic, str) or not topic:
                raise TargetError(f"{where}: topic must be a non-empty string")
            payload_hex = entry["payload_hex"]
            if (
                not isinstance(payload_hex, str)
                or not payload_hex
                or len(payload_hex) % 2
                or any(char not in "0123456789abcdef" for char in payload_hex)
            ):
                raise TargetError(f"{where}: payload_hex must be lowercase even-length hex")
            fields = entry["fields"]
            if not isinstance(fields, dict):
                raise TargetError(f"{where}: fields must be an object")
            _reject_nonfinite_floats(fields, where, "fields")
            _reject_unencodable_strings(topic, where, "topic")
            _reject_unencodable_strings(payload_hex, where, "payload_hex")
            _reject_unencodable_strings(fields, where, "fields")
            annotations[key] = {
                "topic": topic,
                "payload_hex": payload_hex,
                "fields": fields,
            }
        self._annotations = annotations

    def apply(self, record):
        payload = _iems_mqtt_payload(record, self.corrupt_key)
        key, raw = _decode_iems_mqtt_payload(payload)
        return [
            {
                "v": TRACE_VERSION,
                "t_ms": record["t_ms"],
                "source": self.name,
                "kind": "point",
                "key": key,
                "value": raw / IEMS_MQTT_POINTS[key]["scale"],
            }
        ]

    def describe(self, record):
        payload = _iems_mqtt_payload(record, self.corrupt_key)
        key, raw = _decode_iems_mqtt_payload(payload)
        return {
            "topic": _iems_mqtt_topic(IEMS_MQTT_POINTS[key]["endpoint"]),
            "payload_hex": payload.hex(),
            "fields": {"value": raw / IEMS_MQTT_POINTS[key]["scale"]},
        }

    def annotation_report(self, records):
        mismatches = []
        for record in records:
            expected = self._annotations[str(record["t_ms"])]
            actual = self.describe(record)
            for field in ("topic", "payload_hex"):
                if expected[field] != actual[field]:
                    mismatches.append(
                        {
                            "field": field,
                            "t_ms": record["t_ms"],
                            "key": record["key"],
                            "expected": expected[field],
                            "actual": actual[field],
                        }
                    )
            wanted = expected["fields"]
            got = actual["fields"]
            for name in sorted(set(wanted) | set(got)):
                if name in wanted and name in got and _canon(wanted[name]) == _canon(got[name]):
                    continue
                mismatches.append(
                    {
                        "field": f"fields.{name}",
                        "t_ms": record["t_ms"],
                        "key": record["key"],
                        "expected": wanted.get(name),
                        "actual": got.get(name),
                    }
                )
        first = mismatches[0] if mismatches else None
        return {"ok": not mismatches, "mismatches": mismatches, "first_divergence": first}


TARGETS = {"mock": MockTarget, "edgec-modbus": EdgecModbusTarget, "iems-mqtt": IemsMqttTarget}


def _reject_constant(name):
    raise ValueError(f"non-finite number {name}")


def _reject_duplicate_keys(pairs):
    obj = {}
    for key, value in pairs:
        if key in obj:
            raise ValueError(f"duplicate key {key!r}")
        obj[key] = value
    return obj


def _reject_unencodable_strings(value, where, path):
    if isinstance(value, str):
        try:
            value.encode("utf-8")
        except UnicodeEncodeError:
            raise TargetError(f"{where}: {path} is not encodable as UTF-8") from None
    elif isinstance(value, dict):
        for index, (key, item) in enumerate(value.items()):
            _reject_unencodable_strings(key, where, f"{path} key {index}")
            _reject_unencodable_strings(item, where, f"{path}[{index}]")
    elif isinstance(value, list):
        for index, item in enumerate(value):
            _reject_unencodable_strings(item, where, f"{path}[{index}]")


def _reject_nonfinite_floats(value, where, path="value"):
    """A literal like 1e999 overflows to inf without parse_constant firing."""
    if isinstance(value, float) and not math.isfinite(value):
        raise TraceError(f"{where}: non-finite number at {path}")
    if isinstance(value, list):
        for index, item in enumerate(value):
            _reject_nonfinite_floats(item, where, f"{path}[{index}]")
    elif isinstance(value, dict):
        for key, item in value.items():
            _reject_nonfinite_floats(item, where, f"{path}.{key}")


def _sha256_file(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def _iems_annotations_path(trace_path):
    path = Path(trace_path)
    return path.with_name(path.stem + ".annotations.json")


def _verify_trace_sha256(path, expected):
    actual = _sha256_file(path)
    expected = expected.strip().lower()
    if actual != expected:
        raise TraceError(
            f"{path}: trace SHA256 mismatch: expected {expected} actual {actual}"
        )


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
    _reject_nonfinite_floats(raw["value"], where)
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
    prepare = getattr(target, "prepare", None)
    if prepare is not None:
        prepare(trace_path, expected)
    actual = []
    try:
        for record in expected:
            actual.extend(target.apply(record))
    finally:
        close = getattr(target, "close", None)
        if close is not None:
            close()
    report = compare(expected, actual)
    annotation_report = getattr(target, "annotation_report", None)
    if annotation_report is not None:
        annotations = annotation_report(expected)
        report["annotations"] = annotations
        if not annotations["ok"]:
            report["ok"] = False
    report["trace"] = str(trace_path)
    report["trace_sha256"] = _sha256_file(trace_path)
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


def _selftest_ok(index, message):
    print(f"selftest {index}/{SELFTEST_CASES} ok: {message}")


def _run_cli(argv):
    stderr = io.StringIO()
    try:
        with contextlib.redirect_stderr(stderr):
            return main(argv), stderr.getvalue()
    except SystemExit as exc:
        return exc.code, stderr.getvalue()


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


def _selftest_iems_mqtt(reports_dir):
    trace_path = IEMS_MQTT_TRACE
    _check(
        _sha256_file(trace_path) == IEMS_MQTT_TRACE_SHA256,
        "pinned golden hash must match samples/iems_mqtt_trace.jsonl;"
        " update IEMS_MQTT_TRACE_SHA256 only with a deliberate trace change",
    )

    try:
        target = IemsMqttTarget()
    except TargetError as exc:
        _check(False, str(exc))
    golden = run_replay(trace_path, target)
    golden_path = reports_dir / "selftest_iems_mqtt.json"
    write_report(golden, golden_path)
    _check(golden["ok"], "IEMS MQTT golden replay must report zero divergence")
    _check(
        golden["expected_records"] == golden["actual_records"] == 8,
        "IEMS MQTT golden replay must return all 8 sample records",
    )
    _check(golden["first_divergence"] is None, "IEMS MQTT golden replay must have no divergence")
    _check(
        golden["annotations"]["ok"] and golden["annotations"]["mismatches"] == [],
        "IEMS MQTT golden frames must match every v3 annotation",
    )
    _selftest_ok(
        11,
        f"IEMS MQTT golden replay and annotations, expected=8, actual=8, report={golden_path}",
    )

    victim = next(
        record
        for record in load_trace(trace_path)
        if record["key"] == "pcs1.reactive_power"
    )
    try:
        corrupt_target = IemsMqttTarget(corrupt_key=victim["key"])
    except TargetError as exc:
        _check(False, str(exc))
    corrupt = run_replay(trace_path, corrupt_target)
    corrupt_path = reports_dir / "selftest_iems_mqtt_corrupt.json"
    write_report(corrupt, corrupt_path)
    _check(not corrupt["ok"], "corrupted IEMS MQTT payload must report a difference")
    first = corrupt["first_divergence"]
    _check(first is not None, "corrupted IEMS MQTT payload must report a first divergence")
    _check(
        first["key"] == victim["key"] and first["t_ms"] == victim["t_ms"],
        f"corrupted IEMS MQTT payload must name {victim['key']} at t_ms={victim['t_ms']}",
    )
    expected_actual = (round(victim["value"] * 10) + 1) / 10
    _check(
        first["fields"].get("value", {}).get("expected") == victim["value"]
        and first["fields"].get("value", {}).get("actual") == expected_actual,
        "corrupted IEMS MQTT report must show the value on both sides",
    )
    annotation_first = corrupt["annotations"]["first_divergence"]
    _check(annotation_first is not None, "corrupted IEMS MQTT frame must diverge from the annotation")
    _check(
        annotation_first["field"] == "payload_hex"
        and annotation_first["t_ms"] == victim["t_ms"],
        f"corrupted IEMS MQTT annotation report must name payload_hex at t_ms={victim['t_ms']}",
    )

    TARGETS["iems-mqtt-corrupt"] = lambda: IemsMqttTarget(corrupt_key=victim["key"])
    try:
        exit_code = main(
            [
                "replay",
                "--trace",
                str(trace_path),
                "--target",
                "iems-mqtt-corrupt",
                "--report",
                str(reports_dir / "selftest_iems_mqtt_corrupt_cli.json"),
            ]
        )
    finally:
        del TARGETS["iems-mqtt-corrupt"]
    _check(exit_code == 1, "replay with a corrupted IEMS MQTT payload must exit 1")
    _selftest_ok(
        12,
        f"corrupted IEMS MQTT payload {victim['key']}, first divergence"
        f" t_ms={first['t_ms']}, exit={exit_code}, report={corrupt_path}",
    )

    annotations_path = _iems_annotations_path(trace_path)
    annotations = json.loads(annotations_path.read_text(encoding="utf-8"))
    victim_key = str(victim["t_ms"])

    def write_variant(directory, name, records, annotation_doc):
        trace_copy = directory / f"{name}.jsonl"
        if records is None:
            trace_copy.write_bytes(trace_path.read_bytes())
        else:
            trace_copy.write_text(
                "".join(json.dumps(record, ensure_ascii=False) + "\n" for record in records),
                encoding="utf-8",
            )
        (directory / f"{name}.annotations.json").write_text(
            json.dumps(annotation_doc, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
        return trace_copy

    with tempfile.TemporaryDirectory(prefix="parity-iems-") as temp_dir:
        temp = Path(temp_dir)

        tampered_doc = json.loads(json.dumps(annotations))
        hex_value = tampered_doc[victim_key]["payload_hex"]
        tampered_doc[victim_key]["payload_hex"] = hex_value[:-1] + (
            "0" if hex_value[-1] != "0" else "1"
        )
        tampered_trace = write_variant(temp, "tampered_annotation", None, tampered_doc)
        tampered_report_path = temp / "tampered_annotation.report.json"
        exit_code, stderr = _run_cli(
            [
                "replay",
                "--trace",
                str(tampered_trace),
                "--target",
                "iems-mqtt",
                "--trace-sha256",
                IEMS_MQTT_TRACE_SHA256,
                "--report",
                str(tampered_report_path),
            ]
        )
        _check(exit_code == 1, f"tampered annotation must exit 1: {stderr}")
        tampered_report = json.loads(tampered_report_path.read_text(encoding="utf-8"))
        annotation_first = tampered_report["annotations"]["first_divergence"]
        _check(
            annotation_first["field"] == "payload_hex"
            and annotation_first["t_ms"] == victim["t_ms"],
            "tampered annotation report must name payload_hex and its t_ms",
        )
        _check(
            tampered_report["first_divergence"] is None,
            "annotation-only tamper must not change the trace comparator verdict",
        )
        _selftest_ok(
            13,
            f"tampered annotation rejected, field=payload_hex t_ms={victim['t_ms']}, exit={exit_code}",
        )

        missing_doc = json.loads(json.dumps(annotations))
        del missing_doc[victim_key]
        missing_trace = write_variant(temp, "missing_annotation", None, missing_doc)
        exit_code, stderr = _run_cli(
            [
                "replay",
                "--trace",
                str(missing_trace),
                "--target",
                "iems-mqtt",
                "--trace-sha256",
                IEMS_MQTT_TRACE_SHA256,
                "--report",
                str(temp / "missing_annotation.report.json"),
            ]
        )
        _check(exit_code == 2, f"missing annotation must exit 2: {stderr}")
        _check(
            "missing annotation" in stderr and victim_key in stderr,
            f"missing annotation error must name t_ms={victim_key}: {stderr}",
        )
        _selftest_ok(14, f"missing annotation rejected, t_ms={victim_key}, exit={exit_code}")

        extra_doc = json.loads(json.dumps(annotations))
        extra_doc[victim_key]["note"] = "not-v3"
        extra_trace = write_variant(temp, "extra_annotation_field", None, extra_doc)
        exit_code, stderr = _run_cli(
            [
                "replay",
                "--trace",
                str(extra_trace),
                "--target",
                "iems-mqtt",
                "--trace-sha256",
                IEMS_MQTT_TRACE_SHA256,
                "--report",
                str(temp / "extra_annotation_field.report.json"),
            ]
        )
        _check(exit_code == 2, f"unknown annotation field must exit 2: {stderr}")
        _check(
            "unknown annotation field" in stderr and "note" in stderr,
            f"unknown annotation field error must name 'note': {stderr}",
        )
        _selftest_ok(15, f"unknown annotation field rejected, exit={exit_code}")

        records = load_trace(trace_path)
        records[2]["key"] = "pcs1.unknown_point"
        unknown_trace = write_variant(temp, "unknown_key", records, annotations)
        exit_code, stderr = _run_cli(
            [
                "replay",
                "--trace",
                str(unknown_trace),
                "--target",
                "iems-mqtt",
                "--trace-sha256",
                _sha256_file(unknown_trace),
                "--report",
                str(temp / "unknown_key.report.json"),
            ]
        )
        _check(exit_code == 2, f"unknown key must exit 2: {stderr}")
        _check(
            "no iems-mqtt mapping" in stderr and "pcs1.unknown_point" in stderr,
            f"unknown key error must name the key: {stderr}",
        )
        _selftest_ok(16, f"unknown key rejected, exit={exit_code}")

        records = load_trace(trace_path)
        records[0]["value"] = 999.0
        tampered_trace = write_variant(temp, "tampered_trace", records, annotations)
        actual_hash = _sha256_file(tampered_trace)
        _check(
            actual_hash != IEMS_MQTT_TRACE_SHA256,
            "tampered IEMS trace must not keep the pinned hash",
        )
        exit_code, stderr = _run_cli(
            [
                "replay",
                "--trace",
                str(tampered_trace),
                "--target",
                "iems-mqtt",
                "--trace-sha256",
                IEMS_MQTT_TRACE_SHA256,
                "--report",
                str(temp / "tampered_trace.report.json"),
            ]
        )
        _check(exit_code == 2, f"tampered IEMS trace hash must exit 2: {stderr}")
        _check(
            IEMS_MQTT_TRACE_SHA256 in stderr and actual_hash in stderr,
            f"tampered IEMS trace error must show expected and actual hashes: {stderr}",
        )
        _selftest_ok(17, f"tampered IEMS trace rejected before replay, exit={exit_code}")

        overflow_records = load_trace(trace_path)
        overflow_record = next(
            record for record in overflow_records if record["key"] == "pcs1.active_power"
        )
        for value, label in ((1e308, "1e+308"), (1e307, "1e+307")):
            overflow_record["value"] = value
            name = f"overflow_{label.replace('+', '')}"
            overflow_trace = write_variant(temp, name, overflow_records, annotations)
            exit_code, stderr = _run_cli(
                [
                    "replay",
                    "--trace",
                    str(overflow_trace),
                    "--target",
                    "iems-mqtt",
                    "--trace-sha256",
                    _sha256_file(overflow_trace),
                    "--report",
                    str(temp / f"{name}.report.json"),
                ]
            )
            _check(exit_code == 2, f"overflow value {label} must exit 2: {stderr}")
            _check(
                "not representable as S32" in stderr and label in stderr,
                f"overflow value {label} error must name the value: {stderr}",
            )
            _check(
                "Traceback" not in stderr,
                f"overflow value {label} must not traceback: {stderr}",
            )
        _selftest_ok(18, "overflow values 1e+308 and 1e+307 rejected cleanly, exit=2")

        duplicate_entry = json.loads(json.dumps(annotations[victim_key]))
        payload_hex = duplicate_entry["payload_hex"]
        duplicate_entry["payload_hex"] = payload_hex[:-1] + (
            "0" if payload_hex[-1] != "0" else "1"
        )
        parts = []
        for key, entry in annotations.items():
            if key == victim_key:
                parts.append(
                    f"{json.dumps(key)}: {json.dumps(duplicate_entry, ensure_ascii=False)}"
                )
            parts.append(f"{json.dumps(key)}: {json.dumps(entry, ensure_ascii=False)}")
        duplicate_trace = temp / "duplicate_key.jsonl"
        duplicate_trace.write_bytes(trace_path.read_bytes())
        (temp / "duplicate_key.annotations.json").write_text(
            "{\n" + ",\n".join(parts) + "\n}\n", encoding="utf-8"
        )
        exit_code, stderr = _run_cli(
            [
                "replay",
                "--trace",
                str(duplicate_trace),
                "--target",
                "iems-mqtt",
                "--trace-sha256",
                IEMS_MQTT_TRACE_SHA256,
                "--report",
                str(temp / "duplicate_key.report.json"),
            ]
        )
        _check(exit_code == 2, f"duplicate annotation key must exit 2: {stderr}")
        _check(
            "duplicate key" in stderr and victim_key in stderr,
            f"duplicate annotation key error must name t_ms={victim_key}: {stderr}",
        )
        _selftest_ok(
            19, f"duplicate annotation key rejected, t_ms={victim_key}, exit={exit_code}"
        )

        topic_doc = json.loads(json.dumps(annotations))
        topic_doc[victim_key]["topic"] = topic_doc[victim_key]["topic"].replace(
            "GetResp", "SetResp"
        )
        topic_trace = write_variant(temp, "topic_only", None, topic_doc)
        topic_report_path = temp / "topic_only.report.json"
        exit_code, stderr = _run_cli(
            [
                "replay",
                "--trace",
                str(topic_trace),
                "--target",
                "iems-mqtt",
                "--trace-sha256",
                IEMS_MQTT_TRACE_SHA256,
                "--report",
                str(topic_report_path),
            ]
        )
        _check(exit_code == 1, f"topic-only annotation mismatch must exit 1: {stderr}")
        topic_report = json.loads(topic_report_path.read_text(encoding="utf-8"))
        topic_first = topic_report["annotations"]["first_divergence"]
        _check(
            topic_first["field"] == "topic" and topic_first["t_ms"] == victim["t_ms"],
            f"topic-only annotation mismatch must report field=topic t_ms={victim['t_ms']}",
        )
        _check(
            topic_report["first_divergence"] is None,
            "topic-only annotation mismatch must not change the trace comparator verdict",
        )
        _selftest_ok(
            20,
            f"topic-only annotation mismatch reported, field=topic t_ms={victim['t_ms']},"
            f" exit={exit_code}",
        )

        fields_doc = json.loads(json.dumps(annotations))
        fields_doc[victim_key]["fields"]["value"] = victim["value"] + 1.0
        fields_trace = write_variant(temp, "fields_only", None, fields_doc)
        fields_report_path = temp / "fields_only.report.json"
        exit_code, stderr = _run_cli(
            [
                "replay",
                "--trace",
                str(fields_trace),
                "--target",
                "iems-mqtt",
                "--trace-sha256",
                IEMS_MQTT_TRACE_SHA256,
                "--report",
                str(fields_report_path),
            ]
        )
        _check(exit_code == 1, f"fields-only annotation mismatch must exit 1: {stderr}")
        fields_report = json.loads(fields_report_path.read_text(encoding="utf-8"))
        fields_first = fields_report["annotations"]["first_divergence"]
        _check(
            fields_first["field"] == "fields.value"
            and fields_first["t_ms"] == victim["t_ms"],
            f"fields-only annotation mismatch must report field=fields.value"
            f" t_ms={victim['t_ms']}",
        )
        _check(
            fields_report["first_divergence"] is None,
            "fields-only annotation mismatch must not change the trace comparator verdict",
        )
        _selftest_ok(
            21,
            f"fields-only annotation mismatch reported, field=fields.value"
            f" t_ms={victim['t_ms']}, exit={exit_code}",
        )

        surrogate_docs = []
        surrogate_topic = json.loads(json.dumps(annotations))
        surrogate_topic[victim_key]["topic"] = "\ud800"
        surrogate_docs.append(("surrogate_topic", surrogate_topic))
        surrogate_field_key = json.loads(json.dumps(annotations))
        surrogate_field_key[victim_key]["fields"]["\ud800"] = 0.0
        surrogate_docs.append(("surrogate_fields_key", surrogate_field_key))
        surrogate_field_value = json.loads(json.dumps(annotations))
        surrogate_field_value[victim_key]["fields"]["value"] = "\ud800"
        surrogate_docs.append(("surrogate_fields_value", surrogate_field_value))
        for name, doc in surrogate_docs:
            surrogate_trace = temp / f"{name}.jsonl"
            surrogate_trace.write_bytes(trace_path.read_bytes())
            (temp / f"{name}.annotations.json").write_text(
                json.dumps(doc, ensure_ascii=True) + "\n", encoding="utf-8"
            )
            report_path = temp / f"{name}.report.json"
            exit_code, stderr = _run_cli(
                [
                    "replay",
                    "--trace",
                    str(surrogate_trace),
                    "--target",
                    "iems-mqtt",
                    "--trace-sha256",
                    IEMS_MQTT_TRACE_SHA256,
                    "--report",
                    str(report_path),
                ]
            )
            _check(exit_code == 2, f"{name} must exit 2: {stderr}")
            _check(
                "not encodable as UTF-8" in stderr,
                f"{name} error must name the UTF-8 rejection: {stderr}",
            )
            _check("Traceback" not in stderr, f"{name} must not traceback: {stderr}")
            _check(
                not report_path.exists(),
                f"{name} must not leave a report behind, report={report_path}",
            )
        _selftest_ok(22, "lone surrogates in topic and fields rejected, exit=2, no report")


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
    _selftest_ok(
        1,
        f"identical replay, expected={mirror['expected_records']},"
        f" actual={mirror['actual_records']}, report={mirror_path}",
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

    _selftest_ok(
        2,
        f"corrupted {first['kind']} {first['key']},"
        f" first divergence t_ms={first['t_ms']}, exit={exit_code}, report={corrupt_path}",
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
    _selftest_ok(
        3,
        f"real EdgeC Modbus RTU codec, expected=14, actual=14, report={edgec_path}",
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

    _selftest_ok(
        4,
        f"corrupted register {corrupt_key}, first divergence t_ms={edgec_first['t_ms']},"
        f" exit={exit_code}, report={edgec_corrupt_path}",
    )

    golden = SAMPLE_TRACE_SHA256
    _check(
        _sha256_file(trace_path) == golden,
        "pinned golden hash must match samples/sample_trace.jsonl;"
        " update SAMPLE_TRACE_SHA256 only with a deliberate trace change",
    )
    ok_path = reports_dir / "selftest_sha256_ok.json"
    exit_code, stderr = _run_cli(
        [
            "replay",
            "--trace",
            str(trace_path),
            "--target",
            "mock",
            "--trace-sha256",
            golden,
            "--report",
            str(ok_path),
        ]
    )
    _check(exit_code == 0, f"replay with the pinned trace hash must exit 0: {stderr}")
    _selftest_ok(
        5, f"pinned trace SHA256 accepted, exit={exit_code}, report={ok_path}"
    )

    with tempfile.TemporaryDirectory(prefix="parity-selftest-") as temp_dir:
        temp = Path(temp_dir)
        raw = [
            json.loads(line)
            for line in trace_path.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ]

        def write_variant(name, records):
            path = temp / name
            path.write_text(
                "".join(json.dumps(record, ensure_ascii=False) + "\n" for record in records),
                encoding="utf-8",
            )
            return path

        tampered = write_variant("tampered.jsonl", [dict(raw[0], value=999.0), *raw[1:]])
        tampered_hash = _sha256_file(tampered)
        _check(tampered_hash != golden, "tampered trace must not keep the pinned hash")
        exit_code, stderr = _run_cli(
            [
                "replay",
                "--trace",
                str(tampered),
                "--target",
                "edgec-modbus",
                "--trace-sha256",
                golden,
                "--report",
                str(temp / "tampered.report.json"),
            ]
        )
        _check(exit_code != 0, "replay with a mismatched trace hash must exit non-zero")
        _check(
            golden in stderr and tampered_hash in stderr,
            f"hash mismatch error must show expected and actual hashes: {stderr}",
        )
        _selftest_ok(
            6,
            f"tampered trace rejected before replay, exit={exit_code}, actual={tampered_hash}",
        )

        missing_field = [dict(record) for record in raw]
        del missing_field[2]["key"]
        malformed = (
            (
                "missing field",
                write_variant("missing_field.jsonl", missing_field),
                "missing field",
            ),
            (
                "bad version",
                write_variant("bad_version.jsonl", [dict(raw[0], v=2), *raw[1:]]),
                "unsupported v",
            ),
            (
                "out of order",
                write_variant("out_of_order.jsonl", [raw[0], raw[2], raw[1], *raw[3:]]),
                "before the previous record",
            ),
        )
        for index, (label, path, reason) in enumerate(malformed, 7):
            exit_code, stderr = _run_cli(
                [
                    "replay",
                    "--trace",
                    str(path),
                    "--target",
                    "mock",
                    "--trace-sha256",
                    _sha256_file(path),
                    "--report",
                    str(temp / f"{path.stem}.report.json"),
                ]
            )
            _check(
                exit_code != 0,
                f"malformed trace ({label}) must exit non-zero even with a matching hash",
            )
            _check(
                reason in stderr,
                f"malformed trace ({label}) must be rejected for {reason!r}: {stderr}",
            )
            _selftest_ok(index, f"{label} rejected, exit={exit_code}")

        nonfinite = temp / "nonfinite.jsonl"
        nonfinite.write_text(
            '{"v": 1, "t_ms": 0, "source": "selftest", "kind": "point",'
            ' "key": "selftest.nonfinite", "value": 1e999}\n',
            encoding="utf-8",
        )
        exit_code, stderr = _run_cli(
            [
                "replay",
                "--trace",
                str(nonfinite),
                "--target",
                "mock",
                "--trace-sha256",
                _sha256_file(nonfinite),
                "--report",
                str(temp / "nonfinite.report.json"),
            ]
        )
        _check(exit_code == 2, f"non-finite value must exit 2: {stderr}")
        _check(
            "non-finite number at value" in stderr,
            f"non-finite value must be rejected by the loader: {stderr}",
        )
        _check("Traceback" not in stderr, f"non-finite value must not traceback: {stderr}")
        _selftest_ok(10, f"non-finite value rejected, exit={exit_code}")

    _selftest_iems_mqtt(reports_dir)

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
    replay.add_argument(
        "--trace-sha256",
        metavar="HEX",
        help="expected SHA256 of the raw trace file; abort before replay if it does not match",
    )

    args = parser.parse_args(argv)
    try:
        if args.command == "selftest":
            return _cmd_selftest()

        if args.trace_sha256 is not None:
            _verify_trace_sha256(args.trace, args.trace_sha256)
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
        if report["first_divergence"] is not None:
            print(
                f"DIFF: first divergence {_describe_first(report['first_divergence'])},"
                f" report={args.report}"
            )
        else:
            divergence = report["annotations"]["first_divergence"]
            print(
                f"DIFF: annotation field {divergence['field']}"
                f" at t_ms={divergence['t_ms']}"
                f" expected {json.dumps(divergence['expected'], ensure_ascii=False)}"
                f" actual {json.dumps(divergence['actual'], ensure_ascii=False)},"
                f" report={args.report}"
            )
        return 1
    except (TraceError, TargetError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
