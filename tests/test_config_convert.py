#!/usr/bin/env python3
"""Golden and fixture tests for the P1-03b config converter."""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONVERTER = os.path.join(REPO_ROOT, "tools", "migration", "convert_config.py")
SCHEMA_TOOL = os.path.join(REPO_ROOT, "tools", "migration", "config_schema.py")
GOLDEN_DIR = os.path.join(REPO_ROOT, "tools", "migration", "golden")
FIXTURES_DIR = os.path.join(REPO_ROOT, "tests", "guards", "fixtures")
FIXTURE_SOURCE = os.path.join(FIXTURES_DIR, "config_convert_src")
SNAPSHOT = "/home/edge/iems_v3/config"
FAMILIES = ("device", "port", "ems", "ems_lc_list", "system", "lc",
            "dev_group", "automatic_control", "mqtt_emms2", "mqtt_north_cfg",
            "bmser_ipc_bin", "mqtt_pub_dev_list", "proto_sys", "north_map",
            "rate_fig", "price", "proto_trigger", "iec104_legacy")
DEVICE_PORT = ("device", "port")

sys.path.insert(0, os.path.join(REPO_ROOT, "tools", "migration"))
import config_schema
import convert_config


def run(argv):
    return subprocess.run(
        [sys.executable] + argv, capture_output=True, text=True)


def convert(family, root, out):
    return run([CONVERTER, "--family", family, "--root", root, "--out", out])


def relative_files(root):
    result = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        for name in sorted(filenames):
            result.append(
                os.path.relpath(os.path.join(dirpath, name), root).replace(
                    os.sep, "/"))
    return sorted(result)


