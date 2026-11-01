"""iems 北向验收：台架给的值，必须原样出现在云端，而且名字对得上。

## 判据三步，顺序有意义

1. **上线帧发过一次** —— 主站不知道节点上线，后面收到多少数据都不算数
2. **metric 名来自产品点位表**（`VOLTAGE_PHASE_A`），不是 `tag_<数字>`
3. **北向读数 == 台架给的值**（±0.5V）

第 3 步是这条用例的价值所在：它把「台架 → 南向采集 → datahub → 北向编码 →
broker → 平台解码」整条链串起来比对。只验"收到了消息"是假绿——
名字错了、值错了、或者是别的点位，都能收到消息。

## 为什么名字单独一条判据

名字错是最难发现的失败：数据在流、值也对，但主站把它归到别的点位下面。
现场表现是"某个点位一直不更新"，而它其实一直在发，只是叫别的名字。

## 前提

`NB_E2E=1` 且本机有 mosquitto；否则 `northbound_probe` 会跳过本文件（测试无效，
不是产品不合格）。
"""
import os
import time

import pytest

from bench import VOLTAGE, bus_seen
from testplatform.cases import case

TOLERANCE_V = 0.5
WAIT_S = 30


@pytest.fixture(scope="module", autouse=True)
def _open_evidence(evidence_session):
    """保住证据会话：没有它 sessionfinish 不写 run-scope.json。"""
    return evidence_session


def _points_within(probe, timeout: float = WAIT_S) -> dict:
    """等到至少一个可解码点位出现（产品按轮询周期推送，不是立刻）。"""
    deadline = time.time() + timeout
    while time.time() < deadline:
        points = dict(probe.points())
        if points:
            return points
        time.sleep(0.5)
    pytest.fail(
        f"{timeout}s 内没收到任何可解码的北向数据帧；"
        f"实际收到 {len(probe.messages)} 条（协议与平台解码器不匹配？）"
    )


@case(id="I_00_00_03_000001", clause="free: 北向报文必须挂在配置的节点身份下", tags=["验收", "北向"])
@pytest.mark.contract
def test_northbound_publishes_under_configured_node(northbound_probe):
    """主题里必须有节点身份：主站按主题区分设备，身份错了数据就挂到别人名下。

    **这里不验上线帧**：观察器在 pytest 启动时才订阅，而产品已经被 run_task 拉起，
    BIRTH 每会话只发一次、早就过去了。重连重发 BIRTH 属于 E3，需要"重启产品"
    的场景，当前一个任务只拉一次产品，表达不了。见 acceptance/northbound/README.md。
    """
    _points_within(northbound_probe)

    node = os.environ.get("MQTT_NODE_ID", "IEMS_NB_E2E")
    topics = [m.topic for m in northbound_probe.messages]
    assert any(node in t for t in topics), (
        f"没有一条主题含节点身份 {node!r}；收到 {topics[:5]} —— "
        "身份来自 env，产品壳不转发就只剩默认值"
    )


@case(id="I_00_00_03_000002", clause="free: 北向 metric 名必须来自产品点位表", tags=["验收", "北向"])
@pytest.mark.contract
def test_northbound_metric_name_comes_from_point_table(northbound_probe):
    """名字必须是人能读的点位名，不是 tag_1001。"""
    points = _points_within(northbound_probe)

    assert "VOLTAGE_PHASE_A" in points, (
        f"没有 VOLTAGE_PHASE_A；收到的点位名是 {sorted(points)} —— "
        "名字错时数据在流、值也对，但主站归到别的点位下，现场表现为"
        "'某个点位一直不更新'"
    )
    fallback = [name for name in points if name.startswith("tag_")]
    assert not fallback, f"出现了回落名 {fallback}：北向取数端口没注入，或 name() 返回了 NULL"


@case(id="I_00_00_03_000003", clause="free: 北向读数必须等于台架给的值", tags=["验收", "北向"])
@pytest.mark.contract
def test_northbound_value_matches_bench(northbound_probe):
    """把整条链串起来比对：台架给 231.7，云端就该看到 231.7。"""
    points = _points_within(northbound_probe)

    assert "VOLTAGE_PHASE_A" in points, f"没有电压点位；收到的是 {sorted(points)}"
    got = points["VOLTAGE_PHASE_A"]
    assert abs(got - VOLTAGE) <= TOLERANCE_V, (
        f"北向报的电压 {got} 与台架给的 {VOLTAGE} 差超过 {TOLERANCE_V}V —— "
        "链路里某一环把值改了（量纲、缩放、或者根本不是同一个点位）"
    )


