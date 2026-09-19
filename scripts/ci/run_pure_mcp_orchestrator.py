#!/usr/bin/env python3
"""
Pure MCP Orchestrator & Real AI Model Synthesis Engine for EdgeOS / EdgeC
依据 docs/modular_2.md 规范实现：
1. 真正通过 FastMCP stdio ClientSession 连接 CRG MCP 与 Review Tools MCP 子进程；
2. 动态 list_tools() 发现工具，并将其注入 Google Gemini (或 Claude / OpenAI) 进行多轮 Function Calling；
3. 将大模型的工具调用真正转发至 live MCP session 执行；
4. 捕获 submit_final_report 结构化终审成果，落盘 final_report.json 与 final_report.md。
"""

import os
import sys
import json
import asyncio
import urllib.request
import urllib.error
from contextlib import AsyncExitStack
from typing import Dict, Any, List, Optional

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

# MCP Server Configurations as specified in modular_2.md
SERVERS = {
    "crg": StdioServerParameters(command="python3", args=["scripts/ci/code_review_graph.py", "serve"]),
    "review_tools": StdioServerParameters(command="python3", args=["scripts/ci/unified_review_mcp.py", "serve"])
}

FINAL_SUBMIT_TOOL = {
    "name": "submit_final_report",
    "description": "完成所有审查、交叉验证与去重分级后，必须且只能调用此工具提交最终审查成果并结束评审",
    "parameters": {
        "type": "OBJECT",
        "properties": {
            "summary": {
                "type": "OBJECT",
                "properties": {
                    "P0": {"type": "INTEGER", "description": "阻断级缺陷数 (崩溃/越界/死锁/安全漏洞/ISR阻塞/Mock硬编码)"},
                    "P1": {"type": "INTEGER", "description": "严重缺陷数 (竞态/未释放资源/缺少核心测试/跨层耦合/不安全函数)"},
                    "P2": {"type": "INTEGER", "description": "一般问题数 (逻辑异味/次要性能缺陷/MTT时间对齐)"},
                    "P3": {"type": "INTEGER", "description": "轻微建议数 (代码风格/Modern C 改进/注释)"}
                },
                "required": ["P0", "P1", "P2", "P3"]
            },
            "blast_radius_summary": {
                "type": "STRING",
                "description": "基于 CRG 图谱分析得出的跨文件依赖影响范围与受影响单测总结"
            },
            "findings": {
                "type": "ARRAY",
                "items": {
                    "type": "OBJECT",
                    "properties": {
                        "file": {"type": "STRING"},
                        "line": {"type": "INTEGER"},
                        "priority": {"type": "STRING"},
                        "source": {"type": "STRING"},
                        "issue": {"type": "STRING"},
                        "fix_suggestion": {"type": "STRING"}
                    },
                    "required": ["file", "line", "priority", "issue", "fix_suggestion"]
                }
            },
            "markdown_report": {
                "type": "STRING",
                "description": "格式整洁、包含统计看板/爆炸半径/缺陷明细的 Markdown 正文"
            }
        },
        "required": ["summary", "blast_radius_summary", "findings", "markdown_report"]
    }
}


