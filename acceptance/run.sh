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
_clean_sockets() {
  # 每轮开始/结束都清:上一轮被 kill 的壳可能把从站 socket 留在 /tmp,
  # DCU 会连到那个死 socket 然后静默跳过 —— 表现为"总线全空"的假红。
  rm -f /tmp/rs485_*.sock
  # 产品壳泄漏(平台只杀壳,壳的子进程可能还活着并占着端口/总线),
  # 不清掉会让下一轮的用例连到上一轮的进程上 —— 这是已经踩过的假绿。
  pkill -f "acceptance/.*/(iems_product|dcu_product|gateway)\.py" 2>/dev/null || true
}
trap _clean_sockets EXIT
_clean_sockets
_clean_sockets

echo "平台: $PLATFORM"
echo "解释器: $PYTHON"
echo "平台版本: ${HAVE:0:12}${PIN:+ (锁定 ${PIN:0:12})}"
echo "产品: ${PRODUCTS[*]}"
echo

# ------------------------------------------------------------------------------
# 北向 E2E 的 broker（**缺省就起**，NB_E2E=0 才关）
#
# 为什么缺省开：北向用例是验收套件的一部分，依赖在就必须真跑。原来只在
# NB_E2E=1 时起，于是 CI（不设这个变量）里北向用例全部跳过，iems_acceptance
# 被判"未完成" —— 整轮验收红，而红的原因跟产品无关，是台架自己没起对东西。
#
# 两个约束，缺一条北向用例就会静默跳过（退出码 3 = 未完成，不是不合格）：
#   1. 必须**在跑任务之前**起：产品是 run_task.py 在 pytest 之前拉起的，
#      broker 与 MQTT_BROKER 必须已经就绪，放 conftest 里就晚一步。
#   2. 必须在这个位置而不是循环之后：本轮踩过——broker 起在任务循环后面，
#      于是所有北向用例都看不到 broker，跳过 3 条，整轮判"未完成"。
# ------------------------------------------------------------------------------
NB_BROKER_PID=""
if [[ "${NB_E2E:-1}" == "1" ]]; then
    if ! command -v mosquitto >/dev/null 2>&1; then
        echo "NB_E2E=1 但本机没有 mosquitto：北向用例会被跳过（这一轮没有北向结论）" >&2
    else
        NB_PORT="$("$PYTHON" -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1]); s.close()')"
        mosquitto -p "$NB_PORT" >/dev/null 2>&1 &
        NB_BROKER_PID=$!
        # 等它真的开始监听，再让产品启动
        for _ in $(seq 1 100); do
            if "$PYTHON" -c "import socket,sys; s=socket.socket(); s.settimeout(0.2); sys.exit(0 if s.connect_ex(('127.0.0.1',$NB_PORT))==0 else 1)"; then
                break
            fi
            sleep 0.05
        done
        # 北向用例要 paho-mqtt（平台的可选依赖组 mqtt）。缺了不会"跳过"，而是失败 ——
        # 因为产品侧真的连不上 broker。这里先说出来，省得对着"未完成"猜。
        if ! "$PYTHON" -c "import paho.mqtt.client" >/dev/null 2>&1; then
            echo "⚠️  缺 paho-mqtt：北向用例会失败（装法：pip install 'paho-mqtt>=1.6'）" >&2
        fi
        export MQTT_BROKER="tcp://127.0.0.1:$NB_PORT"
        export MQTT_PROTOCOL="${MQTT_PROTOCOL:-MqttJson}"
        export MQTT_GROUP_ID="${MQTT_GROUP_ID:-EdgeOS}"
        echo "北向 broker 已就绪：$MQTT_BROKER（协议 $MQTT_PROTOCOL）"
    fi
fi

_nb_cleanup() {
    if [[ -n "$NB_BROKER_PID" ]]; then
        kill "$NB_BROKER_PID" 2>/dev/null || true
        wait "$NB_BROKER_PID" 2>/dev/null || true
        NB_BROKER_PID=""
    fi
}
trap _nb_cleanup EXIT INT TERM

