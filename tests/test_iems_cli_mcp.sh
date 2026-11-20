#!/usr/bin/env bash
# iems-cli MCP surface: an unknown tool must fail the process, and the tools
# count in the list header must match what the dispatcher reports.
set -euo pipefail

CLI="$1"

set +e
out=$("$CLI" mcp call no_such_tool '{}' 2>&1)
rc=$?
set -e
if [[ $rc -eq 0 ]]; then
    echo "unknown MCP tool exited 0:" >&2
    echo "$out" >&2
    exit 1
fi
echo "$out" | grep -q '"code":.*-32601' || {
    echo "unknown tool did not report -32601:" >&2
    echo "$out" >&2
    exit 1
}

if ! out=$("$CLI" mcp list 2>&1); then
    echo "mcp list failed" >&2
    exit 1
fi
header_tools=$(printf '%s\n' "$out" | sed -n 's/.*Providers, \([0-9][0-9]*\) Tools).*/\1/p' | head -1)
dispatcher_tools=$(printf '%s\n' "$out" |
    sed -n 's/.*Total Active MCP Tools Available in Dispatcher: \([0-9][0-9]*\).*/\1/p' | head -1)
if [[ -z "$header_tools" || -z "$dispatcher_tools" ]]; then
    echo "missing tool counts in mcp list output:" >&2
    printf '%s\n' "$out" >&2
    exit 1
fi
if [[ "$header_tools" != "$dispatcher_tools" ]]; then
    echo "header tools ($header_tools) != dispatcher tools ($dispatcher_tools)" >&2
    exit 1
fi

echo "iems-cli MCP surface OK (unknown tool rc=$rc, tools=$header_tools)"
