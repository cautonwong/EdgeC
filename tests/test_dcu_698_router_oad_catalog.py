#!/usr/bin/env python3
import json
import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
HEADER = REPO_ROOT / "cmd" / "dcu" / "dcu_router.h"
CATALOG = REPO_ROOT / "docs" / "design" / "DCU_698_OI_CATALOG.json"
OAD_DEFINE = re.compile(r"^#define\s+(OAD_[A-Z0-9_]+)\s+(0x[0-9A-Fa-f]+)", re.MULTILINE)


def oad_macros(text: str) -> dict[str, int]:
    return {name: int(value, 16) for name, value in OAD_DEFINE.findall(text.replace("\\\n", " "))}


def catalog_ois() -> dict[int, dict]:
    with CATALOG.open(encoding="utf-8") as handle:
        return {int(entry["oi"], 16): entry for entry in json.load(handle)["ois"]}


class RouterOadCatalogTest(unittest.TestCase):
    def test_every_oad_macro_oi_is_in_the_catalog(self):
        ois = catalog_ois()
        for name, value in oad_macros(HEADER.read_text(encoding="utf-8")).items():
            oi = value >> 16
            self.assertIn(
                oi,
                ois,
                f"{name}=0x{value:08X} uses OI 0x{oi:04X}, which is absent from the catalog",
            )

    def test_zero_seq_current_macro_is_absent_or_zero_sequence_current(self):
        macros = oad_macros(HEADER.read_text(encoding="utf-8"))
        if "OAD_ZERO_SEQ_CURRENT" not in macros:
            return
        value = macros["OAD_ZERO_SEQ_CURRENT"]
        oi = value >> 16
        entry = catalog_ois()[oi]
        self.assertIn(
            "零序",
            entry["name"],
            f"OAD_ZERO_SEQ_CURRENT=0x{value:08X} resolves to OI 0x{oi:04X} "
            f"{entry['name']!r}; table A.3 defines no zero-sequence current OI",
        )


if __name__ == "__main__":
    unittest.main()
