#!/usr/bin/env python3
"""
Unified Code Review MCP Server & Industrial Analysis Engine for EdgeOS / EdgeC
依据 docs/modular_2.md 规范实现：
提供标准 MCP stdio 服务，暴露：
- `scan_ocr_rules`: Alibaba OCR 工业级规约模式匹配
- `match_company_rules`: .github/company_rules.json 业务红线规则
- `run_clang_tidy_check`: clang-tidy 增量行 AST 检查
- `run_cppcheck_analysis`: cppcheck 增量数据流分析
"""

import os
import sys
import re
import json
import asyncio
import subprocess
from typing import List, Dict, Set, Any, Optional

try:
    from mcp.server.mcpserver import MCPServer
    HAS_MCP = True
except ImportError:
    HAS_MCP = False


def get_git_diff_lines(base: str = "origin/main") -> Dict[str, Set[int]]:
    """获取当前分支相较于 base 分支的改动行号集合"""
    changed_lines: Dict[str, Set[int]] = {}
    candidate_bases = [base, "main", "HEAD~1", "HEAD~5"]
    diff_output = ""
    for candidate in candidate_bases:
        try:
            cmd = ["git", "diff", "--unified=0", candidate, "HEAD"]
            res = subprocess.run(cmd, capture_output=True, text=True, check=False)
            if res.returncode == 0 and res.stdout.strip():
                diff_output = res.stdout
                break
        except Exception:
            continue

    if not diff_output:
        try:
            cmd = ["git", "diff", "--unified=0", "HEAD"]
            res = subprocess.run(cmd, capture_output=True, text=True, check=False)
            diff_output = res.stdout
        except Exception:
            pass

    cur_file = None
    for line in diff_output.splitlines():
        if line.startswith("+++ b/"):
            cur_file = line[6:]
            if cur_file not in changed_lines:
                changed_lines[cur_file] = set()
        elif line.startswith("@@") and cur_file:
            try:
                plus_part = line.split("+")[1].split(" ")[0]
                parts = plus_part.split(",")
                start = int(parts[0])
                count = int(parts[1]) if len(parts) > 1 else 1
                for l in range(start, start + count):
                    changed_lines[cur_file].add(l)
            except Exception:
                pass

    return changed_lines


