#!/usr/bin/env python3
import ast
import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

IDL_SCRIPTS = [
    REPO_ROOT / "scripts" / "build_exhaustive_idls.py",
    REPO_ROOT / "scripts" / "build_clean_human_readable_idls.py",
    REPO_ROOT / "scripts" / "parse_full_pdf_schemas.py",
    REPO_ROOT / "scripts" / "parse_exhaustive_698_dlms_idls.py",
]

FORBIDDEN_2002_TERMS = ("ZERO_SEQ", "零序", "VOLTAGE_PHASE_C", "C相电压")


def _extract_2002_entries(script_path: Path) -> list[tuple[str, ...]]:
    tree = ast.parse(script_path.read_text(encoding="utf-8"), filename=str(script_path))
    entries = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Tuple) and len(node.elts) >= 3:
            first, second = node.elts[0], node.elts[1]
            is_int_2002 = isinstance(first, ast.Constant) and first.value == 0x20020200
            is_str_2002 = (
                isinstance(second, ast.Constant)
                and isinstance(second.value, str)
                and second.value.lower() == "0x20020200"
            )
            if is_int_2002 or is_str_2002:
                vals = tuple(
                    str(elt.value) if isinstance(elt, ast.Constant) else ast.dump(elt)
                    for elt in node.elts
                )
                entries.append(vals)
    return entries


class DcuScriptsOadTest(unittest.TestCase):
    def test_idl_scripts_do_not_mislabel_0x20020200(self):
        for script in IDL_SCRIPTS:
            entries = _extract_2002_entries(script)
            self.assertTrue(entries, f"{script.name} missing 0x20020200 definition")
            for entry in entries:
                joined = " ".join(entry)
                for term in FORBIDDEN_2002_TERMS:
                    self.assertNotIn(
                        term,
                        joined,
                        f"{script.name} maps 0x20020200 to forbidden term {term!r}: {entry}",
                    )
                self.assertIn("OAD_VOLTAGE_PHASE_ANGLE", joined)
                self.assertIn("电压相角", joined)

    def test_dcu_698_rules_test_uses_snprintf_not_strcpy(self):
        rules_test = REPO_ROOT / "tests" / "test_dcu_698_rules.c"
        text = rules_test.read_text(encoding="utf-8")
        self.assertIsNone(
            re.search(r"\bstrcpy\s*\(", text),
            "tests/test_dcu_698_rules.c must not use strcpy",
        )
        self.assertIsNotNone(
            re.search(
                r'snprintf\s*\(\s*b->path\s*,\s*sizeof\s*\(\s*b->path\s*\)\s*,\s*"/tmp/dcu_698_rules_XXXXXX"\s*\)',
                text,
            ),
            "tests/test_dcu_698_rules.c must initialize b->path via snprintf",
        )


if __name__ == "__main__":
    unittest.main()
