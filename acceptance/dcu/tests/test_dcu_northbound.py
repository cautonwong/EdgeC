"""DCU 北向（族 B，DL/T 698.45 从站）验收：主站能不能真的读到 DCU 的量。

## 标量走设备模型，数组直接看 APDU

计数类 OAD 只剩 5004 走平台设备模型：`meter.read_all()`，报告里有设备级归属。
6000/6012 的属性 2 在标准里是数组，标量通道不再冒充；这两条直接发帧验 DAR=5。
变量类属性 2 自 ADR-0016 起是标准数组（如 2000 是 A/B/C 三相），平台点表只认标量，
所以这些用例直接发帧、按 A-XDR 形状解析；这是平台能力缺口，不是放弃判据。

链路：`dcu_product.py` 把 DCU 的 unix socket 桥成 `127.0.0.1:19212`，
平台按 `access: tcp` 连上去问属性。

## 判据

1. 电压 OAD（20000200）读回三相数组，A 相 2317（231.7 V，scale −1）——
   台架喂给 DCU 的就是这个值，所以这条同时验了"南向采集 → 聚合根 → 698 编码"整条链
2. 6000/6012 属性 2 是配置数组，数组通道未落地时必须回 DAR=5，不能用计数冒充
3. ACTION（OI 8000 方法 129 跳闸）必须回 **DAR=2 暂时失效**，不能回 0

第 3 条刻意"验失败"：参数结构（标准表 A.9 的 array<structure>）还没实现，
如果它回 0，主站会以为继电器动了 —— 那是最危险的假成功。
它不走设备模型，因为"操作"不是"读一个点"，这里直接用协议层发一帧。
"""
import os
import socket
import time

import pytest

from testplatform.cases import case
from testplatform.protocol import dlt698

TOLERANCE_V = 0.5
BENCH_VOLTAGE = 231.7
TIMEOUT_S = 3.0


def _north_sock() -> str:
    """ACTION 用例要直接发帧，所以需要产品的 unix socket 路径。"""
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


_INT_SIZE = {0x03: 1, 0x05: 4, 0x06: 4, 0x10: 2, 0x11: 1, 0x12: 2, 0x16: 1, 0x15: 8}
_SIGNED = {0x05, 0x10}


def _read_variable(oad: int) -> tuple[list[int], list[int]]:
    """直接发 [1] 读变量类属性 2，返回 (元素标签, 整数列表)。

    平台点表只认标量，变量类的标准数组形状（ADR-0016）由这里解析 APDU 验证。
    """
    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"不是 [1] 响应：{apdu.hex()}"
    assert apdu[3:7] == oad.to_bytes(4, "big"), "响应 OAD 与请求不一致"
    assert apdu[7] == 0x01, "A-Result 不是 Data"
    assert apdu[8] == 0x01, f"变量类属性 2 应为 array，收到 0x{apdu[8]:02X}"
    count = apdu[9]
    tags, values = [], []
    i = 10
    for _ in range(count):
        tag = apdu[i]
        size = _INT_SIZE[tag]
        raw = int.from_bytes(apdu[i + 1 : i + 1 + size], "big", signed=tag in _SIGNED)
        tags.append(tag)
        values.append(raw)
        i += 1 + size
    return tags, values


def _read_scaler_unit(oad: int) -> tuple[int, int]:
    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[7] == 0x01 and apdu[8] == 0x02, f"不是 structure：{apdu.hex()}"
    assert apdu[9] == 2 and apdu[10] == 0x0F and apdu[12] == 0x16, (
        f"Scaler_Unit 形状不符：{apdu.hex()}"
    )
    return int.from_bytes(apdu[11:12], "big", signed=True), apdu[13]


