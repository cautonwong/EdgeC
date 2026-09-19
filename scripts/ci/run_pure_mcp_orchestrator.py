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

    model_name = os.environ.get("GEMINI_MODEL", "gemini-2.5-flash-lite")
    gemini_url = f"https://generativelanguage.googleapis.com/v1beta/models/{model_name}:generateContent?key={api_key}"

    exit_stack = AsyncExitStack()
    tool_to_session: Dict[str, ClientSession] = {}
    gemini_tool_declarations = []

    print("🔌 [MCP Orchestrator] 正在连接 CRG 与 Review Tools MCP stdio 服务...")
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
请调度挂载的 MCP 工具完成本次 PR 审查：
1. 首先必须调用 get_review_context_tool 获取变更文件的全库调用拓扑与爆炸半径；
2. 接着必须调用 scan_ocr_rules 识别阿里高危规约模式 (NPE/ISR阻塞/超时/Mock硬编码)；
3. 接着调用 match_company_rules 与 run_clang_tidy_check / run_cppcheck_analysis 收集硬事实；
4. 综合所有证据排除虚警，严格对照 EdgeOS 宪法 (物理真实性/ISR安全/四平面隔离) 定级 P0~P3；
5. 最后必须且只能调用 submit_final_report 提交完整的结构化审查成果。"""
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
            print(f"🧠 [Round {turn + 1}/{MAX_ROUNDS}] 发送会话至 Google Gemini ({model_name}) 思考中...")
            payload = {
                "system_instruction": system_instruction,
                "contents": contents,
                "tools": [{"function_declarations": gemini_tool_declarations}]
            }

            req = urllib.request.Request(
                gemini_url,
                data=json.dumps(payload).encode('utf-8'),
                headers={"Content-Type": "application/json"}
            )

            try:
                with urllib.request.urlopen(req, timeout=60) as resp:
                    resp_data = json.loads(resp.read().decode('utf-8'))
            except Exception as e:
                print(f"❌ Gemini API 调用失败: {e}")
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
