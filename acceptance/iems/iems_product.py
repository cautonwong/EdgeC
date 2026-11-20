"""iems 产品壳:把 test-platform 的 launch 契约翻译成 iems 能接受的形态。

iems 的硬事实(都核过源码):
  1. cmd/iems/main.c:19-20  `(void)argc; (void)argv;`   —— 不认命令行参数
  2. cmd/iems/main.c:44-53  EDGE_DB_PATH / EDGE_REST_PORT / EDGE_WAL_PATH …
                            —— 配置只能走 env
  3. cmd/iems/iems_app.c:253-300  启动后每 500ms 去 connect 总线 socket
                            —— 从站必须先起
  4. cmd/iems/iems_task_loader.c:44-79  轮询任务不是产品内置的:
     它从 sqlite 里 `poll_tasks JOIN inventory` 读,并且要求
     `matrix_meta.is_ready = '1'`。库是空的 → 一帧都不发。

所以壳做四件事,顺序不能换:
  ① provision 一份 sqlite(matrix_meta / inventory / poll_tasks)
  ② 在 /tmp/vbus0.sock 当 Modbus RTU 从站,把收到的请求记进 bus.json
  ③ listen 127.0.0.1:${PORT},再由 EDGE_REST_PORT 让 iems 的 REST 落在这个端口
  ④ exec iems,它死了壳不退(退出码落 iems_exit.json)

用法: python iems_product.py --port 23456
      python iems_product.py --port 23456 --slaves 1,3          # 多从站:控制写路由用例
      python iems_product.py --port 23456 --transport uart     # 真实 UART 形态:PTY 对
      python iems_product.py --port 23456 --transport tcp      # RTU over TCP
      python iems_product.py --port 23456 --transport mbap     # Modbus TCP(MBAP 帧)
      python iems_product.py --port 23456 --start-gate         # 等 pytest 订上观察器再起产品
      python iems_product.py --port 23456 --expect-boot-fail   # 启动闸门用:不给总线
"""
import argparse
import asyncio
import pty
import signal
import json
import os
import socket
import sqlite3
import struct
import subprocess
import sys
import threading
import time
import tty
from pathlib import Path

from bench import (BRIDGE_PORT, BUS_LOG, BUS_ID, DB_PATH, DROPPED_LOG, IEMS_BIN,
                   IEMS_SRC_DIRS, NORTH_SOCK, RUN_DIR, SOCK, SLAVE_ID,
                   TAGS_BY_SLAVE, VOLTAGE_REGISTER, describe, staleness)
from testplatform.protocol import modbus_rtu
from testplatform.protocol.crc import crc16_modbus

_lock = threading.Lock()
_fault = {"kind": None, "left": 0}
# 慢应答延迟:既有用例要 3s(超过超时),积压用例要 1.2s(低于超时但慢于轮询)
_slow_ms = 3000
# 北向桥的上游错误只喊一次：每帧都喊会把 product.err 刷满，反而看不见别的。
_bridge_error_reported = False
# iems 北向从站是单客户端语义:并发开两路上游会被产品拒掉。平台侧可能同时有
# 多条 TCP 连接(每条连接一个桥线程),上游交换必须串行,close 完再开下一条。
_upstream_lock = threading.Lock()
# 掉链:服务 N 帧后主动断掉这条南向连接,模拟现场断线(产品不许自己重连)
_drop = {"after": 0, "done": False}
# 真实流式传输把每次应答拆成两段写:一次 read 不是一帧,对端必须自己攒。
# UDS 台架保持整帧写,既有用例的时序不受影响。
_split_writes = False
_seen = {"requests": 0, "first_request_hex": None, "addresses": [],
         "reads": [], "writes": []}
# 可变寄存器文件按从站分开:多从站共用一个,unit 1 的写会改到 unit 3 的读数。
# 0 号是电压镜像（既有用例依赖它恒为台架值）。
_regs = {}
_slaves = {SLAVE_ID}


def _dump_json(path, payload) -> None:
    """原子替换:用例随时在轮询读,write_text 会先截断再写,读到空文件就炸。"""
    tmp = Path(str(path) + ".tmp")
    tmp.write_text(json.dumps(payload, ensure_ascii=False), encoding="utf-8")
    os.replace(tmp, path)


def _should_drop() -> bool:
    """第 N 帧之后断链。只在锁内判一次,之后的帧不再更新计数。"""
    with _lock:
        if not _drop["after"] or _drop["done"] or _seen["requests"] < _drop["after"]:
            return False
        _drop["done"] = True
        _dump_json(DROPPED_LOG, {"requests": _seen["requests"]})
        return True


