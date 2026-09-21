#!/usr/bin/env python3
"""
Pure MCP Orchestrator & Real AI Model Synthesis Engine for EdgeOS / EdgeC
Issue #321 架构闭环实现：
1. 集成 Review Manifest 与 Tool Execution Ledger；
2. 调度 CRG 与 Unified Review MCP Tools 进行多轮交叉分析；
3. 硬性拦截 submit_final_report 并通过 EvidenceValidator 进行机器级真实性校验；
4. 在无 API Secret 或网络异常时提供确定性本地扫描兜底 (synthesize_deterministic_fallback_report)。
"""

import os
import sys
import json
import asyncio
import urllib.request
import urllib.error
from contextlib import AsyncExitStack
from typing import Dict, Any, List, Optional, Union

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

from review_manifest import ReviewManifest, compute_review_manifest
from tool_ledger import ToolExecutionLedger, ToolStatus
from evidence_validator import EvidenceValidator, ValidationResult
from unified_review_mcp import AlibabaOCREngine
from code_review_graph import CodeReviewGraph

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
            "identity": {
                "type": "OBJECT",
                "properties": {
                    "base_sha": {"type": "STRING"},
                    "head_sha": {"type": "STRING"},
                    "changed_files_hash": {"type": "STRING"}
                }
            },
            "summary": {
                "type": "OBJECT",
                "properties": {
                    "P0": {"type": "INTEGER"},
                    "P1": {"type": "INTEGER"},
                    "P2": {"type": "INTEGER"},
                    "P3": {"type": "INTEGER"}
                },
                "required": ["P0", "P1", "P2", "P3"]
            },
            "blast_radius_summary": {"type": "STRING"},
            "findings": {
                "type": "ARRAY",
                "items": {
                    "type": "OBJECT",
                    "properties": {
                        "file": {"type": "STRING"},
                        "line": {"type": "INTEGER"},
                        "priority": {"type": "STRING"},
                        "source": {"type": "STRING"},
                        "rule_id": {"type": "STRING"},
                        "issue": {"type": "STRING"},
                        "fix_suggestion": {"type": "STRING"}
                    },
                    "required": ["file", "line", "priority", "issue", "fix_suggestion"]
                }
            },
            "markdown_report": {"type": "STRING"}
        },
        "required": ["summary", "blast_radius_summary", "findings", "markdown_report"]
    }
}


def synthesize_deterministic_fallback_report(
    manifest: ReviewManifest,
    ledger: Optional[ToolExecutionLedger] = None
) -> Dict[str, Any]:
    """
    确定性本地扫描引擎兜底：
    当缺少大模型 API Key 或网络中断时，通过本地 OCR 规则库 + CRG 图谱构建严密报告。
    """
    if ledger is None:
        ledger = ToolExecutionLedger(required_tools=manifest.required_tools)

    # 1. 运行本地 Alibaba OCR 扫描
    ledger.start_tool("scan_ocr_rules", {"files": manifest.changed_files})
    ocr = AlibabaOCREngine()
    raw_findings = ocr.scan_files(manifest.changed_files, manifest.changed_lines)
    ledger.finish_tool(
        "scan_ocr_rules",
        ToolStatus.FINDINGS if raw_findings else ToolStatus.SUCCESS,
        findings_count=len(raw_findings),
        output_summary=f"Found {len(raw_findings)} issues via local OCR"
    )

    # 2. 标记其余必需工具在 fallback 模式下的确定性执行
    for tool_name in manifest.required_tools:
        if tool_name != "scan_ocr_rules":
            ledger.start_tool(tool_name)
            ledger.finish_tool(tool_name, ToolStatus.FALLBACK, findings_count=0, output_summary="Fallback local bypass")

    # 3. 计算爆炸半径与图谱上下文
    crg_context = {}
    if os.path.exists("build/crg.sqlite") or os.path.exists(".crg.sqlite"):
        db_path = "build/crg.sqlite" if os.path.exists("build/crg.sqlite") else ".crg.sqlite"
        try:
            crg = CodeReviewGraph(root_dir=".", db_path=db_path)
            crg_context = crg.query_review_context(manifest.changed_files)
        except Exception:
            crg_context = {}

    blast_summary = f"Impacted modules: {crg_context.get('impacted_modules', [])}, Risk score: {crg_context.get('risk_score', 0.0)}"

    # 4. 统计 Summary
    summary = {"P0": 0, "P1": 0, "P2": 0, "P3": 0}
    for f in raw_findings:
        prio = f.get("priority", "P3")
        if prio in summary:
            summary[prio] += 1
        else:
            summary["P3"] += 1

    # 5. 生成 Markdown 报告
    md_lines = [
        "# EdgeOS AI Code Review Final Report (Deterministic Fallback)",
        "",
        f"- **Base SHA**: `{manifest.base_sha}`",
        f"- **Head SHA**: `{manifest.head_sha}`",
        f"- **Files Changed**: {len(manifest.changed_files)}",
        f"- **Diff Hash**: `{manifest.changed_files_hash[:16]}`",
        "",
        "## 📊 质量门禁指标 (Summary)",
        f"- **P0 (阻断级)**: {summary['P0']}",
        f"- **P1 (严重级)**: {summary['P1']}",
        f"- **P2 (警告级)**: {summary['P2']}",
        f"- **P3 (建议级)**: {summary['P3']}",
        "",
        f"## 🌐 爆炸半径与依赖分析",
        f"{blast_summary}",
        "",
        "## 🔍 审查发现列表 (Findings)",
    ]
    if not raw_findings:
        md_lines.append("✅ 未检测到违反嵌入式工业经验规约的缺陷。")
    else:
        for idx, f in enumerate(raw_findings, 1):
            md_lines.append(f"### {idx}. [{f.get('priority', 'P3')}] {f.get('rule_id', '')} - {f.get('issue', '')}")
            md_lines.append(f"- **位置**: `{f.get('file')}:{f.get('line')}`")
            md_lines.append(f"- **修复建议**: {f.get('fix_suggestion')}")
            md_lines.append("")

    report = {
        "identity": {
            "base_sha": manifest.base_sha,
            "head_sha": manifest.head_sha,
            "changed_files_hash": manifest.changed_files_hash
        },
        "summary": summary,
        "blast_radius_summary": blast_summary,
        "findings": raw_findings,
        "markdown_report": "\n".join(md_lines)
    }

    # 6. 执行严密校验
    validator = EvidenceValidator(manifest=manifest, ledger=ledger)
    validation = validator.validate(report)
    if not validation.is_valid:
        print(f"⚠️ Fallback report validation warning: {validation.errors}")

    return report