class AlibabaOCREngine:
    def __init__(self, root_dir: str = "."):
        self.root_dir = root_dir

    def scan_files(self, files: List[str], diff_lines: Dict[str, Set[int]]) -> List[Dict[str, Any]]:
        findings = []
        for f in files:
            if not f.endswith(('.c', '.h', '.cpp', '.hpp')) or not os.path.exists(f):
                continue
            
            lines_to_check = diff_lines.get(f, set())
            try:
                with open(f, 'r', encoding='utf-8', errors='ignore') as fp:
                    content_lines = fp.readlines()
            except Exception:
                continue

            for idx, line in enumerate(content_lines, 1):
                if lines_to_check and idx not in lines_to_check:
                    continue

                stripped = line.strip()
                if stripped.startswith("//") or stripped.startswith("/*") or stripped.startswith("*"):
                    continue

                if re.search(r'\b(strcpy|strcat|sprintf|vsprintf|gets)\s*\(', line):
                    match = re.search(r'\b(strcpy|strcat|sprintf|vsprintf|gets)\b', line).group(1)
                    findings.append({
                        "file": f,
                        "line": idx,
                        "rule_id": "OCR-MEM-02",
                        "priority": "P1",
                        "source": "Alibaba OCR",
                        "issue": f"检测到不安全的 C 字符串/缓冲区操作函数 `{match}`: `{stripped}`",
                        "code_snippet": stripped,
                        "fix_suggestion": "建议替换为带边界长度保护的 `snprintf`、`strncpy` 或 `strlcpy`，防止栈溢出与内存越界。"
                    })

                if re.search(r'while\s*\(\s*(volatile\s+)?(\*[a-zA-Z0-9_\->\(\)]*(REG|STATUS|FLAG|CTRL|reg|status|flag|ctrl|val)[a-zA-Z0-9_\->\(\)]*)\s*(&|==|!=)', line):
                    if "timeout" not in line.lower() and "retry" not in line.lower() and "count" not in line.lower():
                        findings.append({
                            "file": f,
                            "line": idx,
                            "rule_id": "RULE-SAFE-02",
                            "priority": "P0",
                            "source": "Alibaba OCR / Company Rules",
                            "issue": f"硬件寄存器标志位轮询缺少超时熔断保护: `{stripped}`",
                            "code_snippet": stripped,
                            "fix_suggestion": "必须加入 SysTick 或循环递减计数器超时保护。"
                        })

                is_isr_context = "_isr" in f.lower() or "irq" in f.lower() or "_isr(" in line.lower() or "irq_handler" in line.lower()
                if is_isr_context:
                    if re.search(r'\b(malloc|free|calloc|realloc|printf|nanosleep|sleep|pthread_mutex_lock|sem_wait)\s*\(', line):
                        match_fn = re.search(r'\b(malloc|free|calloc|realloc|printf|nanosleep|sleep|pthread_mutex_lock|sem_wait)\b', line).group(1)
                        findings.append({
                            "file": f,
                            "line": idx,
                            "rule_id": "RULE-SAFE-01",
                            "priority": "P0",
                            "source": "Alibaba OCR / Company Rules",
                            "issue": f"严禁在中断服务函数 (ISR) 中调用动态堆内存分配或阻塞接口 `{match_fn}`: `{stripped}`",
                            "code_snippet": stripped,
                            "fix_suggestion": "移除阻塞调用与动态堆分配，使用静态预分配 RingBuffer 或 FromISR 异步无锁唤醒通知。"
                        })

                if re.search(r'\b(val|voltage|current|temp|freq|power)\s*=\s*(220\.0|380\.0|50\.0|25\.0);', line) and "test" not in f.lower() and "mock" not in f.lower():
                    findings.append({
                        "file": f,
                        "line": idx,
                        "rule_id": "RULE-EDGEOS-03",
                        "priority": "P0",
                        "source": "EdgeOS Physical Truth",
                        "issue": f"物理真实性违规：检测到硬编码 Mock 常量占位符入库 `{stripped}`",
                        "code_snippet": stripped,
                        "fix_suggestion": "入库数据必须源自物理寄存器驱动、DataHub 影子总线或算法实际产出，严禁硬编码固定物理量。"
                    })

        return findings


ocr_engine = AlibabaOCREngine()


def scan_ocr_rules(files: List[str]) -> str:
    valid_files = [f for f in files if os.path.exists(f)]
    if not valid_files:
        return "OCR: No valid files to scan."
    try:
        res = subprocess.run(["ocr", "delegate", "rule"] + valid_files, capture_output=True, text=True, timeout=10)
        if res.returncode == 0 and res.stdout.strip():
            return res.stdout
    except Exception:
        pass
    diff_lines = get_git_diff_lines()
    findings = ocr_engine.scan_files(valid_files, diff_lines)
    return json.dumps(findings, indent=2, ensure_ascii=False) if findings else "OCR: PASS (No violations detected)."


def match_company_rules(repo_path: str = ".") -> str:
    rule_file = os.path.join(repo_path, ".github", "company_rules.json")
    if os.path.exists(rule_file):
        try:
            with open(rule_file, "r", encoding="utf-8") as f:
                return f.read()
        except Exception as e:
            return f"Error reading company rules: {str(e)}"
    return "No company rules found in .github/company_rules.json."