class TestConfigConvert(unittest.TestCase):
    def convert_all(self, source_root, output_root):
        for family in FAMILIES:
            proc = convert(family, source_root, output_root)
            self.assertEqual(
                proc.returncode, 0, proc.stdout + proc.stderr)

    def test_fixture_source_golden_is_byte_exact(self):
        """CI main path: sanitized fixture source -> golden, byte for byte."""
        with tempfile.TemporaryDirectory() as first:
            self.convert_all(FIXTURE_SOURCE, first)
            golden_files = set()
            for family in FAMILIES:
                golden_root = os.path.join(GOLDEN_DIR, family)
                files = relative_files(golden_root)
                self.assertTrue(files, f"no golden files under {golden_root}")
                for relative_path in files:
                    golden_files.add(relative_path)
                    with open(os.path.join(golden_root, relative_path), "rb") as handle:
                        expected = handle.read()
                    produced_path = os.path.join(first, relative_path)
                    self.assertTrue(
                        os.path.isfile(produced_path),
                        f"converter did not produce {relative_path}")
                    with open(produced_path, "rb") as handle:
                        actual = handle.read()
                    self.assertEqual(
                        actual, expected,
                        f"golden mismatch for {relative_path}")
            self.assertEqual(
                set(relative_files(first)), golden_files,
                "converter produced files outside the golden set")

            with tempfile.TemporaryDirectory() as second:
                self.convert_all(FIXTURE_SOURCE, second)
                for relative_path in sorted(golden_files):
                    with open(os.path.join(first, relative_path), "rb") as handle:
                        first_bytes = handle.read()
                    with open(os.path.join(second, relative_path), "rb") as handle:
                        second_bytes = handle.read()
                    self.assertEqual(
                        first_bytes, second_bytes,
                        f"conversion is not deterministic for {relative_path}")

            proc = run([SCHEMA_TOOL, "--root", first, "--mode", "converted"])
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertIn("RESULT OK", proc.stdout)

    def test_real_snapshot_converts_locally(self):
        """Secondary local check; byte-exact pinning is the fixture test above."""
        if not os.path.isdir(SNAPSHOT):
            self.skipTest(
                f"real snapshot {SNAPSHOT} not found; "
                "local snapshot conversion SKIPPED")
        expected = {
            "device": ("WROTE 8 file(s)", "6 symbolic channel(s)",
                       "12 host:port endpoint(s)", "18 template(s)"),
            "port": ("WROTE 1 file(s)", "kept=167", "ignored=2"),
            "ems": ("WROTE 1 file(s)", "kept=13", "deprecated=5"),
            "ems_lc_list": ("WROTE 1 file(s)", "kept=30"),
            "system": ("WROTE 1 file(s)", "kept=16", "ignored=2"),
            "lc": ("WROTE 6 file(s)", "kept=24"),
            "dev_group": ("10 device no(s) checked",),
            "automatic_control": (
                "WROTE 2 file(s)", "27 point reference(s) not checked"),
            "mqtt_emms2": (
                "WROTE 2 file(s)", "secret_replaced=1", "secret_unset=1"),
            "mqtt_north_cfg": ("WROTE 1 file(s)", "kept=18"),
            "bmser_ipc_bin": ("WROTE 1 file(s)", "kept=629",
                              "170 tag reference(s) not checked"),
            "mqtt_pub_dev_list": ("WROTE 1 file(s)", "kept=55",
                                  "50 tag(s) unverified"),
            "proto_sys": ("WROTE 1 file(s)", "kept=5"),
            "north_map": ("WROTE 1 file(s)", "kept=1184",
                          "PASS      north_map.point address",
                          "194 point address(es)"),
            "rate_fig": ("WROTE 1 file(s)", "kept=840"),
            "price": ("WROTE 1 file(s)", "kept=24"),
            "proto_trigger": ("WROTE 1 file(s)", "kept=2"),
            "iec104_legacy": ("WROTE 1 file(s)", "kept=895"),
        }
        with tempfile.TemporaryDirectory() as output:
            results = {}
            for family in FAMILIES:
                proc = convert(family, SNAPSHOT, output)
                results[family] = proc
                self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
                for needle in expected[family]:
                    self.assertIn(needle, proc.stdout, f"{family}: {needle}")
            self.assertEqual(
                set(relative_files(output)),
                set(relative_files(FIXTURE_SOURCE)),
                "snapshot conversion produced an unexpected file set")
            self.assertEqual(
                set(relative_files(output)), set(relative_files(SNAPSHOT)),
                "snapshot conversion did not cover every JSON file")
            with open(os.path.join(SNAPSHOT, "stactrl_mqtt.json"),
                      "r", encoding="utf-8") as handle:
                source_pass = json.load(handle)["pass"]
            with open(os.path.join(output, "stactrl_mqtt.json"),
                      "r", encoding="utf-8") as handle:
                converted_text = handle.read()
            self.assertIn("$secret:mqtt_emms2.stactrl_mqtt.pass", converted_text)
            if source_pass:
                self.assertNotIn(source_pass, converted_text)
                self.assertNotIn(
                    source_pass, results["mqtt_emms2"].stdout,
                    "plaintext stactrl pass leaked into the conversion report")
            with open(os.path.join(output, "HXD610.json"),
                      "r", encoding="utf-8") as handle:
                self.assertEqual(json.load(handle)["password"], "")
            proc = run([SCHEMA_TOOL, "--root", output, "--mode", "converted"])
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertIn("RESULT OK", proc.stdout)
            proc = run([SCHEMA_TOOL, "--root", SNAPSHOT])
            self.assertNotIn("SKIP", proc.stdout)
            self.assertIn("0 uncovered file(s)", proc.stdout)

    def test_fixture_conversion_is_deterministic(self):
        fixture = os.path.join(FIXTURES_DIR, "good_convert_config")
        with tempfile.TemporaryDirectory() as first, \
                tempfile.TemporaryDirectory() as second:
            for family in DEVICE_PORT:
                for output in (first, second):
                    proc = convert(family, fixture, output)
                    self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertEqual(relative_files(first), relative_files(second))
            for relative_path in relative_files(first):
                with open(os.path.join(first, relative_path), "rb") as handle:
                    first_bytes = handle.read()
                with open(os.path.join(second, relative_path), "rb") as handle:
                    second_bytes = handle.read()
                self.assertEqual(first_bytes, second_bytes, relative_path)
                self.assertIn(b'"schema_version": "1.0.0"', first_bytes)
            proc = run([SCHEMA_TOOL, "--root", first, "--mode", "converted"])
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def test_port_drops_ignored_keys(self):
        fixture = os.path.join(FIXTURES_DIR, "good_convert_config")
        with tempfile.TemporaryDirectory() as output:
            proc = convert("port", fixture, output)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertIn("ignored=2", proc.stdout)
            with open(os.path.join(output, "port_config.json"),
                      "r", encoding="utf-8") as handle:
                document = handle.read()
            self.assertNotIn('"ver"', document)
            self.assertNotIn('"type"', document)

    def test_plaintext_secret_becomes_reference(self):
        fixture = os.path.join(FIXTURES_DIR, "bad_convert_plaintext_secret")
        plaintext = "placeholder-not-a-secret"
        with tempfile.TemporaryDirectory() as output:
            proc = run([CONVERTER, "--family", "device",
                        "--root", fixture, "--out", output])
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertNotIn(plaintext, proc.stdout + proc.stderr)
            with open(os.path.join(output, "device_config.json"),
                      "r", encoding="utf-8") as handle:
                document = handle.read()
            self.assertNotIn(plaintext, document)
            self.assertIn("$secret:device.device_config.password", document)

    def test_dry_run_reports_counts(self):
        fixture = os.path.join(FIXTURES_DIR, "good_convert_config")
        proc = run([CONVERTER, "--family", "device",
                    "--root", fixture, "--dry-run"])
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        self.assertIn("file device_config.json: kept=7", proc.stdout)
        self.assertIn("-> OK", proc.stdout)
        self.assertIn("RESULT OK", proc.stdout)

    def test_fixture_source_family_reports(self):
        """Every supported family reports its dry-run counts on the fixtures."""
        expected = {
            "device": ("file device_config.json: kept=63",
                       "PASS      device.channel", "18 template(s)"),
            "port": ("kept=167", "ignored=2"),
            "ems": ("file ems_cfg.json: kept=13", "dropped=5", "deprecated=5"),
            "ems_lc_list": ("file ems_lc_list.json: kept=30",),
            "system": ("file system_cfg.json: kept=16", "ignored=2"),
            "lc": ("file lc/lc_cfg.json: kept=24",
                   "file lc_devs/LC1/lc_user_cfg.json: kept=9"),
            "dev_group": ("PASS      dev_group.mp/no", "10 device no(s) checked"),
            "automatic_control": ("4 point reference(s) not checked",),
            "mqtt_emms2": ("file HXD610.json: kept=14", "secret_unset=1",
                           "file stactrl_mqtt.json: kept=9", "secret_replaced=1"),
            "mqtt_north_cfg": ("file north/mqtt_north_cfg.json: kept=24",
                               "secret_replaced=1"),
            "bmser_ipc_bin": ("file bmser_ipc_bin.json: kept=12",
                              "UNCHECKED bmser_ipc_bin.tag",
                              "2 tag reference(s)"),
            "mqtt_pub_dev_list": ("file mqtt_pub_dev_list.json: kept=5",
                                  "UNCHECKED mqtt_pub_dev_list.tags",
                                  "3 tag(s) unverified"),
            "proto_sys": ("file proto_sys.json: kept=6",),
            "north_map": ("file tag_addr_map.json: kept=29",
                          "PASS      north_map.point address",
                          "3 point address(es)"),
            "rate_fig": ("file rate_fig.json: kept=44",),
            "price": ("file price.json: kept=7",),
            "proto_trigger": ("file proto_trigger_cfg.json: kept=2",),
            "iec104_legacy": ("file iec104_config.json: kept=9",),
        }
        for family in FAMILIES:
            with self.subTest(family=family):
                proc = run([CONVERTER, "--family", family,
                            "--root", FIXTURE_SOURCE, "--dry-run"])
                self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
                for needle in expected[family]:
                    self.assertIn(needle, proc.stdout, f"{family}: {needle}")

    def test_mqtt_secret_fixtures_are_replaced(self):
        plaintext = "placeholder-not-a-secret"
        with tempfile.TemporaryDirectory() as output:
            proc = convert("mqtt_emms2", FIXTURE_SOURCE, output)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertNotIn(plaintext, proc.stdout + proc.stderr)
            self.assertIn("secret_replaced=1", proc.stdout)
            self.assertIn("secret_unset=1", proc.stdout)
            with open(os.path.join(output, "stactrl_mqtt.json"),
                      "r", encoding="utf-8") as handle:
                stactrl = handle.read()
            self.assertNotIn(plaintext, stactrl)
            self.assertIn("$secret:mqtt_emms2.stactrl_mqtt.pass", stactrl)
            with open(os.path.join(output, "HXD610.json"),
                      "r", encoding="utf-8") as handle:
                self.assertEqual(json.load(handle)["password"], "")
        with tempfile.TemporaryDirectory() as output:
            proc = convert("mqtt_north_cfg", FIXTURE_SOURCE, output)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertNotIn(plaintext, proc.stdout + proc.stderr)
            self.assertIn("secret_replaced=1", proc.stdout)
            with open(os.path.join(output, "north", "mqtt_north_cfg.json"),
                      "r", encoding="utf-8") as handle:
                north = handle.read()
            self.assertNotIn(plaintext, north)
            self.assertIn(
                "$secret:mqtt_north_cfg.north.mqtt_north_cfg.standard.1.pass",
                north)

    def run_mutated_fixture(self, family, relative_path, mutate):
        source = tempfile.TemporaryDirectory()
        self.addCleanup(source.cleanup)
        output = tempfile.TemporaryDirectory()
        self.addCleanup(output.cleanup)
        shutil.copytree(FIXTURE_SOURCE, source.name, dirs_exist_ok=True)
        path = os.path.join(source.name, relative_path)
        with open(path, "r", encoding="utf-8") as handle:
            document = json.load(handle)
        mutate(document)
        with open(path, "w", encoding="utf-8") as handle:
            json.dump(document, handle)
        return convert(family, source.name, output.name)

    def test_p2p3_family_negatives(self):
        cases = (
            ("bmser_ipc_bin", "bmser_ipc_bin.json",
             lambda document: document["dev"][0].pop("dev_id"),
             "MISSING_VALUE dev[0].dev_id"),
            ("mqtt_pub_dev_list", "mqtt_pub_dev_list.json",
             lambda document: document["pub_dev_list"][0]["tags"].append(7),
             "TYPE_MISMATCH pub_dev_list[0].tags[2]"),
            ("proto_sys", "proto_sys.json",
             lambda document: document.__setitem__("sample_rate", "60"),
             "TYPE_MISMATCH sample_rate"),
            ("north_map", "tag_addr_map.json",
             lambda document: document["pts"][0]["pointInfo"][0].__setitem__(1, "3"),
             "ONEOF_NO_MATCH tag_addr_map.json"),
            ("rate_fig", "rate_fig.json",
             lambda document: document[0]["write"].__setitem__("send_0", "68AA"),
             "TYPE_MISMATCH [0].write.send_0"),
            ("price", "price.json",
             lambda document: document["price"][0].__setitem__("top", "high"),
             "TYPE_MISMATCH price[0].top"),
            ("proto_trigger", "proto_trigger_cfg.json",
             lambda document: document["triggers"].append({}),
             "MAX_ITEMS triggers"),
            ("iec104_legacy", "iec104_config.json",
             lambda document: document["pts"][0]["pointInfo"][0].__setitem__(2, 5),
             "TYPE_MISMATCH pts[0].pointInfo[0][2]"),
        )
        for family, relative_path, mutate, expected in cases:
            with self.subTest(family=family):
                proc = self.run_mutated_fixture(family, relative_path, mutate)
                self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)
                self.assertIn(expected, proc.stdout)

    def test_north_map_address_cross_check_rejects_low_address(self):
        def lower_address(document):
            document["pts"][0]["pointInfo"][0][2] = "1"

        proc = self.run_mutated_fixture(
            "north_map", "tag_addr_map.json", lower_address)
        self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)
        self.assertIn(
            "address '1' is below start_registers 40001", proc.stdout)

    def test_unknown_key_in_p2_family_fails(self):
        proc = self.run_mutated_fixture(
            "proto_sys", "proto_sys.json",
            lambda document: document.__setitem__("unexpected", 1))
        self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)
        self.assertIn("UNKNOWN_KEY unexpected", proc.stdout)

    def test_rate_fig_uses_config_envelope(self):
        with tempfile.TemporaryDirectory() as output:
            proc = convert("rate_fig", FIXTURE_SOURCE, output)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            with open(os.path.join(output, "rate_fig.json"),
                      "r", encoding="utf-8") as handle:
                document = json.load(handle)
        self.assertEqual(set(document), {"schema_version", "config"})
        self.assertEqual(document["schema_version"], "1.0.0")
        self.assertIsInstance(document["config"], list)

    def test_prefix_items_preserve_positions(self):
        schema = {
            "type": "array",
            "minItems": 2,
            "maxItems": 2,
            "prefixItems": [{"type": "string"}, {"type": "integer"}],
        }
        stats = convert_config.Stats()
        row = ["METER1.P", 3]
        self.assertEqual(
            convert_config.convert_value(
                row, schema, [], "north_map", "tuple_fixture.json", stats),
            row)
        self.assertEqual(stats.kept, 2)
        with self.assertRaisesRegex(
                convert_config.ConvertError, "no item schema at 2"):
            convert_config.convert_value(
                ["METER1.P", 3, "extra"], schema, [], "north_map",
                "tuple_fixture.json", convert_config.Stats())

    def test_dev_group_reference_check_runs(self):
        def run_group(root, output, document=None, drop=None):
            if document is not None:
                with open(os.path.join(root, "dev_group_cfg.json"), "w",
                          encoding="utf-8") as handle:
                    json.dump(document, handle)
            if drop:
                os.remove(os.path.join(root, drop))
            return convert("dev_group", root, output)

        with tempfile.TemporaryDirectory() as source, \
                tempfile.TemporaryDirectory() as output:
            shutil.copytree(FIXTURE_SOURCE, source, dirs_exist_ok=True)
            with open(os.path.join(source, "dev_group_cfg.json"),
                      "r", encoding="utf-8") as handle:
                groups = json.load(handle)
            groups[0]["devices"][0]["mp"] = "LC9"
            proc = run_group(source, output, groups)
            self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)
            self.assertIn("BAD_REFERENCE [0].devices[0].mp", proc.stdout)
        with tempfile.TemporaryDirectory() as source, \
                tempfile.TemporaryDirectory() as output:
            shutil.copytree(FIXTURE_SOURCE, source, dirs_exist_ok=True)
            with open(os.path.join(source, "dev_group_cfg.json"),
                      "r", encoding="utf-8") as handle:
                groups = json.load(handle)
            groups[0]["devices"][0]["no"] = "NOPE"
            proc = run_group(source, output, groups)
            self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)
            self.assertIn("BAD_REFERENCE [0].devices[0].no", proc.stdout)
        with tempfile.TemporaryDirectory() as source, \
                tempfile.TemporaryDirectory() as output:
            shutil.copytree(FIXTURE_SOURCE, source, dirs_exist_ok=True)
            proc = run_group(source, output, drop="ems_lc_list.json")
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertIn("UNCHECKED dev_group.mp/no", proc.stdout)

    def test_secret_placeholders_are_unique_per_path(self):
        first = convert_config.secret_placeholder(
            "device", "lc/device_config.json", ["password"])
        second = convert_config.secret_placeholder(
            "device", "lc_devs/LC1/device_config.json", ["password"])
        self.assertEqual(first, "$secret:device.lc.device_config.password")
        self.assertEqual(
            second, "$secret:device.lc_devs.LC1.device_config.password")
        self.assertNotEqual(first, second)
        hxd = convert_config.secret_placeholder(
            "mqtt_emms2", "HXD610.json", ["password"])
        stactrl = convert_config.secret_placeholder(
            "mqtt_emms2", "stactrl_mqtt.json", ["pass"])
        self.assertNotEqual(hxd, stactrl)
        for name in (first, second, hxd, stactrl):
            self.assertRegex(name, config_schema.SECRET_REF)

    def test_oneof_selects_unique_branch(self):
        schema = {"oneOf": [
            {"type": "object", "properties": {"a": {"type": "integer"}},
             "additionalProperties": False},
            {"type": "object", "properties": {"b": {"type": "integer"}},
             "additionalProperties": False}]}
        stats = convert_config.Stats()
        self.assertEqual(
            convert_config.convert_value(
                {"a": 1}, schema, [], "device", "branch_fixture.json", stats),
            {"a": 1})
        self.assertEqual(stats.kept, 1)

    def test_oneof_ambiguous_branch_fails(self):
        schema = {"oneOf": [{"type": "integer"}, {"type": "integer"}]}
        with self.assertRaisesRegex(
                convert_config.ConvertError, "ONEOF_AMBIGUOUS"):
            convert_config.convert_value(
                1, schema, [], "device", "branch_fixture.json",
                convert_config.Stats())

    def test_oneof_no_match_fails(self):
        schema = {"oneOf": [{"type": "string"}, {"type": "boolean"}]}
        with self.assertRaisesRegex(
                convert_config.ConvertError, "ONEOF_NO_MATCH"):
            convert_config.convert_value(
                1, schema, [], "device", "branch_fixture.json",
                convert_config.Stats())

    def test_anyof_uses_first_matching_branch(self):
        branch = {"type": "object", "properties": {"a": {"type": "integer"}},
                  "additionalProperties": False}
        self.assertIs(
            convert_config.select_branch(
                {"a": 1}, {"anyOf": [branch, branch]}, [], "branch_fixture.json"),
            branch)


if __name__ == "__main__":
    unittest.main()
