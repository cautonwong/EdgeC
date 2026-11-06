#!/usr/bin/env bash
# WebUI 平台用例的验收入口（判据在 test-platform 的 tests_web）。
#
#   ./acceptance/run_webui.sh
#
# 为什么单独一个入口：设备验收是"每个产品一套 tasks/*.yaml、由 run_task.py 拉起产品壳"，
# 而 WebUI 这套是"起 webd 的行为替身（mock-server）+ 打 HTTP"，没有设备、没有产品壳。
# 硬塞进 run.sh 的任务模型只会让两边都别扭；这里共用同一套"找平台 + 核 pin"的规则，
# 判据与证据仍然全部来自平台。
#
# 环境变量：
#   TESTPLATFORM  平台仓库路径（缺省依次找 ../test-platform、/workspaces/test-platform）
#   PYTHON        解释器（缺省用平台的 .venv/bin/python，没有就问 python3）
#   TESTPLATFORM_SKIP_PIN=1  不校验 acceptance/testplatform.sha
#
# 退出码沿用平台门禁口径：0 合格 / 1 不合格 / 2 测试无效 / 4 平台错误。
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"

_find_platform() {
    if [[ -n "${TESTPLATFORM:-}" ]]; then echo "$TESTPLATFORM"; return; fi
    for candidate in "$ROOT/../test-platform" "/workspaces/test-platform"; do
        [[ -f "$candidate/tests_web/conftest.py" ]] && { echo "$candidate"; return; }
    done
    echo ""
}

PLATFORM="$(_find_platform)"
if [[ -z "$PLATFORM" ]]; then
    echo "找不到 test-platform:设 TESTPLATFORM=<平台仓库路径>" >&2
    exit 4
fi
PLATFORM="$(cd "$PLATFORM" && pwd)"

PYTHON="${PYTHON:-$PLATFORM/.venv/bin/python}"
[[ -x "$PYTHON" ]] || PYTHON="$(command -v python3)"

# 平台独立进化，EdgeC 不跟着漂：和设备验收同一套 pin 规则。
PIN="$(sed -n 's/^\([0-9a-f]\{7,40\}\)$/\1/p' "$HERE/testplatform.sha" 2>/dev/null | head -1)"
HAVE="$(git -C "$PLATFORM" rev-parse HEAD 2>/dev/null || echo unknown)"
if [[ -n "$PIN" && "${TESTPLATFORM_SKIP_PIN:-0}" != 1 && "$HAVE" != unknown ]]; then
    if [[ "$HAVE" != "$PIN"* && "$PIN" != "$HAVE"* ]]; then
        echo "平台版本不对:锁的 ${PIN:0:12},拿到 ${HAVE:0:12}" >&2
        echo "  要么 checkout 锁定的那个 commit,要么改 acceptance/testplatform.sha(一次升级 PR)" >&2
        exit 4
    fi
fi

if ! command -v node >/dev/null 2>&1; then
    echo "缺 node:WebUI 的 Registry 是构建产物，必须先生成（不落库）" >&2
    exit 4
fi

# Registry 工具用 js-yaml（来自 workspace 依赖）。没装 node_modules 时这里就红，
# 而不是等到 registry 命令里抛一个看不懂的 "Cannot find package"。
if ! (cd "$ROOT/web" && node -e "require.resolve('js-yaml')") >/dev/null 2>&1; then
    echo "缺 web 依赖（js-yaml）：先 cd web && npm ci --ignore-scripts" >&2
    exit 4
fi

if ! "$PYTHON" -c "import pytest" >/dev/null 2>&1; then
    echo "缺 pytest:装法 $PYTHON -m pip install 'pytest>=8.0' 'pyyaml>=6.0'" >&2
    exit 4
fi

echo "平台: $PLATFORM"
echo "解释器: $PYTHON"
echo "平台版本: ${HAVE:0:12}${PIN:+ (锁定 ${PIN:0:12})}"

# Registry 是构建产物（不入库）。不生成的话 mock-server 起不来，
# 用例会全被 skip —— 那是"测试无效"，不是"产品合格"。
echo "生成 Registry…"
(cd "$ROOT/web" && node tooling/cli.mjs registry --all >/dev/null)

EVIDENCE="$ROOT/reports/acceptance/webui/evidence"
rm -rf "$ROOT/reports/acceptance/webui"
mkdir -p "$EVIDENCE"

set +e
(
    cd "$PLATFORM" || exit 4
    EDGEC_WEB_DIR="$ROOT/web" WEBUI_EVIDENCE_DIR="$EVIDENCE" \
        "$PYTHON" -m pytest tests_web -q
)
code=$?
set -e

# 证据归档：缺证据 = evidence_broken，不能当成通过（平台口径）。
summary_code=0
"$PYTHON" "$PLATFORM/tests_web/summarize.py" "$EVIDENCE" || summary_code=$?

echo
echo "证据: $EVIDENCE"
echo "汇总: $EVIDENCE/summary.json"
if [ "$code" -eq 0 ] && [ "$summary_code" -ne 0 ]; then
    code=$summary_code
fi
exit "$code"