def run_clang_tidy_check(files: List[str], compile_commands_dir: str = "build") -> List[str]:
    diff_lines = get_git_diff_lines()
    cpp_files = [f for f in files if f.endswith(('.c', '.cpp', '.cc', '.h', '.hpp')) and os.path.exists(f)]
    findings: List[str] = []
    if not cpp_files:
        return ["clang-tidy: No C/C++ files to scan."]

    comp_db_paths = [compile_commands_dir, "build", ".", "EdgeC/build"]
    actual_comp_dir = None
    for p in comp_db_paths:
        if os.path.exists(os.path.join(p, "compile_commands.json")):
            actual_comp_dir = p
            break

    for f in cpp_files:
        matched_diff = None
        for df in diff_lines:
            if df == f or f.endswith(df) or df.endswith(f):
                matched_diff = diff_lines[df]
                break
        if matched_diff is not None and len(matched_diff) == 0:
            continue
        try:
            cmd = ["clang-tidy", f]
            if actual_comp_dir:
                cmd.append(f"-p={actual_comp_dir}")
            else:
                cmd.extend([
                    "--",
                    "-I.",
                    "-Iinclude",
                    "-Imodules/edge_core/include",
                    "-Imodules/edge_security/include",
                    "-Imodules/edge_stream/include",
                    "-Imodules/edge_io/include",
                    "-Imodules/edge_infra/include",
                    "-Imodules/edge_algo/include",
                    "-Itests",
                ])
            p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
            for line in p.stdout.splitlines():
                if ("warning:" in line or "error:" in line) and f in line:
                    if matched_diff is not None and len(matched_diff) > 0:
                        try:
                            lineno = int(line.split(":")[1])
                            if lineno in matched_diff:
                                findings.append(line.strip())
                        except (ValueError, IndexError):
                            pass
                    elif matched_diff is None:
                        findings.append(line.strip())
        except FileNotFoundError:
            return ["clang-tidy not installed in system PATH."]
        except Exception as e:
            findings.append(f"clang-tidy error on {f}: {str(e)}")

    return findings if findings else ["clang-tidy: PASS"]


def run_cppcheck_analysis(files: List[str]) -> List[str]:
    diff_lines = get_git_diff_lines()
    cpp_files = [f for f in files if f.endswith(('.c', '.cpp', '.cc', '.h', '.hpp')) and os.path.exists(f)]
    if not cpp_files:
        return ["Cppcheck: No C/C++ files to scan."]
    cmd = ["cppcheck", "--enable=warning,portability", "--template={file}:{line}: [{severity}] {message}"] + cpp_files
    try:
        p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
        lines: List[str] = []
        combined_output = (p.stderr + "\n" + p.stdout).splitlines()
        for line in combined_output:
            if not line.strip():
                continue
            matched = False
            for f, l_set in diff_lines.items():
                if f in line:
                    matched = True
                    if l_set:
                        try:
                            lineno = int(line.split(":")[1])
                            if lineno in l_set:
                                lines.append(line.strip())
                        except (ValueError, IndexError):
                            pass
            if not matched and not diff_lines and ("[" in line and "]" in line):
                lines.append(line.strip())
        return lines if lines else ["Cppcheck: PASS"]
    except FileNotFoundError:
        return ["cppcheck not installed in system PATH."]
    except Exception as e:
        return [f"Cppcheck error: {str(e)}"]


def check_isr_safety(files: List[str]) -> List[Dict[str, Any]]:
    """专项检查：ISR 中断上下文中的阻塞操作与动态堆分配 (P0)"""
    diff_lines = get_git_diff_lines()
    all_findings = ocr_engine.scan_files([f for f in files if os.path.exists(f)], diff_lines)
    return [f for f in all_findings if f.get("rule_id") == "RULE-SAFE-01"]


def check_hardware_polling_timeout(files: List[str]) -> List[Dict[str, Any]]:
    """专项检查：硬件寄存器/标志位轮询超时熔断机制 (P0)"""
    diff_lines = get_git_diff_lines()
    all_findings = ocr_engine.scan_files([f for f in files if os.path.exists(f)], diff_lines)
    return [f for f in all_findings if f.get("rule_id") == "RULE-SAFE-02"]


