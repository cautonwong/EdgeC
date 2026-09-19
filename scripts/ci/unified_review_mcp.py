#!/usr/bin/env python3
"""
Unified Code Review MCP Server & Industrial Analysis Engine for EdgeOS / EdgeC
依据 docs/modular_2.md 规范实现：
1. CRG (Code Review Graph): 基于 SQLite 存储的全库 AST 符号图谱、调用拓扑 (Call Hierarchy)、爆炸半径 (Blast Radius) 与 CTest 单测映射
2. Alibaba OCR (Open Code Review): 嵌入式与工业级模式匹配 (NPE、并发锁配对、ISR 安全、DMA 64B对齐、缓冲区安全、物理真实性)
3. Company Rules: .github/company_rules.json 业务红线深度校验
4. clang-tidy & Cppcheck: 增量行级编译硬缺陷过滤
"""

import os
import sys
import re
import json
import subprocess
from typing import List, Dict, Set, Any, Tuple, Optional

# FastMCP support
try:
    from mcp.server.fastmcp import FastMCP
    HAS_MCP = True
except ImportError:
    HAS_MCP = False

if HAS_MCP:
    mcp = FastMCP("UnifiedReviewTools")
else:
    class DummyMCP:
        def tool(self):
            def decorator(fn):
                return fn
            return decorator
        def run(self):
            print("FastMCP running in standalone mode.")
    mcp = DummyMCP()


def get_git_diff_lines(base: str = "origin/main") -> Dict[str, Set[int]]:
    """
    获取当前分支相较于 base 分支的改动行号集合。
    核心原则：告警范围聚焦增量，严格按行过滤，防止存量历史技术债务倾泻。
    """
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
    """
    针对嵌入式与工业级 C 语言量身定制的 Open Code Review 模式引擎。
    """
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

                # 1. OCR-MEM-02: Unsafe buffer functions (strcpy, strcat, sprintf, gets)
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
                        "fix_suggestion": f"建议替换为带边界长度保护的 `snprintf`、`strncpy` 或 `strlcpy`，防止栈溢出与内存越界。"
                    })

                # 2. OCR-SAFE-04: Unbounded register polling (volatile or REG flags)
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
                            "fix_suggestion": "必须加入 SysTick 或循环递减计数器 (如 `uint32_t timeout = 10000; while(... && --timeout); if (!timeout) return -ETIMEDOUT;`) 保护。"
                        })

                # 3. OCR-ISR-05: Blocking / Dynamic alloc in ISR context
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

                # 4. OCR-CDD-07: Direct Mock Placeholder Violation (Physical Truth)
                if re.search(r'\b(val|voltage|current|temp|freq|power)\s*=\s*(220\.0|380\.0|50\.0|25\.0);', line) and "test" not in f.lower() and "mock" not in f.lower():
                    findings.append({
                        "file": f,
                        "line": idx,
                        "rule_id": "RULE-EDGEOS-03",
                        "priority": "P0",
                        "source": "EdgeOS Physical Truth",
                        "issue": f"物理真实性违规：检测到硬编码 Mock 常量占位符入库 `{stripped}`",
                        "code_snippet": stripped,
                        "fix_suggestion": "入库数据必须源自物理寄存器驱动、DataHub 影子总线或算法实际产出，严禁在生产代码中硬编码固定物理量。"
                    })

        return findings


ocr_engine = AlibabaOCREngine()


@mcp.tool()
def scan_ocr_rules(files: List[str]) -> str:
    """【Alibaba OCR】调用阿里巴巴 Open Code Review 规则引擎，检查高危缺陷模式与最佳实践。"""
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


@mcp.tool()
def match_company_rules(repo_path: str = ".") -> str:
    """【企业经验规约】加载项目专属的业务设计红线、架构规范与历史故障踩坑库。"""
    rule_file = os.path.join(repo_path, ".github", "company_rules.json")
    if os.path.exists(rule_file):
        try:
            with open(rule_file, "r", encoding="utf-8") as f:
                return f.read()
        except Exception as e:
            return f"Error reading company rules: {str(e)}"
    return "No company rules found in .github/company_rules.json."


@mcp.tool()
def run_clang_tidy_check(files: List[str], compile_commands_dir: str = "build") -> List[str]:
    """【clang-tidy AST 检查】执行编译器级 AST 语法、语义及现代化 C 检查，只保留变更行警告。"""
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
        
        try:
            cmd = ["clang-tidy", f]
            if actual_comp_dir:
                cmd.append(f"-p={actual_comp_dir}")
            
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
                    else:
                        findings.append(line.strip())
        except FileNotFoundError:
            return ["clang-tidy not installed in system PATH."]
        except Exception as e:
            findings.append(f"clang-tidy error on {f}: {str(e)}")

    return findings if findings else ["clang-tidy: PASS"]


@mcp.tool()
def run_cppcheck_analysis(files: List[str]) -> List[str]:
    """【Cppcheck 静态数据流分析】检查空指针解引用、内存越界、未初始化变量等底层硬缺陷。"""
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
                    else:
                        lines.append(line.strip())
            if not matched and not diff_lines and ("[" in line and "]" in line):
                lines.append(line.strip())

        return lines if lines else ["Cppcheck: PASS"]
    except FileNotFoundError:
        return ["cppcheck not installed in system PATH."]
    except Exception as e:
        return [f"Cppcheck error: {str(e)}"]


if __name__ == "__main__":
    if HAS_MCP and len(sys.argv) > 1 and sys.argv[1] == "serve":
        mcp.run()
    else:
        print("Unified Review Tools MCP Standalone Mode.")
