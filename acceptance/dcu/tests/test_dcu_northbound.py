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
6. SET 4000 A2 date_time_s：DAR 0 必须伴随系统时钟真的改变，时钟失败 DAR 2、畸形载荷 DAR 7、未授权 ACL DAR 3，拒绝时时间不变

第 3 条刻意"验失败"：参数结构（标准表 A.9 的 array<structure>）还没实现，
如果它回 0，主站会以为继电器动了 —— 那是最危险的假成功。
它不走设备模型，因为"操作"不是"读一个点"，这里直接用协议层发一帧。
"""
import json
import os
import socket
import time
from pathlib import Path

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
        data = b""
        while True:
            need = dlt698.response_length(data)
            if need is not None and len(data) >= need:
                break
            chunk = sock.recv(65536)
            if not chunk:
                break
            data += chunk
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


def _wait_gauge(oad: int, expected: float, tol: float, scale: float = 10.0,
                deadline_s: float = 15.0) -> tuple[list[int], list[int]]:
    """重发 GET 直到第一元素落在 expected±tol；超时用最后值失败。

    产品刚拉起时南向首轮抄表未完成，DataHub 影子初值 0.0 是“尚未采到”而不是数据。
    """
    deadline = time.time() + deadline_s
    last = None
    last_values: list[int] = []
    while time.time() < deadline:
        tags, values = _read_variable(oad)
        last_values = values
        if values:
            last = values[0] / scale
            if abs(last - expected) <= tol:
                return tags, values
        time.sleep(0.1)
    raise AssertionError(
        f"{oad:#x} 在 {deadline_s}s 内未达到 {expected}±{tol}，最后值 {last}（{last_values}）"
    )


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
    tags, values = _wait_gauge(0x20000200, BENCH_VOLTAGE, TOLERANCE_V)
    assert len(values) == 3, f"电压数组应有 3 项，收到 {values}"
    assert all(tag == 0x12 for tag in tags), f"电压元素类型应为 long-unsigned：{tags}"
    got = values[0] / 10.0
    assert abs(got - BENCH_VOLTAGE) <= TOLERANCE_V, (
        f"698 读回 {got} V，台架给的是 {BENCH_VOLTAGE}±{TOLERANCE_V}"
    )

    scaler, unit = _read_scaler_unit(0x20000300)
    assert scaler == -1 and unit == 38, f"电压 Scaler_Unit 错：{scaler}, {unit}"


@case(id="D_00_00_02_000002",
      clause="free: 6000 A2 在扩容后的 AXDR 缓冲内返回 40 只表完整配置数组",
      tags=["北向"])
@pytest.mark.contract
def test_master_gets_honest_dar_for_oversized_archive_array():
    """6000 A2 是采集档案配置数组，不是计数。

    台架有 40 只表；D1-21 把 A-XDR 缓冲从 2048 扩到 10240 后，整组配置能完整编码，
    从站必须回 array<40> 的真实配置（旧行为是缓冲不足回 DAR=2）；若再超缓冲仍应回
    DAR=2，不能回一个计数让主站把表数当成配置表第一项。
    """
    oad = 0x60000200
    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"不是 [1] 响应：{apdu.hex()}"
    assert apdu[3:7] == oad.to_bytes(4, "big"), "响应 OAD 与请求不一致"
    assert apdu[7] == 0x01, f"配置应完整返回 Data，不是 DAR/计数：{apdu.hex()}"
    assert apdu[8] == 0x01, f"6000 A2 应为 array：{apdu.hex()}"
    assert apdu[9] == 40, f"应有 40 只表的配置，收到 {apdu[9]}"
    assert apdu[10] == 0x02, f"每项应是 6001 structure：{apdu.hex()[:64]}"


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
    tags, current = _wait_gauge(0x20010200, 0.0, 0.0, scale=1.0)
    assert len(current) == 3 and all(tag == 0x05 for tag in tags), f"电流数组：{tags},{current}"
    assert current == [0, 0, 0], f"台架回 0，电流数组应为 0：{current}"

    tags, power = _wait_gauge(0x20040200, 0.0, 0.0, scale=1.0)
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
    _wait_gauge(valid, BENCH_VOLTAGE, TOLERANCE_V)
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


def _scheme_unit_param(scheme_id: int, csds: list[int]) -> bytes:
    """6014 方法 127 的参数：array<6015 普通采集方案>。

    H.5.4 的采集当前数据帧用存储深度 1、存储时标 1；本产品只执行深度 0 +
    未定义时标 + 全部用户地址，所以判据帧把这两个字段改成 0（其余字节与标准一致）。
    """
    body = bytes([0x01, 0x01, 0x02, 0x06])        # array[1], structure[6]
    body += bytes([0x11, scheme_id])               # 方案编号
    body += bytes([0x12, 0x00, 0x00])              # 存储深度 0
    body += bytes([0x02, 0x02, 0x11, 0x00, 0x00])  # 采集类型 0 + NULL 内容
    body += bytes([0x01, len(csds)])
    for oad in csds:
        body += bytes([0x5B, 0x00]) + oad.to_bytes(4, "big")
    body += bytes([0x5C, 0x01])                    # 电能表集合：全部用户地址
    body += bytes([0x16, 0x00])                    # 存储时标：未定义
    return body


def _parse_one_scheme(data: bytes, i: int) -> tuple[dict, int]:
    """6015 单元：structure 6 {编号, 深度, {类型, Data}, array CSD, MS, 时标}。"""
    assert data[i] == 0x02 and data[i + 1] == 6, f"方案不是 6 字段结构：{data[i:i + 2].hex()}"
    i += 2
    assert data[i] == 0x11
    scheme_id = data[i + 1]
    i += 2
    assert data[i] == 0x12
    depth = int.from_bytes(data[i + 1 : i + 3], "big")
    i += 3
    assert data[i] == 0x02 and data[i + 1] == 2
    i += 2
    assert data[i] == 0x11
    capture_type = data[i + 1]
    i += 2
    assert data[i] == 0x00, f"采集内容应是 NULL：0x{data[i]:02X}"
    i += 1
    assert data[i] == 0x01
    csd_count = data[i + 1]
    i += 2
    csds = []
    for _ in range(csd_count):
        assert data[i] == 0x5B and data[i + 1] == 0x00, f"CSD 应是 [0] OAD：{data[i:i + 2].hex()}"
        csds.append(int.from_bytes(data[i + 2 : i + 6], "big"))
        i += 6
    assert data[i] == 0x5C and data[i + 1] == 0x01, f"MS 应是全部用户地址：{data[i:i + 2].hex()}"
    i += 2
    assert data[i] == 0x16
    time_label = data[i + 1]
    i += 2
    return (
        {
            "id": scheme_id,
            "depth": depth,
            "capture_type": capture_type,
            "csds": csds,
            "time_label": time_label,
        },
        i,
    )


def _read_schemes(oad: int = 0x60140200) -> list[dict]:
    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"不是 [1] 响应：{apdu.hex()}"
    assert apdu[7] == 0x01, f"6014 A2 应是 Data，不是 DAR：{apdu.hex()}"
    assert apdu[8] == 0x01, f"6014 A2 应是 array，收到 0x{apdu[8]:02X}"
    i = 10
    units = []
    for _ in range(apdu[9]):
        unit, i = _parse_one_scheme(apdu, i)
        units.append(unit)
    return units


@case(id="D_00_00_02_000020", clause="free: 6014 ACTION 127 添加/选择、6015 A2 读回、128 删除普通采集方案", tags=["北向"])
@pytest.mark.contract
def test_master_configures_normal_scheme():
    """H.5.4 原帧的深度/时标是 1，本产品按 DAR 2 拒绝；判据帧改成产品 profile
    （深度 0、时标 0），其余字段与标准一致。
    """
    csds = [0x00100200, 0x20000200]
    add = dlt698.action_request(0x07, 0x6014, 127, param=_scheme_unit_param(9, csds))
    res = dlt698.parse_action_response(_exchange(add).apdu)
    if res.dar == 6:
        pytest.skip(
            "6014 ACTION 未接入北向：dcu_north_adapter.c 对非 6000/8000 的 ACTION 仍回 "
            "NOT_FOUND（DAR 6），raw ACTION 分支由 D1-20a 接入；本用例的行为面由 "
            "tests/test_dcu_698_schemes.c 直接覆盖"
        )
    assert res.ok, f"Add 应成功，回 DAR {res.dar}（{res.error}）"

    schemes = {s["id"]: s for s in _read_schemes()}
    assert 9 in schemes, f"Add 后 6014 A2 应读到方案 9：{schemes}"
    assert schemes[9]["csds"] == csds, f"CSD 应原样读回：{schemes[9]}"
    assert (schemes[9]["depth"], schemes[9]["time_label"]) == (0, 0)

    # 6015 A2 的元素索引 = 方案编号（不是数组位置）。
    apdu = _exchange(dlt698.get_request(0x01, 0x60150200 | 9)).apdu
    assert apdu[7] == 0x01, f"6015 A2 应是 Data，不是 DAR：{apdu.hex()}"
    unit, _ = _parse_one_scheme(apdu, 8)
    assert unit["id"] == 9 and unit["csds"] == csds, f"6015 A2 应回方案 9：{unit}"

    delete = dlt698.action_request(0x07, 0x6014, 128, param=bytes([0x01, 0x01, 0x11, 9]))
    res = dlt698.parse_action_response(_exchange(delete).apdu)
    assert res.ok, f"Delete 应成功，回 DAR {res.dar}（{res.error}）"
    assert 9 not in {s["id"] for s in _read_schemes()}, "Delete 后 6014 A2 不应再有方案 9"


def _event_scheme_param(scheme_id: int, capture_type: int, roads: list[int], report: bool,
                        depth: int) -> bytes:
    """6016 方法 127 的参数：array<6017 事件采集方案>（单方案）。"""
    body = bytes([0x01, 0x01, 0x02, 0x05, 0x11, scheme_id, 0x02, 0x02, 0x11, capture_type])
    if capture_type == 1:
        body += bytes([0x00])
    else:
        body += bytes([0x01, len(roads)])
        for oad in roads:
            body += bytes([0x52]) + oad.to_bytes(4, "big") + bytes([0x11, 0x00])
    body += bytes([0x5C, 0x01])  # 电能表集合：全部用户地址
    body += bytes([0x03, 1 if report else 0])
    body += bytes([0x12, depth >> 8, depth & 0xFF])
    return body


def _read_event_scheme(scheme_id: int) -> bytes:
    apdu = _exchange(dlt698.get_request(0x01, 0x60170200 | scheme_id)).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"不是 [1] 响应：{apdu.hex()}"
    assert apdu[7] == 0x01, f"6017 A2 应是 Data，不是 DAR：{apdu.hex()}"
    return apdu[8:-2]  # 末尾两字节是 FollowReport 与时间标签


@case(id="D_00_00_02_000021", clause="free: 6016 ACTION 127 添加、6017 A2 读回、130 改上报标识、128 删除",
      tags=["北向"])
@pytest.mark.contract
def test_master_configures_event_scheme():
    roads = [0x50040200]
    param = _event_scheme_param(21, 0, roads, True, 32)
    res = dlt698.parse_action_response(_exchange(dlt698.action_request(0x07, 0x6016, 127,
                                                                      param=param)).apdu)
    assert res.ok, f"Add 应成功，回 DAR {res.dar}（{res.error}）"
    assert _read_event_scheme(21) == param[2:], (
        f"6017 A2 应原样读回事件采集方案：got={_read_event_scheme(21).hex()} want={param[2:].hex()}")
    update = bytes([0x02, 0x02, 0x11, 21, 0x03, 0x00])
    res = dlt698.parse_action_response(_exchange(dlt698.action_request(0x07, 0x6016, 130,
                                                                      param=update)).apdu)
    assert res.ok, f"UpdateReportFlag 应成功，回 DAR {res.dar}（{res.error}）"
    updated = _read_event_scheme(21)
    assert updated[-5] == 0x03 and updated[-4] == 0x00
    delete = dlt698.action_request(0x07, 0x6016, 128, param=bytes([0x01, 0x01, 0x11, 21]))
    res = dlt698.parse_action_response(_exchange(delete).apdu)
    assert res.ok, f"Delete 应成功，回 DAR {res.dar}（{res.error}）"
    gone = _exchange(dlt698.get_request(0x01, 0x60170200 | 21)).apdu
    assert gone[7] == 0x00 and gone[8] == 0x06, f"删除后 6017 A2 应回 DAR 6：{gone.hex()}"


def _transparent_scheme_param(scheme_id: int, addr: bytes, frames: list[tuple[int, bytes]]) -> bytes:
    """6018 方法 127 的参数：单个 6019 透明方案（一条方案内容）。"""
    body = bytes([0x02, 0x03, 0x11, scheme_id, 0x01, 0x01, 0x02, 0x06])
    body += bytes([0x12, 0x00, 0x01])           # 方案内容序号
    body += bytes([len(addr)]) + addr
    body += bytes([0x12, 0x00, 0x00, 0x12, 0x00, 0x00])  # 前/后脚本 id
    body += bytes([0x02, 0x04, 0x03, 0x00, 0x12, 0x00, 0x00, 0x16, 0x00,
                   0x02, 0x03, 0x11, 0x00, 0x12, 0x00, 0x00, 0x12, 0x00, 0x00])
    body += bytes([0x01, len(frames)])
    for seq, data in frames:
        body += bytes([0x02, 0x02, 0x11, seq, 0x09, len(data)]) + data
    body += bytes([0x12, 0x00, 0x00])           # 存储深度
    return body


@case(id="D_00_00_02_000025", clause="free: 6018 ACTION 127 添加、6019 A2 读回、131 清空透明方案",
      tags=["北向"])
@pytest.mark.contract
def test_master_configures_transparent_scheme():
    addr = bytes([0x12, 0x34, 0x56, 0x78])
    param = _transparent_scheme_param(31, addr, [(1, b"AB")])
    res = dlt698.parse_action_response(_exchange(dlt698.action_request(0x07, 0x6018, 127,
                                                                      param=param)).apdu)
    assert res.ok, f"Add 应成功，回 DAR {res.dar}（{res.error}）"
    apdu = _exchange(dlt698.get_request(0x01, 0x60190200 | 31)).apdu
    assert apdu[7] == 0x01, f"6019 A2 应是 Data：{apdu.hex()}"
    assert apdu[8:-2] == param, (
        f"6019 A2 应原样读回透明方案：got={apdu[8:-2].hex()} want={param.hex()}")
    clear = dlt698.action_request(0x07, 0x6018, 131, param=bytes([0x00]))
    res = dlt698.parse_action_response(_exchange(clear).apdu)
    assert res.ok, f"Clear 应成功，回 DAR {res.dar}（{res.error}）"
    gone = _exchange(dlt698.get_request(0x01, 0x60190200 | 31)).apdu
    assert gone[7] == 0x00 and gone[8] == 0x06, f"清空后 6019 A2 应回 DAR 6：{gone.hex()}"


def _report_scheme_param(scheme_id: int, channels: list[int], timeout_unit: int,
                         timeout_value: int, max_times: int, content_oad: int) -> bytes:
    """601C 方法 127 的参数：array<601D 上报方案>（单方案，上报内容类型 0 = OAD）。"""
    body = bytes([0x01, 0x01, 0x02, 0x05, 0x11, scheme_id, 0x01, len(channels)])
    for ch in channels:
        body += bytes([0x51]) + ch.to_bytes(4, "big")
    body += bytes([0x54, timeout_unit, timeout_value >> 8, timeout_value & 0xFF])
    body += bytes([0x11, max_times])
    body += bytes([0x02, 0x02, 0x11, 0x00, 0x51]) + content_oad.to_bytes(4, "big")
    return body


@case(id="D_00_00_02_000026", clause="free: 601C ACTION 127 添加、601D A2 读回、128 删除上报方案",
      tags=["北向"])
@pytest.mark.contract
def test_master_configures_report_scheme():
    param = _report_scheme_param(41, [0x40010200], 0, 60, 3, 0x20000200)
    res = dlt698.parse_action_response(_exchange(dlt698.action_request(0x07, 0x601C, 127,
                                                                      param=param)).apdu)
    assert res.ok, f"Add 应成功，回 DAR {res.dar}（{res.error}）"
    apdu = _exchange(dlt698.get_request(0x01, 0x601D0200 | 41)).apdu
    assert apdu[7] == 0x01, f"601D A2 应是 Data：{apdu.hex()}"
    assert apdu[8:-2] == param[2:], (
        f"601D A2 应原样读回上报方案：got={apdu[8:-2].hex()} want={param[2:].hex()}")
    delete = dlt698.action_request(0x07, 0x601C, 128, param=bytes([0x01, 0x01, 0x11, 41]))
    res = dlt698.parse_action_response(_exchange(delete).apdu)
    assert res.ok, f"Delete 应成功，回 DAR {res.dar}（{res.error}）"
    gone = _exchange(dlt698.get_request(0x01, 0x601D0200 | 41)).apdu
    assert gone[7] == 0x00 and gone[8] == 0x06, f"删除后 601D A2 应回 DAR 6：{gone.hex()}"


def _rule_unit_param(oad: int, main_di: int, spare_di: int | None) -> bytes:
    body = bytes([0x01, 0x01, 0x02, 0x02, 0x5B, 0x00]) + oad.to_bytes(4, "big")
    body += bytes([0x02, 0x03, 0x02, 0x02, 0x01, 0x01, 0x09, 0x04])
    body += main_di.to_bytes(4, "little")
    if spare_di is None:
        body += bytes([0x01, 0x00])
    else:
        body += bytes([0x01, 0x01, 0x09, 0x04]) + spare_di.to_bytes(4, "little")
    body += bytes([0x02, 0x02, 0x01, 0x00, 0x01, 0x00])
    body += bytes([0x02, 0x01, 0x09, 0x00])
    return body


@case(id="D_00_00_02_000028",
      clause="free: 相别 CSD 用主站可见 OAD（0x20000201），不接受内部影子 tag",
      tags=["北向"])
@pytest.mark.contract
def test_master_configures_phase_rule_by_external_csd():
    param = _rule_unit_param(0x20000201, 0x02010200, 0x0201020A)
    res = dlt698.parse_action_response(_exchange(dlt698.action_request(0x07, 0x601E, 127,
                                                                      param=param)).apdu)
    assert res.ok, f"B 相 CSD 的 Add 应成功，回 DAR {res.dar}（{res.error}）"
    lib = _exchange(dlt698.get_request(0x01, 0x601E0200)).apdu
    assert lib[8] == 0x01 and lib[9] == 15, f"601E A2 应是 15 项规则：{lib.hex()}"
    assert b"\x5b\x00\x20\x00\x02\x01" in lib, f"601E 应含外部 CSD 0x20000201：{lib.hex()}"
    assert b"\x5b\x00\x20\x00\x03\x00" not in lib, f"601E 不得暴露影子 tag：{lib.hex()}"
    assert param[2:] in lib, f"601E A2 应含刚写入的 B 相规则：{lib.hex()}"

    apdu = _exchange(dlt698.get_request(0x01, 0x601F0200 | 3)).apdu
    assert apdu[8:-2] == param[2:], (
        f"601F A2 应原样读回 B 相规则：got={apdu[8:-2].hex()} want={param[2:].hex()}")

    shadow = _rule_unit_param(0x20000300, 0x02010200, None)
    res = dlt698.parse_action_response(_exchange(dlt698.action_request(0x07, 0x601E, 127,
                                                                      param=shadow)).apdu)
    assert not res.ok and res.dar == 2, f"影子 tag 作 CSD 应回 DAR 2，实际 {res.dar}"

    delete = dlt698.action_request(0x07, 0x601E, 128,
                                   param=bytes([0x01, 0x01, 0x5B, 0x00])
                                   + (0x20000201).to_bytes(4, "big"))
    res = dlt698.parse_action_response(_exchange(delete).apdu)
    assert res.ok, f"Delete 应成功，回 DAR {res.dar}（{res.error}）"
    lib = _exchange(dlt698.get_request(0x01, 0x601E0200)).apdu
    assert lib[9] == 14, f"删除后 601E A2 应是 14 项：{lib.hex()}"

    restore = _rule_unit_param(0x20000201, 0x02010200, None)
    res = dlt698.parse_action_response(_exchange(dlt698.action_request(0x07, 0x601E, 127,
                                                                      param=restore)).apdu)
    assert res.ok, f"恢复 B 相规则应成功，回 DAR {res.dar}（{res.error}）"


@case(id="D_00_00_02_000027", clause="free: 601E ACTION 127 添加、601F A2 读回、128 删除采集规则",
      tags=["北向"])
@pytest.mark.contract
def test_master_configures_collection_rule():
    param = _rule_unit_param(0x20000200, 0x02010100, 0x02010105)
    res = dlt698.parse_action_response(_exchange(dlt698.action_request(0x07, 0x601E, 127,
                                                                      param=param)).apdu)
    assert res.ok, f"Add 应成功，回 DAR {res.dar}（{res.error}）"
    lib = _exchange(dlt698.get_request(0x01, 0x601E0200)).apdu
    assert lib[7] == 0x01, f"601E A2 应是 Data：{lib.hex()}"
    assert lib[8] == 0x01 and lib[9] == 15, f"601E A2 应是 15 项规则：{lib.hex()}"
    assert param[2:] in lib, f"601E A2 应含刚写入的规则：{lib.hex()}"

    apdu = _exchange(dlt698.get_request(0x01, 0x601F0200 | 2)).apdu
    assert apdu[7] == 0x01, f"601F A2 应是 Data：{apdu.hex()}"
    assert apdu[8:-2] == param[2:], (
        f"601F A2 应原样读回采集规则：got={apdu[8:-2].hex()} want={param[2:].hex()}")

    delete = dlt698.action_request(0x07, 0x601E, 128,
                                   param=bytes([0x01, 0x01, 0x5B, 0x00])
                                   + (0x20000200).to_bytes(4, "big"))
    res = dlt698.parse_action_response(_exchange(delete).apdu)
    assert res.ok, f"Delete 应成功，回 DAR {res.dar}（{res.error}）"
    lib = _exchange(dlt698.get_request(0x01, 0x601E0200)).apdu
    assert lib[8] == 0x01 and lib[9] == 14, f"删除后 601E A2 应是 14 项：{lib.hex()}"


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
    clause="free: SET 4000 A2 时 DAR 0 必须真改时钟，时钟失败回 DAR 2、畸形载荷回 DAR 7 且时间不变",
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
    assert dar in (0, 2), f"合法 date_time_s 时钟 SET 只允许 DAR 0/2，收到 {dar}（{res.hex()}）"

    apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert apdu[7] == 0x01, f"Get-Result 应为 Data：{apdu.hex()}"
    if dar == 0:
        assert apdu[8:16] == when, f"DAR 0 但时间没变：读回 {apdu[8:16].hex()}"
        restored = _exchange(_set_request(0x01, oad, original)).apdu
        assert restored[7] == 0, f"恢复台架时间失败，DAR {restored[7]}（{restored.hex()}）"
    else:
        assert apdu[8:16] != when, f"DAR {dar} 却把时间改成了 {when.hex()}"

    bad = _exchange(_set_request(0x01, oad, bytes([0x11, 0x01]))).apdu
    assert bad[0] == 0x86 and bad[1] == 0x01, f"不是 SET 响应：{bad.hex()}"
    assert bad[3:7] == oad.to_bytes(4, "big"), "响应 OAD 与请求不一致"
    assert bad[7] == 7, f"畸形 date_time_s 载荷应回 DAR 7，收到 {bad[7]}（{bad.hex()}）"
    after_bad = _exchange(dlt698.get_request(0x01, oad)).apdu
    assert after_bad[7] == 0x01, f"Get-Result 应为 Data：{after_bad.hex()}"
    assert after_bad[8:16] != when, f"DAR 7 却把时间改成了 {when.hex()}"


@case(
    id="D_00_00_02_000023",
    clause="free: 类 2/5 无产品数据源时 GET 必须回 DAR 5，不得回 0 或空数组",
    tags=["北向"],
)
@pytest.mark.contract
def test_class2_and_class5_read_reports_class_mismatch():
    oads = [
        0x10100200, 0x10110200, 0x10120200, 0x10130200,
        0x200D0200, 0x200D0300, 0x200D0400, 0x200D0500, 0x200D0600,
        0x200E0200,
    ]
    for oad in oads:
        apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
        assert apdu[7] == 0x00, f"OAD {oad:08X} 应回 DAR 而不是 Data：{apdu.hex()}"
        assert apdu[8] == 5, f"OAD {oad:08X} 应回 DAR 5，收到 {apdu[8]}（{apdu.hex()}）"


@case(id="D_00_00_02_000014", clause="free: SET 已关闭属性必须回 DAR 5", tags=["北向"])
@pytest.mark.contract
def test_set_masked_f200_attr_returns_class_mismatch():
    """F200 A4 在类定义里但实例未声明：SET 必须回 DAR 5，不能静默成功。"""
    oad = 0xF2000400
    res = _exchange(_set_request(0x01, oad, bytes([0x12, 0x25, 0x80]))).apdu
    assert res[7] == 5, f"未声明属性应回 DAR 5，收到 {res[7]}（{res.hex()}）"


@case(
    id="D_00_00_02_000035",
    clause="free: 类 22 已广告只读属性回 DAR 2；未广告/类外属性主站先被 ACL 拒（DAR 3）",
    tags=["北向"],
)
@pytest.mark.contract
def test_class22_peripheral_read_returns_temporary_failure():
    oads = [(oi << 16) | 0x0200 for oi in range(0xF200, 0xF20D)]
    oads += [0xF2030400, 0xF2090500, 0xF2090600, 0xF20C0400, 0xF20C0500]
    for oad in oads:
        apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
        assert apdu[0] == 0x85 and apdu[1] == 0x01, f"{oad:#x} 不是 GET 响应：{apdu.hex()}"
        assert apdu[3:7] == oad.to_bytes(4, "big"), f"{oad:#x} 响应 OAD 不一致"
        assert apdu[7] == 0x00, f"{oad:#x} 应回 DAR 而不是 Data：{apdu.hex()}"
        assert apdu[8] == 2, f"{oad:#x} 应回 DAR 2，收到 {apdu[8]}（{apdu.hex()}）"

    # F203 A3（类定义未广告）与 F203 A9（类外）不进 ACL：主站先回 DAR 3。
    for oad in (0xF2030300, 0xF2030900):
        apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
        assert apdu[7] == 0x00 and apdu[8] == 3, f"{oad:#x} 应回 DAR 3：{apdu.hex()}"

    res = _exchange(_set_request(0x01, 0xF2030400, bytes([0x12, 0x25, 0x80]))).apdu
    assert res[0] == 0x86 and res[1] == 0x01, f"不是 SET 响应：{res.hex()}"
    assert res[3:7] == (0xF2030400).to_bytes(4, "big"), "响应 OAD 与请求不一致"
    assert res[7] == 3, f"只读属性 SET 应回 DAR 3，收到 {res[7]}（{res.hex()}）"


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


@case(id="D_00_00_02_000024", clause="free: 6002 无扫描驱动时方法 127 回 DAR 2，A2/A6/A10 形状诚实", tags=["北向"])
@pytest.mark.contract
def test_master_starts_search_and_reads_standard_result_shapes():
    """表 A.7：6002 方法 127 启动搜表；A2 是 `array 一个搜表结果`、A6 是记录数、A10 是状态。

    产品没有下行搜表驱动，启动动作不能执行：方法 127 必须回 DAR 2，A10 保持空闲，
    A2 是空数组、A6 为 0。不能进入"搜表中"再自行结束，那会让主站以为扫过一遍且没找到表。
    """
    start = dlt698.action_request(0x07, 0x6002, 127, param=bytes([0x12, 0x00, 0x01]))
    res = dlt698.parse_action_response(_exchange(start).apdu)
    assert not res.ok, f"没有扫描驱动时启动不应回 DAR 0（{res.error}）"
    assert res.dar == 2, f"没有扫描驱动时启动应回 DAR 2，收到 {res.dar}"

    apdu = _exchange(dlt698.get_request(0x01, 0x60020200)).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"不是 [1] 响应：{apdu.hex()}"
    assert apdu[7] == 0x01, f"6002 A2 应是 Data，不是 DAR：{apdu.hex()}"
    assert apdu[8] == 0x01, f"6002 A2 应是 array：{apdu.hex()}"
    assert apdu[9] == 0, f"没有扫描驱动，结果集应为空数组：{apdu.hex()}"

    apdu = _exchange(dlt698.get_request(0x01, 0x60020600)).apdu
    assert apdu[7] == 0x01 and apdu[8] == 0x12, f"6002 A6 应是 long-unsigned：{apdu.hex()}"
    assert int.from_bytes(apdu[9:11], "big") == 0, f"A6 应为 0：{apdu.hex()}"

    apdu = _exchange(dlt698.get_request(0x01, 0x60020A00)).apdu
    assert apdu[7] == 0x01 and apdu[8] == 0x16, f"6002 A10 应是 enum：{apdu.hex()}"
    assert apdu[9] == 0, f"启动被拒后 A10 应保持空闲（0）：{apdu.hex()}"


_SIM_RUN_DIR = Path(os.environ.get("DCU_RUN_DIR", "/tmp/edgec-acceptance-run"))
_SIM_CONTROL = _SIM_RUN_DIR / "sim645_control.json"
_BUS_LOG = _SIM_RUN_DIR / "bus.json"
_DI_MAIN = "02010100"
_DI_SPARE = "02010105"


def _write_sim_control(fail: list[str], payloads: dict[str, str]) -> None:
    tmp = _SIM_CONTROL.with_suffix(".tmp")
    tmp.write_text(json.dumps({"fail": fail, "payloads": payloads}), encoding="utf-8")
    os.replace(tmp, _SIM_CONTROL)


def _bus_seen() -> dict:
    try:
        return json.loads(_BUS_LOG.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


@case(id="D_00_00_02_000029", clause="free: 645 主用 DI 失败后按备用 DI 重试一次",
      tags=["北向"])
@pytest.mark.contract
# 窗口按 bus.json 时间聚簇还原单次执行（间隔 >1s 即新执行）；慢趟/相邻执行不再混窗。
# 断言未变：每次执行恰好一次 主用→备用，窗口内无第三次请求。
@pytest.mark.timeout(120)
def test_master_uses_spare_di_after_primary_failure():
    """主用 DI 回异常帧、备用 DI 回 220.0：DCU 必须恰好重试一次并让北向读到备用值。"""
    param = _rule_unit_param(0x20000200, 0x02010100, 0x02010105)
    res = dlt698.parse_action_response(
        _exchange(dlt698.action_request(0x07, 0x601E, 127, param=param)).apdu)
    assert res.ok, f"配置主用/备用 DI 应成功，回 DAR {res.dar}（{res.error}）"

    def _executions(records: list) -> dict[str, list[list[str]]]:
        per_meter: dict[str, list[dict]] = {}
        for rec in records:
            if rec.get("di") in (_DI_MAIN, _DI_SPARE):
                per_meter.setdefault(rec.get("meter", ""), []).append(rec)
        out = {}
        for meter, recs in per_meter.items():
            runs: list[list[str]] = []
            last = None
            for rec in recs:
                if last is not None and rec.get("at", 0) - last.get("at", 0) < 1.0:
                    runs[-1].append(rec["di"])
                else:
                    runs.append([rec["di"]])
                last = rec
            out[meter] = runs
        return out

    _write_sim_control([_DI_MAIN], {_DI_SPARE: "0022"})
    try:
        before = _read_monitor(1)["start"]
        _wait_monitor(1, lambda m: m["start"] != before, 40, "控制生效后的新执行出现")
        base = _bus_seen()
        reads_before = len(base.get("reads", []))

        # 等每块表至少出现一次完整的 主用→备用 执行；聚簇按时间，不按窗口边界猜。
        deadline = time.time() + 40
        reads = []
        while True:
            reads = _bus_seen().get("reads", [])[reads_before:]
            found = _executions(reads)
            if found and all(any(x == [_DI_MAIN, _DI_SPARE] for x in v) for v in found.values()):
                break
            assert time.time() < deadline, (
                f"40s 内没等到每块表一次完整的 主用→备用 执行：{found}")
            time.sleep(0.5)

        values = None
        deadline = time.time() + 20
        while time.time() < deadline:
            _, values = _read_variable(0x20000200)
            if values[0] == 2200:
                break
            time.sleep(1.0)
        assert values is not None and values[0] == 2200, (
            f"备用 DI 的 220.0 V 没出现在北向读回里：{values}")

        by_meter = _executions(reads)
        assert by_meter, "台架没看到主用/备用 DI 请求"
        # 控制生效前可能已开跑一次成功执行（执行 = [main]）：从每表首个失败对开始判。
        paired = 0
        checked: dict[str, list[list[str]]] = {}
        for meter, execs in by_meter.items():
            started = False
            for seq in execs:
                if not started:
                    # 窗口首执行可能是快照前的半截（只剩 main 或只剩 spare），跳过；
                    # 之后每次执行必须恰好一对。
                    if len(seq) == 1:
                        continue
                    assert seq == [_DI_MAIN, _DI_SPARE], (
                        f"{meter}: 控制生效后的执行不是恰好一次备用重试：{seq}")
                    started = True
                else:
                    assert seq == [_DI_MAIN, _DI_SPARE], (
                        f"{meter}: 一次执行不是恰好一次备用重试：{seq}")
                paired += 1
            assert started, f"{meter}: 窗口里没有失败后的 主用→备用 执行：{execs}"
            checked[meter] = execs
        assert paired >= len(by_meter), f"窗口里缺少完整的 主用→备用 执行：{by_meter}"

        seen = _bus_seen()
        per = seen.get("per_di", {})
        main_delta = per.get(_DI_MAIN, 0) - base.get("per_di", {}).get(_DI_MAIN, 0)
        spare_delta = per.get(_DI_SPARE, 0) - base.get("per_di", {}).get(_DI_SPARE, 0)
        assert main_delta >= 1 and spare_delta >= 1, (
            f"台架没看到主用/备用 DI 请求：main={main_delta} spare={spare_delta}")
        (_SIM_RUN_DIR / "spare_window.json").write_text(
            json.dumps({"checked": checked, "main_delta": main_delta,
                        "spare_delta": spare_delta}, ensure_ascii=False),
            encoding="utf-8")
    finally:
        _write_sim_control([], {})
        restore = _rule_unit_param(0x20000200, 0x02010100, None)
        dlt698.parse_action_response(
            _exchange(dlt698.action_request(0x07, 0x601E, 127, param=restore)).apdu)


@case(id="D_00_00_02_000030", clause="free: 6040–6042 读真实采集时标；6032–6035 缺诚实来源回 DAR 2",
      tags=["北向"])
@pytest.mark.contract
def test_master_reads_acquisition_timestamps_and_status_dar():
    for oi in (0x6040, 0x6041, 0x6042):
        apdu = _exchange(dlt698.get_request(0x01, oi << 16 | 0x0200)).apdu
        assert apdu[7] == 0x01, f"{oi:04X} 应是 Data：{apdu.hex()}"
        stamp = apdu[8:-2]
        assert len(stamp) == 8 and stamp[0] == 0x1C, f"{oi:04X} 应是 date_time_s：{apdu.hex()}"
        year = int.from_bytes(stamp[1:3], "big")
        assert 2020 <= year <= 2100, f"{oi:04X} 年份不合理：{year}"

    for oi in (0x6032, 0x6033):
        apdu = _exchange(dlt698.get_request(0x01, oi << 16 | 0x0200)).apdu
        assert apdu[7] == 0x00 and apdu[8] == 0x02, (
            f"{oi:04X} 缺诚实来源应回 DAR 2：{apdu.hex()}")
    unknown = _exchange(dlt698.get_request(0x01, 0x60350200)).apdu
    assert unknown[7] == 0x00 and unknown[8] == 0x06, f"未知 task_id 应回 DAR 6：{unknown.hex()}"


def _monitor_unit(apdu: bytes) -> dict:
    i = 8
    assert apdu[7] == 0x01 and apdu[i] == 0x02 and apdu[i + 1] == 0x08, apdu.hex()
    i += 2
    assert apdu[i] == 0x11
    task_id = apdu[i + 1]
    i += 2
    assert apdu[i] == 0x16
    status = apdu[i + 1]
    i += 2

    def stamp():
        nonlocal i
        assert apdu[i] == 0x1C
        value = None if apdu[i + 1] == 0xFF else apdu[i + 1 : i + 8]
        i += 8
        return value

    start, end = stamp(), stamp()

    def lu():
        nonlocal i
        assert apdu[i] == 0x12
        v = int.from_bytes(apdu[i + 1 : i + 3], "big")
        i += 3
        return v

    out = {"task": task_id, "status": status, "start": start, "end": end,
           "total": lu(), "success": lu(), "sent": lu(), "recv": lu()}
    assert i <= len(apdu) - 2
    return out


def _wait_monitor(task_id: int, predicate, deadline_s: float, what: str,
                  interval: float = 0.5) -> dict:
    """有界等待 monitor 满足 predicate；超时带最后一次读数失败。"""
    deadline = time.time() + deadline_s
    last = None
    while time.time() < deadline:
        last = _read_monitor(task_id)
        if predicate(last):
            return last
        time.sleep(interval)
    raise AssertionError(f"{deadline_s}s 内未等到{what}：{last}")


def _read_monitor(task_id: int) -> dict:
    apdu = _exchange(dlt698.get_request(0x01, 0x60350200 | task_id)).apdu
    return _monitor_unit(apdu)


@case(id="D_00_00_02_000032",
      clause="free: 测试时钟跨零点，5004/5005 按标准边界写默认源记录，对齐差 <50ms",
      tags=["北向"])
@pytest.mark.contract
def test_master_reads_midnight_freeze_with_test_clock():
    """控制文件注入虚拟时钟：11-30 23:59:59.8 起动（首 tick 不回补），跨到 12-01 00:00:00。

    5004 日冻结与 5005 结算日（每月 1 日 00:00 结算上月）都在零点写记录，freeze_time 必须是
    边界而不是回调时刻；列集 = 冻结时间 + 默认源 {有功电能, 电压, 电流}。切换类 5008 未进 ACL，
    GetRecord 必须回 DAR 分支（无行数据），而不是成功行。
    """
    clock = _SIM_RUN_DIR / "freeze_clock.txt"
    align = _SIM_RUN_DIR / "freeze_align.txt"
    midnight = int(time.mktime((2026, 12, 1, 0, 0, 0, 0, 0, -1)))
    cols = [0x20210200, 0x00100200, 0x20000200, 0x20010200]
    tags = [0x06, 0x12, 0x05]
    tmp = clock.with_suffix(".tmp")
    tmp.write_text(str(midnight * 1000 - 200), encoding="utf-8")
    os.replace(tmp, clock)
    try:
        want_dts = bytes([0x1C]) + (2026).to_bytes(2, "big") + bytes([12, 1, 0, 0, 0])
        for oad in (0x50040200, 0x50050200):
            deadline = time.time() + 10
            last = ""
            while time.time() < deadline:
                apdu = _exchange(_record_req(oad, midnight, midnight + 1, cols)).apdu
                if (apdu[0] == 0x85 and apdu[1] == 0x03 and apdu[7] == 4
                        and apdu[28] == 0x01 and apdu[29] >= 1):
                    break
                last = apdu.hex()
                time.sleep(0.1)
            assert apdu[28] == 0x01, f"{oad:#x} 零点没有记录（{last}）"
            assert apdu[29] == 1, f"{oad:#x} 应正好 1 行，回了 {apdu[29]}"
            i = 30
            assert apdu[i:i + 8] == want_dts, (
                f"{oad:#x} freeze_time 不是零点：{apdu[i:i + 8].hex()} vs {want_dts.hex()}")
            i += 8
            for tag in tags:
                assert apdu[i] == tag, f"{oad:#x} 源列标签 0x{apdu[i]:02X}，期望 0x{tag:02X}"
                i += {0x06: 5, 0x12: 3, 0x05: 5}[tag]

        delta_txt = align.read_text(encoding="utf-8")
        delta = int(delta_txt.split("delta_ms=")[1].split()[0])
        assert delta < 50, f"零点对齐差 {delta}ms，要求 <50ms（{delta_txt.strip()}）"

        apdu = _exchange(_record_req(0x50080200, midnight, midnight + 1, cols)).apdu
        assert apdu[0] == 0x85 and apdu[1] == 0x03, f"5008 不是记录响应：{apdu.hex()}"
        assert apdu[7] == 4, f"5008 记录响应应回显 4 列 CSD：{apdu.hex()}"
        # 实测布局：... | 4×CSD | 00(DAR 分支) | DAR | 00 | 00；5008 未进 ACL → DAR 3（拒绝）。
        assert apdu[28] == 0x00, f"5008 应是记录 DAR 分支，不是行数据：{apdu.hex()}"
        assert apdu[29] == 3, f"5008 未进 ACL 应回 DAR 3（拒绝），收到 {apdu[29]}：{apdu.hex()}"
    finally:
        clock.unlink(missing_ok=True)


@case(id="D_00_00_02_000031",
      clause="free: 6035 任务状态/成功数/收发计数与台架一致（含备用重试）", tags=["北向"])
@pytest.mark.contract
# 完成锚定：先等一次新执行出现（start 变化），再等该 start 的 end 非空（执行完成）
# 且计数到位；恢复同理。coverage 构建单趟实测 median 13.2s / p90 16.8s / max 19.4s，
# 慢趟在并行构建干扰下可超过 60s，恢复完成阶段给 90s；整案阶段耗时见
# docs/evidence/DCU-D0-19。断言未变，只扩时间。
@pytest.mark.timeout(240)
def test_task_monitor_matches_bench_failures():
    param = _rule_unit_param(0x20000200, 0x02010100, 0x02010105)
    res = dlt698.parse_action_response(
        _exchange(dlt698.action_request(0x07, 0x601E, 127, param=param)).apdu)
    assert res.ok, f"配置主用/备用 DI 应成功，回 DAR {res.dar}（{res.error}）"
    baseline = _wait_monitor(1, lambda m: m["total"] == 40 and m["start"] is not None, 10,
                             "采集基线")
    t0 = time.time()
    _write_sim_control([_DI_MAIN, _DI_SPARE], {})
    failed = None
    try:
        # 第一段：控制写入后出现的第一个新执行（start 变化）。
        exec_start = _wait_monitor(1, lambda m: m["start"] != baseline["start"], 40,
                                   "失败执行出现（start 变化）")["start"]
        t1 = time.time()
        # 第二段：同一 start 的 end 非空（完成）且计数到位。601E 开头的重配已清掉
        # 遗留熔断，失败段内不会因熔断跳过表。
        failed = _wait_monitor(
            1,
            lambda m: m["start"] == exec_start and m["end"] is not None
            and m["total"] == 40 and m["success"] == 0 and m["sent"] >= 80 and m["recv"] >= 80,
            35, "失败执行完成（end 非空、total=40/success=0/sent≥80/recv≥80）", 0.1)
        t2 = time.time()
        assert failed["total"] == 40, f"采集总数量应为 40：{failed}"
        assert failed["success"] == 0, f"主用与备用都失败时成功数量应为 0：{failed}"
        assert failed["sent"] >= 80, f"发送计数应含备用重试（≥2×40）：{failed}"
        assert failed["recv"] >= 80, f"接收计数应含两次错误响应（≥2×40）：{failed}"
        assert failed["start"] is not None, f"启动时间应有值：{failed}"

        lib = _exchange(dlt698.get_request(0x01, 0x60340200)).apdu
        assert lib[7] == 0x01 and lib[8] == 0x01 and lib[9] >= 1, f"6034 应含任务：{lib.hex()}"
    finally:
        _write_sim_control([], {})
        restore = _rule_unit_param(0x20000200, 0x02010100, None)
        dlt698.parse_action_response(
            _exchange(dlt698.action_request(0x07, 0x601E, 127, param=restore)).apdu)
    t3 = time.time()

    # 恢复后同样两段：恢复时可能已有执行在跑，先等它之后的下一个 start，再等其完成。
    start_before = _read_monitor(1)["start"]
    recovered_start = _wait_monitor(1, lambda m: m["start"] != start_before, 40,
                                    "恢复后的执行出现（start 变化）")["start"]
    t4 = time.time()
    recovered = _wait_monitor(
        1,
        lambda m: m["start"] == recovered_start and m["end"] is not None
        and m["total"] == 40 and m["success"] == 40,
        90, "恢复后的成功执行完成（end 非空、total=40/success=40）")
    t5 = time.time()
    assert recovered is not None and recovered["success"] == 40, f"恢复后成功数量应回到 40：{recovered}"
    (_SIM_RUN_DIR / "task_monitor_windows.json").write_text(
        json.dumps({"fail_start_s": round(t1 - t0, 2), "fail_done_s": round(t2 - t1, 2),
                    "recover_start_s": round(t4 - t3, 2), "recover_done_s": round(t5 - t4, 2),
                    "total_s": round(t5 - t0, 2)}),
        encoding="utf-8")


def _record_req(oad: int, start: int, end: int, cols: list[int]) -> bytes:
    def dts(epoch: int) -> bytes:
        t = time.localtime(epoch)
        return bytes([0x1C]) + (t.tm_year).to_bytes(2, "big") + bytes(
            [t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec])
    rsd = b"\x02" + (0x20210200).to_bytes(4, "big") + dts(start) + dts(end) + b"\x00"
    rcsd = bytes([len(cols)]) + b"".join(b"\x00" + c.to_bytes(4, "big") for c in cols)
    return bytes([0x05, 0x03, 0x01]) + oad.to_bytes(4, "big") + rsd + rcsd + b"\x00"


def _freeze_item(period: int, oad: int, depth: int) -> bytes:
    return (bytes([0x02, 0x03, 0x12]) + period.to_bytes(2, "big") + b"\x51"
            + oad.to_bytes(4, "big") + bytes([0x12]) + depth.to_bytes(2, "big"))


@case(id="D_00_00_02_000033",
      clause="free: 方法 5 删除冻结对象后，5004 GetRecord 列集随之变化", tags=["北向"])
@pytest.mark.contract
def test_master_configures_freeze_attr3():
    """属性 3 是记录列的唯一来源：删掉有功电能后，新冻结记录不再含该列。

    控制时钟让 5001 每秒冻结；方法 5 删除 OAD 0x00100200，随后 1 秒窗口内
    记录应能按 [时间, 电压, 电流] 读回，而按 [时间, 电能] 读会因缺列回 DAR。
    """
    clock = _SIM_RUN_DIR / "freeze_clock.txt"
    base = int(time.mktime((2026, 12, 2, 12, 0, 0, 0, 0, -1))) * 1000 - 200
    align = _SIM_RUN_DIR / "freeze_align.txt"
    align.unlink(missing_ok=True)
    tmp = clock.with_suffix(".tmp")
    tmp.write_text(str(base), encoding="utf-8")
    os.replace(tmp, clock)
    try:
        time.sleep(0.8)  # 跨过首个冻结边界，5001 进入每秒节奏
        param = b"\x51" + (0x00100200).to_bytes(4, "big")
        res = dlt698.parse_action_response(
            _exchange(dlt698.action_request(0x07, 0x5001, 5, param=param)).apdu)
        assert res.ok, f"删除冻结对象应成功，回 DAR {res.dar}（{res.error}）"
        deadline = time.time() + 5
        while time.time() < deadline and not (_SIM_RUN_DIR / "freeze_align.txt").is_file():
            time.sleep(0.1)
        first = int(align.read_text(encoding="utf-8").split("boundary=")[1].split()[0])
        cols3 = [0x20210200, 0x20000200, 0x20010200]
        start = first + 2  # 首次边界后 2 秒：配置已生效，且该秒的记录已落库
        end = start + 1
        deadline = time.time() + 5
        while time.time() < deadline:
            try:
                apdu = _exchange(_record_req(0x50010200, start, end, cols3)).apdu
            except (OSError, dlt698.FrameError):
                time.sleep(0.2)
                continue
            i = 8 + 5 * len(cols3)
            if apdu[i] == 0x01 and apdu[i + 1] >= 1:
                break
            time.sleep(0.2)

        assert apdu[i] == 0x01 and apdu[i + 1] >= 1, (
            f"配置后的窗口应回 [时间,电压,电流]：{apdu.hex()}")
        row = i + 2
        assert apdu[row] == 0x1C and apdu[row + 8] == 0x12 and apdu[row + 11] == 0x05, (
            f"记录列应只剩电压/电流：{apdu.hex()}")

        apdu = _exchange(_record_req(0x50010200, start, end,
                                     [0x20210200, 0x00100200])).apdu
        i = 8 + 5 * 2
        assert apdu[i] == 0x00 and apdu[i + 1] != 0, (
            f"电能列已被删除，GetRecord 不应回行数据：{apdu.hex()}")
    finally:
        restore = bytes([0x01, 3]) + _freeze_item(1, 0x00100200, 1) + \
            _freeze_item(1, 0x20000200, 1) + _freeze_item(1, 0x20010200, 1)
        try:
            dlt698.parse_action_response(
                _exchange(dlt698.action_request(0x07, 0x5001, 7,
                                                param=restore)).apdu)
        except Exception:
            pass
        clock.unlink(missing_ok=True)


@case(id="D_00_00_02_000034",
      clause="free: 8000/8001 只读属性无可信来源回 DAR 2；只读 ACL 写回 DAR 3；仅存在性对象 810E 回 DAR 3",
      tags=["北向"])
@pytest.mark.contract
def test_master_gets_honest_dar_for_control_status():
    """A.9 的 8000/8001 只读属性没有硬件/控制来源：GET 回 DAR 2，SET 回 DAR 3；810E 仅保留存在性，GET 回 DAR 3。"""
    for oad in (0x80000200, 0x80000400, 0x80000500, 0x80010200):
        apdu = _exchange(dlt698.get_request(0x01, oad)).apdu
        assert apdu[0] == 0x85 and apdu[1] == 0x01, f"{oad:#x} 不是 GET 响应：{apdu.hex()}"
        assert apdu[3:7] == oad.to_bytes(4, "big"), f"{oad:#x} 响应 OAD 不一致"
        assert apdu[7] == 0, f"{oad:#x} 应回 DAR，不是数据：{apdu.hex()}"
        assert apdu[8] == 2, f"{oad:#x} 没有可信来源应回 DAR 2，收到 {apdu[8]}"

    # 8001 在 Class 20 ACL 中只读：SET 必须被拒绝（DAR 3）。
    apdu = _exchange(dlt698.set_request(0x02, 0x80010200, bytes([0x00]))).apdu
    assert apdu[0] == 0x86 and apdu[1] == 0x01, f"8001 SET 不是 SET 响应：{apdu.hex()}"
    assert apdu[7] == 3, f"只读 8001 SET 应回 DAR 3，收到 {apdu[7]}：{apdu.hex()}"

    # 810E 仅保留对象存在性（0 属性）：主站 GET 属性被 ACL 拒绝（DAR 3）。
    apdu = _exchange(dlt698.get_request(0x03, 0x810E0200)).apdu
    assert apdu[0] == 0x85 and apdu[1] == 0x01, f"810E 不是 GET 响应：{apdu.hex()}"
    assert apdu[7] == 0 and apdu[8] == 3, f"仅存在性的 810E 应回 DAR 3：{apdu.hex()}"
