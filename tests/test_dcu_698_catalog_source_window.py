#!/usr/bin/env python3
"""The 698 catalog must not depend on line numbers in docs/698_parsed.md.

The generator used to scan a fixed line window (6298..9450). A source extraction
that shifted its line numbering moved table rows out of that window and the
catalog quietly lost them.
"""

import json
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
GENERATOR = REPO_ROOT / "scripts" / "extract_dcu_698_catalog.py"
SOURCE = REPO_ROOT / "docs" / "698_parsed.md"
CATALOG_JSON = REPO_ROOT / "docs" / "design" / "DCU_698_OI_CATALOG.json"
CATALOG_FIXTURE = REPO_ROOT / "tests" / "fixtures" / "dcu_698_catalog_expected.h"

APPENDIX_A = re.compile(r"^附\s*录\s*A$")
TABLE_A7 = re.compile(r"^\s*表\s*A\.7\s+采集监控类对象标识定义$", re.M)


def run_generator(text: str) -> tuple[subprocess.CompletedProcess, Path]:
    root = Path(tempfile.mkdtemp(prefix="dcu698-window-"))
    source = root / "docs" / "698_parsed.md"
    source.parent.mkdir(parents=True)
    source.write_text(text, encoding="utf-8")
    proc = subprocess.run(
        [sys.executable, "-I", str(GENERATOR), "--root", str(root)],
        capture_output=True,
        text=True,
        cwd=str(root),
    )
    return proc, root


class CatalogSourceWindowTest(unittest.TestCase):
    def setUp(self):
        self.source = SOURCE.read_text(encoding="utf-8")
        self.lines = self.source.split("\n")
        self.roots: list[Path] = []

    def tearDown(self):
        for root in self.roots:
            shutil.rmtree(root, ignore_errors=True)

    def run_text(self, text: str) -> tuple[subprocess.CompletedProcess, Path]:
        proc, root = run_generator(text)
        self.roots.append(root)
        return proc, root

    def sub_once(self, text: str, pattern: re.Pattern, replacement) -> str:
        new, count = pattern.subn(replacement, text, count=1)
        self.assertEqual(count, 1, f"fixture no longer has a single match for {pattern.pattern!r}")
        return new

    def test_shifted_source_keeps_the_catalog(self):
        # 140 blank lines push F300/F301 past the old fixed window's end, so the
        # old code dropped them and still returned success.
        shifted = "\n" * 140 + self.source
        proc, root = self.run_text(shifted)
        self.assertEqual(proc.returncode, 0, proc.stderr)

        generated = json.loads((root / "docs/design/DCU_698_OI_CATALOG.json").read_text(encoding="utf-8"))
        expected = json.loads(CATALOG_JSON.read_text(encoding="utf-8"))
        got = {row["oi"]: row for row in generated["ois"]}
        want = {row["oi"]: row for row in expected["ois"]}
        for row in (*got.values(), *want.values()):
            row.pop("source_line", None)
        missing = sorted(set(want) - set(got))
        self.assertEqual(
            set(got), set(want),
            f"shifted source changed the OI set; missing {missing}, extra {sorted(set(got) - set(want))}",
        )
        for oi in sorted(got):
            self.assertEqual(got[oi], want[oi], f"shifted source changed OI {oi}")
        self.assertEqual(generated["oi_methods"], expected["oi_methods"],
                         "shifted source changed the per-OI method sets")
        self.assertEqual(generated["classes"], expected["classes"],
                         "shifted source changed the class tables")
        self.assertEqual(
            (root / "tests/fixtures/dcu_698_catalog_expected.h").read_bytes(),
            CATALOG_FIXTURE.read_bytes(),
            "shifted source produced a different generated fixture",
        )

    def test_missing_appendix_a_title_is_rejected(self):
        idx = next(i for i, line in enumerate(self.lines) if APPENDIX_A.match(line.strip()))
        self.lines[idx] = "附 录 Q"
        proc, _ = self.run_text("\n".join(self.lines))
        self.assertNotEqual(proc.returncode, 0, "generator accepted a source with no Appendix A title")
        self.assertIn("Appendix A", proc.stderr)

    def test_duplicate_appendix_a_title_is_rejected(self):
        idx = next(i for i, line in enumerate(self.lines) if APPENDIX_A.match(line.strip()))
        self.lines.insert(idx, "附 录 A")
        proc, _ = self.run_text("\n".join(self.lines))
        self.assertNotEqual(proc.returncode, 0, "generator accepted a duplicate Appendix A title")
        self.assertIn("Appendix A", proc.stderr)

    def test_missing_object_table_title_is_rejected(self):
        text = self.sub_once(self.source, TABLE_A7, "表A.7 采集监控类对象标识定义（OCR 缺失）")
        proc, _ = self.run_text(text)
        self.assertNotEqual(proc.returncode, 0, "generator accepted a source missing table A.7")
        self.assertIn("A.7", proc.stderr)

    def test_duplicate_object_table_title_is_rejected(self):
        text = self.sub_once(self.source, TABLE_A7, lambda m: f"{m.group(0)}\n{m.group(0)}")
        proc, _ = self.run_text(text)
        self.assertNotEqual(proc.returncode, 0, "generator accepted a duplicate table A.7 title")
        self.assertIn("A.7", proc.stderr)

    def test_table_without_object_header_is_rejected(self):
        title = TABLE_A7.search(self.source)
        self.assertIsNotNone(title, "fixture no longer has the A.7 table title")
        head = self.source.index("\n", title.end()) + 1
        tail = self.source[head:]
        header = re.search(r"对象名称", tail)
        self.assertIsNotNone(header, "fixture no longer has an A.7 table header")
        cut = head + header.start()
        text = self.source[:cut] + "对象名" + self.source[cut + len("对象名称"):]
        proc, _ = self.run_text(text)
        self.assertNotEqual(proc.returncode, 0, "generator accepted a table title with no header")
        self.assertIn("header", proc.stderr)


if __name__ == "__main__":
    unittest.main()
