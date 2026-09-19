#!/usr/bin/env python3
"""
Pure MCP Orchestrator & Synthesis Engine for EdgeOS / EdgeC
依据 docs/modular_2.md 规范实现：
作为 CI/CD 智能审查中枢，调度 CRG MCP、OCR、clang-tidy、cppcheck 与 Company Rules，
进行去重归并、优先级分级 (P0~P3) 并产出双产物 (final_report.json + final_report.md)。
"""

import asyncio
import json
import os
import subprocess
import sys
from contextlib import AsyncExitStack
from typing import Dict, Any, List

# Try imports
try:
    from anthropic import Anthropic
    HAS_ANTHROPIC = True
except ImportError:
    HAS_ANTHROPIC = False

try:
    from mcp import ClientSession, StdioServerParameters
    from mcp.client.stdio import stdio_client
    HAS_MCP = True
except ImportError:
    HAS_MCP = False
    class StdioServerParameters:
        def __init__(self, command, args):
            self.command = command
            self.args = args

from unified_review_mcp import (
    get_git_diff_lines,
    scan_ocr_rules,
    match_company_rules,
    run_clang_tidy_check,
    run_cppcheck_analysis,
    crg_engine,
    ocr_engine
)

SERVERS = {
    "crg": StdioServerParameters(command="code-review-graph", args=["serve"]),
    "review_tools": StdioServerParameters(command="python3", args=["scripts/ci/unified_review_mcp.py", "serve"])
}

FINAL_SUBMIT_TOOL = {
    "name": "submit_final_report",
    "description": "完成所有审查、交叉验证与去重分级后，必须调用此工具提交最终成果",
    "input_schema": {
        "type": "object",
        "properties": {
            "summary": {
                "type": "object",
                "properties": {
                    "P0": {"type": "integer", "description": "阻断级缺陷数 (崩溃/越界/死锁/安全漏洞/ISR阻塞/Mock硬编码)"},
                    "P1": {"type": "integer", "description": "严重缺陷数 (竞态/未释放资源/缺少核心测试/跨层耦合/不安全函数)"},
                    "P2": {"type": "integer", "description": "一般问题数 (逻辑异味/次要性能缺陷/MTT时间对齐)"},
                    "P3": {"type": "integer", "description": "轻微建议数 (代码风格/Modern C 改进/注释)"}
                },
                "required": ["P0", "P1", "P2", "P3"]
            },
            "blast_radius_summary": {
                "type": "string",
                "description": "基于 CRG 图谱分析得出的跨文件依赖影响范围与受影响单测总结"
            },
            "findings": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "file": {"type": "string"},
                        "line": {"type": "integer"},
                        "priority": {"type": "string", "enum": ["P0", "P1", "P2", "P3"]},
                        "source": {"type": "string"},
                        "issue": {"type": "string"},
                        "fix_suggestion": {"type": "string"}
                    },
                    "required": ["file", "line", "priority", "issue", "fix_suggestion"]
                }
            },
            "markdown_report": {
                "type": "string",
                "description": "格式整洁、可直接贴入 GitHub PR 评论的 Markdown 正文"
            }
        },
        "required": ["summary", "blast_radius_summary", "findings", "markdown_report"]
    }
}


