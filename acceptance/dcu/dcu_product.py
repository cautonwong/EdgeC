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
import threading
import time
from pathlib import Path

from bench import (BUS_LOG, CHANNELS, DB_PATH, DCU_BIN, DI_VOLTAGE,
                   METER_ADDRESS, PER_CHANNEL, RUN_DIR, bench_voltage_bytes)
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
    cur.execute("CREATE TABLE IF NOT EXISTS telemetry (timestamp INTEGER, tag_id INTEGER, "
                "value REAL, device_id INTEGER, quality INTEGER)")
    cur.execute("CREATE TABLE IF NOT EXISTS event_history (meter_address TEXT, oi INTEGER, "
                "start_time INTEGER, end_time INTEGER, snapshot_value REAL, "
                "PRIMARY KEY (meter_address, oi, start_time))")
    cur.execute("CREATE TABLE IF NOT EXISTS dcu_freeze_history (meter_address TEXT, "
                "freeze_oad INTEGER, freeze_time INTEGER, source_oad INTEGER, "
                "frozen_value REAL, PRIMARY KEY (meter_address, freeze_oad, "
                "freeze_time, source_oad))")
    cur.execute("CREATE TABLE IF NOT EXISTS dcu_telemetry_latest (device_id INTEGER, "
                "tag_id INTEGER, value REAL, timestamp INTEGER, PRIMARY KEY (device_id, tag_id))")
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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, required=True)
    args = parser.parse_args()

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