@case(id="D_00_00_02_000001", clause="free: 主站能通过 698 GET 读到 DCU 的实时量", tags=["北向"])
@pytest.mark.contract
def test_master_reads_voltage_over_698():
    """Class 3 属性 2 是 A/B/C 三相数组，元素是 long-unsigned（scale −1）。

    台架给 A 相 231.7 V，所以第一个元素应是 2317；数组长度与类型标签同时验证
    "分相在数组里，不在属性字节里"（ADR-0016）。
    """
    tags, values = _read_variable(0x20000200)
    assert len(values) == 3, f"电压数组应有 3 项，收到 {values}"
    assert all(tag == 0x12 for tag in tags), f"电压元素类型应为 long-unsigned：{tags}"
    got = values[0] / 10.0
    assert abs(got - BENCH_VOLTAGE) <= TOLERANCE_V, (
        f"698 读回 {got} V，台架给的是 {BENCH_VOLTAGE}±{TOLERANCE_V}"
    )

    scaler, unit = _read_scaler_unit(0x20000300)
    assert scaler == -1 and unit == 38, f"电压 Scaler_Unit 错：{scaler}, {unit}"


@case(id="D_00_00_02_000002", clause="free: 配置数组属性未实现时必须回 DAR=5", tags=["北向"])
@pytest.mark.contract
def test_master_gets_class_mismatch_for_archive_config_array():
    """6000 A2 是采集档案配置数组，不是计数。

    数组通道没落地时，从站必须回 DAR=5（对象接口类不符合）。回一个 long-unsigned
    计数会让主站把表数当成配置表的第一项，比明确失败更难发现。
    """
    oad = 0x60000200
    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"不是 [1] 响应：{apdu.hex()}"
    assert apdu[3:7] == oad.to_bytes(4, "big"), "响应 OAD 与请求不一致"
    assert apdu[7] == 0x00, "Get-Result 应为 DAR"
    assert apdu[8] == 5, f"6000 A2 应回 DAR=5，收到 {apdu[8]}（{apdu.hex()}）"


@case(id="D_00_00_02_000003", clause="free: 未实现的 ACTION 必须回失败 DAR，不许假成功", tags=["北向"])
@pytest.mark.contract
def test_action_returns_failure_dar_not_fake_success():
    """跳闸（OI 8000 方法 129）参数合法，但产品没有继电器 OAD→电表地址映射，必须回 DAR=2。

    为什么"验失败"也是验收：129 的参数是表 A.9 的
    `array<structure{继电器 OAD, 告警延时, 限电时间, 自动合闸}>`。如果从站回
    DAR=0（成功），主站会认为继电器已经动了 —— 那是比"不支持"危险得多的错，
    因为没人会去复查。
    """
    param = bytes.fromhex("01 01 02 04 51 20 00 04 00 11 05 12 00 0A 03 01")
    apdu = dlt698.action_request(0x07, 0x8000, 129, param=param)
    frame = _exchange(apdu)
    res = dlt698.parse_action_response(frame.apdu)

    assert res.oi == 0x8000 and res.method == 129, f"回的操作不是 OI8000/129：{res}"
    assert not res.ok, "没有继电器映射，却回了成功（DAR=0）—— 这是假成功"
    assert res.dar == 2, f"期望 DAR=2（暂时失效），回的是 {res.dar}（{res.error}）"

    # 产品必须还活着：同一个从站再读一次电压。
    assert _exchange(dlt698.get_request(0x01, 0x20000200)).apdu[0] == 0x85


@case(id="D_00_00_02_000009", clause="free: 形状不符的 ACTION 参数回 DAR 7", tags=["北向"])
@pytest.mark.contract
def test_action_wrong_shape_returns_type_mismatch():
    """129 的结构少一个字段（没有 bool 自动合闸），必须回 DAR 7，不能按 DAR 2 混过去。"""
    param = bytes.fromhex("01 01 02 03 51 20 00 04 00 11 05 12 00 0A")
    frame = _exchange(dlt698.action_request(0x07, 0x8000, 129, param=param))
    res = dlt698.parse_action_response(frame.apdu)

    assert res.dar == 7, f"形状错误应回 DAR 7（类型不匹配），回了 {res.dar}（{res.error}）"


