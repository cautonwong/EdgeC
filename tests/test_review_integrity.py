#!/usr/bin/env python3
"""
Test Suite for AI Code Review Integrity, Canonical Manifest, Tool Ledger & Evidence Validator
Issue #321 验收用例
"""

import os
import sys
import json
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../scripts/ci"))
from review_manifest import ReviewManifest, compute_review_manifest
from tool_ledger import ToolExecutionLedger, ToolStatus
from evidence_validator import EvidenceValidator, ValidationResult


class TestReviewIntegrity(unittest.TestCase):

    def setUp(self):
        self.tmp_dir = tempfile.TemporaryDirectory()
        self.test_file = os.path.join(self.tmp_dir.name, "test_target.c")
        with open(self.test_file, "w", encoding="utf-8") as f:
            f.write("""#include <stdio.h>
#include <string.h>

void process_data(const char *src) {
    char buf[64];
    strcpy(buf, src); // line 6
    printf("%s\\n", buf);
}
""")

    def tearDown(self):
        self.tmp_dir.cleanup()

    def test_canonical_manifest_generation(self):
        """验证 Review Manifest 结构、哈希与必检工具矩阵生成"""
        manifest = compute_review_manifest(
            repository="cautonwong/EdgeC",
            pr_number=321,
            base_sha="abcdef123456",
            head_sha="789012abcdef",
            changed_files=[self.test_file],
            changed_lines={self.test_file: {5, 6, 7}},
            required_tools=["scan_ocr_rules", "get_review_context_tool", "run_clang_tidy_check"]
        )

        self.assertEqual(manifest.repository, "cautonwong/EdgeC")
        self.assertEqual(manifest.pr_number, 321)
        self.assertEqual(manifest.base_sha, "abcdef123456")
        self.assertEqual(manifest.head_sha, "789012abcdef")
        self.assertEqual(manifest.diff_status, "SUCCESS")
        self.assertIn(self.test_file, manifest.changed_files)
        self.assertTrue(len(manifest.changed_files_hash) > 0)
        self.assertIn("scan_ocr_rules", manifest.required_tools)

    def test_manifest_empty_and_error_states(self):
        """验证 Diff 为空或异常时的显式状态建模"""
        manifest_empty = compute_review_manifest(
            repository="cautonwong/EdgeC",
            base_sha="aaa",
            head_sha="aaa",
            changed_files=[],
            changed_lines={},
            diff_status="EMPTY"
        )
        self.assertEqual(manifest_empty.diff_status, "EMPTY")

        manifest_err = compute_review_manifest(
            repository="cautonwong/EdgeC",
            base_sha="aaa",
            head_sha="bbb",
            changed_files=[],
            changed_lines={},
            diff_status="ERROR"
        )
        self.assertEqual(manifest_err.diff_status, "ERROR")

    def test_tool_ledger_lifecycle_and_coverage(self):
        """验证 Tool Execution Ledger 状态流转与必需工具覆盖判定"""
        required = ["scan_ocr_rules", "check_buffer_security", "run_clang_tidy_check"]
        ledger = ToolExecutionLedger(required_tools=required)

        # 初始状态：全部 NOT_RUN
        self.assertFalse(ledger.is_complete())
        self.assertEqual(ledger.get_status("scan_ocr_rules"), ToolStatus.NOT_RUN)

        # 执行中
        ledger.start_tool("scan_ocr_rules", {"files": ["foo.c"]})
        self.assertEqual(ledger.get_status("scan_ocr_rules"), ToolStatus.RUNNING)
        self.assertFalse(ledger.is_complete())

        # 完成部分工具
        ledger.finish_tool("scan_ocr_rules", ToolStatus.FINDINGS, findings_count=2)
        ledger.finish_tool("check_buffer_security", ToolStatus.SUCCESS, findings_count=0)
        self.assertFalse(ledger.is_complete(), "仍有工具未执行")

        # 工具异常
        ledger.finish_tool("run_clang_tidy_check", ToolStatus.ERROR, error_message="clang-tidy crash")
        self.assertFalse(ledger.is_complete(), "存在 ERROR 工具时不应视为 complete")

        # 修复重试后变绿
        ledger.finish_tool("run_clang_tidy_check", ToolStatus.SUCCESS, findings_count=0)
        self.assertTrue(ledger.is_complete())

    def test_evidence_validator_rejects_nonexistent_file(self):
        """验证 Validator 拒绝不存在的文件 findings"""
        manifest = compute_review_manifest(
            repository="cautonwong/EdgeC",
            base_sha="base",
            head_sha="head",
            changed_files=[self.test_file],
            changed_lines={self.test_file: {6}},
            required_tools=["scan_ocr_rules"]
        )
        ledger = ToolExecutionLedger(required_tools=["scan_ocr_rules"])
        ledger.finish_tool("scan_ocr_rules", ToolStatus.FINDINGS, findings_count=1)

        validator = EvidenceValidator(manifest=manifest, ledger=ledger)
        report = {
            "identity": {
                "base_sha": "base",
                "head_sha": "head",
                "changed_files_hash": manifest.changed_files_hash
            },
            "summary": {"P0": 0, "P1": 1, "P2": 0, "P3": 0},
            "findings": [{
                "file": "nonexistent/ghost_file.c",
                "line": 10,
                "priority": "P1",
                "rule_id": "OCR-MEM-02",
                "source": "AlibabaOCR",
                "issue": "Insecure string copy",
                "fix_suggestion": "Use safe copy"
            }]
        }
        res = validator.validate(report)
        self.assertFalse(res.is_valid)
        self.assertIn("nonexistent/ghost_file.c", res.errors[0])
        self.assertIn("does not exist", res.errors[0])

    def test_evidence_validator_rejects_invalid_line_number(self):
        """验证 Validator 拒绝越界的行号"""
        manifest = compute_review_manifest(
            repository="cautonwong/EdgeC",
            base_sha="base",
            head_sha="head",
            changed_files=[self.test_file],
            changed_lines={self.test_file: {6}},
            required_tools=["scan_ocr_rules"]
        )
        ledger = ToolExecutionLedger(required_tools=["scan_ocr_rules"])
        ledger.finish_tool("scan_ocr_rules", ToolStatus.FINDINGS, findings_count=1)

        validator = EvidenceValidator(manifest=manifest, ledger=ledger)
        report = {
            "identity": {
                "base_sha": "base",
                "head_sha": "head",
                "changed_files_hash": manifest.changed_files_hash
            },
            "summary": {"P0": 0, "P1": 1, "P2": 0, "P3": 0},
            "findings": [{
                "file": self.test_file,
                "line": 9999,  # 超出行数
                "priority": "P1",
                "rule_id": "OCR-MEM-02",
                "source": "AlibabaOCR",
                "issue": "Insecure copy",
                "fix_suggestion": "Fix"
            }]
        }
        res = validator.validate(report)
        self.assertFalse(res.is_valid)
        self.assertIn("out of bounds", res.errors[0])

    def test_evidence_validator_summary_mismatch(self):
        """验证机器重算 actual_summary 与自报 summary 不一致时拦截"""
        manifest = compute_review_manifest(
            repository="cautonwong/EdgeC",
            base_sha="base",
            head_sha="head",
            changed_files=[self.test_file],
            changed_lines={self.test_file: {6}},
            required_tools=["scan_ocr_rules"]
        )
        ledger = ToolExecutionLedger(required_tools=["scan_ocr_rules"])
        ledger.finish_tool("scan_ocr_rules", ToolStatus.FINDINGS, findings_count=1)

        validator = EvidenceValidator(manifest=manifest, ledger=ledger)
        report = {
            "identity": {
                "base_sha": "base",
                "head_sha": "head",
                "changed_files_hash": manifest.changed_files_hash
            },
            "summary": {"P0": 0, "P1": 0, "P2": 0, "P3": 0},  # 自报 0 个缺陷
            "findings": [{
                "file": self.test_file,
                "line": 6,
                "priority": "P1",  # 实际有 1 个 P1
                "rule_id": "OCR-MEM-02",
                "source": "AlibabaOCR",
                "issue": "strcpy buffer overflow risk",
                "fix_suggestion": "Use safe copy"
            }]
        }
        res = validator.validate(report)
        self.assertFalse(res.is_valid)
        self.assertIn("Summary mismatch", res.errors[0])

    def test_evidence_validator_deduplication(self):
        """验证重复 findings 自动去重并重新核算"""
        manifest = compute_review_manifest(
            repository="cautonwong/EdgeC",
            base_sha="base",
            head_sha="head",
            changed_files=[self.test_file],
            changed_lines={self.test_file: {6}},
            required_tools=["scan_ocr_rules"]
        )
        ledger = ToolExecutionLedger(required_tools=["scan_ocr_rules"])
        ledger.finish_tool("scan_ocr_rules", ToolStatus.FINDINGS, findings_count=1)

        validator = EvidenceValidator(manifest=manifest, ledger=ledger)
        duplicate_findings = [
            {
                "file": self.test_file,
                "line": 6,
                "priority": "P1",
                "rule_id": "OCR-MEM-02",
                "source": "AlibabaOCR",
                "issue": "Insecure strcpy",
                "fix_suggestion": "Fix"
            },
            {
                "file": self.test_file,
                "line": 6,
                "priority": "P1",
                "rule_id": "OCR-MEM-02",
                "source": "AlibabaOCR",
                "issue": "Insecure strcpy duplicate",
                "fix_suggestion": "Fix"
            }
        ]
        report = {
            "identity": {
                "base_sha": "base",
                "head_sha": "head",
                "changed_files_hash": manifest.changed_files_hash
            },
            "summary": {"P0": 0, "P1": 1, "P2": 0, "P3": 0},
            "findings": duplicate_findings
        }
        res = validator.validate(report)
        self.assertTrue(res.is_valid)
        self.assertEqual(len(res.sanitized_findings), 1, "应自动去重为 1 条")
        self.assertEqual(res.computed_summary["P1"], 1)

    def test_evidence_validator_incomplete_ledger_blocked(self):
        """验证未执行全部必需工具时阻止提交报告"""
        manifest = compute_review_manifest(
            repository="cautonwong/EdgeC",
            base_sha="base",
            head_sha="head",
            changed_files=[self.test_file],
            changed_lines={self.test_file: {6}},
            required_tools=["scan_ocr_rules", "run_clang_tidy_check"]
        )
        ledger = ToolExecutionLedger(required_tools=["scan_ocr_rules", "run_clang_tidy_check"])
        # 只执行了 scan_ocr_rules，clang-tidy 为 NOT_RUN
        ledger.finish_tool("scan_ocr_rules", ToolStatus.SUCCESS)

        validator = EvidenceValidator(manifest=manifest, ledger=ledger)
        report = {
            "identity": {
                "base_sha": "base",
                "head_sha": "head",
                "changed_files_hash": manifest.changed_files_hash
            },
            "summary": {"P0": 0, "P1": 0, "P2": 0, "P3": 0},
            "findings": []
        }
        res = validator.validate(report)
        self.assertFalse(res.is_valid)
        self.assertIn("Mandatory tools not completed", res.errors[0])


if __name__ == "__main__":
    unittest.main()