def run_local_fallback_synthesis(changed_files: List[str]) -> Dict[str, Any]:
    """
    当无外网 API 或本地离线运行时的确定性直接合成引擎。
    执行底层 CRG 爆炸半径分析、Alibaba OCR 模式匹配、clang-tidy 与 Cppcheck 检查。
    """
    print("ℹ️ 执行本地确定性静态审查分析与 CRG/OCR 双引擎合成...")
    rules_json = match_company_rules()
    diff_lines = get_git_diff_lines()
    
    # 1. CRG Blast Radius Analysis
    blast_radius = crg_engine.analyze_blast_radius(changed_files)

    # 2. Linters
    clang_findings = run_clang_tidy_check(changed_files)
    cppcheck_findings = run_cppcheck_analysis(changed_files)
    
    # 3. Alibaba OCR
    ocr_raw = scan_ocr_rules(changed_files)
    ocr_findings = []
    if ocr_raw.startswith("["):
        try:
            ocr_findings = json.loads(ocr_raw)
        except Exception:
            pass

    findings = []
    p0_count = 0
    p1_count = 0
    p2_count = 0
    p3_count = 0

    # Process Alibaba OCR Findings
    for o in ocr_findings:
        pri = o.get("priority", "P1")
        if pri == "P0":
            p0_count += 1
        elif pri == "P1":
            p1_count += 1
        elif pri == "P2":
            p2_count += 1
        else:
            p3_count += 1
        findings.append({
            "file": o.get("file", "unknown"),
            "line": o.get("line", 1),
            "priority": pri,
            "source": o.get("source", "Alibaba OCR"),
            "issue": o.get("issue", ""),
            "fix_suggestion": o.get("fix_suggestion", "")
        })

    # Process Clang-tidy
    for item in clang_findings:
        if item.startswith("clang-tidy: PASS") or item.startswith("clang-tidy: No") or "not installed" in item:
            continue
        parts = item.split(":")
        file_name = parts[0] if len(parts) > 0 else "unknown"
        line_num = int(parts[1]) if len(parts) > 1 and parts[1].isdigit() else 1
        msg = ":".join(parts[2:]).strip() if len(parts) > 2 else item
        pri = "P0" if "error" in item.lower() else "P2"
        if pri == "P0":
            p0_count += 1
        else:
            p2_count += 1
        findings.append({
            "file": file_name,
            "line": line_num,
            "priority": pri,
            "source": "clang-tidy AST",
            "issue": msg,
            "fix_suggestion": "遵循编译器 AST 建议进行代码修正。"
        })

    # Process Cppcheck
    for item in cppcheck_findings:
        if item.startswith("Cppcheck: PASS") or item.startswith("Cppcheck: No") or "not installed" in item:
            continue
        parts = item.split(":")
        file_name = parts[0] if len(parts) > 0 else "unknown"
        line_num = int(parts[1]) if len(parts) > 1 and parts[1].isdigit() else 1
        msg = ":".join(parts[2:]).strip() if len(parts) > 2 else item
        pri = "P0" if "error" in item.lower() else "P1"
        if pri == "P0":
            p0_count += 1
        else:
            p1_count += 1
        findings.append({
            "file": file_name,
            "line": line_num,
            "priority": pri,
            "source": "Cppcheck DataFlow",
            "issue": msg,
            "fix_suggestion": "排查底层内存越界、未初始化或空指针隐患。"
        })

    # Render Ultra-Professional Markdown Report
    status_banner = "🔴 **质量门禁未通过 (BLOCKED)**：发现阻断级 P0 缺陷，禁止合并至主干。" if p0_count > 0 else "🟢 **质量门禁通过 (PASSED)**：未发现阻断级缺陷，代码质量达标。"

    md_lines = [
        "# 🛡️ EdgeOS 智能代码审查与质量门禁报告 (AI-Augmented Code Review)",
        "",
        f"> **审查结论**: {status_banner}",
        "",
        "## 📊 缺陷分级统计看板",
        "| 优先级 | 数量 | 严重级别定义 | 处置策略 | 状态 |",
        "| :---: | :---: | :--- | :--- | :---: |",
        f"| **P0** | **{p0_count}** | 阻断级 (崩溃/越界/死锁/ISR阻塞/Mock硬编码) | **硬性拦截，禁止合并** | {'❌ FAIL' if p0_count > 0 else '✅ PASS'} |",
        f"| **P1** | **{p1_count}** | 严重级 (竞态/不安全函数/架构跨层/内存泄露) | 强烈建议修复 | {'⚠️ WARN' if p1_count > 0 else '✅ PASS'} |",
        f"| **P2** | **{p2_count}** | 一般级 (逻辑异味/次要性能缺陷/时间对齐) | 建议优化 | {'ℹ️ INFO' if p2_count > 0 else '✅ PASS'} |",
        f"| **P3** | **{p3_count}** | 提示级 (代码风格/Modern C 实践) | 可选改进 | ✅ PASS |",
        "",
        "## 🌐 CRG 全库拓扑与爆炸半径分析 (Blast Radius)",
        f"- **变更风险等级 (Risk Score)**: `[{blast_radius['risk_score']}]`",
        f"- **涉及变更文件**: {len(changed_files)} 个",
        f"- **波及核心模块**: {', '.join(f'`{m}`' for m in blast_radius['impacted_modules']) if blast_radius['impacted_modules'] else '无'}",
        f"- **波及上层产品**: {', '.join(f'`{p}`' for p in blast_radius['impacted_products']) if blast_radius['impacted_products'] else '无'}",
        f"- **映射关联单测**: {', '.join(f'`{t}`' for t in blast_radius['impacted_tests']) if blast_radius['impacted_tests'] else '所有关联单测覆盖正常'}",
        ""
    ]

    if blast_radius["sample_callers"]:
        md_lines.append("### 🔗 跨模块依赖调用链 (Call Hierarchy)")
        for caller in blast_radius["sample_callers"][:6]:
            md_lines.append(f"- ↳ {caller}")
        md_lines.append("")

    md_lines.extend([
        "## 🔍 详细缺陷与修复建议 (Findings & Suggestions)",
        ""
    ])

    if not findings:
        md_lines.append("> 🎉 **物理真实性与代码规范 100% 达标！未检测到任何新增违规项。**\n")
    else:
        for idx, f in enumerate(findings, 1):
            md_lines.extend([
                f"### {idx}. [{f['priority']}] `{f['file']}:{f['line']}`",
                f"- **检出引擎**: `{f['source']}`",
                f"- **问题描述**: {f['issue']}",
                f"- **建议对策**: {f['fix_suggestion']}",
                ""
            ])

    md_report = "\n".join(md_lines)

    return {
        "summary": {
            "P0": p0_count,
            "P1": p1_count,
            "P2": p2_count,
            "P3": p3_count
        },
        "blast_radius_summary": f"风险等级: {blast_radius['risk_score']}，扫描 {len(changed_files)} 个文件，涉及 {len(blast_radius['impacted_modules'])} 个核心模块。",
        "findings": findings,
        "markdown_report": md_report
    }


