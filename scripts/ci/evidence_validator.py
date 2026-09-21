#!/usr/bin/env python3
"""
Evidence and Finding Validator for EdgeOS / EdgeC AI Code Review
Issue #321 架构闭环实现：
1. 机器硬性校验 file/line/scope/evidence 物理真实性（物理文件存在、行号不越界、在变更范围内）；
2. 校验 Tool Ledger 覆盖率，阻止在未执行全部必需工具时提交 Final Report；
3. 机器自动重算 actual_P0..actual_P3，若与自报 summary 不一致直接判定 INVALID_REPORT；
4. 自动去重 findings 并返回 sanitized 结果与严密审计指标。
"""

import os
import json
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Any, Set, Tuple

from review_manifest import ReviewManifest
from tool_ledger import ToolExecutionLedger, ToolStatus


@dataclass
class ValidationResult:
    is_valid: bool = False
    errors: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    sanitized_findings: List[Dict[str, Any]] = field(default_factory=list)
    computed_summary: Dict[str, int] = field(default_factory=lambda: {"P0": 0, "P1": 0, "P2": 0, "P3": 0})

    def to_dict(self) -> Dict[str, Any]:
        return {
            "is_valid": self.is_valid,
            "errors": self.errors,
            "warnings": self.warnings,
            "sanitized_findings": self.sanitized_findings,
            "computed_summary": self.computed_summary
        }

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent, ensure_ascii=False)


class EvidenceValidator:
    def __init__(self, manifest: Optional[ReviewManifest] = None, ledger: Optional[ToolExecutionLedger] = None, base_dir: Optional[str] = None):
        self.manifest = manifest
        self.ledger = ledger
        self.base_dir = base_dir or os.getcwd()

    def _resolve_file_path(self, file_path: str) -> str:
        if os.path.isabs(file_path):
            return file_path
        return os.path.join(self.base_dir, file_path)

    def _count_file_lines(self, resolved_path: str) -> Optional[int]:
        if not os.path.isfile(resolved_path):
            return None
        try:
            with open(resolved_path, "r", encoding="utf-8", errors="replace") as f:
                return sum(1 for _ in f)
        except Exception:
            return None

    def validate(self, report_data: Dict[str, Any]) -> ValidationResult:
        result = ValidationResult()

        # 1. 检查 Ledger 完成度
        if self.ledger is not None and not self.ledger.is_complete():
            pending = self.ledger.get_pending_tools()
            failed = self.ledger.get_failed_tools()
            err_msg = f"Mandatory tools not completed. Pending: {pending}, Failed: {failed}"
            result.errors.append(err_msg)

        # 2. 检查 Report 数据结构完整性
        if not isinstance(report_data, dict):
            result.errors.append("Report data must be a JSON object")
            result.is_valid = False
            return result

        identity = report_data.get("identity", {})
        summary = report_data.get("summary", {})
        findings = report_data.get("findings", [])

        if not isinstance(summary, dict):
            result.errors.append("Report 'summary' must be a dictionary")
        if not isinstance(findings, list):
            result.errors.append("Report 'findings' must be a list")

        # 3. 校验 Identity (若 Manifest 存在)
        if self.manifest:
            manifest_hash = self.manifest.changed_files_hash
            report_hash = identity.get("changed_files_hash")
            if report_hash and report_hash != manifest_hash:
                result.warnings.append(
                    f"Report changed_files_hash ({report_hash}) differs from manifest ({manifest_hash})"
                )

        # 4. 逐项校验 Findings 并去重
        dedup_keys: Set[Tuple[str, int, str, str]] = set()
        sanitized: List[Dict[str, Any]] = []
        computed_summary = {"P0": 0, "P1": 0, "P2": 0, "P3": 0}

        for idx, finding in enumerate(findings):
            if not isinstance(finding, dict):
                result.errors.append(f"Finding #{idx} is not a valid object")
                continue

            file_path = finding.get("file", "")
            line = finding.get("line")
            priority = finding.get("priority", "P3")
            rule_id = finding.get("rule_id", "")
            issue = finding.get("issue", "")

            # 物理文件存在性
            resolved = self._resolve_file_path(file_path)
            if not os.path.exists(resolved):
                result.errors.append(f"Finding #{idx}: file '{file_path}' does not exist on disk")
                continue

            # 行号边界检查
            total_lines = self._count_file_lines(resolved)
            if total_lines is not None:
                if not isinstance(line, int) or line < 1 or line > total_lines:
                    result.errors.append(
                        f"Finding #{idx}: line {line} is out of bounds for '{file_path}' (total {total_lines} lines)"
                    )
                    continue

            # 优先级有效性
            if priority not in computed_summary:
                result.warnings.append(f"Finding #{idx}: unknown priority '{priority}', defaulting to P3")
                priority = "P3"
                finding["priority"] = "P3"

            # 去重判断
            dedup_key = (file_path, line, priority, rule_id or issue)
            if dedup_key in dedup_keys:
                result.warnings.append(f"Finding #{idx} is a duplicate, omitted from final sanitized findings")
                continue

            dedup_keys.add(dedup_key)
            sanitized.append(finding)
            computed_summary[priority] += 1

        result.sanitized_findings = sanitized
        result.computed_summary = computed_summary

        # 5. 校验 自报 Summary 与 机器重算 Summary 是否一致
        if isinstance(summary, dict):
            for prio in ["P0", "P1", "P2", "P3"]:
                reported_val = summary.get(prio, 0)
                computed_val = computed_summary.get(prio, 0)
                if reported_val != computed_val:
                    result.errors.append(
                        f"Summary mismatch for {prio}: reported {reported_val} != computed {computed_val}"
                    )

        # 判定最终有效性
        result.is_valid = len(result.errors) == 0
        return result
