#!/usr/bin/env python3
"""
Test Suite for AI Code Review Orchestrator, SQLite CRG & Alibaba OCR Engine
包含：
1. SQLite CRG 全库关系图谱与爆炸半径测试
2. Alibaba OCR 规则深度匹配 (ISR/超时/物理真实性/字符串安全)
3. 质量门禁阻断与放行判定 (Red-Green 闭环验证)
"""

import os
import sys
import json
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../scripts/ci"))
from code_review_graph import CodeReviewGraph
from unified_review_mcp import (
    get_git_diff_lines,
    match_company_rules,
    AlibabaOCREngine
)
from run_pure_mcp_orchestrator import run_mcp_orchestration


class TestCIOrchestratorDeep(unittest.TestCase):

    def setUp(self):
        self.tmp_dir = tempfile.TemporaryDirectory()
        self.db_path = os.path.join(self.tmp_dir.name, "test_crg.sqlite")

    def tearDown(self):
        self.tmp_dir.cleanup()

    def test_company_rules_integrity(self):
        """验证企业经验规约完整性"""
        rules_str = match_company_rules()
        self.assertTrue(rules_str.startswith("["), "Company rules should be valid JSON array")
        rules = json.loads(rules_str)
        rule_ids = {r["id"] for r in rules}
        self.assertIn("RULE-SAFE-01", rule_ids)
        self.assertIn("RULE-SAFE-02", rule_ids)
        self.assertIn("RULE-EDGEOS-03", rule_ids)
        self.assertIn("RULE-EDGEOS-04", rule_ids)
        self.assertIn("RULE-EDGEOS-05", rule_ids)
        self.assertIn("RULE-EDGEOS-06", rule_ids)
        self.assertIn("RULE-EDGEOS-07", rule_ids)

    def test_sqlite_crg_graph_indexing(self):
        """验证 SQLite CRG 关系图谱构建与爆炸半径解析"""
        crg = CodeReviewGraph(root_dir=".", db_path=self.db_path)
        crg.build(verbose=False)

        blast = crg.query_review_context(["modules/edge_infra/npu/npu_ascend_acl.c"])
        self.assertIn("risk_score", blast)
        self.assertIn("impacted_modules", blast)
        self.assertIn("edge_infra", blast["impacted_modules"])
        self.assertIn("impacted_tests", blast)

    def test_alibaba_ocr_isr_safety_violation(self):
        """验证 Alibaba OCR 对 ISR 阻塞/堆分配违规拦截 (P0)"""
        ocr = AlibabaOCREngine()
        bad_isr_code = """
        void gpio_exti_isr(void) {
            char *buf = (char *)malloc(128);
            printf("Interrupt triggered!\\n");
            free(buf);
        }
        """
        isr_file = os.path.join(self.tmp_dir.name, "bad_isr.c")
        with open(isr_file, "w") as f:
            f.write(bad_isr_code)

        findings = ocr.scan_files([isr_file], {isr_file: {3, 4, 5}})
        rule_ids = [f["rule_id"] for f in findings]
        priorities = [f["priority"] for f in findings]

        self.assertIn("RULE-SAFE-01", rule_ids)
        self.assertIn("P0", priorities)

    def test_alibaba_ocr_unbounded_polling_violation(self):
        """验证 Alibaba OCR 对无超时寄存器轮询的拦截 (P0)"""
        ocr = AlibabaOCREngine()
        bad_polling_code = """
        int wait_for_hardware_ready(volatile uint32_t *reg) {
            while (*reg & 0x01);
            return 0;
        }
        """
        poll_file = os.path.join(self.tmp_dir.name, "bad_poll.c")
        with open(poll_file, "w") as f:
            f.write(bad_polling_code)

        findings = ocr.scan_files([poll_file], {poll_file: {3}})
        rule_ids = [f["rule_id"] for f in findings]
        self.assertIn("RULE-SAFE-02", rule_ids)

    def test_alibaba_ocr_mock_placeholder_violation(self):
        """验证 Alibaba OCR 对物理真实性违规 (Mock 占位符入库) 的拦截 (P0)"""
        ocr = AlibabaOCREngine()
        bad_mock_code = """
        void read_grid_voltage(double *out_val) {
            double voltage = 220.0;
            *out_val = voltage;
        }
        """
        mock_file = os.path.join(self.tmp_dir.name, "bad_driver.c")
        with open(mock_file, "w") as f:
            f.write(bad_mock_code)

        findings = ocr.scan_files([mock_file], {mock_file: {3}})
        rule_ids = [f["rule_id"] for f in findings]
        self.assertIn("RULE-EDGEOS-03", rule_ids)

    def test_alibaba_ocr_unsafe_string_copy(self):
        """验证 Alibaba OCR 对不安全字符串复制函数的警告 (P1)"""
        ocr = AlibabaOCREngine()
        bad_str_code = """
        void copy_tag_name(char *dst, const char *src) {
            sprintf(dst, "tag_%s", src);
        }
        """
        str_file = os.path.join(self.tmp_dir.name, "bad_str.c")
        with open(str_file, "w") as f:
            f.write(bad_str_code)

        findings = ocr.scan_files([str_file], {str_file: {3}})
        rule_ids = [f["rule_id"] for f in findings]
        self.assertIn("OCR-MEM-02", rule_ids)

    def test_clean_synthesis_pass(self):
        """验证干净代码分析流程及确定性本地扫描兜底"""
        import asyncio
        from review_manifest import compute_review_manifest
        from tool_ledger import ToolExecutionLedger
        from run_pure_mcp_orchestrator import synthesize_deterministic_fallback_report

        manifest = compute_review_manifest(changed_files=[])
        ledger = ToolExecutionLedger(required_tools=manifest.required_tools)
        report = asyncio.run(run_mcp_orchestration(manifest, ledger))
        if not report:
            report = synthesize_deterministic_fallback_report(manifest, ledger)

        self.assertIsNotNone(report)
        self.assertIn("summary", report)
        self.assertIn("P0", report["summary"])
        self.assertIn("findings", report)


if __name__ == "__main__":
    unittest.main()
