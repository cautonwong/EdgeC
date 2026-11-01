"""台架契约:产品壳和用例共用的那几个常量与路径。

台架真值不在这里写死 —— 从 configs/devices.yaml 的 simulated_645 读,
保证"平台认为的表值"和"喂给 DCU 的字节"是同一份。
"""
import os
import sys
from pathlib import Path

import yaml

HERE = Path(__file__).resolve().parent
DEVICES = HERE / "configs" / "devices.yaml"

# 运行目录:产品壳把 db 和总线计数写在这里,用例从这里读。
# 运行目录可覆盖:多个人/多个任务同时跑时共用一个目录会互相踩(数据被抢、被杀)。
RUN_DIR = Path(os.environ.get("DCU_RUN_DIR") or "/tmp/edgec-acceptance-run")
DB_PATH = RUN_DIR / "dcu_runtime.db"
BUS_LOG = RUN_DIR / "bus.json"

# DCU 二进制的位置与"是否过期"由 acceptance/build_probe.py 统一判定:
# 构建根走 EDGEC_BUILD_DIR(CI 用 build/,开发常在 /tmp),产品级仍可用 DCU_BIN 覆盖。
# 写死仓库内 build/ 踩过一次:对着三周前的二进制跑,新 env 变量全被无视。
_ACCEPTANCE_ROOT = HERE.parent
if str(_ACCEPTANCE_ROOT) not in sys.path:
    sys.path.insert(0, str(_ACCEPTANCE_ROOT))
from build_probe import describe, resolve_product_binary, staleness  # noqa: E402

_REPO_ROOT = HERE.parents[1]
DCU_SRC_DIRS = (_REPO_ROOT / "cmd" / "dcu", _REPO_ROOT / "modules")

DCU_BIN = resolve_product_binary("dcu", explicit=os.environ.get("DCU_BIN"))

# 台架只开 DLT645 通道:main.c 把 0-21 硬编码成 DLT698。
CHANNELS = list(range(22, 32))
PER_CHANNEL = 4
METER_ADDRESS = "000000000001"
DI_VOLTAGE = "02010100"


def bench_points() -> dict:
    """simulated_645 里声明的 {表地址: {数据标识: 原始字节}}。"""
    manifest = yaml.safe_load(DEVICES.read_text(encoding="utf-8"))
    device = next(d for d in manifest["devices"] if d["id"] == "bench_meter_01")
    return device["connection"]["simulated_645"]


def bench_voltage_bytes() -> bytes:
    raw = bench_points()[METER_ADDRESS][DI_VOLTAGE]
    return bytes.fromhex(raw)


def bench_voltage() -> float:
    """低字节先的 BCD,scale 0.1 —— 与点表里的 encoding/scale 一致。"""
    raw = bench_voltage_bytes()
    digits = "".join(f"{b:02x}" for b in reversed(raw))
    return int(digits) * 0.1


def meter_addresses() -> list[str]:
    """产品壳 provision 进 dcu_runtime.db 的 40 只表。"""
    return [f"{i:012d}" for i in range(1, len(CHANNELS) * PER_CHANNEL + 1)]
