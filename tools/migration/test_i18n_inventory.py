#!/usr/bin/env python3
"""Self-test for the i18n inventory.

Expected values come from the P6-05b ticket, the P6-05a report, and independent
source greps, never from the inventory's own output: 616 po entries / 615
messages, 472/2/48/93 classification, 19 reverse gaps, 634 key-table rows,
mo headers 526/426, alarminfo 16/15 and 3821/3705 with 149/33 single-language
records, 1299 UI rows with the category split below, exactly 7 msgstr
containment positions (8 po keys), and 1 msgid containment at
WaveformScope100Hz.vue:17.

Every validation gate has a negative fixture that trips only that gate, so
disabling one gate turns this suite red.
"""

import contextlib
import io
import json
import re
import sys
import tempfile
import unittest
from collections import Counter
from pathlib import Path
from unittest import mock

TOOL_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOL_DIR))

import i18n_inventory as inv  # noqa: E402

EXPECTED_CLASSIFICATION = {
    "C_SOURCE": 472,
    "OTHER_TEXT": 2,
    "BINARY_ONLY": 48,
    "MISSING": 93,
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
EXPECTED_UI_ROWS = 1299
EXPECTED_MSGSTR_HITS = {
    ("web/products/iems/product.yaml", 97): {"Advanced", "Advanced features"},
    ("web/permissions/catalog.yaml", 117): {"Reboot the device"},
    ("web/permissions/catalog.yaml", 126): {"Restart the service"},
    ("web/openapi/core.yaml", 381): {"Reboot the device"},
    ("web/openapi/core.yaml", 590): {"Read configuration"},
    ("web/openapi/core.yaml", 652): {"Write configuration"},
    ("web/core/src/modules/dcu/PowerQuality.vue", 278): {"PF"},
}
EXPECTED_MSGSTR_KEY_HITS = 8
EXPECTED_MSGSTR_MATCH_KEYS = {
    ("web/products/iems/product.yaml", 97): "Advanced",
    ("web/permissions/catalog.yaml", 117): "Reboot the device",
    ("web/permissions/catalog.yaml", 126): "Restart the service",
    ("web/openapi/core.yaml", 381): "Reboot the device",
    ("web/openapi/core.yaml", 590): "Read configuration",
    ("web/openapi/core.yaml", 652): "Write configuration",
    ("web/core/src/modules/dcu/PowerQuality.vue", 278): "PF",
}
EXPECTED_MSGID_HITS = {
    ("web/products/smart_box/src/components/WaveformScope100Hz.vue", 17): {
        "Precision"
    },
}
EXPECTED_TABLE_ROWS = 634
EXPECTED_REVERSE_GAPS = 19
EXPECTED_ALARM = {
    "zh_vendor_models": 16,
    "en_vendor_models": 15,
    "zh_tags": 3821,
    "en_tags": 3705,
    "zh_only": 149,
    "en_only": 33,
}
MO_FIRMWARE = "firmware/ipk_ems_core/data/opt/lvt/app/global/language/zh_CN/LC_MESSAGES/language.mo"
MO_PATCH = "tools/creat_patch/up_files_patch/up_files/opt/lnxall_app/app/global/language/zh_CN/LC_MESSAGES/language.mo"


def _copy_entries(entries):
    return [dict(entry, flags=list(entry["flags"])) for entry in entries]


def _copy_rows(rows):
    return [dict(row) for row in rows]


def _expected_owner(file):
    for prefix, owner in (
        ("web/core/", "core"),
        ("web/products/iems/", "iems"),
        ("web/products/dcu/", "dcu"),
        ("web/products/smart_box/", "smart_box"),
        ("web/permissions/", "shared"),
        ("web/schemas/", "shared"),
        ("web/openapi/", "shared"),
    ):
        if file.startswith(prefix):
            return owner
    return None


def _offset_line(row):
    path, _, line = row["evidence"].rpartition(":")
    row["evidence"] = f"{path}:{int(line) + 1}"


class InventorySelfTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.entries = inv.parse_po(inv.PO_PATH.read_text(encoding="utf-8"))
        cls.messages = inv.messages_of(cls.entries)
        cls.key_rows, cls.class_counts = inv.classify_messages(cls.messages, inv.IEMS_ROOT)
        cls.reverse_gaps = inv.find_reverse_gaps(cls.messages, inv.IEMS_ROOT)
        cls.ui_rows = inv.extract_ui_strings(inv.REPO_ROOT)
        cls.matches, cls.key_hits = inv.match_ui(cls.ui_rows, cls.messages)

    def _assert_single_error(self, call, token):
        with self.assertRaises(inv.ValidationError) as raised:
            call()
        message = str(raised.exception)
        self.assertIn(token, message)
        self.assertNotIn("; ", message, f"fixture must trip exactly one check: {message}")

    def _main(self, argv):
        stdout, stderr = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            rc = inv.main(argv)
        return rc, stdout.getvalue(), stderr.getvalue()

    def _fake_git(self, iems_sha=None, iems_status="", web_status=""):
        def fake(root, *args):
            root = Path(root)
            if "rev-parse" in args:
                if root == inv.IEMS_ROOT:
                    return inv.IEMS_SHA if iems_sha is None else iems_sha
                return "edgec-sha"
            if "status" in args:
                return iems_status if root == inv.IEMS_ROOT else web_status
            return ""

        return fake

    # -- po structure -------------------------------------------------------

    def test_po_structure_matches_p6_05a(self):
        stats = inv.validate_po(self.entries)
        self.assertEqual(len(self.entries), 616, "616 entries including the header")
        self.assertEqual(len(self.messages), 615, "615 translatable messages")
        self.assertEqual(stats["empty_msgstr"], 0)
        self.assertEqual(stats["c_format"], 6)
        self.assertEqual(stats["multiline"], 2)
        self.assertEqual(stats["msgid_eq_msgstr"], 2)

    def test_each_po_check_rejects_its_defect(self):
        cases = [
            (
                "entries count",
                lambda entries: entries.append(dict(entries[0])),
                "entries 617 != 616",
            ),
            (
                "message count",
                lambda entries: entries.__setitem__(
                    slice(None), entries[:-1] + [dict(entries[0])]
                ),
                "messages 614 != 615",
            ),
            (
                "duplicate msgid",
                lambda entries: entries[-1].__setitem__("msgid", entries[1]["msgid"]),
                "duplicate msgid",
            ),
            (
                "empty msgstr",
                lambda entries: entries[1].__setitem__("msgstr", ""),
                "empty msgstr 1 != 0",
            ),
            (
                "c-format count",
                lambda entries: entries[46]["flags"].remove("c-format"),
                "c-format 5 != 6",
            ),
            (
                "placeholder mismatch",
                lambda entries: entries[46].__setitem__(
                    "msgstr", entries[46]["msgstr"].replace("%s", "%d")
                ),
                "c-format placeholder mismatch",
            ),
            (
                "multiline count",
                lambda entries: entries[1].__setitem__("multiline", True),
                "multiline 3 != 2",
            ),
            (
                "msgid==msgstr count",
                lambda entries: entries[1].__setitem__("msgstr", entries[1]["msgid"]),
                "msgid==msgstr 3 != 2",
            ),
        ]
        for name, mutate, token in cases:
            with self.subTest(check=name):
                entries = _copy_entries(self.entries)
                mutate(entries)
                self._assert_single_error(lambda: inv.validate_po(entries), token)

    def test_deleted_key_is_rejected(self):
        # 616 entries but 614 messages: only the 615-message check can catch it.
        broken = self.entries[:-1] + [self.entries[0]]
        self.assertEqual(len(broken), 616)
        self._assert_single_error(lambda: inv.validate_po(broken), "messages 614 != 615")

    # -- classification and evidence ---------------------------------------

    def test_classification_matches_p6_05a(self):
        self.assertEqual(len(self.key_rows), 615)
        self.assertEqual(self.class_counts, EXPECTED_CLASSIFICATION)
        self.assertEqual(len({row["msgid"] for row in self.key_rows}), 615)
        self.assertEqual(
            len(inv.render_keys_tsv(self.key_rows, self.reverse_gaps).splitlines()) - 1,
            EXPECTED_TABLE_ROWS,
            "615 forward key rows plus 19 reverse-gap rows",
        )

    def test_key_row_validation_matrix(self):
        rows = self.key_rows
        other_index = next(
            index for index, row in enumerate(rows) if row["classification"] == "OTHER_TEXT"
        )
        binary_index = next(
            index for index, row in enumerate(rows) if row["classification"] == "BINARY_ONLY"
        )
        missing_indexes = [
            index
            for index, row in enumerate(rows)
            if row["classification"] == "MISSING"
        ]
        missing_index = missing_indexes[0]

        def duplicate(rows):
            rows[1] = dict(rows[0])

        cases = [
            ("duplicate rows", duplicate, "duplicate key rows"),
            ("classification counts", lambda rows: rows.pop(0), "delta {'C_SOURCE': -1}"),
            (
                "evidence format",
                lambda rows: rows[0].__setitem__("evidence", "garbage"),
                "evidence is not file:line",
            ),
            (
                "C_SOURCE path",
                lambda rows: rows[0].__setitem__("evidence", f"{inv.PO_REL}:1"),
                "C_SOURCE evidence is not C source",
            ),
            (
                "OTHER_TEXT path",
                lambda rows: rows[other_index].__setitem__("evidence", rows[0]["evidence"]),
                "OTHER_TEXT evidence is C or binary",
            ),
            (
                "BINARY_ONLY path",
                lambda rows: rows[binary_index].__setitem__("evidence", f"{inv.PO_REL}:1"),
                "BINARY_ONLY evidence is not the old binary",
            ),
            (
                "MISSING path",
                lambda rows: rows[missing_index].__setitem__("evidence", rows[0]["evidence"]),
                "MISSING evidence is not the po",
            ),
            (
                "C_SOURCE content",
                lambda rows: _offset_line(rows[0]),
                "is line",
            ),
            (
                "OTHER_TEXT content",
                lambda rows: _offset_line(rows[other_index]),
                "is line",
            ),
            (
                "BINARY_ONLY content",
                lambda rows: _offset_line(rows[binary_index]),
                "is line",
            ),
            (
                "MISSING content",
                lambda rows: rows[missing_index].__setitem__(
                    "evidence", f"{inv.PO_REL}:1"
                ),
                "is not a po msgid line",
            ),
            (
                "MISSING identity",
                lambda rows: rows[missing_index].__setitem__(
                    "evidence", rows[missing_indexes[1]]["evidence"]
                ),
                "is not a po msgid line",
            ),
        ]
        for name, mutate, token in cases:
            with self.subTest(check=name):
                rows = _copy_rows(self.key_rows)
                mutate(rows)
                self._assert_single_error(lambda: inv.validate_key_rows(rows), token)

    def test_swapped_evidence_is_rejected(self):
        rows = _copy_rows(self.key_rows)
        indexes = [index for index, row in enumerate(rows) if row["classification"] == "C_SOURCE"]
        rows[indexes[0]]["evidence"], rows[indexes[1]]["evidence"] = (
            rows[indexes[1]]["evidence"],
            rows[indexes[0]]["evidence"],
        )
        with self.assertRaises(inv.ValidationError) as raised:
            inv.validate_key_rows(rows)
        self.assertIn("is line", str(raised.exception))

    def test_swapped_missing_evidence_is_rejected(self):
        rows = _copy_rows(self.key_rows)
        indexes = [
            index
            for index, row in enumerate(rows)
            if row["classification"] == "MISSING"
        ]
        rows[indexes[0]]["evidence"], rows[indexes[1]]["evidence"] = (
            rows[indexes[1]]["evidence"],
            rows[indexes[0]]["evidence"],
        )
        with self.assertRaises(inv.ValidationError) as raised:
            inv.validate_key_rows(rows)
        self.assertIn("is not a po msgid line", str(raised.exception))

    def test_reverse_gaps_match_p6_05a(self):
        self.assertEqual(len(self.reverse_gaps), EXPECTED_REVERSE_GAPS)
        found = {(gap["file"], gap["line"], gap["text"]) for gap in self.reverse_gaps}
        self.assertIn(
            ("src/ems/deviceLayer.c", 2278, "Executed repower of the %d strategy"), found
        )
        self.assertIn(("src/opcua/ua-server.c", 1602, "Alarm"), found)

    def test_reverse_gap_count_is_validated(self):
        self._assert_single_error(
            lambda: inv.validate_reverse_gaps(self.reverse_gaps[:-1]),
            "reverse gaps 18 != 19",
        )

    def test_auxiliary_validation_matrix(self):
        mo_counts = {MO_FIRMWARE: 526, MO_PATCH: 426}
        cases = [
            (
                "empty po size",
                lambda: inv.validate_auxiliary(1, "PO_PATH=../files", mo_counts),
                "files/zh_CN.po is not 0 bytes",
            ),
            (
                "update_po target",
                lambda: inv.validate_auxiliary(0, "PO_PATH=../other", mo_counts),
                "no longer points at ../files",
            ),
            (
                "mo headers",
                lambda: inv.validate_auxiliary(
                    0, "PO_PATH=../files", {MO_FIRMWARE: 525, MO_PATCH: 426}
                ),
                "mo headers",
            ),
        ]
        for name, call, token in cases:
            with self.subTest(check=name):
                self._assert_single_error(call, token)

    def test_mo_headers_match_p6_05a(self):
        self.assertEqual(inv.read_mo_count(inv.IEMS_ROOT / MO_FIRMWARE), 526)
        self.assertEqual(inv.read_mo_count(inv.IEMS_ROOT / MO_PATCH), 426)

    def test_alarminfo_counts_match_p6_05a(self):
        self.assertEqual(inv.alarminfo_counts(inv.IEMS_ROOT), EXPECTED_ALARM)

    def test_alarm_drift_is_rejected(self):
        drifted = dict(EXPECTED_ALARM, zh_tags=3820)
        self._assert_single_error(lambda: inv.validate_alarm(drifted), "alarminfo")

    # -- EdgeC UI extraction ------------------------------------------------

    def test_ui_rows_match_independent_extraction(self):
        self.assertEqual(len(self.ui_rows), EXPECTED_UI_ROWS)
        self.assertEqual(dict(Counter(row["category"] for row in self.ui_rows)), EXPECTED_UI_CATEGORIES)
        for row in self.ui_rows:
            self.assertEqual(row["owner"], _expected_owner(row["file"]), row["file"])
            self.assertNotIn("dist", row["file"].split("/"))
            self.assertNotIn("generated", row["file"].split("/"))
            self.assertNotIn("node_modules", row["file"].split("/"))
            self.assertGreater(row["line"], 0)
            self.assertTrue((inv.REPO_ROOT / row["file"]).is_file(), row["file"])

    def test_greater_than_text_is_preserved(self):
        texts = {(row["file"], row["line"]): row["text"] for row in self.ui_rows}
        self.assertEqual(
            texts[("web/core/src/components/TOUArbitrageWidget.vue", 132)],
            "Optimal: > 92.0%",
        )
        self.assertEqual(
            texts[("web/core/src/modules/devices/ChannelTopologyHeatmap.vue", 180)],
            "SNR > 20dB（良好）",
        )
        nodes = [
            text[start:end].strip()
            for text in ["<div>Optimal: > 92.0%</div>"]
            for start, end in inv._text_nodes(text)
        ]
        self.assertIn("Optimal: > 92.0%", nodes)

    def test_ui_validation_matrix(self):
        core_index = next(
            index
            for index, row in enumerate(self.ui_rows)
            if row["file"].startswith("web/core/src/") and row["owner"] == "core"
        )
        permissions_index = next(
            index
            for index, row in enumerate(self.ui_rows)
            if row["file"].startswith("web/permissions/")
        )
        schema_index = next(
            index
            for index, row in enumerate(self.ui_rows)
            if row["category"] == "schema_title"
        )

        def missing_file(rows):
            rows[core_index]["file"] = "web/core/src/does-not-exist.vue"

        def excluded_path(rows):
            rows[core_index]["file"] = "web/core/dist/does-not-exist.vue"
            rows[core_index]["owner"] = "core"

        def wrong_owner(rows):
            rows[permissions_index]["owner"] = "core"

        def unexpected_path(rows):
            rows[core_index]["file"] = "web/elsewhere/does-not-exist.vue"

        def category_drift(rows):
            rows[schema_index]["category"] = "schema_hint"

        cases = [
            ("bad line", lambda rows: rows[core_index].__setitem__("line", 0), "bad line 0"),
            ("missing file", missing_file, "missing source web/core/src/does-not-exist.vue"),
            (
                "excluded path",
                excluded_path,
                "excluded UI source path web/core/dist/does-not-exist.vue",
            ),
            (
                "owner follows path",
                wrong_owner,
                "path-derived 'shared'",
            ),
            (
                "unexpected path",
                unexpected_path,
                "unexpected UI source path web/elsewhere/does-not-exist.vue",
            ),
            ("category counts", category_drift, "UI category counts"),
        ]
        for name, mutate, token in cases:
            with self.subTest(check=name):
                rows = _copy_rows(self.ui_rows)
                mutate(rows)
                self._assert_single_error(
                    lambda: inv.validate_ui_rows(rows, inv.REPO_ROOT), token
                )

    def test_stale_dist_premise_is_validated(self):
        real_read = inv.read_text
        stale = inv.REPO_ROOT / "web/dist/index.html"

        def fake_read(path):
            return '<html lang="fr">' if Path(path) == stale else real_read(path)

        with mock.patch.object(inv, "read_text", side_effect=fake_read):
            self._assert_single_error(
                lambda: inv.validate_ui_rows(self.ui_rows, inv.REPO_ROOT),
                "web/dist/index.html",
            )

    def test_missing_declared_inputs_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(inv.ValidationError) as raised:
                inv._extract_schemas([], Path(tmp))
            self.assertIn("no schema json", str(raised.exception))

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "web/openapi").mkdir(parents=True)
            (root / "web/products/dcu").mkdir(parents=True)
            (root / "web/openapi/core.yaml").write_text("", encoding="utf-8")
            (root / "web/products/dcu/openapi.extension.yaml").write_text(
                "", encoding="utf-8"
            )
            with self.assertRaises(inv.ValidationError) as raised:
                inv._extract_openapi([], root)
            self.assertIn(
                "web/products/iems/openapi.extension.yaml", str(raised.exception)
            )

        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(inv.ValidationError) as raised:
                inv.check_edgec_inputs(Path(tmp))
            self.assertIn("web/schemas/network.firewall.json", str(raised.exception))

    def test_manifest_indent_drift_is_rejected(self):
        manifest = (inv.REPO_ROOT / "web/core/modules.manifest.yaml").read_text(
            encoding="utf-8"
        )
        drifted = re.sub(r"(?m)^        title:", "         title:", manifest)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "web/core/modules.manifest.yaml"
            path.parent.mkdir(parents=True)
            path.write_text(drifted, encoding="utf-8")
            extracted = []
            inv._extract_manifest(extracted, Path(tmp))
        self.assertEqual([row["category"] for row in extracted], ["manifest_group"] * 6)
        rows = [
            row
            for row in self.ui_rows
            if row["category"] not in ("manifest_group", "manifest_page")
        ] + extracted
        with self.assertRaises(inv.ValidationError) as raised:
            inv.validate_ui_rows(rows, inv.REPO_ROOT)
        message = str(raised.exception)
        self.assertIn("UI category counts", message)
        self.assertIn("'manifest_page': -28", message)
        self.assertNotIn("; ", message)

    # -- input state --------------------------------------------------------

    def test_input_state_validation_matrix(self):
        cases = [
            (
                "pinned sha",
                dict(iems_sha="0" * 40),
                "!= pinned",
            ),
            (
                "dirty iems tree",
                dict(iems_status=" M src/x.c\n"),
                "iems_v3 is dirty",
            ),
            (
                "dirty web tree",
                dict(web_status=" M web/core/src/x.vue\n"),
                "EdgeC web/ is dirty",
            ),
        ]
        for name, git_kwargs, token in cases:
            with self.subTest(check=name):
                with mock.patch.object(inv, "_git", side_effect=self._fake_git(**git_kwargs)):
                    self._assert_single_error(inv.check_inputs, token)

    def test_missing_iems_tree_is_rejected(self):
        with mock.patch.object(inv, "IEMS_ROOT", Path("/nonexistent-iems-v3")):
            self._assert_single_error(inv.check_inputs, "missing input tree")

    def test_check_inputs_rejects_missing_edgec_tree(self):
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(
            inv, "_git", side_effect=self._fake_git()
        ), mock.patch.object(inv, "REPO_ROOT", Path(tmp)):
            with self.assertRaises(inv.ValidationError) as raised:
                inv.check_inputs()
        self.assertIn("missing EdgeC input", str(raised.exception))

    # -- containment --------------------------------------------------------

    def test_expected_containment_matches(self):
        msgstr_hits = [match for match in self.matches if match["side"] == "msgstr"]
        hit_keys = {}
        for msg_id, file, line in self.key_hits:
            hit_keys.setdefault((file, line), set()).add(msg_id)
        self.assertEqual(hit_keys, EXPECTED_MSGSTR_HITS, "each position pins its po keys")
        self.assertEqual(len(hit_keys), 7, "position-level count")
        self.assertEqual(len(self.key_hits), EXPECTED_MSGSTR_KEY_HITS, "key-level count")
        self.assertEqual(
            {
                (match["ui_file"], match["ui_line"]): match["msg_key"]
                for match in msgstr_hits
            },
            EXPECTED_MSGSTR_MATCH_KEYS,
            "each match row pins its emitted po key",
        )
        self.assertEqual(len(self.matches), 8, "7 msgstr positions plus 1 msgid hit")
        msgid_hits = [match for match in self.matches if match["side"] == "msgid"]
        msgid_keys = {}
        for match in msgid_hits:
            msgid_keys.setdefault((match["ui_file"], match["ui_line"]), set()).add(
                match["msg_key"]
            )
        self.assertEqual(msgid_keys, EXPECTED_MSGID_HITS)
        self.assertEqual([match for match in msgstr_hits if match["length"] >= 8], [])
        for match in self.matches:
            self.assertGreaterEqual(
                match["length"], 8 if match["side"] == "msgid" else 4
            )

    def test_match_validation_matrix(self):
        def drop_position(matches, keys):
            matches = _copy_rows(matches)
            matches.pop(next(i for i, match in enumerate(matches) if match["side"] == "msgstr"))
            return matches, keys

        def drop_key(matches, keys):
            keys = set(keys)
            keys.discard(("Advanced features", "web/products/iems/product.yaml", 97))
            return matches, keys

        def drop_all_keys_at_position(matches, keys):
            return matches, {
                hit for hit in keys if hit[1:] != ("web/permissions/catalog.yaml", 117)
            }

        def swap_key_identity(matches, keys):
            matches = _copy_rows(matches)
            next(
                match
                for match in matches
                if (match["ui_file"], match["ui_line"])
                == ("web/permissions/catalog.yaml", 117)
            )["msg_key"] = "Restart the service"
            keys = set(keys)
            keys.discard(("Reboot the device", "web/permissions/catalog.yaml", 117))
            keys.add(("Restart the service", "web/permissions/catalog.yaml", 117))
            return matches, keys

        def swap_msgid_identity(matches, keys):
            matches = _copy_rows(matches)
            next(match for match in matches if match["side"] == "msgid")[
                "msg_key"
            ] = "G100 Enable"
            return matches, keys

        def long_msgstr(matches, keys):
            matches = _copy_rows(matches)
            next(m for m in matches if m["side"] == "msgstr")["length"] = 8
            return matches, keys

        def drop_msgid(matches, keys):
            return [m for m in matches if m["side"] != "msgid"], keys

        def drift_msgid(matches, keys):
            matches = _copy_rows(matches)
            next(m for m in matches if m["side"] == "msgid")["ui_line"] += 1
            return matches, keys

        cases = [
            ("position set", drop_position, "msgstr match positions"),
            (
                "key count",
                drop_key,
                "msgstr keys at web/products/iems/product.yaml:97",
            ),
            (
                "key position",
                drop_all_keys_at_position,
                "msgstr keys at web/permissions/catalog.yaml:117",
            ),
            (
                "key identity",
                swap_key_identity,
                "msgstr keys at web/permissions/catalog.yaml:117",
            ),
            ("length >= 8", long_msgstr, "length >= 8"),
            ("msgid hit", drop_msgid, "msgid match positions"),
            ("msgid position", drift_msgid, "msgid match positions"),
            (
                "msgid key identity",
                swap_msgid_identity,
                "msgid keys at web/products/smart_box/src/components/WaveformScope100Hz.vue:17",
            ),
        ]
        for name, mutate, token in cases:
            with self.subTest(check=name):
                matches, key_hits = mutate(_copy_rows(self.matches), set(self.key_hits))
                self._assert_single_error(
                    lambda: inv.validate_matches(matches, key_hits), token
                )

    def test_exact_containment_not_approximate(self):
        self.assertTrue(inv.is_hit("总功率因数（cosφ）", "功率因数", 4))
        self.assertTrue(inv.is_hit("读取 配置分区", "读取配置", 4), "whitespace is stripped")
        self.assertFalse(inv.is_hit("重启设配", "重启设备", 4), "one-char near miss is not a hit")
        self.assertFalse(inv.is_hit("重启设", "重启设备", 4), "short haystack is not a hit")
        self.assertFalse(inv.is_hit("重启设备", "重启设备", 5), "threshold is on the needle")

    # -- CLI ----------------------------------------------------------------

    def test_deleted_key_exits_2(self):
        broken = self.entries[:-1] + [self.entries[0]]
        with mock.patch.object(inv, "parse_po", return_value=broken):
            with tempfile.TemporaryDirectory() as out:
                self.assertEqual(self._main(["--check", "--out", out])[0], 2)

    def test_duplicate_key_exits_2(self):
        with mock.patch.object(
            inv, "parse_po", return_value=self.entries + [self.entries[1]]
        ):
            with tempfile.TemporaryDirectory() as out:
                self.assertEqual(self._main(["--check", "--out", out])[0], 2)

    def test_misclassified_key_exits_2(self):
        rows = _copy_rows(self.key_rows)
        next(row for row in rows if row["classification"] == "MISSING")[
            "classification"
        ] = "C_SOURCE"
        with mock.patch.object(
            inv, "classify_messages", return_value=(rows, self.class_counts)
        ):
            with tempfile.TemporaryDirectory() as out:
                self.assertEqual(self._main(["--check", "--out", out])[0], 2)

    def test_missing_input_exits_2(self):
        with mock.patch.object(inv, "IEMS_ROOT", Path("/nonexistent-iems-v3")):
            with tempfile.TemporaryDirectory() as out:
                self.assertEqual(self._main(["--check", "--out", out])[0], 2)

    def test_reverse_gap_drift_exits_2(self):
        with mock.patch.object(
            inv, "find_reverse_gaps", return_value=self.reverse_gaps[:-1]
        ):
            with tempfile.TemporaryDirectory() as out:
                self.assertEqual(self._main(["--check", "--out", out])[0], 2)

    def test_alarm_drift_exits_2(self):
        with mock.patch.object(
            inv, "alarminfo_counts", return_value=dict(EXPECTED_ALARM, zh_tags=3820)
        ):
            with tempfile.TemporaryDirectory() as out:
                self.assertEqual(self._main(["--check", "--out", out])[0], 2)

    def test_extract_ui_strings_validates_its_output(self):
        with mock.patch.object(inv, "_extract_schemas", lambda rows, root: None):
            with self.assertRaises(inv.ValidationError) as raised:
                inv.extract_ui_strings(inv.REPO_ROOT)
        self.assertIn("UI category counts", str(raised.exception))

    def test_match_validation_is_wired(self):
        dropped = ("web/permissions/catalog.yaml", 117)
        matches = [
            match
            for match in self.matches
            if (match["ui_file"], match["ui_line"]) != dropped
        ]
        key_hits = {hit for hit in self.key_hits if hit[1:] != dropped}
        with mock.patch.object(inv, "match_ui", return_value=(matches, key_hits)):
            with tempfile.TemporaryDirectory() as out:
                self.assertEqual(self._main(["--check", "--out", out])[0], 2)

    def test_failed_check_does_not_create_out_dir(self):
        broken = self.entries[:-1] + [self.entries[0]]
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "run"
            with mock.patch.object(inv, "parse_po", return_value=broken):
                self.assertEqual(self._main(["--check", "--out", str(out)])[0], 2)
            self.assertFalse(out.exists(), "a failed check must not create the output dir")

    def test_check_reports_counts_and_caliber(self):
        with tempfile.TemporaryDirectory() as out:
            rc, stdout, stderr = self._main(["--check", "--out", out])
            self.assertEqual(rc, 0, stderr)
            summary = json.loads((Path(out) / "summary.json").read_text(encoding="utf-8"))
            report = (Path(out) / "report.md").read_text(encoding="utf-8")
        self.assertIn("msgstr_positions=7", stdout)
        self.assertIn("msgstr_keys=8", stdout)
        counts = summary["counts"]
        self.assertEqual(counts["classification"], EXPECTED_CLASSIFICATION)
        self.assertEqual(counts["key_rows"], 615)
        self.assertEqual(counts["table_rows"], EXPECTED_TABLE_ROWS)
        self.assertEqual(counts["edgec_ui_strings"], EXPECTED_UI_ROWS)
        self.assertEqual(counts["msgstr_matches_positions"], 7)
        self.assertEqual(counts["msgstr_matches_keys"], 8)
        self.assertEqual(counts["msgid_matches"], 1)
        self.assertEqual(summary["inputs"]["iems_v3"]["sha"], inv.IEMS_SHA)
        self.assertIn("7 positions / 8 keys", report)
        self.assertIn("Advanced features", report)
        self.assertIn("web/products/iems/product.yaml:97", report)

    def test_tsv_renderers_are_deterministic(self):
        self.assertEqual(
            inv.render_keys_tsv(self.key_rows, self.reverse_gaps),
            inv.render_keys_tsv(self.key_rows, self.reverse_gaps),
        )
        self.assertEqual(
            inv.render_ui_tsv(self.ui_rows), inv.render_ui_tsv(self.ui_rows)
        )
        self.assertEqual(
            inv.render_matches_tsv(self.matches), inv.render_matches_tsv(self.matches)
        )
        self.assertEqual(
            json.dumps(inv.alarminfo_counts(inv.IEMS_ROOT), sort_keys=True, ensure_ascii=False),
            json.dumps(inv.alarminfo_counts(inv.IEMS_ROOT), sort_keys=True, ensure_ascii=False),
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