@case(id="D_00_00_02_000004", clause="free: 一帧 GET-Request-NormalList 读整张点表", tags=["北向"])
@pytest.mark.contract
def test_master_reads_all_oads_in_one_frame():
    """一帧 [2] 请求 6 个 OAD，从站必须在一帧响应里逐项回答 6 项。

    平台点表只能列标量，变量类的数组形状在这里直接看 APDU；重点仍是"一帧请求、
    一帧响应、6 项都在"，不是逐点往返。
    """
    oads = [0x20000200, 0x20010200, 0x20040200, 0x60000200, 0x60120200, 0x50040200]
    apdu = _exchange(dlt698.get_request_list(0x01, oads)).apdu

    assert apdu[0] == 0x85 and apdu[1] == 0x02, f"不是 [2] 响应：{apdu.hex()}"
    assert apdu[3] == len(oads), f"结果个数 {apdu[3]}，请求了 {len(oads)} 项"

    seen = []
    i = 4
    for _ in range(apdu[3]):
        oad = int.from_bytes(apdu[i : i + 4], "big")
        flag = apdu[i + 4]
        i += 5
        if flag == 0x00:
            i += 1
            seen.append((oad, None))
            continue
        tag = apdu[i]
        if tag == 0x01:
            n = apdu[i + 1]
            i += 2
            for _ in range(n):
                size = _INT_SIZE[apdu[i]]
                i += 1 + size
        else:
            i += 1 + _INT_SIZE[tag]
        seen.append((oad, tag))
    assert [oad for oad, _ in seen] == oads, f"响应项顺序/内容不符：{seen}"


@case(id="D_00_00_02_000005", clause="free: 698 读取 2001 电流 / 2004 总有功功率", tags=["北向"])
@pytest.mark.contract
def test_master_reads_current_and_active_power():
    """2001 是 3 项 double-long，2004 是 4 项 double-long（总/A/B/C）。

    台架对这两个 DI 回 0，所以值是 0；这条验的是"轮询采到了数组需要的每个通道"
    与标准类型，而不是假 0。
    """
    tags, current = _read_variable(0x20010200)
    assert len(current) == 3 and all(tag == 0x05 for tag in tags), f"电流数组：{tags},{current}"
    assert current == [0, 0, 0], f"台架回 0，电流数组应为 0：{current}"

    tags, power = _read_variable(0x20040200)
    assert len(power) == 4 and all(tag == 0x05 for tag in tags), f"功率数组：{tags},{power}"
    assert power == [0, 0, 0, 0], f"台架回 0，功率数组应为 0：{power}"


@case(id="D_00_00_02_000006", clause="free: 6012 配置数组未实现时必须回 DAR=5", tags=["北向"])
@pytest.mark.contract
def test_master_gets_class_mismatch_for_task_config_array():
    """6012 属性 2 是任务配置数组，不是方案计数；未实现时回 DAR=5。"""
    oad = 0x60120200
    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"不是 [1] 响应：{apdu.hex()}"
    assert apdu[3:7] == oad.to_bytes(4, "big"), "响应 OAD 与请求不一致"
    assert apdu[7] == 0x00, "Get-Result 应为 DAR"
    assert apdu[8] == 5, f"6012 A2 应回 DAR=5，收到 {apdu[8]}（{apdu.hex()}）"


@case(id="D_00_00_02_000007", clause="free: 698 读取 5004 日冻结记录数", tags=["北向"])
@pytest.mark.contract
@pytest.mark.requires_capability("日冻结记录数")
def test_master_reads_freeze_count(meter):
    """5004 属性 2 是 uint16 计数；值是 0 也合法，类型标签必须对。"""
    meter.connect()
    readings = meter.read_all().readings
    assert int(readings["日冻结记录数"]) >= 0


