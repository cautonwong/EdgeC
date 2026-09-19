#!/usr/bin/env python3
"""
Test MCP Stdio Client Connection & Dynamic Tool Discovery
"""
import asyncio
import os
import sys
from contextlib import AsyncExitStack
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

SERVERS = {
    "crg": StdioServerParameters(command="/usr/local/bin/code-review-graph", args=["serve"]),
    "review_tools": StdioServerParameters(command="python3", args=["scripts/ci/unified_review_mcp.py", "serve"])
}

async def test_mcp_connection():
    exit_stack = AsyncExitStack()
    try:
        discovered_tools = []
        for name, config in SERVERS.items():
            print(f"🔌 Connecting to MCP server [{name}] ...")
            r_stream, w_stream = await exit_stack.enter_async_context(stdio_client(config))
            session = await exit_stack.enter_async_context(ClientSession(r_stream, w_stream))
            await session.initialize()
            tools_resp = await session.list_tools()
            for t in tools_resp.tools:
                discovered_tools.append((name, t.name, t.description))
                print(f"  - [{name}] Tool: {t.name} -> {t.description[:60]}...")
            
            # Test calling a tool
            if name == "crg":
                res = await session.call_tool("get_review_context_tool", {"changed_files": ["modules/edge_infra/npu/npu_ascend_acl.c"]})
                print(f"  ✅ Call get_review_context_tool success, output bytes: {len(str(res.content))}")
            elif name == "review_tools":
                res = await session.call_tool("match_company_rules", {})
                print(f"  ✅ Call match_company_rules success, output bytes: {len(str(res.content))}")

        print(f"\n🎉 Successfully discovered {len(discovered_tools)} MCP tools over live stdio ClientSession!")
    finally:
        await exit_stack.aclose()

if __name__ == "__main__":
    asyncio.run(test_mcp_connection())
