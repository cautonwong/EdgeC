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


def _latest_voltage(timeout: float = 8.0):
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
        "DCU 在 8 秒内没有向台架发出任何 DLT645 读请求(/tmp/rs485_22..31.sock)。"
        "落库的读数是 dcu_twin_source 合成的,不是从总线读来的。"
    )

    # 3. 落库值必须等于台架值。
    assert value is not None, "DCU 没有把台架表的电压写进 dcu_telemetry_latest"
    assert abs(value - expected) <= TOLERANCE_V, (
        f"DCU 落库 {value:.1f} V,台架读出来的是 {expected:.1f} V(容差 {TOLERANCE_V} V)"
    )