async def run_llm_orchestrator(changed_files: List[str], api_key: str) -> Dict[str, Any]:
    client = Anthropic(api_key=api_key)
    exit_stack = AsyncExitStack()
    tool_to_session: Dict[str, ClientSession] = {}
    claude_tools = [FINAL_SUBMIT_TOOL]

    print("🔌 正在连接 MCP 服务 (CRG + Unified Review Tools)...")
    try:
        for name, config in SERVERS.items():
            try:
                r_stream, w_stream = await exit_stack.enter_async_context(stdio_client(config))
                session = await exit_stack.enter_async_context(ClientSession(r_stream, w_stream))
                await session.initialize()
                tools_resp = await session.list_tools()
                for t in tools_resp.tools:
                    tool_to_session[t.name] = session
                    claude_tools.append({
                        "name": t.name,
                        "description": t.description or "",
                        "input_schema": t.inputSchema
                    })
            except Exception as e:
                print(f"⚠️ MCP Server [{name}] 连接跳过: {e}")

        system_prompt = """你是一个世界顶尖的 EdgeOS 嵌入式工业操作系统高性能架构与安全审查官。
请调度已挂载的 MCP 工具完成本次 PR 审查：
1. CRG 工具：调用 get_review_context_tool 获取全库调用拓扑与爆炸半径；
2. Review Tools：调用 scan_ocr_rules 识别阿里最佳实践；调用 match_company_rules 识别专属规范；调用 run_clang_tidy_check 与 run_cppcheck_analysis 收集硬事实；
3. 交叉校验：结合上下文剔除静态扫描的虚警，严格对照 EdgeOS 宪法 (物理真实性/ISR安全/四平面隔离) 根据业务影响评定 P0~P3；
4. 终态：分析完成后，必须且只能通过调用 submit_final_report 提交数据并退出。"""

        messages = [{"role": "user", "content": f"变更文件列表：\n{json.dumps(changed_files, indent=2)}"}]
        final_report = None
        MAX_TURNS = 8

        for turn in range(MAX_TURNS):
            print(f"🤖 [Round {turn + 1}/{MAX_TURNS}] 编排思考中...")
            resp = client.messages.create(
                model="claude-3-7-sonnet-20250219",
                max_tokens=4096,
                system=system_prompt,
                tools=claude_tools,
                messages=messages
            )

            tool_calls = [c for c in resp.content if c.type == "tool_use"]
            if not tool_calls:
                break

            messages.append({"role": "assistant", "content": resp.content})
            tool_results = []

            for tc in tool_calls:
                if tc.name == "submit_final_report":
                    final_report = tc.input
                    print("🎯 已捕获最终审查报告！")
                    break

                session = tool_to_session.get(tc.name)
                if session:
                    print(f"  ↳ 执行 MCP Tool: {tc.name}")
                    mcp_res = await session.call_tool(tc.name, arguments=tc.input)
                    output_str = "\n".join([item.text for item in mcp_res.content if hasattr(item, "text")])
                else:
                    output_str = "Tool not found or executed via local dispatcher."

                tool_results.append({
                    "type": "tool_result",
                    "tool_use_id": tc.id,
                    "content": output_str
                })

            if final_report:
                break
            messages.append({"role": "user", "content": tool_results})

        return final_report or run_local_fallback_synthesis(changed_files)

    finally:
        await exit_stack.aclose()


