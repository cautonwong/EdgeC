"""控制写必须落到点位真正所在的南向从站,没有路由的写必须被拒绝。

## 三条断言

1. **正的写落到 unit 3**:`meter.write("有功功率", 42.0)` 之后,台架必须在
   unit 3、地址 12 上收到一帧 FC16,两个寄存器是 [0, 42](ABCD 字序)。
2. **不许写到 unit 1**:同一地址 12 不许出现在 unit 1 的写里。总线上有两个
   从站时,写错从站是动了另一块表 —— 现场是误动作,不是少一条数据。
3. **没有路由的写必须被拒**:`频率`(产品 tag 1004)没有 poll task,北向
   Modbus 从站必须回异常应答,主站侧抛 `SlaveExceptionResponse`,南向一帧
   写都不许出现。

## 为什么只看写应答是假绿

Modbus FC06 的应答就是请求帧的逐字节回显,FC16 的应答只回起始地址与寄存器数。
两者都不包含"设备真的执行了"的信息。产品完全可以把写吞掉、或者写到别的从站,
回显照样一模一样。判据必须落在台架记录到的**下一跳写**(`bus.json` 的 `writes`),
那才是产品真正发到 RS485 上的东西。

还原:这条用例改的是 unit 3 地址 12/13 的寄存器(产品 tag 1003 的影子),
写完必须写回 0,别把 42 留给后面的用例。
"""
import pytest

from bench import (ADDRESS_FREQUENCY, ADDRESS_POWER, SLAVE_ID, SLAVE_POWER,
                   bus_seen)
from testplatform.cases import case
from testplatform.protocol.modbus_rtu import SlaveExceptionResponse


@pytest.fixture(scope="module", autouse=True)
def _open_evidence(evidence_session):
    """保住证据会话:没有它 sessionfinish 不写 run-scope.json。"""
    return evidence_session


def _writes() -> list:
    return bus_seen().get("writes") or []


@case(id="I_00_00_04_000001", clause="IEMS-CTRL-1", tags=["验收", "控制", "路由"])
def test_control_write_routes_to_the_point_slave(meter):
    """正写路由、不许写错从站、无路由的写必须被拒。"""
    # dut 的连接由用例自己建:组合根只连 role: support 的设备。单独跑本任务时
    # 没有别的用例替它连,少了这一步 write 会炸在 transport 的 _sock=None 上。
    meter.connect()
    try:
        before = len(_writes())
        meter.write("有功功率", 42.0)
        new = _writes()[before:]

        routed = [w for w in new
                  if w.get("unit") == SLAVE_POWER and w["address"] == ADDRESS_POWER]
        assert routed, (
            f"台架没收到 unit {SLAVE_POWER} 地址 {ADDRESS_POWER} 的南向写;"
            f"这次新增的写是 {new} —— 产品把 tag 1003 路由到了别的从站/地址,"
            "或者根本没下发(北向写应答不能证明写生效)"
        )
        assert routed[-1]["values"] == [0, 42], (
            f"南向 FC16 的寄存器是 {routed[-1]['values']},期望 [0, 42]"
            "(ABCD 字序:高字在前;台架给的工程值是 42)"
        )

        wrong_slave = [w for w in new
                       if w.get("unit") == SLAVE_ID and w["address"] == ADDRESS_POWER]
        assert not wrong_slave, (
            f"同一笔写也发到了 unit {SLAVE_ID} 地址 {ADDRESS_POWER}: {wrong_slave} —— "
            "多从站总线上这是动了另一块表,现场是误动作"
        )

        before = len(_writes())
        with pytest.raises(SlaveExceptionResponse):
            meter.write("频率", 50.0)
        after = _writes()[before:]
        assert not after, (
            f"没有 poll task 的频率点(address {ADDRESS_FREQUENCY})还是产生了南向写: {after} "
            "—— 北向从站必须回异常应答,不能回显成功;产品也不许猜从站地址"
        )
    finally:
        meter.write("有功功率", 0.0)
