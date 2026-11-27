#!/usr/bin/env python3
"""The 7.3 method tables and the per-OI method numbers must be checked as sets.

The generator used to collapse a class's method table to its largest number and
only scan Appendix A for per-OI methods. A source that dropped an internal
method number, added an extra one, or gave an OI an undefined number below 127
still exited 0 and was written out.
"""

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
OUTPUTS = (
    "docs/design/DCU_698_OI_CATALOG.md",
    "docs/design/DCU_698_OI_CATALOG.json",
    "docs/design/DCU_698_CLASS_RECONCILIATION.md",
    "tests/fixtures/dcu_698_catalog_expected.h",
)

CLASS_7_HEAD = re.compile(r"^\s*7\.3\.7\s+")
CLASS_8_HEAD = re.compile(r"^\s*7\.3\.8\s+")
CLASS_7_METHOD_2 = re.compile(r"^2[．.]\s*执行(?:\s|$)")
CLASS_7_METHOD_5 = re.compile(r"^5[．.]\s*删除一个事件关联对象属性")
OI_4307_METHOD_127 = re.compile(r"^4307\s+19\s+水表\s+方法 127：出厂启用（参数）$")


def run_generator(text: str) -> tuple[subprocess.CompletedProcess, Path]:
    root = Path(tempfile.mkdtemp(prefix="dcu698-methods-"))
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


class CatalogMethodSetsTest(unittest.TestCase):
    def setUp(self):
        self.lines = SOURCE.read_text(encoding="utf-8").split("\n")
        self.roots: list[Path] = []

    def tearDown(self):
        for root in self.roots:
            shutil.rmtree(root, ignore_errors=True)

    def run_lines(self, lines: list[str]) -> subprocess.CompletedProcess:
        proc, root = run_generator("\n".join(lines))
        self.roots.append(root)
        return proc

    def class_7_bounds(self) -> tuple[int, int]:
        start = next(i for i, line in enumerate(self.lines) if CLASS_7_HEAD.match(line))
        end = next(i for i, line in enumerate(self.lines)
                   if i > start and CLASS_8_HEAD.match(line))
        return start, end

    def one_class_7_line(self, pattern: re.Pattern) -> int:
        start, end = self.class_7_bounds()
        hits = [i for i in range(start, end) if pattern.match(self.lines[i].strip())]
        self.assertEqual(len(hits), 1, f"fixture no longer has one Class 7 line matching {pattern.pattern!r}")
        return hits[0]

    def test_missing_internal_method_is_rejected(self):
        lines = list(self.lines)
        lines.pop(self.one_class_7_line(CLASS_7_METHOD_5))
        proc = self.run_lines(lines)
        self.assertNotEqual(proc.returncode, 0,
                            "generator accepted a Class 7 method table without method 5")
        self.assertIn("class 7", proc.stderr)
        self.assertIn("missing 5", proc.stderr)

    def test_extra_method_is_rejected(self):
        lines = list(self.lines)
        anchor = self.one_class_7_line(CLASS_7_METHOD_2)
        lines.insert(anchor + 1, "             3．注入方法                                          可选")
        proc = self.run_lines(lines)
        self.assertNotEqual(proc.returncode, 0,
                            "generator accepted an extra Class 7 method 3")
        self.assertIn("class 7", proc.stderr)
        self.assertIn("extra 3", proc.stderr)

    def test_oi_method_outside_the_class_set_is_rejected(self):
        lines = list(self.lines)
        hit = next((i for i, line in enumerate(lines)
                    if OI_4307_METHOD_127.match(line.strip())), None)
        self.assertIsNotNone(hit, "fixture no longer has the 4307 method 127 row")
        lines[hit] = lines[hit].replace("方法 127：出厂启用（参数）", "方法 126：出厂启用（参数）")
        proc = self.run_lines(lines)
        self.assertNotEqual(proc.returncode, 0,
                            "generator accepted OI 4307 method 126 outside class 19")
        self.assertIn("4307", proc.stderr)
        self.assertIn("126", proc.stderr)

    def test_real_source_keeps_generated_outputs_byte_identical(self):
        proc = self.run_lines(self.lines)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        root = self.roots[-1]
        for relpath in OUTPUTS:
            with self.subTest(path=relpath):
                self.assertEqual(
                    (root / relpath).read_bytes(),
                    (REPO_ROOT / relpath).read_bytes(),
                    f"real source produced a different {relpath}",
                )

    def test_repository_catalog_check_is_green(self):
        proc = subprocess.run(
            [sys.executable, "-I", str(GENERATOR), "--check", "--root", str(REPO_ROOT)],
            capture_output=True,
            text=True,
        )
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)


if __name__ == "__main__":
    unittest.main()