def check_physical_truth_violations(files: List[str]) -> List[Dict[str, Any]]:
    """专项检查：物理真实性违规（硬编码 Mock 常量占位符入库）(P0)"""
    diff_lines = get_git_diff_lines()
    all_findings = ocr_engine.scan_files([f for f in files if os.path.exists(f)], diff_lines)
    return [f for f in all_findings if f.get("rule_id") == "RULE-EDGEOS-03"]


def check_buffer_security(files: List[str]) -> List[Dict[str, Any]]:
    """专项检查：不安全 C 字符串与缓冲区操作 (strcpy, sprintf 等) (P1)"""
    diff_lines = get_git_diff_lines()
    all_findings = ocr_engine.scan_files([f for f in files if os.path.exists(f)], diff_lines)
    return [f for f in all_findings if f.get("rule_id") == "OCR-MEM-02"]


def get_git_diff_context(base: str = "origin/main") -> Dict[str, List[int]]:
    """获取当前分支相较于基准分支的改动文件及增量改动行号列表"""
    raw = get_git_diff_lines(base)
    return {k: sorted(list(v)) for k, v in raw.items()}


if HAS_MCP:
    server = MCPServer("UnifiedReviewTools")

    @server.tool(name="scan_ocr_rules", description="【Alibaba OCR】调用阿里巴巴 Open Code Review 规则引擎扫描工业全量规约")
    def tool_scan_ocr(files: List[str]) -> str:
        return scan_ocr_rules(files)

    @server.tool(name="match_company_rules", description="【企业经验规约】加载 .github/company_rules.json 业务设计红线")
    def tool_match_rules(repo_path: str = ".") -> str:
        return match_company_rules(repo_path)

    @server.tool(name="check_isr_safety", description="【ISR 专项检查】检测中断上下文中的堆分配、休眠与互斥锁违规 (P0 阻断)")
    def tool_check_isr(files: List[str]) -> str:
        return json.dumps(check_isr_safety(files), indent=2, ensure_ascii=False)

    @server.tool(name="check_hardware_polling_timeout", description="【硬件轮询专项】检测寄存器状态位轮询是否具备超时熔断保护 (P0 阻断)")
    def tool_check_poll(files: List[str]) -> str:
        return json.dumps(check_hardware_polling_timeout(files), indent=2, ensure_ascii=False)

    @server.tool(name="check_physical_truth_violations", description="【物理真实性专项】检测入库数据是否存在硬编码 Mock 常量占位符 (P0 阻断)")
    def tool_check_mock(files: List[str]) -> str:
        return json.dumps(check_physical_truth_violations(files), indent=2, ensure_ascii=False)

    @server.tool(name="check_buffer_security", description="【缓冲区安全专项】检测 strcpy / sprintf / gets 等不安全字符串操作 (P1 严重)")
    def tool_check_buf(files: List[str]) -> str:
        return json.dumps(check_buffer_security(files), indent=2, ensure_ascii=False)

    @server.tool(name="run_clang_tidy_check", description="【clang-tidy AST 检查】执行增量行级编译器 AST 语法语义检查")
    def tool_clang_tidy(files: List[str]) -> str:
        return json.dumps(run_clang_tidy_check(files), indent=2, ensure_ascii=False)

    @server.tool(name="run_cppcheck_analysis", description="【Cppcheck 静态数据流分析】检查空指针解引用与内存越界")
    def tool_cppcheck(files: List[str]) -> str:
        return json.dumps(run_cppcheck_analysis(files), indent=2, ensure_ascii=False)

    @server.tool(name="get_git_diff_context", description="【Git 增量上下文】获取当前 PR/分支变更文件的精确改动行号列表")
    def tool_git_diff(base: str = "origin/main") -> str:
        return json.dumps(get_git_diff_context(base), indent=2, ensure_ascii=False)


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "serve":
        if HAS_MCP:
            asyncio.run(server.run_stdio_async())
        else:
            print("MCPServer not installed", file=sys.stderr)
    else:
        print("Unified Review Tools MCP Standalone Mode.")


if __name__ == "__main__":
    main()