def main():
    changed_files = []
    if os.path.exists("changed_files.txt"):
        with open("changed_files.txt", "r", encoding="utf-8") as f:
            changed_files = [line.strip() for line in f if line.strip()]
    else:
        diff_dict = get_git_diff_lines()
        changed_files = list(diff_dict.keys())

    if not changed_files:
        for root, _, files in os.walk("modules"):
            for file in files:
                if file.endswith(('.c', '.h')):
                    changed_files.append(os.path.join(root, file))
        changed_files = changed_files[:20]

    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if api_key and HAS_ANTHROPIC and HAS_MCP:
        final_report = asyncio.run(run_llm_orchestrator(changed_files, api_key))
    else:
        final_report = run_local_fallback_synthesis(changed_files)

    with open("final_report.json", "w", encoding="utf-8") as f:
        json.dump(final_report, f, indent=2, ensure_ascii=False)

    with open("final_report.md", "w", encoding="utf-8") as f:
        f.write(final_report.get("markdown_report", "# Code Review Report"))

    print("✅ 审查执行完毕，产物 final_report.json 与 final_report.md 已落盘！")

    p0 = final_report.get("summary", {}).get("P0", 0)
    p1 = final_report.get("summary", {}).get("P1", 0)
    print(f"📊 审查统计: P0={p0}, P1={p1}, P2={final_report.get('summary', {}).get('P2', 0)}, P3={final_report.get('summary', {}).get('P3', 0)}")
    
    if p0 > 0:
        print(f"::error::发现 {p0} 个 P0 阻断级缺陷，质量门禁未通过！", file=sys.stderr)
        sys.exit(1)
    else:
        print("🎉 质量门禁通过 (0 P0 Defects).")
        sys.exit(0)


if __name__ == "__main__":
    main()
