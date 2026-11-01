"""iems 台架契约:产品壳和用例共用的常量与路径。

iems 与 DCU 的形状不同:iems 自己是 Modbus 主站,南向总线 0 挂
`/tmp/vbus0.sock`(cmd/iems/iems_driver_adapter.c:265-291)。所以台架在这条
总线上当**从站**,iems 来读;断言面是 iems 自己的 REST
(`EDGE_REST_PORT` → modules/edge_io/src/rest_server.c:492-508)。
"""
import json
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent

RUN_DIR = Path(os.environ.get("IEMS_RUN_DIR") or "/tmp/edgec-iems-run")
DB_PATH = RUN_DIR / "iems.db"
BUS_LOG = RUN_DIR / "bus.json"
DROPPED_LOG = RUN_DIR / "dropped.json"

# 产品二进制的位置与"是否过期"由 acceptance/build_probe.py 统一判定:
# 构建根走 EDGEC_BUILD_DIR(CI 用 build/,开发常在 /tmp),产品级仍可用 IEMS_BIN 覆盖。
# 这里不自己拼路径 —— 写死过一次,结果对着三周前的二进制跑出绿的结论。
_ACCEPTANCE_ROOT = HERE.parent
if str(_ACCEPTANCE_ROOT) not in sys.path:
    sys.path.insert(0, str(_ACCEPTANCE_ROOT))
from build_probe import describe, resolve_product_binary, staleness  # noqa: E402

_REPO_ROOT = HERE.parents[1]
# 过期判定只比较产品代码与公共模块:改这两处的 .c/.h 必须重编。
IEMS_SRC_DIRS = (_REPO_ROOT / "cmd" / "iems", _REPO_ROOT / "modules")

IEMS_BIN = resolve_product_binary("iems", explicit=os.environ.get("IEMS_BIN"))

# 南向总线 0:RS485_Main / UART。总线 1 是 CAN,总线 10 是北向从站,iems 自己封了
# 北向自轮询(iems_task_loader.c:30 `if (bus_id == 10) return 0;`),台架不管它。
# 南向 socket 也按任务隔离:共用一个的话,上一个任务残留的 iems 仍连着它,
# 下一个任务的台架 bind 时就是"连到了别人的进程"(表现为 0.1 秒假绿/假红)。
SOCK = str(Path(os.environ.get("IEMS_RUN_DIR") or "/tmp/edgec-iems-run") / "vbus0.sock")
BUS_ID = 0
SLAVE_ID = 1

# 台架真值。tag 1001 = VOLTAGE_PHASE_A(cmd/iems/iems_driver_adapter.c:362)。
# 寄存器 0 给 231.7V,0.1 分辨率 → 2317。
TAG_VOLTAGE = 1001
TAG_CURRENT = 1002
VOLTAGE = 231.7
VOLTAGE_REGISTER = int(VOLTAGE * 10)

# 北向从站(iems_app.c 的 setup_slave 绑的路径,由 IEMS_NORTH_SOCK 决定)。
# 平台只认 serial / tcp,不认 unix socket,所以台架把这条 unix socket 桥成 TCP:
# 平台驱动的是 iems 自己对外发布的那份镜像,不是 REST 自说自话。
# 端口写死是因为 devices.yaml 是静态清单,读不到 ${PORT};换端口两边都要改。
#
# ⚠️ 排查这条链时**先清掉残留的产品壳进程**：它们占住 BRIDGE_PORT 19211，
# 新壳的桥起不来，探针连到的是**旧壳的桥**（指向已删除的 socket），
# 现象是"TCP 连得上但产品不回应"。我曾据此误判成"从站引擎绑的是契约表路径"——
# 隔离一直是好的，是残留进程在骗人。
#   ps -eo pid,args | grep "python iems_product"   # 有残留就 kill
NORTH_SOCK = str(RUN_DIR / "north.sock")
BRIDGE_PORT = 19211


def dropped() -> dict:
    """掉链用例的记录:第几帧之后台架主动断了南向连接。文件缺失 = 还没断。"""
    try:
        return json.loads(DROPPED_LOG.read_text(encoding="utf-8"))
    except FileNotFoundError:
        return {}


def bus_seen() -> dict:
    """产品壳轮询到的总线请求。文件缺失 = 从站没起来,iems 连了个空 socket。"""
    try:
        return json.loads(BUS_LOG.read_text(encoding="utf-8"))
    except FileNotFoundError:
        return {"requests": 0, "first_request_hex": None}


def rest(port: int, path: str, timeout: float = 3.0) -> dict:
    import urllib.error
    import urllib.request

    url = f"http://127.0.0.1:{port}{path}"
    try:
        with urllib.request.urlopen(url, timeout=timeout) as resp:
            return json.loads(resp.read().decode("utf-8") or "{}")
    except urllib.error.HTTPError as exc:
        return {"http_status": exc.code, "body": exc.read().decode("utf-8", "replace")}
    except OSError as exc:  # URLError/超时/ConnectionReset 都是它的子类
        # ConnectionReset(产品刚起、监听还没完全就绪)也要能重试,不能炸穿等待循环
        return {"error": str(exc)}


def latest(port: int, tag_id: int) -> dict:
    return rest(port, f"/api/v1/data/latest/{tag_id}")


def port_of() -> int:
    return int((RUN_DIR / "port.txt").read_text(encoding="utf-8").strip())
