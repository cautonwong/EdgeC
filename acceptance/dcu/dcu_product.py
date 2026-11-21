"""DCU 产品壳:把 test-platform 的 launch 契约翻译成 DCU 能接受的形态。

DCU 不读 argv、只从 env 取配置，并在启动时一次性连 rs485 从站 socket；
所以壳必须先起从站、备好数据库，再拉起 DCU。

壳做四件事,顺序不能换:
  ① 起 DLT645 从站(/tmp/rs485_22..31.sock),并把收到的请求数记进 bus.json
  ② provision 一份临时 sqlite(40 只 DLT645 表,通道 22-31)
  ③ listen 127.0.0.1:${PORT}  —— 平台的就绪探针要的是这个
  ④ exec DCU(带 EDGEC_DATA_DIR)

用法: python dcu_product.py --port 23456
"""
import argparse
import asyncio
import json
import os
import socket
import sqlite3
import subprocess
import sys
import threading
import time
from pathlib import Path

from bench import (BUS_LOG, CHANNELS, DB_PATH, DCU_BIN, DCU_SRC_DIRS,
                   DI_VOLTAGE, PER_CHANNEL, RUN_DIR,
                   bench_voltage_bytes, describe, dump_json_atomic, staleness)
from testplatform.protocol import dlt645

_lock = threading.Lock()
_seen = {"requests": 0, "per_di": {}, "first_request_hex": None}


def _record_control(meter: str, cmd: int, frame: bytes) -> None:
    """记一帧跳合闸/报警/保电命令（标准 7.13，控制码 1CH）。"""
    with _lock:
        _seen.setdefault("controls", []).append(
            {"meter": meter, "cmd": cmd, "at": time.time(), "frame_hex": frame.hex()}
        )
        dump_json_atomic(BUS_LOG, _seen)


def _record(data_id: str, request: bytes) -> None:
    with _lock:
        _seen["requests"] += 1
        _seen["per_di"][data_id] = _seen["per_di"].get(data_id, 0) + 1
        if _seen["first_request_hex"] is None:
            _seen["first_request_hex"] = request.hex()
        dump_json_atomic(BUS_LOG, _seen)


def provision() -> None:
    RUN_DIR.mkdir(parents=True, exist_ok=True)
    for suffix in ("", "-wal", "-shm"):
        p = Path(str(DB_PATH) + suffix)
        if p.exists():
            p.unlink()
    conn = sqlite3.connect(DB_PATH)
    cur = conn.cursor()
    cur.execute("""
    CREATE TABLE meter_inventory (
        address TEXT PRIMARY KEY, protocol TEXT, baud INTEGER, channel INTEGER,
        wiring_mode INTEGER, collector_addr TEXT, asset_no TEXT,
        pt_ratio INTEGER, ct_ratio INTEGER
    )""")
    rows, n = [], 0
    for chan in CHANNELS:
        for _ in range(PER_CHANNEL):
            n += 1
            addr = f"{n:012d}"
            rows.append((addr, "DLT645", 2400, chan, 1, "123456789012", addr, 1, 1))
    cur.executemany("INSERT INTO meter_inventory VALUES (?,?,?,?,?,?,?,?,?)", rows)
    # 其余表由 DCU 自己建:CREATE TABLE IF NOT EXISTS 对已存在的表是 no-op。
    conn.commit()
    conn.close()


async def _serve(chan: int, value: bytes) -> None:
    path = f"/tmp/rs485_{chan}.sock"
    try:
        os.unlink(path)
    except FileNotFoundError:
        pass

    async def handle(reader, writer):
        try:
            while True:
                head = await reader.read(256)
                if not head:
                    break
                try:
                    address, data_id = dlt645.parse_read_request(head)
                except ValueError as read_exc:
                    # 控制帧（1CH，标准 7.13）数据域以密码开头，读帧以 DI 开头：
                    # 不能共用一个解析器。
                    try:
                        meter, cmd, _data = dlt645.parse_control_request(head)
                    except ValueError:
                        _record("<unparsed>", head)
                        continue
                    _record_control(meter, cmd, head)
                    writer.write(dlt645.build_control_response(meter))
                    await writer.drain()
                    continue
                _record(data_id, head)
                payload = value if data_id == DI_VOLTAGE else b"\x00\x00\x00\x00"
                try:
                    writer.write(dlt645.build_read_response(address, data_id, payload))
                except ValueError as exc:
                    raise RuntimeError(f"台架收到无法编码的地址 {address!r}") from exc
                await writer.drain()
        finally:
            writer.close()

    server = await asyncio.start_unix_server(handle, path=path)
    async with server:
        await server.serve_forever()


def _serve_port(port: int, ready: threading.Event) -> None:
    with socket.socket() as srv:
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind(("127.0.0.1", port))
        srv.listen(8)
        ready.set()
        while True:
            conn, _ = srv.accept()
            conn.close()


async def _unix_buses(value: bytes) -> None:
    await asyncio.gather(*(_serve(c, value) for c in CHANNELS))