async def run_mcp_orchestration(changed_files: List[str]) -> Dict[str, Any]:
    api_key = os.environ.get("GEMINI_API_KEY") or os.environ.get("GOOGLE_API_KEY")
    if not api_key:
        api_key = "AQ.Ab8RN6JjQQbeAe19odItD0pkm3TikbMOLJxwJ8CtgKq53j26YQ"

    primary_model = os.environ.get("GEMINI_MODEL", "gemini-3.8-flash")
    candidate_models = [primary_model]
    for m in ["gemini-2.5-flash-lite", "gemini-2.5-flash"]:
        if m not in candidate_models:
            candidate_models.append(m)

    active_model = candidate_models[0]

    exit_stack = AsyncExitStack()
    tool_to_session: Dict[str, ClientSession] = {}
    gemini_tool_declarations = []

    print(f"🔌 [MCP Orchestrator] 正在连接 CRG 与 Review Tools MCP stdio 服务...")
    try:
        for name, config in SERVERS.items():
            r_stream, w_stream = await exit_stack.enter_async_context(stdio_client(config))
            session = await exit_stack.enter_async_context(ClientSession(r_stream, w_stream))
            await session.initialize()
            
            tools_resp = await session.list_tools()
            for t in tools_resp.tools:
                tool_to_session[t.name] = session
                
                # Convert tool schema to Gemini format
                props = {}
                required_fields = []
                schema = getattr(t, "input_schema", None) or getattr(t, "inputSchema", None)
                if schema:
                    for k, v in schema.get("properties", {}).items():
                        p_type = v.get("type", "STRING").upper()
                        if p_type == "ARRAY":
                            props[k] = {"type": "ARRAY", "items": {"type": "STRING"}}
                        elif p_type == "INTEGER":
                            props[k] = {"type": "INTEGER"}
                        elif p_type == "BOOLEAN":
                            props[k] = {"type": "BOOLEAN"}
                        else:
                            props[k] = {"type": "STRING"}
                    required_fields = schema.get("required", [])

                gemini_tool_declarations.append({
                    "name": t.name,
                    "description": t.description or t.name,
                    "parameters": {
                        "type": "OBJECT",
                        "properties": props,
                        "required": required_fields
                    }
                })
                print(f"  ↳ 发现并注册 MCP Tool: `{t.name}` (来源: {name})")

        # Register FINAL_SUBMIT_TOOL
        gemini_tool_declarations.append(FINAL_SUBMIT_TOOL)

        system_instruction = {
            "role": "system",
            "parts": [{
                "text": """你是一个世界顶尖的 EdgeOS 嵌入式工业操作系统架构与安全代码审查官。
请根据需要调度已挂载的全部 MCP 工具（CRG 拓扑/爆炸半径/依赖/单测、Alibaba OCR 工业规约、ISR 安全专项、硬件超时专项、物理真实性专项、缓冲区安全专项、企业经验红线、clang-tidy 与 Cppcheck）完成本次 PR 审查：
1. 拓扑与影响面：可调用 get_review_context_tool / get_symbol_callers / get_affected_tests / get_blast_radius_summary；
2. 工业规约与安全：可调用 scan_ocr_rules / check_isr_safety / check_hardware_polling_timeout / check_physical_truth_violations / check_buffer_security；
3. 企业规则与静态分析：调用 match_company_rules / run_clang_tidy_check / run_cppcheck_analysis；
4. 综合所有证据交叉验证剔除误报，严格按 EdgeOS 宪法进行 P0~P3 评级；
5. 最后必须且只能调用 submit_final_report 提交完整的结构化终审成果。"""
            }]
        }

        contents = [{
            "role": "user",
            "parts": [{
                "text": f"请对本次 PR 变更进行全面代码审查。变更文件列表如下：\n{json.dumps(changed_files, indent=2)}"
            }]
        }]

        MAX_ROUNDS = 8
        final_report = None

        for turn in range(MAX_ROUNDS):
            print(f"🧠 [Round {turn + 1}/{MAX_ROUNDS}] 发送会话至 Google Gemini ({active_model}) 思考中...")
            payload = {
                "system_instruction": system_instruction,
                "contents": contents,
                "tools": [{"function_declarations": gemini_tool_declarations}],
                "generationConfig": {
                    "temperature": 0.2,
                    "maxOutputTokens": 4096
                }
            }

            resp_data = None
            models_to_try = [active_model] + [m for m in candidate_models if m != active_model]
            for model_attempt in models_to_try:
                url = f"https://generativelanguage.googleapis.com/v1beta/models/{model_attempt}:generateContent?key={api_key}"
                req = urllib.request.Request(
                    url,
                    data=json.dumps(payload).encode('utf-8'),
                    headers={"Content-Type": "application/json"}
                )
                try:
                    with urllib.request.urlopen(req, timeout=45) as resp:
                        resp_data = json.loads(resp.read().decode('utf-8'))
                        active_model = model_attempt
                        break
                except Exception as e:
                    print(f"⚠️ Gemini 模型 [{model_attempt}] 响应异常: {e}")
                    continue

            if not resp_data:
                print("❌ 所有候选 Gemini 模型均调用失败")
                break

            candidates = resp_data.get("candidates", [])
            if not candidates:
                print("⚠️ Gemini 未返回有效 candidate")
                break

            candidate_content = candidates[0].get("content", {})
            parts = candidate_content.get("parts", [])
            contents.append(candidate_content)

            has_func_call = False
            response_parts = []

            for part in parts:
                if "functionCall" in part:
                    has_func_call = True
                    fc = part["functionCall"]
                    fn_name = fc["name"]
                    fn_args = fc.get("args", {})
                    print(f"  ↳ 🤖 Gemini 发起 MCP 工具调用: `{fn_name}` -> {json.dumps(fn_args, ensure_ascii=False)[:80]}")

                    if fn_name == "submit_final_report":
                        final_report = fn_args
                        print("🎯 成功捕获 Gemini 产出的最终结构化审查报告！")
                        return final_report

                    session = tool_to_session.get(fn_name)
                    if session:
                        mcp_res = await session.call_tool(fn_name, arguments=fn_args)
                        output_content = [item.text for item in mcp_res.content if hasattr(item, "text")]
                        output_str = "\n".join(output_content)
                        if len(output_str) > 3000:
                            output_str = output_str[:3000] + "... [truncated for context efficiency]"
                        try:
                            output_obj = json.loads(output_str)
                        except Exception:
                            output_obj = {"result": output_str}
                    else:
                        output_obj = {"error": f"Tool {fn_name} not found"}

                    response_parts.append({
                        "functionResponse": {
                            "name": fn_name,
                            "response": {"output": output_obj}
                        }
                    })

            if has_func_call and response_parts:
                if turn >= 2:
                    response_parts.append({
                        "text": "各维度 MCP 工具事实已收集完毕，请调用 submit_final_report 提交最终结构化审查报告。"
                    })
                contents.append({
                    "role": "user",
                    "parts": response_parts
                })
            else:
                text_out = "".join([p.get("text", "") for p in parts if "text" in p])
                print(f"ℹ️ Gemini 模型输出文本: {text_out[:120]}...")
                contents.append({
                    "role": "user",
                    "parts": [{"text": "请基于以上工具分析结果，调用 submit_final_report 函数提交最终结构化报告。"}]
                })

        return final_report or {}

    finally:
        await exit_stack.aclose()


def main():
    changed_files = []
    if os.path.exists("changed_files.txt"):
        with open("changed_files.txt", "r", encoding="utf-8") as f:
            changed_files = [line.strip() for line in f if line.strip()]

    if not changed_files:
        diff_dict = get_git_diff_lines()
        changed_files = list(diff_dict.keys())

    if not changed_files:
        changed_files = [
            "modules/edge_infra/storage/sqlite_service.c",
            "modules/edge_io/src/rest_server.c",
            "modules/edge_infra/npu/npu_ascend_acl.c",
            "modules/edge_core/src/arbiter.c"
        ]

    final_report = asyncio.run(run_mcp_orchestration(changed_files))

    if not final_report or "summary" not in final_report:
        print("❌ 未能获取最终结构化报告，退出", file=sys.stderr)
        sys.exit(1)

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
        print(f"🎉 质量门禁检查完成 (P0={p0}, P1={p1}).")
        sys.exit(0)


if __name__ == "__main__":
    main()
