"""DCU 北向（族 B，DL/T 698.45 从站）验收：主站能不能真的读到 DCU 的量。

## 数组/记录型属性直接看 APDU

6000/6012/5004 的属性 2 在标准里是数组（5004 是冻结数据数组），标量通道不再冒充；
6000 超缓冲回 DAR=2、5004 未落地回 DAR=5；6012 A2 已按 array<6013 单元> 返回。
5004 的冻结记录由 GetRecord 读取，单独按时间范围验。
设备级标量读数走 3000 属性 4（最大事件记录数），让北向任务有设备级归属。
变量类属性 2 自 ADR-0016 起是标准数组（如 2000 是 A/B/C 三相），平台点表只认标量，
所以这些用例直接发帧、按 A-XDR 形状解析；这是平台能力缺口，不是放弃判据。

链路：`dcu_product.py` 把 DCU 的 unix socket 桥成 `127.0.0.1:19212`，
平台按 `access: tcp` 连上去问属性。

## 判据

1. 电压 OAD（20000200）读回三相数组，A 相 2317（231.7 V，scale −1）——
   台架喂给 DCU 的就是这个值，所以这条同时验了"南向采集 → 聚合根 → 698 编码"整条链
2. 6000 属性 2 是配置数组：超出 A-XDR 缓冲时回 DAR=2（暂时失效），不能用计数冒充；
   6012 属性 2 是任务配置数组（首启播种默认任务）；5004 属性 2 未落地，回 DAR=5
3. ACTION（OI 8000 方法 129 跳闸）必须回 **DAR=2 暂时失效**，不能回 0
4. SET 3000 A11/A12 合法值回 DAR 0 且可读回；类型不符 DAR 7；未声明属性 DAR 5
5. Class 20：同一 socket 上预连接 → CONNECT → ACL → RELEASE 全链路
6. SET 4000 A2 date_time_s：DAR 0 必须伴随系统时钟真的改变，拒绝时 DAR 2/3 且时间不变

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

_AXDR_FIXED = {
    0x03: 1, 0x05: 4, 0x06: 4, 0x0F: 1, 0x10: 2, 0x11: 1, 0x12: 2, 0x14: 8,
    0x15: 8, 0x16: 1, 0x17: 4, 0x18: 8, 0x1B: 7, 0x1C: 7, 0x51: 4, 0x54: 3,
}


def _skip_axdr(data: bytes, i: int) -> int:
    tag = data[i]
    i += 1
    if tag == 0x00:
        return i
    if tag in (0x01, 0x02):
        count = data[i]
        i += 1
        for _ in range(count):
            i = _skip_axdr(data, i)
        return i
    if tag in (0x09, 0x0A):
        n = data[i]
        return i + 1 + n
    assert tag in _AXDR_FIXED, f"未知 A-XDR 标签 0x{tag:02X}（{data[max(0, i - 8):i + 8].hex()}）"
    return i + _AXDR_FIXED[tag]


def _parse_task_units(apdu: bytes, count_index: int) -> list[dict]:
    """解析 6012 A2 的 array<6013 结构>，只取验收关心的字段。"""
    count = apdu[count_index]
    i = count_index + 1
    units = []
    for _ in range(count):
        assert apdu[i] == 0x02 and apdu[i + 1] == 12, f"6013 单元结构形状错：{apdu[i:i + 4].hex()}"
        i += 2
        assert apdu[i] == 0x11
        task_id = apdu[i + 1]
        i += 2
        assert apdu[i] == 0x54, "TI 必须是 0x54 | unit | value_be16"
        unit = apdu[i + 1]
        value = int.from_bytes(apdu[i + 2 : i + 4], "big")
        i += 4
        scheme_type = apdu[i + 1]
        i += 2
        scheme_id = apdu[i + 1]
        i += 2
        i += 8  # 开始时间 date_time_s
        i += 8  # 结束时间
        i += 4  # 延时 TI
        i += 2  # 优先级 unsigned
        status = apdu[i + 1]
        i += 2
        i += 3  # 前脚本 long-unsigned
        i += 3  # 后脚本
        assert apdu[i] == 0x02 and apdu[i + 1] == 2
        period_type = apdu[i + 3]
        periods = apdu[i + 5]
        i += 6 + 4 * periods
        units.append(
            {
                "task_id": task_id,
                "unit": unit,
                "value": value,
                "scheme_type": scheme_type,
                "scheme_id": scheme_id,
                "status": status,
                "period_type": period_type,
            }
        )
    return units


def _read_task_units(oad: int = 0x60120200) -> list[dict]:
    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"不是 [1] 响应：{apdu.hex()}"
    assert apdu[7] == 0x01, f"6012 A2 应是 Data，不是 DAR：{apdu.hex()}"
    assert apdu[8] == 0x01, f"6012 A2 应是 array，收到 0x{apdu[8]:02X}"
    return _parse_task_units(apdu, 9)


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


def _recv_frame(sock: socket.socket) -> dlt698.Frame:
    buf = b""
    while True:
        need = dlt698.response_length(buf)
        if need is not None and len(buf) >= need:
            return dlt698.parse_frame(buf[:need])
        chunk = sock.recv(4096)
        assert chunk, "从站关闭了连接"
        buf += chunk


class _NorthSession:
    """One socket, many frames: CONNECT/RELEASE and the ACL live in the session."""

    def __init__(self):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.settimeout(TIMEOUT_S)
        self.sock.connect(_north_sock())

    def exchange(self, apdu: bytes) -> dlt698.Frame:
        self.sock.sendall(dlt698.build_frame(apdu, sa=b"\x01"))
        return _recv_frame(self.sock)

    def __enter__(self):
        return self

    def __exit__(self, *_exc):
        self.sock.close()


def _set_request(piid: int, oad: int, data: bytes) -> bytes:
    """SET-Request-Normal：`06 | 01 | PIID | OAD(4) | Data | 时间标签`。"""
    return bytes([0x06, 0x01, piid]) + oad.to_bytes(4, "big") + data + b"\x00"


def _connect_request(piid: int = 0x01) -> bytes:
    """CONNECT-Request：版本 0x0010，窗口 1，APDU 上限 1024，超时 60 s。"""
    body = (
        bytes([0x00, 0x10])
        + bytes(8)
        + bytes(16)
        + bytes([0x00, 0x04, 0x00, 0x04, 0x01])
        + bytes([0x04, 0x00])
        + bytes([0x00, 0x00, 0x00, 0x3C])
        + b"\x00"
    )
    return bytes([0x02, piid]) + body + b"\x00"


def _release_request(piid: int = 0x01) -> bytes:
    return bytes([0x03, piid, 0x00])


def _get_dar(apdu: bytes) -> int:
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"不是 [1] 响应：{apdu.hex()}"
    assert apdu[7] == 0x00, "Get-Result 应为 DAR"
    return apdu[8]


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


@case(id="D_00_00_02_000002", clause="free: 配置数组放不进缓冲时回 DAR=2，不许用计数冒充", tags=["北向"])
@pytest.mark.contract
def test_master_gets_honest_dar_for_oversized_archive_array():
    """6000 A2 是采集档案配置数组，不是计数。

    台架有 40 只表；每只表的 6001 结构编码后约 80 字节，整组超过 A-XDR 的 2048
    字节缓冲。此时从站必须回 DAR=2（暂时失效），不能回一个 long-unsigned 计数让
    主站把表数当成配置表的第一项。
    """
    oad = 0x60000200
    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"不是 [1] 响应：{apdu.hex()}"
    assert apdu[3:7] == oad.to_bytes(4, "big"), "响应 OAD 与请求不一致"
    assert apdu[7] == 0x00, "Get-Result 应为 DAR"
    assert apdu[8] == 2, f"超缓冲的 6000 A2 应回 DAR=2，收到 {apdu[8]}（{apdu.hex()}）"


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
        i = _skip_axdr(apdu, i)
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


@case(id="D_00_00_02_000006", clause="free: 6012 属性 2 是任务配置数组，首启播种默认任务", tags=["北向"])
@pytest.mark.contract
def test_master_gets_task_config_array():
    units = _read_task_units(0x60120200)
    assert units, "6012 A2 不应是空数组：首启必须播种默认任务"
    first = units[0]
    assert first["task_id"] == 1, f"默认任务 ID 应是 1，收到 {first}"
    assert (first["unit"], first["value"]) == (0, 5), f"默认周期应是 5 秒：{first}"
    assert first["scheme_type"] == 1 and first["scheme_id"] == 1, f"默认方案应是 1：{first}"


@case(id="D_00_00_02_000007", clause="free: 5004 属性 2 是冻结数据数组，Normal GET 未实现时回 DAR=5", tags=["北向"])
@pytest.mark.contract
def test_master_gets_class_mismatch_for_freeze_array():
    """5004 属性 2 是冻结数据数组而不是条数；Normal GET 未实现时必须回 DAR=5，记录走 GetRecord。"""
    oad = 0x50040200
    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"不是 [1] 响应：{apdu.hex()}"
    assert apdu[3:7] == oad.to_bytes(4, "big"), "响应 OAD 与请求不一致"
    assert apdu[7] == 0x00, "Get-Result 应为 DAR"
    assert apdu[8] == 5, f"5004 A2 应回 DAR=5，收到 {apdu[8]}（{apdu.hex()}）"


@case(id="D_00_00_02_000017", clause="free: 698 读取 3000 属性 4 最大事件记录数（设备级读数）", tags=["北向"])
@pytest.mark.contract
@pytest.mark.requires_capability("最大事件记录数")
def test_master_reads_event_max_records(meter):
    """设备级标量读数：3000 属性 4 最大事件记录数，让北向任务有设备级归属。"""
    meter.connect()
    readings = meter.read_all().readings
    assert int(readings["最大事件记录数"]) >= 0


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


def _archive_unit_param(address: str, seq: int = 0, port: int = 22) -> bytes:
    """ACTION 127 的参数：6001 A2 结构（配置序号 + Basic + Extended + Annex）。"""
    addr = address.encode()
    return (
        bytes([0x02, 0x04, 0x12, seq >> 8, seq & 0xFF])
        + bytes([0x02, 0x0A])
        + bytes([0x09, len(addr)]) + addr
        + bytes([0x16, 6])          # 波特率 9600
        + bytes([0x16, 2])          # DL/T645-2007
        + bytes([0x51, 0, 0, 0, port])  # 端口 OAD，末字节 = 通道
        + bytes([0x09, 0])          # 通信密码
        + bytes([0x11, 1])          # 费率个数
        + bytes([0x11, 0])          # 用户类型
        + bytes([0x16, 1])          # 接线方式：单相
        + bytes([0x12, 0x08, 0x98])  # 额定电压 220.0 V（换算 -1）
        + bytes([0x12, 0x01, 0xF4])  # 额定电流 50.0 A（换算 -1）
        + bytes([0x02, 0x04, 0x09, 0, 0x09, 0, 0x12, 0, 1, 0x12, 0, 1])
        + bytes([0x01, 0x00])       # 附属信息：空数组
    )


@case(id="D_00_00_02_000018", clause="free: 主站可以动态加表、读回配置单元、再删表", tags=["北向"])
@pytest.mark.contract
def test_master_adds_and_removes_archive_unit():
    """6000 方法 127 Add → 6001 A2 读回 → 方法 131 Delete → 6001 A2 回 DAR 6。

    台架预置 40 只表，新表的配置序号由产品分配为 41。判据是产品自己的档案
    （6001 A2）确实变了，而不是只看 ACTION 的 DAR。
    """
    address = "000000000041"
    add = dlt698.action_request(0x07, 0x6000, 127, param=_archive_unit_param(address))
    res = dlt698.parse_action_response(_exchange(add).apdu)
    assert res.ok, f"Add 应成功，回 DAR {res.dar}（{res.error}）"

    oad = 0x60010200 | 41
    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[7] == 0x01 and apdu[8] == 0x02, f"6001 A2 应是结构：{apdu.hex()}"
    assert address.encode() in apdu, f"配置单元地址不在响应里：{apdu.hex()}"

    seq = bytes([0x12, 0x00, 41])
    delete = dlt698.action_request(0x07, 0x6000, 131, param=seq)
    res = dlt698.parse_action_response(_exchange(delete).apdu)
    assert res.ok, f"Delete 应成功，回 DAR {res.dar}（{res.error}）"

    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[7] == 0x00 and apdu[8] == 6, f"删除后 6001 A2 应回 DAR 6：{apdu.hex()}"


def _task_unit_param(task_id: int, unit: int, value: int, status: int = 1,
                     period_type: int = 0) -> bytes:
    """6012 方法 127 的参数：array<6013 任务配置单元>。

    TI 是表 24 的扁平形式 0x54 | unit | value_be16，不是内层结构。
    """
    return (
        bytes([0x01, 0x01])          # array[1]
        + bytes([0x02, 12])         # structure[12]
        + bytes([0x11, task_id])    # 任务 ID
        + bytes([0x54, unit]) + value.to_bytes(2, "big")  # 执行频率 TI
        + bytes([0x16, 1])          # 方案类型 normal
        + bytes([0x11, 1])          # 方案编号 1
        + bytes([0x1C]) + bytes(7)  # 开始时间：无窗口
        + bytes([0x1C]) + bytes(7)  # 结束时间：无窗口
        + bytes([0x54, 0, 0, 0])    # 延时 TI 0
        + bytes([0x11, 0])          # 优先级 0
        + bytes([0x16, status])     # 状态
        + bytes([0x12, 0, 0])       # 前脚本 0
        + bytes([0x12, 0, 0])       # 后脚本 0
        + bytes([0x02, 2, 0x16, period_type, 0x01, 0])  # 时段：空表
    )


@case(id="D_00_00_02_000019", clause="free: 6012 ACTION 127 改周期 / 127 重下发 / 128 删除，A2 全程可回读", tags=["北向"])
@pytest.mark.contract
def test_master_changes_task_config_via_action():
    add = dlt698.action_request(0x07, 0x6012, 127, param=_task_unit_param(7, 0, 60))
    res = dlt698.parse_action_response(_exchange(add).apdu)
    assert res.ok, f"Add 应成功，回 DAR {res.dar}（{res.error}）"

    units = {u["task_id"]: u for u in _read_task_units()}
    assert 7 in units, f"Add 后 A2 应读到任务 7：{units}"
    assert (units[7]["unit"], units[7]["value"]) == (0, 60), f"周期应是 60 秒：{units[7]}"

    change = dlt698.action_request(0x07, 0x6012, 127, param=_task_unit_param(7, 0, 120))
    res = dlt698.parse_action_response(_exchange(change).apdu)
    assert res.ok, f"改周期应成功，回 DAR {res.dar}（{res.error}）"
    units = {u["task_id"]: u for u in _read_task_units()}
    assert (units[7]["unit"], units[7]["value"]) == (0, 120), f"周期应变为 120 秒：{units[7]}"

    delete = dlt698.action_request(0x07, 0x6012, 128, param=bytes([0x01, 0x01, 0x11, 7]))
    res = dlt698.parse_action_response(_exchange(delete).apdu)
    assert res.ok, f"Delete 应成功，回 DAR {res.dar}（{res.error}）"
    units = {u["task_id"]: u for u in _read_task_units()}
    assert 7 not in units, f"Delete 后 A2 不应再有任务 7：{units}"


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


@case(id="D_00_00_02_000011", clause="free: SET 3000 A11 枚举合法值回 DAR 0 且可读回", tags=["北向"])
@pytest.mark.contract
def test_set_event_report_flag_round_trip():
    oad = 0x30000B00
    res = _exchange(_set_request(0x01, oad, bytes([0x16, 0x01]))).apdu
    assert res[0] == 0x86 and res[1] == 0x01, f"不是 SET 响应：{res.hex()}"
    assert res[3:7] == oad.to_bytes(4, "big"), "响应 OAD 与请求不一致"
    assert res[7] == 0, f"合法 SET 应回 DAR 0，收到 {res[7]}"

    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[7] == 0x01 and apdu[8] == 0x16, f"A11 应是 enum：{apdu.hex()}"
    assert apdu[9] == 1, f"读回的上报标识是 {apdu[9]}，SET 写的是 1"


@case(id="D_00_00_02_000012", clause="free: SET 3000 A12 布尔合法值回 DAR 0 且可读回", tags=["北向"])
@pytest.mark.contract
def test_set_event_valid_flag_round_trip():
    oad = 0x30000C00
    res = _exchange(_set_request(0x01, oad, bytes([0x03, 0x00]))).apdu
    assert res[7] == 0, f"合法 SET 应回 DAR 0，收到 {res[7]}"

    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[7] == 0x01 and apdu[8] == 0x03, f"A12 应是 bool：{apdu.hex()}"
    assert apdu[9] == 0, f"读回的有效标识是 {apdu[9]}，SET 写的是 0"


@case(id="D_00_00_02_000013", clause="free: SET 类型不符必须回 DAR 7", tags=["北向"])
@pytest.mark.contract
def test_set_event_wrong_type_returns_type_mismatch():
    oad = 0x30000B00
    res = _exchange(_set_request(0x01, oad, bytes([0x0A, 0x01, 0x41]))).apdu
    assert res[7] == 7, f"类型不符应回 DAR 7，收到 {res[7]}（{res.hex()}）"


@case(
    id="D_00_00_02_000022",
    clause="free: SET 4000 A2 时 DAR 0 必须真改时钟，失败则 DAR 2/3 且时间不变",
    tags=["北向"],
)
@pytest.mark.contract
def test_set_datetime_moves_clock_or_reports_failure():
    oad = 0x40000200
    before = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert before[7] == 0x01, f"Get-Result 应为 Data：{before.hex()}"
    original = before[8:16]

    when = bytes([0x1C, 0x07, 0xE9, 0x01, 0x02, 0x03, 0x04, 0x05])
    assert when != original, "台架系统时钟恰好等于目标时间，无法区分 DAR 0 与失败"

    res = _exchange(_set_request(0x01, oad, when)).apdu
    assert res[0] == 0x86 and res[1] == 0x01, f"不是 SET 响应：{res.hex()}"
    assert res[3:7] == oad.to_bytes(4, "big"), "响应 OAD 与请求不一致"
    dar = res[7]
    assert dar in (0, 2, 3), f"时钟 SET 只允许 DAR 0/2/3，收到 {dar}（{res.hex()}）"

    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[7] == 0x01, f"Get-Result 应为 Data：{apdu.hex()}"
    if dar == 0:
        assert apdu[8:16] == when, f"DAR 0 但时间没变：读回 {apdu[8:16].hex()}"
        restored = _exchange(_set_request(0x01, oad, original)).apdu
        assert restored[7] == 0, f"恢复台架时间失败，DAR {restored[7]}（{restored.hex()}）"
    else:
        assert apdu[8:16] != when, f"DAR {dar} 却把时间改成了 {when.hex()}"


@case(id="D_00_00_02_000014", clause="free: SET 已关闭属性必须回 DAR 5", tags=["北向"])
@pytest.mark.contract
def test_set_masked_f200_attr_returns_class_mismatch():
    """F200 A4 在类定义里但实例未声明：SET 必须回 DAR 5，不能静默成功。"""
    oad = 0xF2000400
    res = _exchange(_set_request(0x01, oad, bytes([0x12, 0x25, 0x80]))).apdu
    assert res[7] == 5, f"未声明属性应回 DAR 5，收到 {res[7]}（{res.hex()}）"


@case(id="D_00_00_02_000015", clause="free: 5004 GetRecord 空范围回空记录表", tags=["北向"])
@pytest.mark.contract
def test_empty_record_range_returns_no_rows():
    """范围 [25 日, 26 日) 没有冻结行：结果 CHOICE 仍是 rows，行数为 0，帧完整。"""
    rsd = (
        bytes.fromhex("0220210200")
        + bytes.fromhex("1c07ea0b19000000")
        + bytes.fromhex("1c07ea0b1a000000")
        + bytes.fromhex("00")
    )
    rcsd = bytes.fromhex("0200202102000000100200")
    req = bytes([0x05, 0x03, 0x01]) + (0x50040200).to_bytes(4, "big") + rsd + rcsd + b"\x00"

    apdu = _exchange(req).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x03, f"不是 [3] 记录响应：{apdu.hex()}"
    assert apdu[18] == 0x01, f"记录结果 CHOICE 0x{apdu[18]:02X}，DAR={apdu[19]}"
    assert apdu[19] == 0, f"空范围应回 0 行，回了 {apdu[19]}"
    assert len(apdu) == 22, f"空记录表帧长 {len(apdu)}，应只有头尾：{apdu.hex()}"


@case(id="D_00_00_02_000016", clause="free: Class 20 连接会话与对象列表 ACL", tags=["北向"])
@pytest.mark.contract
def test_class20_session_and_object_list_acl():
    """同一 socket 上走完 预连接 → CONNECT → 未列对象 DAR 3 → RELEASE。

    预连接窗口允许读类 20 属性 2（否则主站无从得知窗口）；CONNECT 后对象列表
    成为 ACL：列表外 OAD 回 DAR 3，列表内照常读；RELEASE 清掉会话。
    """
    with _NorthSession() as sess:
        pre = sess.exchange(dlt698.get_request(0x01, 0x44000200)).apdu
        assert pre[7] == 0x01 and pre[8] == 0x01, f"预连接读对象列表失败：{pre.hex()}"

        conn = sess.exchange(_connect_request()).apdu
        assert conn[0] == 0x82, f"不是 CONNECT 响应：{conn.hex()}"
        assert conn[71] == 0, f"CONNECT 结果 {conn[71]}，期望 0"

        denied = sess.exchange(dlt698.get_request(0x01, 0x99990000)).apdu
        assert _get_dar(denied) == 3, f"列表外 OI 应回 DAR 3，收到 {denied[8]}"

        allowed = sess.exchange(dlt698.get_request(0x01, 0x20000200)).apdu
        assert allowed[7] == 0x01 and allowed[8] == 0x01, f"列表内 OI 应可读：{allowed.hex()}"

        rel = sess.exchange(_release_request()).apdu
        assert rel[0] == 0x83 and rel[2] == 0, f"RELEASE 失败：{rel.hex()}"

        again = sess.exchange(dlt698.get_request(0x01, 0x99990000)).apdu
        assert _get_dar(again) == 3, "RELEASE 后仍在预连接窗口，列表外 OI 仍是 DAR 3"