def _note_write(unit: int, address: int, values: list[int], function: int) -> None:
    """记下主站写进来的值。写应答只是回显，**不能拿它证明写生效了**。

    unit/values 是多从站台架的判据面:同一个地址写给 unit 1 还是 unit 3,
    现场是两个不同的设备。value 保留单寄存器写的旧含义,FC16 取合包值。
    function 是判定"北向的 FC06 有没有原样变成南向 FC06"的唯一证据:只看
    values 分不出单寄存器写和一寄存器 FC16。
    """
    with _lock:
        unit_regs = _regs.setdefault(unit, {})
        for i, value in enumerate(values):
            unit_regs[address + i] = value
        combined = int.from_bytes(b"".join(v.to_bytes(2, "big") for v in values), "big")
        _seen["writes"].append({
            "unit": unit,
            "function": function,
            "address": address,
            "value": values[0] if len(values) == 1 else combined,
            "values": values,
            "at": time.time(),
        })
        _dump_json(BUS_LOG, _seen)


def _record(request: bytes, unit: int, function: int, address: int, count: int = 0) -> None:
    with _lock:
        _seen["requests"] += 1
        if _seen["first_request_hex"] is None:
            _seen["first_request_hex"] = request.hex()
        if address not in _seen["addresses"]:
            _seen["addresses"].append(address)
        if function in (modbus_rtu.READ_HOLDING_REGISTERS, modbus_rtu.READ_INPUT_REGISTERS):
            _seen["reads"].append({
                "unit": unit, "function": function, "address": address,
                "count": count, "at": time.time(),
            })
        _dump_json(BUS_LOG, _seen)


def _parse_request(frame: bytes) -> tuple[str, int, int, int, int, list[int]]:
    """一帧 → (kind, unit, function, address, count, values)。

    读请求没有 values;FC06 一个值;FC16 按寄存器拆成列表。三种帧都试一遍,
    而不是按功能码分支 —— 功能码在哪些帧里合法由协议模块判,台架不重复一份。
    """
    try:
        unit, function, address, count = modbus_rtu.parse_read_request(frame)
        return "read", unit, function, address, count, []
    except ValueError:
        pass
    try:
        unit, function, address, value = modbus_rtu.parse_write_single_request(frame)
        return "write", unit, function, address, 1, [value]
    except ValueError:
        pass
    unit, function, address, count, payload = modbus_rtu.parse_write_multiple_request(frame)
    values = [int.from_bytes(payload[i:i + 2], "big") for i in range(0, len(payload), 2)]
    return "write_multi", unit, function, address, count, values


async def _read_frame(reader) -> bytes:
    """按功能码收满一帧。read(256) 拿到半帧时解析失败,会被记成产品发坏帧。"""
    head = await reader.readexactly(2)
    if head[1] == modbus_rtu.WRITE_MULTIPLE_REGISTERS:
        fixed = await reader.readexactly(5)  # 起始地址 2 + 寄存器数 2 + 字节数 1
        data = await reader.readexactly(int(fixed[4]) + 2)
        return head + fixed + data
    return head + await reader.readexactly(6)


def provision(slaves: set[int], protocol: str = "MODBUS") -> None:
    """iems 的轮询任务表。空库 = 不轮询，所以这不是可选步骤。

    每个从站一条 inventory;poll_tasks 按 bench.TAGS_BY_SLAVE 挂:
    设备 1 采集 1001/1002,设备 3 挂 1003 —— 控制写才有唯一路由。
    protocol 决定产品用哪个驱动(ModbusTCP → MBAP 帧,其余 → RTU)。
    """
    RUN_DIR.mkdir(parents=True, exist_ok=True)
    for suffix in ("", "-wal", "-shm"):
        p = Path(str(DB_PATH) + suffix)
        if p.exists():
            p.unlink()
    conn = sqlite3.connect(DB_PATH)
    conn.executescript("""
    CREATE TABLE IF NOT EXISTS matrix_meta (key TEXT PRIMARY KEY, val TEXT);
    CREATE TABLE IF NOT EXISTS inventory (id INTEGER PRIMARY KEY, addr TEXT, protocol TEXT);
    CREATE TABLE IF NOT EXISTS poll_tasks (bus_id INTEGER, device_id INTEGER, tag_id INTEGER);
    """)
    conn.execute("INSERT INTO matrix_meta (key, val) VALUES ('is_ready', '1')")
    conn.executemany("INSERT INTO inventory (id, addr, protocol) VALUES (?, ?, ?)",
                     [(slave, str(slave), protocol) for slave in sorted(slaves)])
    conn.executemany("INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (?, ?, ?)",
                     [(BUS_ID, slave, tag)
                      for slave in sorted(slaves)
                      for tag in TAGS_BY_SLAVE.get(slave, ())])
    conn.commit()
    conn.close()


