#!/usr/bin/env python3
"""Nightly wrapper self-test: A2 injected divergence and A4 missing harness/sidecar."""

import hashlib
import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

PARITY_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(PARITY_DIR))

import run_nightly
import run_parity


def _sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def _read_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))


class NightlySelfTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)

    def _stage(self, name, with_harness):
        """Copy the parity tool into a temp dir so fixtures never touch the repo."""
        parity = Path(self.tmp.name) / name
        parity.mkdir()
        shutil.copy2(PARITY_DIR / "run_parity.py", parity / "run_parity.py")
        shutil.copytree(PARITY_DIR / "samples", parity / "samples")
        if with_harness:
            harness = PARITY_DIR / "edgec_target" / "edgec_modbus_target"
            self.assertTrue(
                harness.is_file(),
                f"{harness} is missing; build it with the cc command in README.md section 1.1",
            )
            (parity / "edgec_target").mkdir()
            shutil.copy2(harness, parity / "edgec_target" / "edgec_modbus_target")
        return parity

    def _diverged_matrix(self, parity):
        samples = parity / "samples"
        original = (samples / "iems_mqtt_trace.jsonl").read_text(encoding="utf-8")
        needle = (
            '"t_ms":1500,"source":"iems_v3","kind":"point",'
            '"key":"meter1.grid_power","value":488.5'
        )
        self.assertIn(needle, original, "injected fixture drifted from iems_mqtt_trace.jsonl")
        injected = samples / "injected_trace.jsonl"
        injected.write_text(original.replace(needle, needle.replace("488.5", "489.5")), encoding="utf-8")
        shutil.copy2(
            samples / "iems_mqtt_trace.annotations.json",
            samples / "injected_trace.annotations.json",
        )
        return [
            {
                "name": "injected-iems-mqtt",
                "trace": str(injected),
                "target": "iems-mqtt",
                "sha256": _sha256(injected),
            }
        ]

    def test_matrix_is_the_declared_three_cells(self):
        expected = {
            ("samples/sample_trace.jsonl", "mock", run_parity.SAMPLE_TRACE_SHA256),
            ("samples/sample_trace.jsonl", "edgec-modbus", run_parity.SAMPLE_TRACE_SHA256),
            ("samples/iems_mqtt_trace.jsonl", "iems-mqtt", run_parity.IEMS_MQTT_TRACE_SHA256),
        }
        actual = {(cell["trace"], cell["target"], cell["sha256"]) for cell in run_nightly.MATRIX}
        self.assertEqual(actual, expected)
        self.assertEqual(len(run_nightly.MATRIX), len(expected))

    def test_a2_injected_divergence_is_rc1_and_names_cell_field_t_ms(self):
        parity = self._stage("a2", with_harness=True)
        matrix = self._diverged_matrix(parity)
        out = Path(self.tmp.name) / "out"
        summary, rc = run_nightly.run(out, parity_dir=parity, matrix=matrix)

        self.assertEqual(rc, 1)
        self.assertFalse(summary["ok"])
        self.assertEqual(summary["selftest"]["rc"], 0)
        self.assertEqual(len(summary["cells"]), 1)
        cell = summary["cells"][0]
        self.assertEqual(cell["name"], "injected-iems-mqtt")
        self.assertEqual(cell["rc"], 1)
        self.assertFalse(cell["report"]["ok"])
        first = cell["report"]["annotations"]["first_divergence"]
        self.assertEqual(first["t_ms"], 1500)
        self.assertIn(first["field"], ("payload_hex", "fields.value"))
        run_dir = out / summary["run_id"]
        self.assertTrue((run_dir / "injected-iems-mqtt.json").is_file())
        self.assertEqual(_read_json(run_dir / "summary.json"), summary)
        self.assertEqual(_read_json(run_dir / "manifest.json")["run_id"], summary["run_id"])

    def test_a5_summary_is_deterministic_across_runs(self):
        parity = self._stage("a5", with_harness=True)
        matrix = self._diverged_matrix(parity)
        first, first_rc = run_nightly.run(Path(self.tmp.name) / "out1", parity_dir=parity, matrix=matrix)
        second, second_rc = run_nightly.run(Path(self.tmp.name) / "out2", parity_dir=parity, matrix=matrix)

        self.assertEqual(first_rc, 1)
        self.assertEqual(second_rc, 1)
        strip = lambda summary: {k: v for k, v in summary.items() if k != "run_id"}
        self.assertEqual(strip(first), strip(second))
        self.assertEqual(first["git_sha"], second["git_sha"])

    def test_a4_missing_harness_fails_closed(self):
        parity = self._stage("a4-harness", with_harness=False)
        out = Path(self.tmp.name) / "out"
        summary, rc = run_nightly.run(out, parity_dir=parity, matrix=run_nightly.MATRIX)

        self.assertEqual(rc, 2)
        self.assertFalse(summary["ok"])
        self.assertNotEqual(summary["selftest"]["rc"], 0)
        self.assertEqual(summary["cells"], [])
        run_dir = out / summary["run_id"]
        self.assertTrue((run_dir / "selftest.log").is_file())
        self.assertIn("edgec_modbus_target", (run_dir / "selftest.log").read_text(encoding="utf-8"))
        self.assertEqual(_read_json(run_dir / "summary.json"), summary)

    def test_a4_missing_sidecar_fails_closed(self):
        parity = self._stage("a4-sidecar", with_harness=True)
        samples = parity / "samples"
        orphan = samples / "orphan_trace.jsonl"
        shutil.copy2(samples / "iems_mqtt_trace.jsonl", orphan)
        matrix = [
            {
                "name": "orphan-iems-mqtt",
                "trace": str(orphan),
                "target": "iems-mqtt",
                "sha256": _sha256(orphan),
            }
        ]
        out = Path(self.tmp.name) / "out"
        summary, rc = run_nightly.run(out, parity_dir=parity, matrix=matrix)

        self.assertEqual(rc, 2)
        self.assertFalse(summary["ok"])
        self.assertEqual(len(summary["cells"]), 1)
        cell = summary["cells"][0]
        self.assertEqual(cell["name"], "orphan-iems-mqtt")
        self.assertEqual(cell["rc"], 2)
        self.assertIsNone(cell["report"])
        self.assertFalse((out / summary["run_id"] / "orphan-iems-mqtt.json").exists())


if __name__ == "__main__":
    unittest.main()
