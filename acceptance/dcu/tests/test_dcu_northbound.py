"""DCU 北向（族 B，DL/T 698.45 从站）验收：主站能不能真的读到 DCU 的量。

## 为什么这个文件自己开 socket，而不走平台设备驱动

平台设备模型要求"一个规约一个驱动 + 一份点表"，698 主站驱动（T3b）还没写。
但"DCU 的 698 从站到底通不通"这件事**不依赖**那个驱动 —— 主站就是发一帧问、
收一帧答。所以这里直接用平台已就绪的 `testplatform.protocol.dlt698` 收发，
判据仍然是平台写的（不是产品自说自话）。

等 T3b 落地后，这个文件可以退役，换成 devices.yaml 里的一个 dut 设备。

## 判据

1. 电压 OAD（20000200）读回 231.7±0.5 —— 台架喂给 DCU 的就是这个值，
   所以这条同时验了"南向采集 → 聚合根 → 698 编码"整条链
2. 档案计数 OAD（60000200）读回 40 —— 台架档案就是 40 只表，类型是 long-unsigned
3. ACTION（OI 8000 方法 129 跳闸）必须回 **DAR=2 暂时失效**，不能回 0

第 3 条是刻意"验失败"的：参数结构（标准表 A.9 的 array<structure>）还没实现，
如果它回 0，主站会以为继电器动了 —— 那是最危险的假成功。
"""
import os
import socket

import pytest

from testplatform.cases import case
from testplatform.protocol import dlt698

TOLERANCE_V = 0.5
OAD_VOLTAGE_A = 0x20000200
OAD_METER_COUNT = 0x60000200
TIMEOUT_S = 3.0


def _north_sock() -> str:
    path = os.environ.get("DCU_NORTH_SOCK")
    if not path:
        pytest.skip("没有 DCU_NORTH_SOCK（北向从站路径由产品壳给）")
    if not os.path.exists(path):
        pytest.skip(f"北向从站还没绑上：{path}（产品没起来？）")
    return path


def _exchange(apdu: bytes) -> dlt698.Frame:
    """发一帧 APDU，收一帧并校验（长度域 / HCS / FCS 全查）。"""
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.settimeout(TIMEOUT_S)
    try:
        sock.connect(_north_sock())
        sock.sendall(dlt698.build_frame(apdu, sa=b"\x01"))
        data = sock.recv(1024)
    finally:
        sock.close()
    assert data, "主站发了请求，从站一个字节都没回"
    return dlt698.parse_frame(data)


@case(id="D_00_00_02_000001", clause="free: 主站能通过 698 GET 读到 DCU 的实时量", tags=["北向"])
@pytest.mark.contract
@pytest.mark.requires_capability("电压")
def test_master_reads_voltage_over_698(meter):
    """主站读 A 相电压：值必须等于台架喂给 DCU 的 231.7V。

    这条把「台架 → 南向 645 采集 → 聚合根 → 698 GET-Response」整条链串起来比对。
    只验"收到一帧"是假绿：类型标签写错时值会整体错位，而帧本身完全合法。

    先用平台自己的 645 驱动读一次台架：一是确认南向是好的（不然 698 读到怪值
    分不清是谁的问题），二是让这一轮有设备级读数 —— 平台规定"没验到设备就不算验过"。
    """
    meter.connect()
    bench_v = meter.read_all().readings["电压"]
    assert abs(bench_v - 231.7) <= TOLERANCE_V, f"台架读数就不对：{bench_v}"
    frame = _exchange(dlt698.get_request(0x01, OAD_VOLTAGE_A))
    res = dlt698.parse_get_response(frame.apdu)

    assert res.oad == OAD_VOLTAGE_A, f"回的是别的 OAD：{res.oad:#010x}"
    scalars = res.scalars()
    assert scalars, "GET-Response 里没有数据段"
    tag, raw = scalars[0]
    assert tag == 0x17, f"电压应当是 float32(0x17)，回的是 {tag:#04x}"

    import struct

    value = struct.unpack(">f", raw)[0]
    assert abs(value - 231.7) <= TOLERANCE_V, (
        f"698 读回 {value}，台架给的是 231.7±{TOLERANCE_V} —— "
        "链路里某一环改了值（量纲、类型标签、或者根本不是同一个 OAD）"
    )


@case(id="D_00_00_02_000002", clause="free: 698 应答的 A-XDR 类型标签必须与点位类型一致", tags=["北向"])
@pytest.mark.contract
def test_master_reads_archive_count_as_long_unsigned():
    """档案计数：值 40，类型必须是 long-unsigned(0x12)。

    单独验这条是因为它和电压的**类型不同**：电压 float32、计数 long-unsigned。
    从站如果一律按同一个类型编，主站解出来就是垃圾值，而帧校验还是过的。
    """
    frame = _exchange(dlt698.get_request(0x01, OAD_METER_COUNT))
    res = dlt698.parse_get_response(frame.apdu)

    tag, raw = res.scalars()[0]
    assert tag == 0x12, f"档案计数应当是 long-unsigned(0x12)，回的是 {tag:#04x}"
    assert int.from_bytes(raw, "big") == 40, (
        f"档案计数 {int.from_bytes(raw, 'big')}，台架档案是 40 只表"
    )


@case(id="D_00_00_02_000003", clause="free: 未实现的操作必须回失败 DAR，不许假成功", tags=["北向"])
@pytest.mark.contract
def test_action_returns_failure_dar_not_fake_success():
    """跳闸（OI 8000 方法 129）当前**未实现**，必须回 DAR=2（暂时失效）。

    为什么"验失败"也是验收：标准表 A.9 规定方法 129 的参数是
    `array<structure{继电器 OAD, 告警延时, 限电时间, 自动合闸}>`，我们还没解它。
    这时候如果从站回 DAR=0（成功），主站会认为继电器已经动了 —— 那是比"不支持"
    危险得多的错，因为没人会去复查。
    """
    # 参数段随便给一个结构（只为触发方法分派），重点看 DAR
    apdu = dlt698.action_request(0x07, 0x8000, 129, param=bytes.fromhex("01 01 00 00 00"))
    frame = _exchange(apdu)
    res = dlt698.parse_action_response(frame.apdu)

    assert res.oi == 0x8000 and res.method == 129, f"回的操作不是 OI8000/129：{res}"
    assert not res.ok, "跳闸还没实现，却回了成功（DAR=0）—— 这是假成功"
    assert res.dar == 2, f"期望 DAR=2（暂时失效），回的是 {res.dar}（{res.error}）"