async def run_mcp_orchestration(
    target: Union[ReviewManifest, List[str]],
    ledger: Optional[ToolExecutionLedger] = None
) -> Dict[str, Any]:
    if isinstance(target, ReviewManifest):
        manifest = target
    else:
        manifest = compute_review_manifest(changed_files=target)

    if ledger is None:
        ledger = ToolExecutionLedger(required_tools=manifest.required_tools)

    api_key = os.environ.get("GEMINI_API_KEY")
    if not api_key:
        print("ℹ️ 未检测到 GEMINI_API_KEY，切换至确定性本地扫描引擎兜底")
        return synthesize_deterministic_fallback_report(manifest, ledger)

    primary_model = os.environ.get("GEMINI_MODEL", "gemini-3.8-flash")
    candidate_models = [primary_model]
    for model in ["gemini-3.8-flash", "gemini-2.5-flash", "gemini-2.5-flash-lite"]:
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
        validator = EvidenceValidator(manifest=manifest, ledger=ledger)

        system_instruction = {"role": "system", "parts": [{"text": f"""
你是 EdgeOS 嵌入式工业系统代码审查官。
你必须对本次 PR 的改动文件进行严格的嵌入式工业规约审查与全库关系图谱爆炸半径分析。
你的审查原则：
1. 必须覆盖所有必需工具清单：{json.dumps(manifest.required_tools)}；
2. 绝对遵循物理真实性，禁止任何 Mock/Demo 占位符入库；
3. 严禁在 ISR 中调用阻塞函数、printf 或 malloc 堆分配；
4. 所有的 findings 必须指向真实存在的物理文件与精确行号；
5. 完成所有工具调用与交叉验证后，必须且只能调用 submit_final_report 提交最终成果。
"""}]}
        contents = [{"role": "user", "parts": [{"text": f"请审查本次 PR 变更文件（Manifest Diff Hash: {manifest.changed_files_hash}）：\n{json.dumps(manifest.changed_files, indent=2)}"}]}]

        for turn in range(10):
            print(f"🧠 [Round {turn + 1}/10] 发送会话至 Google Gemini ({active_model}) 思考中...")
            payload = {
                "system_instruction": system_instruction,
                "contents": contents,
                "tools": [{"function_declarations": gemini_tool_declarations}],
                "generationConfig": {"temperature": 0.2, "maxOutputTokens": 4096}
            }
            response_data = None
            for model in [active_model] + [m for m in candidate_models if m != active_model]:
                url = f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={api_key}"
                request = urllib.request.Request(
                    url,
                    data=json.dumps(payload).encode("utf-8"),
                    headers={"Content-Type": "application/json"}
                )
                try:
                    with urllib.request.urlopen(request, timeout=45) as response:
                        response_data = json.loads(response.read().decode("utf-8"))
                        active_model = model
                        break
                except Exception as error:
                    print(f"⚠️ Gemini 模型 [{model}] 响应异常: {error}")

            if not response_data:
                print("⚠️ 未能从大模型获取有效响应，转入确定性本地扫描兜底")
                return synthesize_deterministic_fallback_report(manifest, ledger)

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
                    # 进行 Evidence & Ledger 机器硬校验
                    val_result = validator.validate(function_args)
                    if val_result.is_valid:
                        print("✅ [Evidence Validator] 最终审查报告机器校验通过！")
                        function_args["findings"] = val_result.sanitized_findings
                        function_args["summary"] = val_result.computed_summary
                        return function_args
                    else:
                        print(f"❌ [Evidence Validator] 拦截不合规报告: {val_result.errors}")
                        # 将校验错误通过 Function Response 打回给大模型要求修正
                        response_parts.append({
                            "functionResponse": {
                                "name": "submit_final_report",
                                "response": {
                                    "status": "REJECTED_INVALID_REPORT",
                                    "errors": val_result.errors,
                                    "instruction": "你的报告未通过机器真实性与工具覆盖校验。请先执行所有未完成的必检工具，并修正文件、行号与 Summary 计数后再重新提交。"
                                }
                            }
                        })
                        continue

                # 正常 MCP 工具调用
                ledger.start_tool(function_name, function_args)
                session = tool_to_session.get(function_name)
                if session:
                    try:
                        result = await session.call_tool(function_name, arguments=function_args)
                        output = "\n".join(item.text for item in result.content if hasattr(item, "text"))
                        status = ToolStatus.SUCCESS
                        if len(output) > 4000:
                            output = output[:4000] + "... [TRUNCATED]"
                            status = ToolStatus.TRUNCATED
                        try:
                            output_object = json.loads(output)
                        except Exception:
                            output_object = {"result": output}
                        ledger.finish_tool(function_name, status, output_summary=output[:200])
                    except Exception as e:
                        output_object = {"error": str(e)}
                        ledger.finish_tool(function_name, ToolStatus.ERROR, error_message=str(e))
                else:
                    output_object = {"error": f"Tool {function_name} not found"}
                    ledger.finish_tool(function_name, ToolStatus.ERROR, error_message="Tool not found")

                response_parts.append({
                    "functionResponse": {
                        "name": function_name,
                        "response": {"output": output_object}
                    }
                })

            if response_parts:
                contents.append({"role": "user", "parts": response_parts})
            else:
                contents.append({"role": "user", "parts": [{"text": "请继续调用审查工具或调用 submit_final_report 提交最终结构化报告。"}]})

        # 超过轮次兜底
        return synthesize_deterministic_fallback_report(manifest, ledger)
    finally:
        await exit_stack.aclose()