def _fail(where: str, exc: BaseException) -> None:
    """台架自己的异常必须留痕。

    这个坑踩过一次:应答帧拼错(把寄存器列表当 payload 传给
    build_read_response)时,协程里抛 TypeError,asyncio 不 retrieve 异常、
    一声不响 —— 从站第 1 帧之后既不回包也不再 read,而验收看到的是
    「产品只发了 1 帧」。台架的 bug 被记成产品的缺陷,是最贵的一种错。
    """
    with _lock:
        _seen.setdefault("errors", []).append(f"{where}: {type(exc).__name__}: {exc}")
        _dump_json(BUS_LOG, _seen)


async def _handle_frame(frame: bytes,
                        record_frame: bytes | None = None) -> tuple[bytes | None, bool]:
    """一帧请求 → (应答帧, 是否继续)。None = 这一帧不回;False = 断开这条连接。

    UDS / TCP / UART 三条传输共用这一份判据:同一帧在台架上怎么算,
    真实链路上就怎么算,不会出现两套规则。record_frame 只影响 bus.json
    里留下的原始字节(MBAP 传真实帧,判据用的合成 RTU 帧不入证据)。
    """
    raw = frame if record_frame is None else record_frame
    try:
        kind, unit, function, address, count, values = _parse_request(frame)
    except ValueError as exc:
        _record(raw, -1, -1, -1)
        _fail("parse_request", exc)
        return None, True
    _record(raw, unit, function, address, count)
    if _should_drop():
        # 不回帧、直接关连接:iems 侧看到的就是链路断了
        return None, False
    if unit not in _slaves:
        return None, False
    if kind == "read":
        payload = b"".join(
            struct.pack(">H", _regs.get(unit, {}).get(address + i, 0))
            for i in range(count)
        )
        response = modbus_rtu.build_read_response(unit, payload, function)
        return await _inject_fault(response, unit, function, count), True
    _note_write(unit, address, values, function)
    if kind == "write":
        return modbus_rtu.build_write_single_response(unit, address, values[0]), True
    return modbus_rtu.build_write_multiple_response(unit, address, count), True


async def _write_response(writer, response: bytes) -> None:
    if not _split_writes or len(response) < 2:
        writer.write(response)
        await writer.drain()
        return
    half = len(response) // 2
    writer.write(response[:half])
    await writer.drain()
    await asyncio.sleep(0.02)
    writer.write(response[half:])
    await writer.drain()


async def _serve(reader, writer):
    try:
        while True:
            try:
                frame = await _read_frame(reader)
            except asyncio.IncompleteReadError:
                break
            response, keep = await _handle_frame(frame)
            if not keep:
                break
            if response is not None:
                await _write_response(writer, response)
    except ConnectionError:
        # 对端被 SIGKILL:内核回 RST/EPIPE,这是崩溃路径的正常收尾,不是台架故障。
        pass
    except OSError as exc:
        _fail("serve", exc)
    except BaseException as exc:
        _fail("serve", exc)
        raise
    finally:
        writer.close()


async def _inject_fault(frame: bytes, unit: int, function: int, count: int):
    """按 --fault 制造应答故障。返回 None 表示这一帧不回(让主站超时)。"""
    with _lock:
        kind, left = _fault["kind"], _fault["left"]
        if kind and left > 0:
            _fault["left"] = left - 1
        else:
            kind = None
    if kind:
        faults_path = RUN_DIR / "faults.json"
        served = []
        if faults_path.is_file():
            try:
                served = json.loads(faults_path.read_text(encoding="utf-8"))
            except ValueError:
                served = []  # 上一轮写了一半的文件,当没有,不要把台架弄崩
        served.append(kind)
        _dump_json(faults_path, served)
    if kind == "slow":
        await asyncio.sleep(max(_slow_ms, 0) / 1000.0)  # 慢于轮询:iems 必须扛住并恢复
        return frame
    if kind == "bad-crc":
        # 载荷故意是个离谱值(999 -> 99.9V),CRC 故意错:主站必须整帧丢弃
        bogus = modbus_rtu.build_read_response(
            unit, b"".join(struct.pack(">H", 999) for _ in range(count)), function
        )
        return bogus[:-1] + bytes([bogus[-1] ^ 0xFF])
    if kind == "silent":
        return None
    return frame


async def _unix_bus() -> None:
    try:
        Path(SOCK).unlink()
    except FileNotFoundError:
        pass
    server = await asyncio.start_unix_server(_serve, path=SOCK)
    async with server:
        await server.serve_forever()


def _frame_size(buf: bytearray) -> int | None:
    """按功能码算整帧长度;None = 还没收全。

    与 _read_frame 的固定长度一致。PTY/TCP 上的读会拆包、合包,一次 read
    的长度不是帧边界 —— 这正是 UDS 验收看不到的那部分。
    """
    if len(buf) < 2:
        return None
    if buf[1] == modbus_rtu.WRITE_MULTIPLE_REGISTERS:
        if len(buf) < 7:
            return None
        return 7 + buf[6] + 2  # 头 2 + 起始/数量/字节数 5 + 数据 + CRC 2
    return 8


