"""最小 IEC104 主站:iems 北向验收专用。

test-platform 没有 IEC104 驱动(它只驱动 serial/tcp 上的 Modbus),而北向这一面
只能从协议字节上验。这里只实现验收用得到的那部分:

  - U 帧:STARTDT ACT/CON、TESTFR ACT/CON
  - I 帧:发总召(C_IC_NA_1)与单点命令(C_SC_NA_1),收并解析 ASDU
  - ASDU:类型 13(M_ME_NC_1,短浮点测量值)与 100(C_IC_NA_1,总召)

没有 t1/t2/t3 重传、没有 k/w 流控。序号只做一件事:把收到的 I 帧在下一帧
里确认掉,否则服务端 k=12 的发送窗口会在十几帧后关死。

不是通用库。换一个对端(例如 iems_v3)时帧结构差异要重新对拍,不能拿它
当兼容性结论。
"""
from __future__ import annotations

import socket
import struct
import time
from dataclasses import dataclass, field

# 类型标识
TYPE_M_ME_NC_1 = 13
TYPE_C_SC_NA_1 = 45
TYPE_C_IC_NA_1 = 100

# 传送原因(低 6 位;bit6 = 否定确认,bit7 = 试验)
COT_SPONT = 3
COT_ACT = 6
COT_ACT_CON = 7
COT_ACT_TERM = 10
COT_UNKNOWN_TYPE_ID = 44
COT_UNKNOWN_COT = 45
COT_UNKNOWN_CA = 46
COT_UNKNOWN_IOA = 47

STARTDT_ACT = bytes([0x68, 0x04, 0x07, 0x00, 0x00, 0x00])
STARTDT_CON = bytes([0x68, 0x04, 0x0B, 0x00, 0x00, 0x00])
STOPDT_ACT = bytes([0x68, 0x04, 0x13, 0x00, 0x00, 0x00])
STOPDT_CON = bytes([0x68, 0x04, 0x23, 0x00, 0x00, 0x00])
TESTFR_ACT = bytes([0x68, 0x04, 0x43, 0x00, 0x00, 0x00])
TESTFR_CON = bytes([0x68, 0x04, 0x83, 0x00, 0x00, 0x00])

# APCI 长度域 = 4 字节控制域 + ASDU;ASDU 上限 249,所以合法范围 [4, 253]
_APCI_CONTROL_BYTES = 4
_APCI_LEN_MIN = 4
_APCI_LEN_MAX = 253


@dataclass
class Asdu:
    """解出来的应用层数据。cot 保留线上原字节,negative/cot_value 再拆。"""

    type_id: int
    sq: bool
    count: int
    cot: int
    casdu: int
    body: bytes
    objects: list = field(default_factory=list)  # type 13: (ioa, value, qds)

    @property
    def cot_value(self) -> int:
        return self.cot & 0x3F

    @property
    def negative(self) -> bool:
        return bool(self.cot & 0x40)

    @property
    def ioa(self) -> int | None:
        if self.objects:
            return self.objects[0][0]
        if len(self.body) >= 3:
            return int.from_bytes(self.body[:3], "little")
        return None

    @property
    def qoi(self) -> int | None:
        if self.type_id == TYPE_C_IC_NA_1 and len(self.body) >= 4:
            return self.body[3]
        return None


@dataclass
class Frame:
    kind: str            # "I" / "S" / "U"
    raw: bytes
    control: int = 0     # U 帧控制字节 / I 帧的 N(S) / S 帧的 N(R)
    asdu: Asdu | None = None

    def __str__(self) -> str:
        if self.asdu is None:
            return f"{self.kind} frame {self.raw.hex()}"
        a = self.asdu
        neg = " neg" if a.negative else ""
        return (f"I type={a.type_id} cot={a.cot_value}{neg} ca={a.casdu} "
                f"objs={a.objects} body={a.body.hex()}")


def parse_asdu(raw: bytes) -> Asdu:
    if len(raw) < 6:
        raise ValueError(f"ASDU 不足 6 字节: {raw.hex()}")
    type_id, vsq, cot = raw[0], raw[1], raw[2]
    casdu = int.from_bytes(raw[4:6], "little")
    body = raw[6:]
    sq = bool(vsq & 0x80)
    count = vsq & 0x7F

    objects = []
    if type_id == TYPE_M_ME_NC_1:
        if sq:
            # 顺序地址:首地址 3 字节,之后每个对象是 float(4) + qds(1)
            base = int.from_bytes(body[:3], "little") if count else 0
            for i in range(count):
                off = 3 + i * 5
                value = struct.unpack_from("<f", body, off)[0]
                objects.append((base + i, value, body[off + 4]))
        else:
            # 独立地址:每个对象是 ioa(3) + float(4) + qds(1)
            for i in range(count):
                off = i * 8
                ioa = int.from_bytes(body[off:off + 3], "little")
                value = struct.unpack_from("<f", body, off + 3)[0]
                objects.append((ioa, value, body[off + 7]))
    return Asdu(type_id, sq, count, cot, casdu, body, objects)


