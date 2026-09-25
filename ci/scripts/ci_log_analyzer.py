#!/usr/bin/env python3
"""
ci_log_analyzer.py - EdgeOS CI 智能日志分析与故障精准提取工具
借鉴自 Fast-DDS CI log_parser.py 实践：
1. 自动提取 ASAN / TSAN / UBSAN 运行时崩溃与未定义行为第一现场；
2. 聚合并去重 CTest 失败用例堆栈；
3. 解析架构门禁 (Plane Isolation, Caller-Allocated Storage, Zero-Alloc) 违规；
4. 生成 GitHub Actions Step Summary Markdown 格式报告。
"""

import sys
import os
import re
import argparse
from typing import List, Dict, Tuple

# 正则匹配规则
_ASAN_CRASH_RE = re.compile(r'==\d+==ERROR: AddressSanitizer: ([^\n]+)')
_TSAN_WARNING_RE = re.compile(r'WARNING: ThreadSanitizer: ([^\n]+)')
_UBSAN_ERROR_RE = re.compile(r'(?P<path>\S+?):(?P<line>\d+):(?P<col>\d+):\s*runtime error:\s*(?P<desc>.+?)$')
_CTEST_FAIL_RE = re.compile(r'^\s*(?P<id>\d+)/(?P<total>\d+)\s+Test\s+#(?P<num>\d+):\s+(?P<name>\S+)\s*\.{3,}\s*\*{3}Failed', re.MULTILINE)
_ARCH_VIOLATION_RE = re.compile(r'(❌\s*\[[^\]]+\][^\n]+)')

def parse_log(log_text: str) -> Dict[str, List]:
    findings = {
        "asan": [],
        "tsan": [],
        "ubsan": [],
        "ctest_fails": [],
        "arch_violations": []
    }

    # 1. AddressSanitizer
    for match in _ASAN_CRASH_RE.finditer(log_text):
        findings["asan"].append(match.group(1).strip())

    # 2. ThreadSanitizer
    for match in _TSAN_WARNING_RE.finditer(log_text):
        findings["tsan"].append(match.group(1).strip())

    # 3. UndefinedBehaviorSanitizer
    ubsan_seen = set()
    for line in log_text.splitlines():
        m = _UBSAN_ERROR_RE.search(line)
        if m:
            key = f"{m.group('path')}:{m.group('line')}:{m.group('desc')}"
            if key not in ubsan_seen:
                ubsan_seen.add(key)
                findings["ubsan"].append({
                    "path": m.group("path"),
                    "line": m.group("line"),
                    "col": m.group("col"),
                    "desc": m.group("desc")
                })

    # 4. CTest Failures
    for match in _CTEST_FAIL_RE.finditer(log_text):
        findings["ctest_fails"].append({
            "num": match.group("num"),
            "name": match.group("name")
        })

    # 5. Architecture Violations
    for match in _ARCH_VIOLATION_RE.finditer(log_text):
        findings["arch_violations"].append(match.group(1).strip())

    return findings

def generate_markdown_report(findings: Dict[str, List], target_name: str = "CI") -> str:
    lines = []
    lines.append(f"## 🛡️ EdgeOS CI Diagnostics & Log Analysis Report ({target_name})")
    lines.append("")

    total_issues = (len(findings["asan"]) + len(findings["tsan"]) + 
                    len(findings["ubsan"]) + len(findings["ctest_fails"]) + 
                    len(findings["arch_violations"]))

    if total_issues == 0:
        lines.append("✅ **Clean Run**: No Sanitizer faults, test regressions, or architectural violations detected.")
        return "\n".join(lines)

    lines.append(f"⚠️ **Detected {total_issues} issue(s) requiring attention:**")
    lines.append("")

    if findings["ctest_fails"]:
        lines.append("### ❌ Failed CTest Cases")
        lines.append("| Test # | Test Case Name |")
        lines.append("|:---|:---|")
        for t in findings["ctest_fails"]:
            lines.append(f"| #{t['num']} | `{t['name']}` |")
        lines.append("")

    if findings["asan"]:
        lines.append("### 💥 AddressSanitizer (ASAN) Violations")
        for item in findings["asan"]:
            lines.append(f"- 🔴 `{item}`")
        lines.append("")

    if findings["ubsan"]:
        lines.append("### ⚡ UndefinedBehaviorSanitizer (UBSAN) Findings")
        lines.append("| Location | Description |")
        lines.append("|:---|:---|")
        for u in findings["ubsan"]:
            lines.append(f"| `{u['path']}:{u['line']}:{u['col']}` | `{u['desc']}` |")
        lines.append("")

    if findings["tsan"]:
        lines.append("### 🔄 ThreadSanitizer (TSAN) Data Race Warnings")
        for item in findings["tsan"]:
            lines.append(f"- ⚠️ `{item}`")
        lines.append("")

    if findings["arch_violations"]:
        lines.append("### 🏛️ Architecture Guard Redlines")
        for item in findings["arch_violations"]:
            lines.append(f"- {item}")
        lines.append("")

    return "\n".join(lines)

def main():
    parser = argparse.ArgumentParser(description="EdgeOS CI Log Analyzer (Fast-DDS style)")
    parser.add_argument("-i", "--input", help="Path to raw CI log file (default: stdin)")
    parser.add_argument("-o", "--output", help="Path to write markdown report (default: stdout)")
    parser.add_argument("-t", "--target", default="EdgeOS-CI", help="Build/Test Target Name")
    parser.add_argument("--strict", action="store_true", help="Exit with non-zero if issues found")
    args = parser.parse_args()

    if args.input:
        with open(args.input, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
    else:
        content = sys.stdin.read()

    findings = parse_log(content)
    report = generate_markdown_report(findings, args.target)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(report + "\n")
    else:
        print(report)

    total_issues = (len(findings["asan"]) + len(findings["tsan"]) + 
                    len(findings["ubsan"]) + len(findings["ctest_fails"]) + 
                    len(findings["arch_violations"]))

    if args.strict and total_issues > 0:
        sys.exit(1)

if __name__ == "__main__":
    main()