# 整轮清空报告目录,不只是逐个产品清:gate 会把 $REPORTS 下所有产品的 summary
# 一起算,只跑一个产品时(`run.sh dcu`)上一轮别的产品的证据会混进这次门禁。
rm -rf "$REPORTS"
mkdir -p "$REPORTS"

product_dirs=()
for product in "${PRODUCTS[@]}"; do
    project="$HERE/$product"
    tasks=( "$project"/tasks/*.yaml )
    if [[ ! -e "${tasks[0]}" ]]; then
        echo "[$product] 跳过:没有 tasks/*.yaml" >&2
        continue
    fi
    # 每轮清空该产品的报告目录:它是一次运行的输出,不是累加日志。
    # 不清会串轮次 —— 上一轮遗留的 stop.request 会让本轮 0 用例直接中断,
    # 上一轮遗留的 <设备>.jsonl 会把旧读数带进本轮的判据(实测过一次:4 条用例
    # 全绿,却被上一轮的 电压=0.0 判成设备不合格)。
    rm -rf "$REPORTS/$product"
    mkdir -p "$REPORTS/$product"
    product_dirs+=( "$REPORTS/$product" )
    # 一个产品可以有多个任务(验收 + 冒烟/启动闸门各自的进程生命周期不同),
    # 所以全跑,不要只取第一个。
    for task in "${tasks[@]}"; do
        [[ -e "$task" ]] || continue
        task_dir="$REPORTS/$product/$(basename "$task" .yaml)"
        echo "[$product] $(basename "$task")"
        # 每个任务一个独立运行目录:共用一个会让上一轮的 sqlite/WAL 被回放,
        # 故障还没注入 REST 就已经有旧读数 —— 用例会绿,但什么都没验到。
        export IEMS_RUN_DIR="$ROOT/runtime/$product/$(basename "$task" .yaml)"
        # 北向从站 socket 也跟着任务走:固定路径时两个任务会互相 unlink
        # 对方正在监听的 socket,现象是"产品突然不回应"。产品壳会原样转发这个
        # env(见 iems_product.py 的 env 白名单),台架的桥连的也是同一个值。
        export DCU_RUN_DIR="$ROOT/runtime/$product/$(basename "$task" .yaml)"
        # 北向主题身份也按任务隔离：不设时产品会退化成 "unknown"，
        # 多个产品/多个任务同时在线就会撞主题，主站分不清是谁的数据。
        export MQTT_NODE_ID="${MQTT_NODE_ID:-EdgeC_${product}_$(basename "$task" .yaml)}"
        export IEMS_NORTH_SOCK="$IEMS_RUN_DIR/north.sock"
        # DCU 的 698 从站同理：固定路径时两个任务会互相 unlink 对方的 socket。
        export DCU_NORTH_SOCK="$DCU_RUN_DIR/north.sock"
        rm -rf "$IEMS_RUN_DIR"
        rm -rf "$DCU_RUN_DIR"
        set +e
        "$PYTHON" "$PLATFORM/tools/run_task.py" "$task" \
            --root "$project" --evidence-dir "$task_dir"
        code=$?
        set -e
        echo "[$product] $(basename "$task" .yaml) 退出码 $code"
        echo
    done
    echo
done


if [[ ${#product_dirs[@]} -eq 0 ]]; then
    echo "没有可跑的产品(每个被点到的产品目录下都没有 tasks/*.yaml)" >&2
    exit 6
fi

_clean_sockets
echo "门禁:"
set +e
# gate 取 <目录>/<任务>/summary.json,正好是"产品目录下的各任务"这一层。
"$PYTHON" "$PLATFORM/tools/gate.py" "${product_dirs[@]}"
gate=$?
set -e
echo
echo "报告: $REPORTS"
echo "归因: $PYTHON $PLATFORM/tools/attribute_round.py $REPORTS/<产品>"
exit "$gate"
