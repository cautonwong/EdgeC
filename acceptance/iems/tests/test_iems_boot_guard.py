"""启动闸门:配了南向却连不上,必须启动失败,不许进重连循环。

为什么单独一层:这条验的是"起不来",产品起来之后才有 REST/镜像可断言,
所以只能挂在任务级,用 smoke(不参与设备验收)。
台架真值来自产品自己退出 + stderr,不是产品对外宣称。
"""
import json
import time
from pathlib import Path

import pytest

from bench import RUN_DIR
from testplatform.cases import case

BOOT = Path(RUN_DIR) / "boot.json"


@pytest.fixture(scope="module", autouse=True)
def _open_evidence(evidence_session):
    # 冒烟用例不碰 composition_root,不主动开会话就不会有 run-scope.json,
    # 归因会把整轮判成"范围缺失"。
    return evidence_session


@case(id="I_00_00_01_000010", clause="IEMS-SVC-2", tags=["冒烟"])
def test_iems_refuses_to_boot_without_a_southbound_bus():
    # 产品要先起、跑到底、退出,壳才写得出事实;这段等待本身就是被测行为的一部分
    # (等 20 秒还没退出 = 它在重连,不是启动失败)。
    deadline = time.time() + 25.0
    while time.time() < deadline and not BOOT.is_file():
        time.sleep(0.5)
    assert BOOT.is_file(), f"25 秒内产品既没退出、壳也没写出启动事实: {BOOT}"
    got = json.loads(BOOT.read_text(encoding="utf-8"))

    assert got["code"] not in (None, 0), (
        "产品在没有任何南向总线的情况下没有退出 —— 又回到重连循环了: "
        f"{got['stderr_tail']}"
    )
    assert got["fatal"], (
        "退出码非 0,但没有那条致命日志,说明是别的原因退的,不能算启动闸门生效: "
        f"{got['stderr_tail']}"
    )
