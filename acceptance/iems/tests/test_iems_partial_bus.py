"""部分南向总线连不上时:iems 必须启动,而且必须在启动汇总行里说清缺哪一路。

为什么这条值钱:台架上 bus 0 有从站、bus 1(CAN)永远不存在。产品如果
"配了就必须全连上"会起不来(验收台架全挂);如果静默降级,运维看不到
缺了一路,数据少了一半还以为是规模问题。合同条款 IEMS-ROBUST-3 定的是
中间那条:能起,但要说。

判据:
  1. 产品起来了,REST 健康检查可用
  2. 启动汇总行存在,且写明 1/2 连上、缺的是 bus 1
"""
import time

import pytest

from bench import RUN_DIR, port_of, rest
from testplatform.cases import case

STARTUP_LOG = RUN_DIR / "iems.log"
BOOT_TIMEOUT_S = 30.0


def _wait_log() -> str:
    deadline = time.time() + BOOT_TIMEOUT_S
    while time.time() < deadline:
        try:
            text = STARTUP_LOG.read_text(encoding="utf-8", errors="replace")
        except FileNotFoundError:
            text = ""
        if "[IEMS-INIT] southbound:" in text:
            return text
        time.sleep(0.5)
    return text


@case(id="I_00_00_01_000040", clause="IEMS-ROBUST-3", tags=["冒烟"])
def test_partial_bus_boots_and_names_the_missing_bus():
    port = port_of()
    health = rest(port, "/api/v1/system/health")
    assert "error" not in health, (
        f"只有一路南向可用时产品没起来(台架上 bus 1/CAN 永远不存在): {health}"
    )

    text = _wait_log()
    summary = [ln.strip() for ln in text.splitlines() if "[IEMS-INIT] southbound:" in ln]
    assert summary, (
        f"启动汇总行没出现({STARTUP_LOG})—— 静默降级:运维看不出缺了一路。"
        "条款要求形如 [IEMS-INIT] southbound: 1/2 connected (bus 1 CAN_Main missing)"
    )
    line = summary[0]
    assert "1/2" in line, f"汇总行没说清连上几路: {line}"
    assert "bus 1" in line, f"汇总行没点名缺哪一路: {line}"


@pytest.fixture(scope="module", autouse=True)
def _open_evidence(evidence_session):
    # 不碰设备的冒烟任务也要证据会话,否则 run-scope.json 不落盘,归因判范围缺失。
    return evidence_session
