#!/usr/bin/env bash
# EdgeC 验收入口:一个产品一个测试项目,一个脚本跑完、一个退出码说话。
#
#   ./acceptance/run.sh              # 跑 acceptance/ 下所有产品
#   ./acceptance/run.sh dcu          # 只跑 dcu(可给多个)
#
# 环境变量:
#   TESTPLATFORM  平台仓库路径(缺省依次找 ../test-platform、/workspaces/test-platform)
#   PYTHON        解释器(缺省用平台的 .venv/bin/python,没有就问 python3)
#   DCU_BIN       DCU 二进制(缺省按 CMake 产物位置找)
#   TESTPLATFORM_SKIP_PIN=1  不校验 acceptance/testplatform.sha(本地调试用)
#
# 退出码 = test-platform 门禁的结论:
#   0 合格 / 1 不合格 / 2 测试无效 / 3 未完成 / 4 平台错误 / 5 没跑过 / 6 没有验收任务
#
# 这个脚本只做四件产品外壳必须做的事:找平台、传路径、清残骸、逐个产品跑 + 汇总门禁。
# 判据、证据、归因都在平台里,这里不重复实现。
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"

_find_platform() {
    if [[ -n "${TESTPLATFORM:-}" ]]; then echo "$TESTPLATFORM"; return; fi
    for candidate in "$ROOT/../test-platform" "/workspaces/test-platform"; do
        [[ -f "$candidate/tools/run_task.py" ]] && { echo "$candidate"; return; }
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

# 产品子进程只继承 PYTHONPATH(不读 ini 的 pythonpath),平台代码得从这里进去。
export PYTHONPATH="$PLATFORM/src${PYTHONPATH:+:$PYTHONPATH}"

if ! "$PYTHON" -c "import testplatform" 2>/dev/null; then
    echo "平台代码 import 不到:$PLATFORM/src(检查 TESTPLATFORM / PYTHON)" >&2
    exit 4
fi

# 平台独立进化,EdgeC 不跟着漂:核对锁定的版本(改这一行就是一次升级 PR)。
PIN_FILE="$HERE/testplatform.sha"
PIN="$(sed -n 's/^\([0-9a-f]\{7,40\}\)$/\1/p' "$PIN_FILE" 2>/dev/null | head -1)"
HAVE="$(git -C "$PLATFORM" rev-parse HEAD 2>/dev/null || echo unknown)"
if [[ -n "$PIN" && "${TESTPLATFORM_SKIP_PIN:-0}" != 1 && "$HAVE" != unknown ]]; then
    if [[ "$HAVE" != "$PIN"* && "$PIN" != "$HAVE"* ]]; then
        echo "平台版本不对:锁的 ${PIN:0:12},拿到 ${HAVE:0:12}" >&2
        echo "  要么 checkout 锁定的那个 commit,要么改 acceptance/testplatform.sha(一次升级 PR)" >&2
        exit 4
    fi
fi

if [[ $# -gt 0 ]]; then
    PRODUCTS=("$@")
else
    mapfile -t PRODUCTS < <(cd "$HERE" && ls -d */tasks/*.yaml 2>/dev/null | cut -d/ -f1 | sort -u)
fi
if [[ ${#PRODUCTS[@]} -eq 0 ]]; then
    echo "acceptance/ 下没有产品(每个产品形如 acceptance/<product>/tasks/*.yaml)" >&2
    exit 6
fi

REPORTS="$ROOT/reports/acceptance"
# DCU 等产品在 /tmp 下挂 unix socket:上一轮被杀掉会留下同名文件,下一轮 connect 全失败
# 却"看起来跑通了"(产品侧静默跳过)。开跑前和收尾各清一次。
_clean_sockets() { rm -f /tmp/rs485_*.sock; }
trap _clean_sockets EXIT
_clean_sockets

echo "平台: $PLATFORM"
echo "解释器: $PYTHON"
echo "平台版本: ${HAVE:0:12}${PIN:+ (锁定 ${PIN:0:12})}"
echo "产品: ${PRODUCTS[*]}"
echo

# 整轮清空报告目录,不只是逐个产品清:gate 会把 $REPORTS 下所有产品的 summary
# 一起算,只跑一个产品时(`run.sh dcu`)上一轮别的产品的证据会混进这次门禁。
rm -rf "$REPORTS"
mkdir -p "$REPORTS"

for product in "${PRODUCTS[@]}"; do
    project="$HERE/$product"
    task="$(ls "$project"/tasks/*.yaml 2>/dev/null | head -1 || true)"
    if [[ -z "$task" ]]; then
        echo "[$product] 跳过:没有 tasks/*.yaml" >&2
        continue
    fi
    echo "[$product] $(basename "$task")"
    # 每轮清空该产品的报告目录:它是一次运行的输出,不是累加日志。
    # 不清会串轮次 —— 上一轮遗留的 stop.request 会让本轮 0 用例直接中断,
    # 上一轮遗留的 <设备>.jsonl 会把旧读数带进本轮的判据(实测过一次:4 条用例
    # 全绿,却被上一轮的 电压=0.0 判成设备不合格)。
    rm -rf "$REPORTS/$product"
    mkdir -p "$REPORTS/$product"
    set +e
    "$PYTHON" "$PLATFORM/tools/run_task.py" "$task" \
        --root "$project" --evidence-dir "$REPORTS/$product"
    code=$?
    set -e
    echo "[$product] 退出码 $code"
    echo
done

_clean_sockets
echo "门禁:"
set +e
"$PYTHON" "$PLATFORM/tools/gate.py" "$REPORTS"
gate=$?
set -e
echo
echo "报告: $REPORTS"
echo "归因: $PYTHON $PLATFORM/tools/attribute_round.py $REPORTS/<产品>"
exit "$gate"
