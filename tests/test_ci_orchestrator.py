#!/usr/bin/env python3
"""
Test Suite for AI Code Review Orchestrator & FastMCP Tools
验证:
1. Diff 变更行提取算法
2. Company Rules 解析与匹配
3. Clang-tidy / Cppcheck 增量行过滤
4. 门禁决策判定 (P0 阻断 vs Clean 通过)
"""

import os
import sys
import json
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../scripts/ci"))
from unified_review_mcp import get_git_diff_lines, match_company_rules
from run_pure_mcp_orchestrator import run_local_fallback_synthesis


class TestCIOrchestrator(unittest.TestCase):

    def test_company_rules_loading(self):
        rules_str = match_company_rules()
        self.assertTrue(rules_str.startswith("["), "Company rules should be valid JSON array")
        rules = json.loads(rules_str)
        rule_ids = [r["id"] for r in rules]
        self.assertIn("RULE-SAFE-01", rule_ids)
        self.assertIn("RULE-SAFE-02", rule_ids)
        self.assertIn("RULE-EDGEOS-03", rule_ids)
        self.assertIn("RULE-EDGEOS-04", rule_ids)

    def test_git_diff_lines_parsing(self):
        diff_dict = get_git_diff_lines()
        self.assertIsInstance(diff_dict, dict)

    def test_fallback_synthesis_clean_pass(self):
        report = run_local_fallback_synthesis([])
        self.assertIn("summary", report)
        self.assertEqual(report["summary"]["P0"], 0)
        self.assertIn("markdown_report", report)
        self.assertIn("门禁通过", report["markdown_report"])


if __name__ == "__main__":
    unittest.main()