def _uart_bus(master_fd: int) -> None:
    """PTY 主端的 RTU 从站:阻塞读 + 自己的缓冲,帧处理复用 _handle_frame。

    从设备端(/dev/pts/N)由产品按真实 UART 打开 —— cfmakeraw、波特率、
    O_NONBLOCK 都在产品侧,这条线程只对待发送的字节负责。
    """
    loop = asyncio.new_event_loop()
    buf = bytearray()
    try:
        while True:
            buf += os.read(master_fd, 256)
            while True:
                size = _frame_size(buf)
                if size is None or len(buf) < size:
                    break
                frame = bytes(buf[:size])
                del buf[:size]
                response, keep = loop.run_until_complete(_handle_frame(frame))
                if not keep:
                    return
                if response:
                    if _split_writes and len(response) > 1:
                        half = len(response) // 2
                        os.write(master_fd, response[:half])
                        time.sleep(0.02)
                        os.write(master_fd, response[half:])
                    else:
                        os.write(master_fd, response)
    except OSError as exc:
        _fail("uart_bus", exc)


async def _tcp_bus(port: int) -> None:
    server = await asyncio.start_server(_serve, "127.0.0.1", port)
    async with server:
        await server.serve_forever()


MBAP_HEADER_LEN = 7


def _mbap_wrap(tid: int, unit: int, pdu: bytes) -> bytes:
    """MBAP 头(事务号 2 + 协议号 2 + 长度 2 + 单元号 1)+ PDU,没有 CRC。"""
    return struct.pack(">HHHB", tid, 0, len(pdu) + 1, unit) + pdu


async def _read_mbap_frame(reader) -> bytes:
    head = await reader.readexactly(MBAP_HEADER_LEN)
    length = int.from_bytes(head[4:6], "big")
    if length < 2 or length > 254:
        return head  # 坏长度:交给 _handle_mbap 记录并丢弃
    return head + await reader.readexactly(length - 1)


async def _handle_mbap(frame: bytes) -> tuple[bytes | None, bool]:
    """MBAP 请求 → MBAP 应答,判据复用 _handle_frame。

    MBAP 的 PDU 与 RTU 的 PDU 逐字节相同,只差帧壳。台架把 PDU 补一个合法
    CRC 冒充 RTU 帧走同一份 _handle_frame,应答再去掉 CRC 套回 MBAP 头 ——
    "这帧算不算数"因此只有一份规则;bus.json 记的仍是真实 MBAP 帧。
    """
    tid, proto, length, unit = struct.unpack(">HHHB", frame[:MBAP_HEADER_LEN])
    pdu = frame[MBAP_HEADER_LEN:]
    if proto != 0 or length != len(pdu) + 1:
        _record(frame, unit, -1, -1)
        _fail("parse_mbap", ValueError(f"bad MBAP header: {frame.hex()}"))
        return None, True
    rtu = bytes([unit]) + pdu
    rtu += struct.pack("<H", crc16_modbus(rtu))
    response, keep = await _handle_frame(rtu, record_frame=frame)
    if response is None:
        return None, keep
    return _mbap_wrap(tid, response[0], response[1:-2]), keep


async def _serve_mbap(reader, writer) -> None:
    try:
        while True:
            try:
                frame = await _read_mbap_frame(reader)
            except asyncio.IncompleteReadError:
                break
            response, keep = await _handle_mbap(frame)
            if not keep:
                break
            if response is not None:
                writer.write(response)
                await writer.drain()
    except (ConnectionError, OSError) as exc:
        _fail("serve_mbap", exc)
    except BaseException as exc:
        _fail("serve_mbap", exc)
        raise
    finally:
        writer.close()


async def _mbap_bus(port: int) -> None:
    server = await asyncio.start_server(_serve_mbap, "127.0.0.1", port)
    async with server:
        await server.serve_forever()


def _note_bridge_error(exc: OSError) -> None:
    """记下"桥连不上产品 socket"这件事，并只喊一次。

    静默丢弃的代价：平台侧只看到"连上了但没应答"，于是判产品不回应；
    而真正的原因可能是路径不一致、产品还没绑上、或二进制根本不认这个 env。
    这两种原因的处理方式完全不同，所以必须把错误本身留下来。
    """
    global _bridge_error_reported
    payload = {"socket": NORTH_SOCK, "error": f"{type(exc).__name__}: {exc}"}
    try:
        (RUN_DIR / "bridge_error.json").write_text(
            json.dumps(payload, ensure_ascii=False), encoding="utf-8"
        )
    except OSError:
        pass
    if not _bridge_error_reported:
        _bridge_error_reported = True
        print(f"⚠️  北向桥连不上 {NORTH_SOCK}: {exc}", file=sys.stderr, flush=True)