@case(id="I_00_00_03_000004", clause="free: 主站写北向必须真的下到南向设备", tags=["验收", "北向"])
@pytest.mark.contract
@pytest.mark.requires_capability("电压")
def test_master_write_reaches_the_southbound_device(meter):
    """闭环的**下行**那一段：平台 → 北向从站 → 产品 → 南向设备。

    为什么必须断言"台架记到了写"，而不是只看写应答：
    Modbus 0x06 的应答就是**回显**，设备什么都没做也会回一个一模一样的帧。
    只看应答的话，"产品把写吞了"和"产品写下去了"长得完全一样 —— 最危险的假成功。

    写的是**镜像寄存器本身**（地址 0），不是业务设定值：真正的设定值要产品先给出
    "名字 → 聚合根方法"的映射，那一步还没做。这条用例验的是通路，不是业务语义。

    写完**必须还原**：这个寄存器就是台架读数，留着 220.0 会让别的用例看到被改过的值。
    """
    target_v = 220.0  # 与台架缺省的 231.7 明显不同，一眼能看出写生效了

    try:
        readback = meter.write("电压", target_v)
        assert abs(readback - target_v) <= TOLERANCE_V, (
            f"回读 {readback} 与写下去的 {target_v} 不一致 —— "
            "产品收下了写，但没有下发到南向总线（或量纲被改了）"
        )

        writes = bus_seen().get("writes") or []
        assert writes, (
            "南向台架一帧写都没收到：北向从站只回了回显，iems 的 accept 没把写转到南向"
            "（看 product.out 的 'IEMS north write' 日志）"
        )
        got = writes[-1]
        assert got["address"] == 0, f"写到了错误的寄存器：{got}（期望 address=0 的电压）"
        assert got["value"] == int(round(target_v * 10)), (
            f"台架收到的原始值 {got['value']} 与期望 {int(round(target_v * 10))} 不一致 —— "
            "缩放被用了两次（raw→eng 与 eng→raw 只该各用一次）"
        )
    finally:
        # 还原台架读数：这条用例改的是别的用例要读的那个寄存器
        meter.write("电压", VOLTAGE)


def _node_id() -> str:
    return os.environ.get("MQTT_NODE_ID", "IEMS_NB_E2E")


def _ncmd_topic() -> str:
    return f"edge/{_node_id()}/NCMD"


def _wait_ack(probe, name: str, timeout: float = 15.0) -> float:
    """等 `<name>.ack` 这条回执，返回它的值（1 成功 / 0 失败）。"""
    key = f"{name}.ack"
    deadline = time.time() + timeout
    while time.time() < deadline:
        points = dict(probe.points())
        if key in points:
            return points[key]
        time.sleep(0.3)
    pytest.fail(
        f"{timeout}s 内没收到回执 {key}；收到的点位是 {sorted(dict(probe.points()))} —— "
        "产品可能执行了但没发回执，主站就分不清'拒绝了'和'没收到'"
    )


@case(id="I_00_00_03_000005", clause="free: 主站下发必须执行并回执", tags=["验收", "北向"])
@pytest.mark.contract
@pytest.mark.requires_capability("电压")
def test_mqtt_downlink_executes_and_acks(northbound_probe, northbound_publisher):
    """MQTT 下行闭环：主站发命令 → 产品执行 → **回执上行**。

    判据三层，缺一层就可能是假绿：
    1. 回执值为 1（产品自己说执行了）
    2. 南向台架**真的记到那一帧写**（产品说执行了 ≠ 真执行了）
    3. 回读一致

    只信 1 是最常见的错：产品完全可以"回个成功然后什么都不做"。
    """
    target_v = 210.0

    try:
        northbound_publisher.publish(
            _ncmd_topic(), b'{"p":"VOLTAGE_PHASE_A","v":210.0}', qos=1
        )
        ack = _wait_ack(northbound_probe, "VOLTAGE_PHASE_A")
        assert ack == 1.0, (
            f"回执是 {ack}（0=失败）—— 看 product.out 的 'IEMS downlink' 日志，"
            "多半是名字没映射到点位，或控制端口拒绝了"
        )

        writes = bus_seen().get("writes") or []
        assert writes, "回执说成功了，但南向台架一帧写都没收到 —— 这就是假成功"
        assert writes[-1]["value"] == int(round(target_v * 10)), (
            f"台架收到 {writes[-1]}，期望原始值 {int(round(target_v * 10))}"
        )
    finally:
        # 还原台架读数，别把 210V 留给别的用例
        northbound_publisher.publish(
            _ncmd_topic(), f'{{"p":"VOLTAGE_PHASE_A","v":{VOLTAGE}}}'.encode(), qos=1
        )
        _wait_ack(northbound_probe, "VOLTAGE_PHASE_A")


@case(id="I_00_00_03_000006", clause="free: 无法执行的下行必须回失败回执", tags=["验收", "北向"])
@pytest.mark.contract
def test_unknown_point_gets_failure_ack(northbound_probe, northbound_publisher):
    """点位名不认识时，**必须回失败回执**，而不是静默丢弃。

    静默丢弃时主站只能等超时，把"设备不认识这个点"记成"设备没响应"——
    排查方向直接错一半。
    """
    northbound_publisher.publish(
        _ncmd_topic(), b'{"p":"NO_SUCH_POINT","v":1}', qos=1
    )
    ack = _wait_ack(northbound_probe, "NO_SUCH_POINT")
    assert ack == 0.0, f"不存在的点位回了成功回执（{ack}）—— 主站会以为真写下去了"
