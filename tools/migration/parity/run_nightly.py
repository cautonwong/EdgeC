#!/usr/bin/env python3
"""Run the parity selftest and the pinned golden replay matrix; fail closed."""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent
REPO_ROOT = BASE_DIR.parents[2]

sys.path.insert(0, str(BASE_DIR))
import run_parity  # noqa: E402  pinned hashes have a single source there

MATRIX = (
    {
        "name": "sample-mock",
        "trace": "samples/sample_trace.jsonl",
        "target": "mock",
        "sha256": run_parity.SAMPLE_TRACE_SHA256,
    },
    {
        "name": "sample-edgec-modbus",
        "trace": "samples/sample_trace.jsonl",
        "target": "edgec-modbus",
        "sha256": run_parity.SAMPLE_TRACE_SHA256,
    },
    {
        "name": "iems-mqtt",
        "trace": "samples/iems_mqtt_trace.jsonl",
        "target": "iems-mqtt",
        "sha256": run_parity.IEMS_MQTT_TRACE_SHA256,
    },
)


def _sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def _git_sha():
    result = subprocess.run(
        ["git", "-C", str(REPO_ROOT), "rev-parse", "HEAD"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"git rev-parse HEAD failed: {result.stderr.strip()}")
    return result.stdout.strip()


def _write_json(data, path):
    Path(path).write_text(
        json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )


def _read_json(path):
    try:
        return json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None


def _run_selftest(parity_dir, run_dir):
    result = subprocess.run(
        [sys.executable, str(Path(parity_dir) / "run_parity.py"), "selftest"],
        capture_output=True,
        text=True,
    )
    (Path(run_dir) / "selftest.log").write_text(
        result.stdout + result.stderr, encoding="utf-8"
    )
    return {"rc": result.returncode, "log": "selftest.log"}


def _report_view(report):
    return {
        "ok": report.get("ok"),
        "expected": report.get("expected_records"),
        "actual": report.get("actual_records"),
        "missing": report.get("missing"),
        "extra": report.get("extra"),
        "mismatches": report.get("mismatches"),
        "first_divergence": report.get("first_divergence"),
        "annotations": report.get("annotations"),
    }


def _run_cell(cell, parity_dir, run_dir):
    trace = Path(cell["trace"])
    if not trace.is_absolute():
        trace = Path(parity_dir) / trace
    report_path = Path(run_dir) / f"{cell['name']}.json"
    result = subprocess.run(
        [
            sys.executable,
            str(Path(parity_dir) / "run_parity.py"),
            "replay",
            "--trace",
            str(trace),
            "--target",
            cell["target"],
            "--trace-sha256",
            cell["sha256"],
            "--report",
            str(report_path),
        ],
        capture_output=True,
        text=True,
    )
    report = _read_json(report_path)
    if report is None:
        # Fail closed: a declared cell that produced no report is never green.
        return _entry(cell, 2, None), result
    if result.returncode not in (0, 1) or bool(report.get("ok")) != (result.returncode == 0):
        return _entry(cell, 2, _report_view(report)), result
    if result.returncode == 1 and report.get("first_divergence") is None and not (
        (report.get("annotations") or {}).get("first_divergence")
    ):
        # Fail closed: rc=1 must name the cell, field and t_ms.
        return _entry(cell, 2, _report_view(report)), result
    return _entry(cell, result.returncode, _report_view(report)), result


def _entry(cell, rc, report):
    return {
        "name": cell["name"],
        "trace": cell["trace"],
        "target": cell["target"],
        "rc": rc,
        "trace_sha256": cell["sha256"],
        "report": report,
    }


def _print_cell(entry, result):
    print(
        f"cell {entry['name']} trace={entry['trace']} target={entry['target']}"
        f" rc={entry['rc']} report={entry['name']}.json"
    )
    if entry["rc"] == 2:
        tail = (result.stderr or result.stdout).strip().splitlines()[-1:]
        if tail:
            print(f"cell {entry['name']}: {tail[0]}", file=sys.stderr)
    if entry["rc"] == 1:
        report = entry["report"]
        first = report.get("first_divergence")
        field = None
        if first is not None:
            kind, key = first["kind"], first["key"]
            field = ",".join(sorted(first["fields"]))
        else:
            first = (report.get("annotations") or {}).get("first_divergence")
            kind, key = None, first["key"]
            field = first["field"]
        print(
            f"DIFF: cell {entry['name']} key={key} field={field}"
            f" t_ms={first['t_ms']}"
        )


def _replace_dir(staging, run_dir):
    if run_dir.exists():
        shutil.rmtree(run_dir)
    os.replace(staging, run_dir)


def _target_versions(parity_dir):
    harness = Path(parity_dir) / "edgec_target" / "edgec_modbus_target"
    return {
        "mock": "in-process",
        "edgec-modbus": _sha256(harness) if harness.is_file() else None,
        "iems-mqtt": "in-process",
    }


def run(out_base, parity_dir=BASE_DIR, matrix=MATRIX):
    git_sha = _git_sha()
    run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ") + "-" + git_sha[:8]
    out_base = Path(out_base)
    out_base.mkdir(parents=True, exist_ok=True)
    run_dir = out_base / run_id
    staging = Path(tempfile.mkdtemp(prefix=".nightly-", dir=out_base))
    try:
        selftest = _run_selftest(parity_dir, staging)
        cells = []
        if selftest["rc"] != 0:
            print(
                f"selftest failed rc={selftest['rc']}; matrix not run", file=sys.stderr
            )
        else:
            for cell in matrix:
                entry, result = _run_cell(cell, parity_dir, staging)
                cells.append(entry)
                _print_cell(entry, result)
        rc = (
            2
            if selftest["rc"] != 0
            else max((cell["rc"] for cell in cells), default=0)
        )
        ok = (
            selftest["rc"] == 0
            and len(cells) == len(matrix)
            and all(
                cell["rc"] == 0 and cell["report"] is not None and cell["report"]["ok"]
                for cell in cells
            )
        )
        summary = {
            "run_id": run_id,
            "git_sha": git_sha,
            "pinned": {
                "SAMPLE_TRACE_SHA256": run_parity.SAMPLE_TRACE_SHA256,
                "IEMS_MQTT_TRACE_SHA256": run_parity.IEMS_MQTT_TRACE_SHA256,
            },
            "selftest": selftest,
            "cells": cells,
            "ok": ok,
        }
        manifest = {
            "run_id": run_id,
            "git_sha": git_sha,
            "pinned": {
                "SAMPLE_TRACE_SHA256": run_parity.SAMPLE_TRACE_SHA256,
                "IEMS_MQTT_TRACE_SHA256": run_parity.IEMS_MQTT_TRACE_SHA256,
            },
            "matrix": [dict(cell) for cell in matrix],
            "targets": _target_versions(parity_dir),
        }
        _write_json(summary, staging / "summary.json")
        _write_json(manifest, staging / "manifest.json")
        _replace_dir(staging, run_dir)
    finally:
        if staging.exists():
            shutil.rmtree(staging, ignore_errors=True)
    print(f"nightly run={run_dir} ok={ok} rc={rc}")
    return summary, rc


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Run the parity selftest and the pinned golden replay matrix."
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=BASE_DIR / "reports" / "nightly",
        help="base directory; each run writes <out>/<run-id>/",
    )
    args = parser.parse_args(argv)
    try:
        _, rc = run(args.out)
    except (RuntimeError, OSError) as exc:
        print(f"nightly error: {exc}", file=sys.stderr)
        return 2
    return rc


if __name__ == "__main__":
    sys.exit(main())