def _north_bridge() -> None:
    """unix socket → TCP:iems 的北向从站只监听 unix socket,而平台只认 serial/tcp。

    一个连接一对 pump;连不上 iems 的 socket 就直接关掉这条 —— 平台那边看到的是
    "连上了但没应答",由它自己记超时,不需要壳替它编一个应答。
    """
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # 上一轮的壳刚死时端口还没完全放出来:重试 10 秒,别让一次 bind 失败就
    # 把整条验收判成"产品没起来"(实测踩过,假红)。
    deadline = time.time() + 30.0
    while True:
        try:
            server.bind(("127.0.0.1", BRIDGE_PORT))
            break
        except OSError as exc:
            if time.time() > deadline:
                # 台架自己起不来:留痕,让用例把原因说清,而不是炸在后台线程里
                (RUN_DIR / "bridge_error.json").write_text(
                    json.dumps({"port": BRIDGE_PORT, "error": str(exc)}, ensure_ascii=False),
                    encoding="utf-8")
                return
            time.sleep(0.5)
    server.listen(8)
    while True:
        conn, _ = server.accept()
        threading.Thread(target=_serve_tcp, args=(conn,), daemon=True).start()


def _connect_upstream() -> socket.socket:
    """连产品那条 unix socket，短暂重试。

    只试一次太脆：连续跑多个任务时，上一个任务的产品刚被平台 SIGKILL，
    新的产品可能还没绑上；一次 ECONNREFUSED 就把整帧丢掉，平台看到的是
    "产品不回应"，而产品其实 200ms 后就正常了（实测：同一任务单跑必过、
    连跑偶发失败，就是这个）。

    重试窗口刻意短（1 秒）：产品真的没绑，仍然会失败 —— 只是晚一点，
    不会被这个重试掩盖掉。
    """
    deadline = time.time() + 1.0
    while True:
        up = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        up.settimeout(2.0)
        try:
            up.connect(NORTH_SOCK)
            return up
        except OSError:
            up.close()
            if time.time() >= deadline:
                raise
            time.sleep(0.1)


def _serve_tcp(conn: socket.socket) -> None:
    """一条 TCP 连接 = 一串请求/应答,每个请求换一条到 iems 的 unix 连接。

    以前两条 pump 线程长期挂在同一条 unix 连接上:iems 一旦关上游(重启北向、
    正常退出、协议层主动断),平台侧就收到 ConnectionError,把产品自己关连接
    记成网络故障。改成每个请求现连一次 —— iems 关连接不再波及 TCP 侧,
    而"不回应"仍然是沉默,平台的超时逻辑照常生效。

    上游交换走全局锁:iems 只服务一个客户端,两条 TCP 连接同时发请求时,
    后一条上游连接会被产品拒掉(与"IEC104 单客户端"同一语义)。
    """
    with conn:
        while True:
            try:
                req = conn.recv(4096)
            except OSError:
                return
            if not req:
                return
            try:
                with _upstream_lock:
                    up = _connect_upstream()
                    try:
                        up.sendall(req)
                        while True:
                            try:
                                chunk = up.recv(4096)
                            except socket.timeout:
                                break
                            if not chunk:
                                break
                            try:
                                conn.sendall(chunk)
                            except OSError:
                                return
                            up.settimeout(0.2)  # 收完一帧就别等下一帧
                    finally:
                        up.close()
            except OSError as exc:
                # 上游不在:这一帧就是没应答,平台自己记超时。
                # 但**必须留痕** —— 静默 continue 时现场只能看到"设备不回应",
                # 而真正的原因是桥连不上产品的 socket(路径不一致 / 产品还没绑上)。
                _note_bridge_error(exc)
                continue


def _hold_port(port: int) -> None:
    """只占住端口让就绪探针通过;不服务任何请求。"""
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", port))
    srv.listen(8)

    def loop() -> None:
        while True:
            try:
                conn, _ = srv.accept()
                conn.close()
            except OSError:
                return

    threading.Thread(target=loop, daemon=True).start()