def parse_frame(raw: bytes) -> Frame:
    if len(raw) < 6 or raw[0] != 0x68:
        raise ValueError(f"不是 APCI 帧: {raw.hex()}")
    first = raw[2]
    if first & 0x03 == 0x03:  # U:控制域最低两位 11
        return Frame("U", raw, control=first)
    if first & 0x03 == 0x01:  # S:01,N(R) 在 4-5 字节
        nr = (raw[4] | (raw[5] << 8)) >> 1
        return Frame("S", raw, control=nr)
    ns = (raw[2] | (raw[3] << 8)) >> 1
    return Frame("I", raw, control=ns, asdu=parse_asdu(raw[6:]))


def build_asdu(type_id: int, cot: int, casdu: int, body: bytes) -> bytes:
    """ASDU 头:类型 | VSQ(count=1,SQ=0) | COT | 源发址 | 公共地址(LE)。"""
    return bytes([type_id, 0x01, cot, 0x00]) + casdu.to_bytes(2, "little") + body


def gi_asdu(casdu: int = 1, qoi: int = 20, cot: int = COT_ACT) -> bytes:
    return build_asdu(TYPE_C_IC_NA_1, cot, casdu, b"\x00\x00\x00" + bytes([qoi]))


def sc_asdu(ioa: int, scs: int = 1, casdu: int = 1, cot: int = COT_ACT) -> bytes:
    return build_asdu(TYPE_C_SC_NA_1, cot, casdu, ioa.to_bytes(3, "little") + bytes([scs]))


class Iec104Client:
    """一条 TCP 连接上的主站。超时单位是秒,异常原样抛给用例。"""

    def __init__(self, host: str, port: int, casdu: int = 1, timeout: float = 5.0):
        self.casdu = casdu
        self.timeout = timeout
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.buf = b""
        self.tx_ns = 0
        self.last_rx_ns: int | None = None

    def close(self) -> None:
        try:
            self.sock.close()
        except OSError:
            pass

    def wait_peer_close(self, timeout: float = 5.0) -> bool:
        """等对端关闭连接:EOF 或 reset 都算;超时/收到数据返回 False。"""
        self.sock.settimeout(timeout)
        try:
            chunk = self.sock.recv(1)
        except ConnectionResetError:
            return True
        except TimeoutError:
            return False
        return not chunk

    def __enter__(self) -> "Iec104Client":
        return self

    def __exit__(self, *_exc) -> None:
        self.close()

    # ---------------------------------------------------------------- 发送

    def send_u(self, frame: bytes) -> None:
        self.sock.sendall(frame)

    def send_s(self) -> None:
        """确认已收到的 I 帧。已经确认过就重发同一个 N(R),服务端无副作用。"""
        if self.last_rx_ns is None:
            return
        nr = (self.last_rx_ns + 1) & 0x7FFF
        self.sock.sendall(b"\x68\x04\x01\x00" + struct.pack("<H", nr << 1))

    def send_i(self, asdu: bytes) -> None:
        nr = ((self.last_rx_ns + 1) if self.last_rx_ns is not None else 0) & 0x7FFF
        apci = (bytes([0x68, len(asdu) + _APCI_CONTROL_BYTES])
                + struct.pack("<HH", (self.tx_ns & 0x7FFF) << 1, nr << 1))
        self.sock.sendall(apci + asdu)
        self.tx_ns = (self.tx_ns + 1) & 0x7FFF

    def send_gi(self, casdu: int | None = None, qoi: int = 20) -> None:
        self.send_i(gi_asdu(self.casdu if casdu is None else casdu, qoi))

    def send_c_sc(self, ioa: int, scs: int = 1, casdu: int | None = None) -> None:
        self.send_i(sc_asdu(ioa, scs, self.casdu if casdu is None else casdu))

    # ---------------------------------------------------------------- 接收

    def _fill(self, need: int, deadline: float) -> None:
        while len(self.buf) < need:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError("等帧超时")
            self.sock.settimeout(remaining)
            chunk = self.sock.recv(4096)
            if not chunk:
                raise ConnectionError("对端关闭了连接")
            self.buf += chunk

    def recv_frame(self, timeout: float | None = None) -> Frame:
        deadline = time.monotonic() + (self.timeout if timeout is None else timeout)
        while True:
            idx = self.buf.find(b"\x68")
            if idx >= 0:
                self.buf = self.buf[idx:]
                break
            # 没有帧头:丢掉垃圾,重新等一个字节
            self.buf = b""
            self._fill(1, deadline)
        if len(self.buf) < 2:
            self._fill(2, deadline)
        total_len = self.buf[1]
        if not _APCI_LEN_MIN <= total_len <= _APCI_LEN_MAX:
            # 对端给了不可能的声明长度:丢掉帧头两字节重新找 0x68
            self.buf = self.buf[2:]
            return self.recv_frame(max(0.01, deadline - time.monotonic()))
        total = total_len + 2
        if len(self.buf) < total:
            self._fill(total, deadline)
        raw, self.buf = self.buf[:total], self.buf[total:]
        frame = parse_frame(raw)
        if frame.kind == "I" and frame.asdu is not None:
            self.last_rx_ns = frame.control
        return frame

    def collect(self, timeout: float = 2.0, until=None) -> list[Frame]:
        """收帧直到 until(frames) 为真或超时。结束时确认一次收到的 I 帧。"""
        frames: list[Frame] = []
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            wait = min(0.5, max(0.01, deadline - time.monotonic()))
            try:
                frame = self.recv_frame(timeout=wait)
            except TimeoutError:
                continue
            frames.append(frame)
            if until is not None and until(frames):
                break
        self.send_s()
        return frames
