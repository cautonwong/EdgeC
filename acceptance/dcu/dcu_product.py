"""DCU 产品壳:把 test-platform 的 launch 契约翻译成 DCU 能接受的形态。

DCU 的三条硬事实逼出这个壳(都已经在源码里核过):
  1. main.c:153-154  `(void)argc; (void)argv;`      —— 它不认命令行参数
  2. main.c:159-160  只读 EDGE_DB_PATH/EDGEC_DATA_DIR —— 配置只能走 env
  3. main.c:214-229  启动时 connect /tmp/rs485_N.sock 一次,失败就静默跳过
     —— 从站必须先起,晚一秒就永远挂不上;它自己也不监听任何 TCP 端口

所以壳做四件事,顺序不能换:
  ① 起 DLT645 从站(/tmp/rs485_22..31.sock),并把收到的请求数记进 bus.json
  ② provision 一份临时 sqlite(40 只 DLT645 表,通道 22-31)
  ③ listen 127.0.0.1:${PORT}  —— 平台的就绪探针要的是这个
  ④ exec DCU(带 EDGEC_DATA_DIR),它死了壳就退

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
                   DI_VOLTAGE, METER_ADDRESS, PER_CHANNEL, RUN_DIR,
                   bench_voltage_bytes, describe, staleness)
from testplatform.protocol import dlt645

_lock = threading.Lock()
_seen = {"requests": 0, "per_di": {}, "first_request_hex": None}


def _record(data_id: str, request: bytes) -> None:
    with _lock:
        _seen["requests"] += 1
        _seen["per_di"][data_id] = _seen["per_di"].get(data_id, 0) + 1
        if _seen["first_request_hex"] is None:
            _seen["first_request_hex"] = request.hex()
        BUS_LOG.write_text(json.dumps(_seen, ensure_ascii=False), encoding="utf-8")


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
    # 其余表全部由 DCU 自己建(dcu_storage.c:51 起)。这里不预先建:
    # CREATE TABLE IF NOT EXISTS 不覆盖已存在的表,壳的列名一旦和 DCU 不一致,
    # DCU 的 INSERT 就静默失败 —— 表现为「跑完了但表是空的」。
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
                except ValueError:
                    _record("<unparsed>", head)
                    continue
                _record(data_id, head)
                payload = value if data_id == DI_VOLTAGE else b"\x00\x00\x00\x00"
                try:
                    writer.write(dlt645.build_read_response(address, data_id, payload))
                except ValueError:
                    # 地址不是 12 位 BCD:照原样回,让产品侧自己判
                    writer.write(dlt645.build_read_response(METER_ADDRESS, data_id, payload))
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


# 北向 698 从站的桥：产品只监听 unix socket，而平台的传输层只认 serial/tcp。
# 端口写死是因为 devices.yaml 是静态清单，读不到 ${PORT}；换端口两边都要改。
NORTH_BRIDGE_PORT = 19212


def _connect_north() -> socket.socket:
    """连产品的 698 socket，短暂重试。

    只试一次太脆：上一个任务的产品刚被 SIGKILL、新的还没绑上时，一次 ECONNREFUSED
    就丢整帧，平台看到的是"产品不回应"，而产品 200ms 后就正常了。
    """
    path = os.environ.get("DCU_NORTH_SOCK") or str(RUN_DIR / "north.sock")
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


def _serve_north(conn: socket.socket) -> None:
    """一个 TCP 连接对一个 unix 连接，逐帧转发。"""
    with conn:
        while True:
            try:
                req = conn.recv(4096)
            except OSError:
                return
            if not req:
                return
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
                    up.settimeout(0.2)  # 收完一帧就别等下一帧
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

    # 先把"验的是哪份二进制"说清楚，再谈结论。
    # 曾经对着三周前的二进制跑出绿的结论：env 里的新变量被旧二进制无视，
    # 现象却像"环境变量没传进去"。所以定位与过期判定必须在启动第一句。
    print(describe(DCU_BIN), flush=True)
    stale = staleness(DCU_BIN, DCU_SRC_DIRS)
    if stale:
        print(f"⚠️  {stale}", file=sys.stderr, flush=True)

    BUS_LOG.parent.mkdir(parents=True, exist_ok=True)
    BUS_LOG.write_text(json.dumps(_seen), encoding="utf-8")
    provision()
    value = bench_voltage_bytes()

    loop = asyncio.new_event_loop()
    threading.Thread(
        target=lambda: loop.run_until_complete(_unix_buses(value)), daemon=True
    ).start()
    # 从站起来之前不 listen:早开探针会在 DCU 还没连上总线时就说"就绪"。
    time.sleep(0.5)
    ready = threading.Event()
    threading.Thread(target=_serve_port, args=(args.port, ready), daemon=True).start()
    ready.wait(5)

    env = {"PATH": "/usr/local/bin:/usr/bin:/bin", "EDGEC_DATA_DIR": str(RUN_DIR)}

    # 北向 broker 由用例侧起（端口随机），只能经壳自己的环境传进来。
    # 必须显式转发：env 是白名单字典，不转发就等于产品永远连默认 1883，
    # 用例会看到"一条都没发"——那不是产品的问题，是壳把配置丢了。
    # DCU_NORTH_SOCK：北向 698 从站的 socket，按任务隔离（同 IEMS 的理由）
    for _k in ("MQTT_BROKER", "MQTT_NODE_ID", "MQTT_GROUP_ID", "MQTT_PROTOCOL", "DCU_NORTH_SOCK",
               "DCU_HEARTBEAT_CYCLES"):
        if os.environ.get(_k):
            env[_k] = os.environ[_k]
    if args.keep_alive:
        env["DCU_HEARTBEAT_CYCLES"] = "0"  # 0 = 一直跑，由平台收尾

    threading.Thread(target=_north_bridge, daemon=True).start()
    proc = subprocess.Popen([DCU_BIN], env=env, cwd=str(RUN_DIR))
    code = proc.wait()
    # DCU 跑 3 个轮询周期后会自己退(退出码 1)。壳不能跟着退:
    # 壳退了平台记的是 product_exit=self_exit(产品不合格),而这一轮要判的是采集。
    # 退出码落文件,由用例自己断言。
    (RUN_DIR / "dcu_exit.json").write_text(
        json.dumps({"code": code}), encoding="utf-8")
    while True:
        time.sleep(1)


if __name__ == "__main__":
    raise SystemExit(main())
