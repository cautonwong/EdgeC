#!/usr/bin/env python3
"""
Pure MCP Orchestrator & Real AI Model Synthesis Engine for EdgeOS / EdgeC
"""

import os
import sys
import json
import asyncio
import urllib.request
import urllib.error
from contextlib import AsyncExitStack
from typing import Dict, Any, List

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

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
            "summary": {"type": "OBJECT", "properties": {
                "P0": {"type": "INTEGER"}, "P1": {"type": "INTEGER"},
                "P2": {"type": "INTEGER"}, "P3": {"type": "INTEGER"}
            }, "required": ["P0", "P1", "P2", "P3"]},
            "blast_radius_summary": {"type": "STRING"},
            "findings": {"type": "ARRAY", "items": {"type": "OBJECT", "properties": {
                "file": {"type": "STRING"}, "line": {"type": "INTEGER"},
                "priority": {"type": "STRING"}, "source": {"type": "STRING"},
                "issue": {"type": "STRING"}, "fix_suggestion": {"type": "STRING"}
            }, "required": ["file", "line", "priority", "issue", "fix_suggestion"]}},
            "markdown_report": {"type": "STRING"}
        },
        "required": ["summary", "blast_radius_summary", "findings", "markdown_report"]
    }
}


async def run_mcp_orchestration(changed_files: List[str]) -> Dict[str, Any]:
    api_key = os.environ.get("GEMINI_API_KEY")
    if not api_key:
        raise RuntimeError("GEMINI_API_KEY is not configured")

    primary_model = os.environ.get("GEMINI_MODEL", "gemini-2.5-flash")
    candidate_models = [primary_model]
    for model in ["gemini-2.5-flash-lite", "gemini-2.5-flash"]:
        if model not in candidate_models:
            candidate_models.append(model)

    active_model = candidate_models[0]
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
            for tool in tools_resp.tools:
                tool_to_session[tool.name] = session
                props = {}
                required_fields = []
                schema = getattr(tool, "input_schema", None) or getattr(tool, "inputSchema", None)
                if schema:
                    for key, value in schema.get("properties", {}).items():
                        prop_type = value.get("type", "STRING").upper()
                        if prop_type == "ARRAY":
                            props[key] = {"type": "ARRAY", "items": {"type": "STRING"}}
                        elif prop_type == "INTEGER":
                            props[key] = {"type": "INTEGER"}
                        elif prop_type == "BOOLEAN":
                            props[key] = {"type": "BOOLEAN"}
                        else:
                            props[key] = {"type": "STRING"}
                    required_fields = schema.get("required", [])
                gemini_tool_declarations.append({
                    "name": tool.name,
                    "description": tool.description or tool.name,
                    "parameters": {"type": "OBJECT", "properties": props, "required": required_fields}
                })
                print(f"  ↳ 发现并注册 MCP Tool: `{tool.name}` (来源: {name})")

        gemini_tool_declarations.append(FINAL_SUBMIT_TOOL)
        system_instruction = {"role": "system", "parts": [{"text": """
你是 EdgeOS 嵌入式工业系统代码审查官。请调度 CRG、工业规则、ISR、硬件超时、物理真实性、缓冲区安全、clang-tidy 和 cppcheck 工具，交叉验证并按 P0~P3 分级。最后必须调用 submit_final_report 提交结构化报告。
"""}]}
        contents = [{"role": "user", "parts": [{"text": f"请审查本次 PR 变更文件：\n{json.dumps(changed_files, indent=2)}"}]}]

        for turn in range(8):
            print(f"🧠 [Round {turn + 1}/8] 发送会话至 Google Gemini ({active_model}) 思考中...")
            payload = {"system_instruction": system_instruction, "contents": contents,
                       "tools": [{"function_declarations": gemini_tool_declarations}],
                       "generationConfig": {"temperature": 0.2, "maxOutputTokens": 4096}}
            response_data = None
            for model in [active_model] + [m for m in candidate_models if m != active_model]:
                url = f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={api_key}"
                request = urllib.request.Request(url, data=json.dumps(payload).encode("utf-8"), headers={"Content-Type": "application/json"})
                try:
                    with urllib.request.urlopen(request, timeout=45) as response:
                        response_data = json.loads(response.read().decode("utf-8"))
                        active_model = model
                        break
                except Exception as error:
                    print(f"⚠️ Gemini 模型 [{model}] 响应异常: {error}")
            if not response_data:
                break
            candidates = response_data.get("candidates", [])
            if not candidates:
                break
            candidate_content = candidates[0].get("content", {})
            contents.append(candidate_content)
            response_parts = []
            for part in candidate_content.get("parts", []):
                if "functionCall" not in part:
                    continue
                function_call = part["functionCall"]
                function_name = function_call["name"]
                function_args = function_call.get("args", {})
                if function_name == "submit_final_report":
                    return function_args
                session = tool_to_session.get(function_name)
                if session:
                    result = await session.call_tool(function_name, arguments=function_args)
                    output = "\n".join(item.text for item in result.content if hasattr(item, "text"))
                    if len(output) > 3000:
                        output = output[:3000] + "... [truncated]"
                    try:
                        output_object = json.loads(output)
                    except Exception:
                        output_object = {"result": output}
                else:
                    output_object = {"error": f"Tool {function_name} not found"}
                response_parts.append({"functionResponse": {"name": function_name, "response": {"output": output_object}}})
            if response_parts:
                contents.append({"role": "user", "parts": response_parts})
            else:
                contents.append({"role": "user", "parts": [{"text": "请调用 submit_final_report 提交最终结构化报告。"}]})
        return {}
    finally:
        await exit_stack.aclose()


def main():
    changed_files = []
    if os.path.exists("changed_files.txt"):
        with open("changed_files.txt", encoding="utf-8") as file:
            changed_files = [line.strip() for line in file if line.strip()]
    if not changed_files:
        raise RuntimeError("No changed files found for review")

    report = asyncio.run(run_mcp_orchestration(changed_files))
    if not report or "summary" not in report:
        print("❌ 未能获取最终结构化报告，退出", file=sys.stderr)
        sys.exit(1)
    with open("final_report.json", "w", encoding="utf-8") as file:
        json.dump(report, file, indent=2, ensure_ascii=False)
    with open("final_report.md", "w", encoding="utf-8") as file:
        file.write(report.get("markdown_report", "# Code Review Report"))
    p0 = report.get("summary", {}).get("P0", 0)
    print(f"📊 审查统计: P0={p0}")
    sys.exit(1 if p0 > 0 else 0)


if __name__ == "__main__":
    main()
