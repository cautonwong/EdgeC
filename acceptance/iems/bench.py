"""iems 台架契约:产品壳和用例共用的常量与路径。

iems 与 DCU 的形状不同:iems 自己是 Modbus 主站,南向总线 0 挂
`/tmp/vbus0.sock`(cmd/iems/iems_driver_adapter.c:265-291)。所以台架在这条
总线上当**从站**,iems 来读;断言面是 iems 自己的 REST
(`EDGE_REST_PORT` → modules/edge_io/src/rest_server.c:492-508)。
"""
import json
import os
from pathlib import Path

HERE = Path(__file__).resolve().parent

RUN_DIR = Path("/tmp/edgec-iems-run")
DB_PATH = RUN_DIR / "iems.db"
BUS_LOG = RUN_DIR / "bus.json"

_CANDIDATES = [HERE.parents[1] / "build/products/iems/bin/iems"]
IEMS_BIN = os.environ.get("IEMS_BIN") or next(
    (str(p) for p in _CANDIDATES if p.exists()), str(_CANDIDATES[0])
)

# 南向总线 0:RS485_Main / UART。总线 1 是 CAN,总线 10 是北向从站,iems 自己封了
# 北向自轮询(iems_task_loader.c:30 `if (bus_id == 10) return 0;`),台架不管它。
SOCK = "/tmp/vbus0.sock"
BUS_ID = 0
SLAVE_ID = 1

# 台架真值。tag 1001 = VOLTAGE_PHASE_A(cmd/iems/iems_driver_adapter.c:362)。
# 寄存器 0 给 231.7V,0.1 分辨率 → 2317。
TAG_VOLTAGE = 1001
TAG_CURRENT = 1002
VOLTAGE = 231.7
VOLTAGE_REGISTER = int(VOLTAGE * 10)

# 北向从站(/tmp/iems_north_slave.sock,iems_app.c:415-431 的 setup_slave)。
# 平台只认 serial / tcp,不认 unix socket,所以台架把这条 unix socket 桥成 TCP:
# 平台驱动的是 iems 自己对外发布的那份镜像,不是 REST 自说自话。
# 端口写死是因为 devices.yaml 是静态清单,读不到 ${PORT};换端口两边都要改。
NORTH_SOCK = "/tmp/iems_north_slave.sock"
BRIDGE_PORT = 19211


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