def main():
    base_sha = os.environ.get("GITHUB_BASE_REF", "origin/main")
    head_sha = os.environ.get("GITHUB_SHA", "HEAD")
    manifest = compute_review_manifest(base_sha=base_sha, head_sha=head_sha)
    ledger = ToolExecutionLedger(required_tools=manifest.required_tools)

    with open("review_manifest.json", "w", encoding="utf-8") as f:
        f.write(manifest.to_json())

    report = asyncio.run(run_mcp_orchestration(manifest, ledger))

    with open("tool_ledger.json", "w", encoding="utf-8") as f:
        f.write(ledger.to_json())

    if not report or "summary" not in report:
        print("❌ 未能获取有效结构化审查报告，强制以 Deterministic Fallback 生成", file=sys.stderr)
        report = synthesize_deterministic_fallback_report(manifest, ledger)

    with open("final_report.json", "w", encoding="utf-8") as file:
        json.dump(report, file, indent=2, ensure_ascii=False)
    with open("final_report.md", "w", encoding="utf-8") as file:
        file.write(report.get("markdown_report", "# Code Review Report"))

    p0 = report.get("summary", {}).get("P0", 0)
    p1 = report.get("summary", {}).get("P1", 0)
    print(f"📊 审查审计完成: P0={p0}, P1={p1}, Ledger Complete={ledger.is_complete()}")
    sys.exit(1 if p0 > 0 else 0)


if __name__ == "__main__":
    main()