# 北向 698 从站的桥：平台没有 unix 传输，静态清单也不能展开 ${PORT}。
NORTH_BRIDGE_PORT = 19212


def _connect_north() -> socket.socket:
    path = os.environ.get("DCU_NORTH_SOCK") or str(RUN_DIR / "north.sock")
    # 上一次产品刚被杀掉时 socket 可能还会拒绝一次连接；单次失败会丢掉整帧。
    deadline = time.time() + 1.0
    while True:
        up = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        up.settimeout(2.0)
        try:
            up.connect(path)
            return up
        except OSError:
            up.close()
            if time.time() >= deadline:
                raise
            time.sleep(0.1)


def _record_north_frame(req: bytes) -> None:
    """帧头（DL/T 698.45）：`68 | L(2) | C(1) | SA描述(1) | SA(n) | CA(1) | HCS(2) | APDU`；
    APDU 起点 = 8 + SA 长度，`APDU[0]` 是服务码、`APDU[1]` 是 CHOICE。
    记录 CHOICE 是为了验证合批：[2] 一帧与 [1] 多帧携带相同的值。"""
    choice = None
    try:
        if req and req[0] == 0x68 and len(req) >= 12:
            sa_len = (req[4] & 0x0F) + 1
            apdu_at = 8 + sa_len
            if apdu_at + 1 < len(req):
                choice = req[apdu_at + 1]
    except (IndexError, ValueError):
        choice = None
    with _lock:
        _seen.setdefault("north_frames", []).append(
            {"choice": choice, "frame_hex": req.hex()}
        )
        dump_json_atomic(BUS_LOG, _seen)


def _serve_north(conn: socket.socket) -> None:
    with conn:
        while True:
            try:
                req = conn.recv(4096)
            except OSError:
                return
            if not req:
                return
            _record_north_frame(req)
            try:
                up = _connect_north()
                up.sendall(req)
            except OSError:
                continue
            try:
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
                    up.settimeout(0.2)
            finally:
                up.close()


def _north_bridge() -> None:
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # 上一轮的壳刚死时端口还没完全放出来：重试，别让一次 bind 失败就把整轮判成"起不来"
    for _ in range(50):
        try:
            server.bind(("127.0.0.1", NORTH_BRIDGE_PORT))
            server.listen(8)
            break
        except OSError:
            time.sleep(0.2)
    else:
        print(f"⚠️  北向桥起不来：127.0.0.1:{NORTH_BRIDGE_PORT} 被占用", flush=True)
        return
    while True:
        try:
            conn, _ = server.accept()
        except OSError:
            return
        threading.Thread(target=_serve_north, args=(conn,), daemon=True).start()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--keep-alive", action="store_true",
                        help="让 DCU 一直跑（缺省跑 3 个心跳周期就自杀）。北向用例要等主站连上来，"
                             "不能跟 3 秒窗口赛跑")
    args = parser.parse_args()

    print(describe(DCU_BIN), flush=True)
    stale = staleness(DCU_BIN, DCU_SRC_DIRS)
    if stale:
        print(f"⚠️  {stale}", file=sys.stderr, flush=True)

    BUS_LOG.parent.mkdir(parents=True, exist_ok=True)
    dump_json_atomic(BUS_LOG, _seen)
    provision()
    value = bench_voltage_bytes()

    loop = asyncio.new_event_loop()
    threading.Thread(
        target=lambda: loop.run_until_complete(_unix_buses(value)), daemon=True
    ).start()
    # 平台的就绪探针只 TCP 连端口：从站起来之前不能 listen。
    time.sleep(0.5)
    ready = threading.Event()
    threading.Thread(target=_serve_port, args=(args.port, ready), daemon=True).start()
    ready.wait(5)

    env = {"PATH": "/usr/local/bin:/usr/bin:/bin", "EDGEC_DATA_DIR": str(RUN_DIR)}
    # 台架固定 demo=1，且刻意不放进下面的转发白名单：外部 env 不能把它关掉。
    env["DCU_DEMO_MODE"] = "1"

    # env 是白名单：没转发的变量落到产品默认值，用例会把这种"沉默"误读成产品没反应。
    for _k in ("MQTT_BROKER", "MQTT_NODE_ID", "MQTT_GROUP_ID", "MQTT_PROTOCOL", "DCU_NORTH_SOCK",
               "DCU_HEARTBEAT_CYCLES", "DCU_CONTROL_DELAY_MS"):
        if os.environ.get(_k):
            env[_k] = os.environ[_k]
    if args.keep_alive:
        env["DCU_HEARTBEAT_CYCLES"] = "0"

    threading.Thread(target=_north_bridge, daemon=True).start()
    proc = subprocess.Popen([DCU_BIN], env=env, cwd=str(RUN_DIR))
    code = proc.wait()
    # 平台把产品自己退出记成 product_exit=self_exit（产品不合格）：壳不能跟着 DCU 退出。
    (RUN_DIR / "dcu_exit.json").write_text(
        json.dumps({"code": code}), encoding="utf-8")
    while True:
        time.sleep(1)


if __name__ == "__main__":
    raise SystemExit(main())