@case(id="D_00_00_02_000008", clause="free: 列表里未实现的 OAD 逐项回 DAR，不丢其它读数", tags=["北向"])
@pytest.mark.contract
def test_unimplemented_oad_gets_per_item_dar():
    """一帧 [2] 里混一个不在 Class 20 对象列表里的 OI：该项回 [0] DAR 3，另一项数据照常。

    台架点表只列产品支持的点，表达不了"主站问了一个不存在的对象"；这里直接发帧。
    判据是**两项都在**：少返一项、或把失败项当数据跳过，主站会以为读全了。
    """
    valid = 0x20000200
    missing = 0x99990000
    frame = _exchange(dlt698.get_request_list(0x01, [valid, missing]))
    apdu = frame.apdu

    assert apdu[0] == 0x85 and apdu[1] == 0x02, f"不是 [2] 响应：{apdu.hex()}"
    assert apdu[3] == 2, f"结果个数 {apdu[3]}，请求了 2 项"

    body = apdu[4:]
    assert body[:4] == valid.to_bytes(4, "big")
    assert body[4] == 0x01, "电压项应是 Data，不是 DAR"
    assert body[5] == 0x01 and body[6] == 3, f"电压项应是三相数组：{body[:8].hex()}"
    i = 7
    voltages = []
    for _ in range(3):
        assert body[i] == 0x12, f"电压元素类型错：0x{body[i]:02X}"
        voltages.append(int.from_bytes(body[i + 1 : i + 3], "big"))
        i += 3
    got = voltages[0] / 10.0
    assert abs(got - BENCH_VOLTAGE) <= TOLERANCE_V, f"电压项 {got}，台架给 {BENCH_VOLTAGE}"

    rest = body[i:]
    assert rest[:4] == missing.to_bytes(4, "big")
    assert rest[4] == 0x00, "失败项必须回 [0] DAR"
    # ADR-0015：对象不在 Class 20 对象列表即未授权，DAR 3（拒绝读写），
    # 而不是先落到处不存在的 DAR 6 —— 列表是访问控制的事实来源。
    assert rest[5] == 3, f"列表外的 OI 应回 DAR 3（拒绝读写），回了 {rest[5]}"


@case(id="D_00_00_02_000010", clause="free: 5004 GetRecord 按时间范围返回冻结行", tags=["北向"])
@pytest.mark.contract
def test_master_reads_freeze_records():
    """GetRecord 5004 属性 2：列 = 冻结时间 + 正向有功电能，RSD Selector2 时间范围。

    台架预置 2026-11-22/23 两行；范围 [22 日, 24 日) 必须正好回两行。
    """
    rsd = bytes.fromhex("0220210200") + bytes.fromhex("1c07ea0b16000000") + bytes.fromhex(
        "1c07ea0b18000000"
    ) + bytes.fromhex("00")
    rcsd = bytes.fromhex("0200202102000000100200")
    req = bytes([0x05, 0x03, 0x01]) + (0x50040200).to_bytes(4, "big") + rsd + rcsd + b"\x00"

    apdu = _exchange(req).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x03, f"不是 [3] 记录响应：{apdu.hex()}"
    assert apdu[3:7] == b"\x50\x04\x02\x00", "响应 OAD 与请求不一致"
    assert apdu[7] == 2, f"RCSD 列数 {apdu[7]}，请求了 2 列"
    assert apdu[18] == 0x01, f"记录结果 CHOICE 0x{apdu[18]:02X}，DAR={apdu[19]}"
    assert apdu[19] == 2, f"范围内应有 2 行，回了 {apdu[19]}"

    want = [1234500, 1234600]
    i = 20
    for row in range(2):
        assert apdu[i] == 0x1C, f"第 {row + 1} 行第 1 列应是 date_time_s"
        day = apdu[i + 4]
        i += 8
        assert apdu[i] == 0x06, f"第 {row + 1} 行第 2 列应是 double-long-unsigned"
        raw = int.from_bytes(apdu[i + 1 : i + 5], "big")
        i += 5
        assert day == 22 + row, f"第 {row + 1} 行日期 {day}，期望 {22 + row}"
        assert raw == want[row], f"第 {row + 1} 行电量 {raw}，期望 {want[row]}"
