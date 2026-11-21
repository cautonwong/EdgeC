"""台架契约:产品壳和用例共用的那几个常量与路径。

台架真值不在这里写死 —— 从 configs/devices.yaml 的 simulated_645 读,
保证"平台认为的表值"和"喂给 DCU 的字节"是同一份。
"""
import json
import os
import sys
import tempfile
from pathlib import Path

import yaml

HERE = Path(__file__).resolve().parent
DEVICES = HERE / "configs" / "devices.yaml"

# 运行目录可由 DCU_RUN_DIR 覆盖；并发任务共用默认目录会互相踩（数据被抢、被杀）。
RUN_DIR = Path(os.environ.get("DCU_RUN_DIR") or "/tmp/edgec-acceptance-run")
DB_PATH = RUN_DIR / "dcu_runtime.db"
BUS_LOG = RUN_DIR / "bus.json"


def dump_json_atomic(path: Path, obj) -> None:
    """同目录临时文件 + fsync + os.replace：读者只会看到完整的旧文件或新文件。"""
    path = Path(path)
    fd, tmp = tempfile.mkstemp(dir=str(path.parent), prefix=path.name + ".", suffix=".tmp")
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            f.write(json.dumps(obj, ensure_ascii=False))
            f.flush()
            os.fsync(f.fileno())
        os.replace(tmp, path)
    except BaseException:
        try:
            os.unlink(tmp)
        except FileNotFoundError:
            pass
        raise

_ACCEPTANCE_ROOT = HERE.parent
if str(_ACCEPTANCE_ROOT) not in sys.path:
    sys.path.insert(0, str(_ACCEPTANCE_ROOT))
from build_probe import describe, resolve_product_binary, staleness  # noqa: E402

_REPO_ROOT = HERE.parents[1]
DCU_SRC_DIRS = (_REPO_ROOT / "cmd" / "dcu", _REPO_ROOT / "modules")

# 二进制身份与过期判定必须先于任何结论：三周前的旧二进制曾跑出过假绿。
DCU_BIN = resolve_product_binary("dcu", explicit=os.environ.get("DCU_BIN"))

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