def _free_port() -> int:
    """要一个此刻空闲的端口。产品不认 0 = 内核分配,IEC104 也不走平台端口。"""
    s = socket.socket()
    try:
        s.bind(("127.0.0.1", 0))
        return int(s.getsockname()[1])
    finally:
        s.close()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--slaves", default="1",
                        help="总线上要响应的 Modbus 从站,逗号分隔(默认 1);"
                             "多从站用于验控制写的路由")
    parser.add_argument("--fault", choices=["bad-crc", "slow", "silent"],
                        help="对前若干帧应答制造故障(故障注入用例用)")
    parser.add_argument("--fault-frames", type=int, default=2)
    parser.add_argument("--slow-ms", type=int, default=3000,
                        help="--fault slow 时每条应答的延迟毫秒数(缺省 3000,超过 2s 超时)")

    parser.add_argument("--drop-after", type=int, default=0,
                        help="服务 N 帧后主动断开南向连接(链路断开用例用)")
    parser.add_argument("--record-startup", action="store_true",
                        help="把 iems 的输出同时落一份到 RUN_DIR/iems.log,供用例断言启动汇总行")
    parser.add_argument("--transport", choices=["uds", "uart", "tcp", "mbap"], default="uds",
                        help="南向总线 0 的物理形态与帧格式:UDS 台架 socket(缺省)、"
                             "UART(PTY 对,产品按 /dev/ 打开)、TCP(RTU over TCP)或"
                             "MBAP(Modbus TCP 帧,inventory 协议写 ModbusTCP)")
    parser.add_argument("--start-gate", action="store_true",
                        help="先占住 ${PORT} 让就绪探针通过,等 RUN_DIR/start.request 再起产品"
                             "(MQTT 观察器要先订上,才看得到每会话只发一次的 BIRTH)")
    parser.add_argument("--expect-boot-fail", action="store_true",
                        help="不给任何总线/从站,产品应该启动失败而不是重连")
    args = parser.parse_args()
    slaves = {int(s) for s in args.slaves.split(",") if s.strip()}
    if not slaves:
        parser.error("--slaves 至少给一个从站地址")
    with _lock:
        _slaves.clear()
        _slaves.update(slaves)
        _regs.clear()
        for slave in slaves:
            _regs[slave] = {}
    if SLAVE_ID in _regs:
        _regs[SLAVE_ID][0] = VOLTAGE_REGISTER
    if args.fault:
        _fault["kind"], _fault["left"] = args.fault, args.fault_frames
    _drop["after"] = args.drop_after
    global _split_writes, _slow_ms
    _split_writes = args.transport in ("uart", "tcp")
    _slow_ms = args.slow_ms

    # 先把"验的是哪份二进制"说清楚，再谈结论。
    # 曾经对着三周前的二进制跑出绿的结论：env 里的新变量被旧二进制无视，
    # 现象却像"环境变量没传进去"。所以定位与过期判定必须在启动第一句。
    print(describe(IEMS_BIN), flush=True)
    stale = staleness(IEMS_BIN, IEMS_SRC_DIRS)
    if stale:
        print(f"⚠️  {stale}", file=sys.stderr, flush=True)

    RUN_DIR.mkdir(parents=True, exist_ok=True)
    _dump_json(BUS_LOG, _seen)
    (RUN_DIR / "faults.json").unlink(missing_ok=True)  # 上一轮的注入记录不能留
    (RUN_DIR / "dropped.json").unlink(missing_ok=True)
    # 传输形态落盘:用例收集期据此只保留匹配的那一档(见 tests 的 transport 标记)。
    # --start-gate 的任务用 "restart" 这一档,把它和 UDS 验收隔开。
    (RUN_DIR / "transport.txt").write_text(
        "restart" if args.start_gate else args.transport, encoding="utf-8")

    # 总线 0 的端点由传输形态决定:产品按 OMNIS_BUS_0_DEV 选路 —
    # tcp:// 走 TCP,type=UART 且 /dev/ 开头走真实 UART(iems_southbound_adapter.c)。
    bus_dev = SOCK
    bus_protocol = "MODBUS"
    uart_master = tcp_port = None
    if not args.expect_boot_fail:
        if args.transport == "uart":
            # 保留 slave 端 fd:主端 read 在从端全关时会 EIO,持有它让读一直可等。
            uart_master, uart_slave = pty.openpty()
            tty.setraw(uart_master)
            bus_dev = os.ttyname(uart_slave)
        elif args.transport in ("tcp", "mbap"):
            tcp_port = _free_port()
            bus_dev = f"tcp://127.0.0.1:{tcp_port}"
            if args.transport == "mbap":
                # 产品按 inventory.protocol 选驱动;ModbusTCP 驱动发 MBAP 帧。
                bus_protocol = "ModbusTCP"
    provision(slaves, bus_protocol)

    if not args.expect_boot_fail:
        loop = asyncio.new_event_loop()
        if args.transport == "mbap":
            bus_target = lambda: loop.run_until_complete(_mbap_bus(tcp_port))
        elif args.transport == "tcp":
            bus_target = lambda: loop.run_until_complete(_tcp_bus(tcp_port))
        else:
            bus_target = lambda: loop.run_until_complete(_unix_bus())
        threading.Thread(target=bus_target, daemon=True).start()
        if args.transport == "uart":
            threading.Thread(target=_uart_bus, args=(uart_master,), daemon=True).start()
        threading.Thread(target=_north_bridge, daemon=True).start()
        time.sleep(0.3)
    # 这个端口不自己占:iems 的 REST 直接绑 EDGE_REST_PORT,平台的就绪探针
    # connect 到的是 iems 自己。壳再起一个假监听会把端口抢走,REST 绑不上,
    # 探针连到的是壳然后被 reset —— 表现为「产品起来了但 REST 全崩」。
    (RUN_DIR / "port.txt").write_text(str(args.port), encoding="utf-8")
    # 北向 IEC104 是产品自己起的 TCP 监听(契约表 bus 11):端口由环境变量给,
    # 产品不认 0 = 内核分配。壳先要一个空闲端口并落文件,用例与产品读同一个值;
    # 写死 2404 在并行任务/残留进程下会撞端口。
    iec104_port = _free_port()
    (RUN_DIR / "iec104_port.txt").write_text(str(iec104_port), encoding="utf-8")

    env = {
        "PATH": "/usr/local/bin:/usr/bin:/bin",
        "EDGEC_LICENSE_KEY": "ENT-EVAL-KEY",
        "EDGE_DB_PATH": str(DB_PATH),
        "EDGE_REST_PORT": str(args.port),
        # 这三个不隔离,上一轮跑留下的 WAL 会被回放:日志里看到
        # "WAL replayed 26753 events",心跳显示的是上一轮的陈旧值。
        "EDGE_WAL_PATH": str(RUN_DIR / "wal.log"),
        "EDGE_SNAP_PATH": str(RUN_DIR / "snapshot.bin"),
        "EDGE_SHM_NAME": f"/iems_acceptance_{args.port}",
        "OMNIS_BUS_0_DEV": bus_dev,
        # 北向从站路径：外部显式给了就用外部的（acceptance/run.sh 按任务隔离），
        # 否则用台架缺省（= RUN_DIR/north.sock，跟着 IEMS_RUN_DIR 走）。
        # 两条路都让"产品绑的"和"桥连的"是同一个值 —— 不一致的现象是
        # "TCP 连得上但产品不回应"，最难查的一类。
        "IEMS_NORTH_SOCK": os.environ.get("IEMS_NORTH_SOCK") or NORTH_SOCK,
        "IEMS_IEC104_PORT": str(iec104_port),
        # broker 地址由用例侧给（端口随机）：env 是白名单字典，
        # 写死就等于用例永远连不上它自己起的那个 broker。
        "MQTT_BROKER": os.environ.get("MQTT_BROKER", "tcp://127.0.0.1:1883"),
    }
    # 主题身份也由用例侧给：台架并发跑多个产品时不能撞主题。
    for _k in ("MQTT_NODE_ID", "MQTT_GROUP_ID", "MQTT_PROTOCOL"):
        if os.environ.get(_k):
            env[_k] = os.environ[_k]
    if args.transport == "uart":
        # 产品默认 9600,这里显式给:验的是"按 OMNIS_BUS_n_BAUD 打开设备",不是默认值。
        env["OMNIS_BUS_0_BAUD"] = "9600"
    if args.expect_boot_fail:
        # 启动闸门:壳自己占住 ${PORT},否则平台的就绪探针会先超时,把
        # 「产品启动失败」记成「平台起不来产品」。同时把 iems 的 REST 挪到
        # 隔壁端口 —— 让它撞上"地址已被占用"会让它卡在那儿不退,测的就不是
        # 南向启动闸门了(实测:它会留在半死状态,20 秒不退出)。
        _hold_port(args.port)
        env["EDGE_REST_PORT"] = str(args.port + 1)
        boot_path = RUN_DIR / "boot.json"
        boot_path.unlink(missing_ok=True)  # 上一轮的结论不能留:测试会先看到它
        stderr_path = RUN_DIR / "boot.err"
        with stderr_path.open("w", encoding="utf-8") as err:
            # 产品的日志走 stdout(edge_log_out 回落到 printf,zlog.conf 也全是
            # >stdout),只有 main.c 那条致命行是 fprintf(stderr)。两个都收,
            # 否则"有没有重连循环"这条证据会被丢掉。
            proc = subprocess.Popen([IEMS_BIN], env=env, cwd=str(RUN_DIR),
                                    stdout=err, stderr=err)
            try:
                code = proc.wait(timeout=20)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()  # 不收尸会留僵尸,而且文件可能还没刷新完
                code = None  # 没自己退出 = 还在重连,这正是要抓的回归
        lines = stderr_path.read_text(encoding="utf-8", errors="replace").splitlines()
        boot_path.write_text(json.dumps({
            "code": code,
            "fatal": [ln for ln in lines if "none connected" in ln],
            "warnings": [ln for ln in lines if "NOT connected" in ln],
            "stderr_tail": lines[-5:],
        }, ensure_ascii=False), encoding="utf-8")
        while True:
            time.sleep(1)

    if args.start_gate:
        # 就绪探针只验 TCP 可连。产品要等 pytest 把 MQTT 观察器订上再起 ——
        # BIRTH 每会话只发一次,订阅晚一步就永远看不到首个上线帧。
        hold = socket.socket()
        hold.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        hold.bind(("127.0.0.1", args.port))
        hold.listen(8)
        start_req = RUN_DIR / "start.request"
        start_req.unlink(missing_ok=True)
        deadline = time.time() + 180
        while not start_req.exists() and time.time() < deadline:
            time.sleep(0.05)
        start_req.unlink(missing_ok=True)
        hold.close()
        time.sleep(0.2)  # 端口先还给产品,否则它 bind REST 时撞上 EADDRINUSE

    if args.record_startup:
        # 跨重启追加:重启用例要读第二段启动的 WAL 回放行。
        (RUN_DIR / "iems.log").unlink(missing_ok=True)

    restart_req = RUN_DIR / "restart.request"
    crash_req = RUN_DIR / "crash.request"
    restarts_path = RUN_DIR / "restarts.json"
    restarts = []
    _dump_json(restarts_path, restarts)
    restart_req.unlink(missing_ok=True)
    crash_req.unlink(missing_ok=True)
    state = {"proc": None}

    def _start_product() -> subprocess.Popen:
        if args.record_startup:
            # tee:产品输出既给平台(product.out 是失败时的第一现场),也落一份到
            # RUN_DIR/iems.log —— 启动汇总行是 IEMS-ROBUST-3 的合同条款,用例要读它。
            log = (RUN_DIR / "iems.log").open("a", encoding="utf-8", buffering=1)
            proc = subprocess.Popen([IEMS_BIN], env=env, cwd=str(RUN_DIR),
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, bufsize=1)

            def _tee():
                for line in proc.stdout:
                    sys.stdout.write(line)
                    log.write(line)
                log.close()

            threading.Thread(target=_tee, daemon=True).start()
        else:
            proc = subprocess.Popen([IEMS_BIN], env=env, cwd=str(RUN_DIR))
        (RUN_DIR / "product.pid").write_text(str(proc.pid), encoding="utf-8")
        state["proc"] = proc
        return proc

    # 平台杀的是壳,不是 iems。不转发信号就会留孤儿:iems 继续占着 REST 端口和
    # /tmp/vbus0.sock,下一个任务可能连到它 —— 那时"通过"的是上一个进程(实测踩过)。
    def _forward(signum, _frame):
        # 先给产品机会走它自己的 SIGTERM 收尾(刷 DB/WAL、拆 socket),
        # 3 秒还不走才强杀 —— 直接 kill 等于把"优雅退出"这段路径从没验过。
        proc = state["proc"]
        if proc is not None and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
        raise SystemExit(128 + signum)

    signal.signal(signal.SIGTERM, _forward)
    signal.signal(signal.SIGINT, _forward)

    # 重启由用例经 RUN_DIR/restart.request(优雅 SIGTERM)或 crash.request(SIGKILL)
    # 请求:同一 env 再起一次,进程号与退出码落 restarts.json。SIGKILL 没有收尾,
    # 验的是掉电现场 —— 持久化只能靠已经落盘的 WAL。
    pending = None
    try:
        while True:
            proc = _start_product()
            if pending is not None:
                pending["new_pid"] = proc.pid
                restarts.append(pending)
                _dump_json(restarts_path, restarts)
                pending = None
            while proc.poll() is None:
                if restart_req.exists():
                    restart_req.unlink(missing_ok=True)
                    proc.terminate()
                    forced = False
                    try:
                        proc.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        proc.wait()
                        forced = True
                    pending = {"at": time.time(), "exit_code": proc.returncode,
                               "forced": forced, "mode": "graceful", "pid": proc.pid}
                    break
                if crash_req.exists():
                    crash_req.unlink(missing_ok=True)
                    proc.kill()  # SIGKILL:不给产品刷 WAL/拆 socket 的机会
                    proc.wait()
                    pending = {"at": time.time(), "exit_code": proc.returncode,
                               "forced": True, "mode": "crash", "pid": proc.pid}
                    break
                time.sleep(0.1)
            else:
                code = proc.returncode
                break
    except BaseException:
        proc = state["proc"]
        if proc is not None and proc.poll() is None:
            proc.kill()
            proc.wait()
        raise
    (RUN_DIR / "iems_exit.json").write_text(json.dumps({"code": code}), encoding="utf-8")
    # iems 是常驻服务:它自己退出了就等于失败。壳必须跟着退,把退出码交给平台
    # —— 壳若留着自己活,平台会记成 platform_killed,崩了的进程反而没有结论。
    raise SystemExit(0 if code == 0 else (code if code and code > 0 else 1))


if __name__ == "__main__":
    raise SystemExit(main())
