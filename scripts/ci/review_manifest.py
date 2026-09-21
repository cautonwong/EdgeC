#!/usr/bin/env python3
"""
Canonical Review Manifest Engine for EdgeOS / EdgeC AI Code Review
Issue #321 架构闭环实现：
1. 统一 Diff Scope，消除 Workflow 与 MCP 两套不同 Git Diff Scope 的语义漂移；
2. 明确建模 Diff 状态 (SUCCESS / EMPTY / ERROR)；
3. 计算确定性的 changed_files_hash 与 review identity 凭证；
4. 明确声明本次 Review 必须覆盖的 mandatory tools 列表。
"""

import os
import sys
import json
import hashlib
import datetime
import subprocess
from dataclasses import dataclass, field, asdict
from typing import List, Dict, Set, Optional, Any


DEFAULT_MANDATORY_TOOLS = [
    "get_review_context_tool",
    "scan_ocr_rules",
    "match_company_rules",
    "check_isr_safety",
    "check_hardware_polling_timeout",
    "check_physical_truth_violations",
    "check_buffer_security"
]


@dataclass
class ReviewManifest:
    repository: str
    base_sha: str
    head_sha: str
    diff_status: str  # "SUCCESS" | "EMPTY" | "ERROR"
    changed_files: List[str]
    changed_lines: Dict[str, List[int]]
    changed_files_hash: str
    required_tools: List[str]
    pr_number: Optional[int] = None
    rules_revision: str = "v2.0-modular"
    created_at: str = field(default_factory=lambda: datetime.datetime.now(datetime.timezone.utc).isoformat())

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent, ensure_ascii=False)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "ReviewManifest":
        return cls(
            repository=data["repository"],
            base_sha=data["base_sha"],
            head_sha=data["head_sha"],
            diff_status=data["diff_status"],
            changed_files=data["changed_files"],
            changed_lines=data["changed_lines"],
            changed_files_hash=data["changed_files_hash"],
            required_tools=data.get("required_tools", DEFAULT_MANDATORY_TOOLS),
            pr_number=data.get("pr_number"),
            rules_revision=data.get("rules_revision", "v2.0-modular"),
            created_at=data.get("created_at", datetime.datetime.now(datetime.timezone.utc).isoformat())
        )


def calculate_changed_files_hash(changed_files: List[str], changed_lines: Dict[str, Any]) -> str:
    """计算变更文件及行号范围的确定性 SHA256 哈希"""
    hasher = hashlib.sha256()
    sorted_files = sorted(changed_files)
    for f in sorted_files:
        hasher.update(f.encode("utf-8"))
        lines = sorted(list(changed_lines.get(f, [])))
        hasher.update(",".join(map(str, lines)).encode("utf-8"))
    return hasher.hexdigest()


def extract_git_diff(base: str = "origin/main") -> tuple[str, Dict[str, List[int]], str]:
    """从 Git 提取 diff 状态、改动文件与行号"""
    candidate_bases = [base, "main", "HEAD~1"]
    diff_output = ""
    diff_status = "SUCCESS"

    for candidate in candidate_bases:
        try:
            cmd = ["git", "diff", "--unified=0", candidate, "HEAD"]
            res = subprocess.run(cmd, capture_output=True, text=True, check=False)
            if res.returncode == 0:
                diff_output = res.stdout
                break
        except Exception:
            continue

    if not diff_output:
        try:
            cmd = ["git", "diff", "--unified=0", "HEAD"]
            res = subprocess.run(cmd, capture_output=True, text=True, check=False)
            if res.returncode == 0:
                diff_output = res.stdout
        except Exception:
            pass

    changed_lines: Dict[str, List[int]] = {}
    cur_file = None
    for line in diff_output.splitlines():
        if line.startswith("+++ b/"):
            cur_file = line[6:]
            if cur_file not in changed_lines:
                changed_lines[cur_file] = []
        elif line.startswith("@@") and cur_file:
            try:
                plus_part = line.split("+")[1].split(" ")[0]
                parts = plus_part.split(",")
                start = int(parts[0])
                count = int(parts[1]) if len(parts) > 1 else 1
                for l in range(start, start + count):
                    changed_lines[cur_file].append(l)
            except Exception:
                pass

    changed_files = list(changed_lines.keys())
    if not changed_files and not diff_output.strip():
        diff_status = "EMPTY"

    return diff_status, changed_lines, diff_output


def compute_review_manifest(
    repository: str = "cautonwong/EdgeC",
    pr_number: Optional[int] = None,
    base_sha: str = "origin/main",
    head_sha: str = "HEAD",
    changed_files: Optional[List[str]] = None,
    changed_lines: Optional[Dict[str, Any]] = None,
    required_tools: Optional[List[str]] = None,
    diff_status: Optional[str] = None
) -> ReviewManifest:
    """计算规范 Review Manifest"""
    if changed_files is None or changed_lines is None:
        extracted_status, extracted_lines, _ = extract_git_diff(base_sha)
        if diff_status is None:
            diff_status = extracted_status
        if changed_files is None:
            changed_files = list(extracted_lines.keys())
        if changed_lines is None:
            changed_lines = extracted_lines
    else:
        if diff_status is None:
            diff_status = "SUCCESS" if changed_files else "EMPTY"

    # 格式化 changed_lines 为 List[int]
    formatted_lines: Dict[str, List[int]] = {}
    for k, v in changed_lines.items():
        formatted_lines[k] = sorted(list(v))

    if required_tools is None:
        required_tools = list(DEFAULT_MANDATORY_TOOLS)

    files_hash = calculate_changed_files_hash(changed_files, formatted_lines)

    return ReviewManifest(
        repository=repository,
        pr_number=pr_number,
        base_sha=base_sha,
        head_sha=head_sha,
        diff_status=diff_status,
        changed_files=changed_files,
        changed_lines=formatted_lines,
        changed_files_hash=files_hash,
        required_tools=required_tools
    )


if __name__ == "__main__":
    manifest = compute_review_manifest()
    print(manifest.to_json())
