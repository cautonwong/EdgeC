"""DCU 采集链路验收:台架给了多少,DCU 必须落多少。

判据三步,顺序有意义 —— 前面不成立时,后面不成立不能算 DCU 的错:
  1. 台架自己读出来的值必须过判据(198-242V),否则是台架问题,不是产品问题
  2. DCU 必须向台架发过 DLT645 读请求
  3. DCU 落库的电压必须等于台架读出来的值(±0.5V)

为什么判据 2 不能省:DCU 的 main.c:178 把 dcu_twin_source(220.0, 5.0) 直接塞进
node_cfg.source_ops,dcu_twin_source.c:19 的 delta=(meter_idx%10)*0.2 会凭空合成读数。
总线一个请求都没有时,telemetry 照样有 240 行。只查"值在 198-242 区间内"是假绿:
合成出来的 220.0/220.2/220.4 全都在区间里。
"""
import json
import sqlite3
import time

import pytest

from bench import BUS_LOG, DB_PATH, METER_ADDRESS
from testplatform.cases import case

TAG_VOLTAGE = 0x20000200
TOLERANCE_V = 0.5
WAIT_S = 16


# cross-round 两阶段读会让值晚一个 interval 才落库;DCU 的 poller_timer
# 周期是 node_cfg.poll_interval_ms = 5000(main.c:193),所以给到 16 秒。
def _latest_voltage(timeout: float = 16.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if DB_PATH.exists():
            try:
                conn = sqlite3.connect(f"file:{DB_PATH}?mode=ro", uri=True)
                row = conn.execute(
                    "SELECT value FROM dcu_telemetry_latest "
                    "WHERE meter_address = ? AND oad = ?",
                    (METER_ADDRESS, TAG_VOLTAGE),
                ).fetchone()
                conn.close()
                if row is not None:
                    return row[0]
            except sqlite3.Error:
                pass
        time.sleep(0.5)
    return None


def _bus_requests() -> int:
    if not BUS_LOG.exists():
        return 0
    return int(json.loads(BUS_LOG.read_text(encoding="utf-8"))["requests"])


def _diagnose(requests: int, value) -> str:
    """断言失败时把"现在是哪一种不通"说清楚,不让人自己猜。"""
    facts = [f"总线请求 {requests} 次", f"落库电压 {value}"]
    if DB_PATH.exists():
        try:
            conn = sqlite3.connect(f"file:{DB_PATH}?mode=ro", uri=True)
            for table in ("dcu_meters", "dcu_telemetry_latest"):
                try:
                    n = conn.execute(f"SELECT count(*) FROM {table}").fetchone()[0]
                    facts.append(f"{table} {n} 行")
                except sqlite3.Error:
                    facts.append(f"{table} 不存在")
            conn.close()
        except sqlite3.Error as exc:
            facts.append(f"读库失败 {exc}")
    return " · ".join(facts)


@case(id="DCU_00_00_01_000001_01", clause="DLT645-4.5.2", tags=["验收"])
@pytest.mark.contract
def test_dcu_polls_the_bench_meter(meter):
    meter.connect()
    bench = meter.read_all()

    # 1. 台架自检:这一条不成立就是台架问题,后面的失败与 DCU 无关。
    assert bench.passed, (
        f"台架自己的读数没过判据:{bench.readings} · 越界 {bench.breaches}"
    )
    expected = bench.readings["电压"]

    # 2. DCU 到底有没有向台架要过数。
    value = _latest_voltage()
    requests = _bus_requests()
    assert requests > 0, (
        f"DCU 在 {WAIT_S} 秒内没有向台架发出任何 DLT645 读请求(/tmp/rs485_22..31.sock) · "
        f"{_diagnose(requests, value)}\n"
        "读入口只有一个:dcu_poller.c 的 source_ops.read_value;全仓库只实现了 "
        "dcu_twin_source_read(合成值),关掉 twin 后 source_ops 为空,总线一个字节都不会发。"
    )

    # 3. 落库值必须等于台架值。
    assert value is not None, "DCU 没有把台架表的电压写进 dcu_telemetry_latest"
    assert abs(value - expected) <= TOLERANCE_V, (
        f"DCU 落库 {value:.1f} V,台架读出来的是 {expected:.1f} V(容差 {TOLERANCE_V} V)"
    )


def _bus_controls(timeout: float = 12.0) -> list:
    """等台架记到 645 控制帧（1CH）。返回记录列表（可能为空）。"""
    deadline = time.time() + timeout
    while time.time() < deadline:
        if BUS_LOG.exists():
            try:
                data = json.loads(BUS_LOG.read_text(encoding="utf-8"))
            except json.JSONDecodeError:
                data = {}
            controls = data.get("controls") or []
            if controls:
                return controls
        time.sleep(0.3)
    return []


@case(id="DCU_00_00_01_000002_01", clause="free: 遥控命令必须真的下到电表（645 控制帧 1CH）", tags=["验收"])
@pytest.mark.contract
def test_dcu_sends_trip_control_frame(meter):
    """DCU 的遥控动作必须变成**一帧真的 645 控制命令**发到表上。

    为什么这条不能省：`dcu_node_execute_control()` 原来只派发一个领域事件就返回 0，
    总线上一个字节都没有 —— 上层（698 主站）会以为继电器动了，现场什么都没发生。
    只验"事件发了"是假绿：事件是内部记账，不是命令出门。

    判据看台架收到的帧本身（标准 7.13）：
      - 控制码 1CH
      - N1 = 1AH（跳闸）
      - 密码首字节 02H（不带安全认证，明文）
    """
    meter.connect()
    meter.read_all()  # 让这一轮有设备级读数

    controls = _bus_controls()
    assert controls, (
        "台架一帧 645 控制命令都没收到（/tmp/rs485_*.sock）· "
        "看 product.out 里有没有 '645 控制帧已下发'："
        "没有就是聚合根没下发，有就是台架解析失败"
    )

    last = controls[-1]
    assert last["cmd"] == 0x1A, f"N1 应为 1AH（跳闸），台架收到 {last['cmd']:#04x}"
    assert last["meter"] == METER_ADDRESS, (
        f"命令发给了 {last['meter']}，台架表是 {METER_ADDRESS}"
    )

    frame = bytes.fromhex(last["frame_hex"])
    assert frame[0] != 0xFE or frame.lstrip(b"\xfe")[0] == 0x68, "前导符之后必须是帧起始符 68H"
    body = frame.lstrip(b"\xfe")
    assert body[8] == 0x1C, f"控制码应为 1CH，收到 {body[8]:#04x}"
    data = bytes((b - 0x33) & 0xFF for b in body[10:])
    assert data[0] == 0x02, f"密码首字节应为密级 02H（明文），收到 {data[0]:#04x}"
