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
"""
import argparse
import asyncio
import json
import socket
import sqlite3
import subprocess
import threading
import time
from pathlib import Path

from bench import (BRIDGE_PORT, BUS_LOG, BUS_ID, DB_PATH, IEMS_BIN,
                   NORTH_SOCK, RUN_DIR, SOCK, SLAVE_ID, TAG_CURRENT,
                   TAG_VOLTAGE, VOLTAGE_REGISTER)
from testplatform.protocol import modbus_rtu

_lock = threading.Lock()
_seen = {"requests": 0, "first_request_hex": None, "addresses": []}


def _record(request: bytes, address: int) -> None:
    with _lock:
        _seen["requests"] += 1
        if _seen["first_request_hex"] is None:
            _seen["first_request_hex"] = request.hex()
        if address not in _seen["addresses"]:
            _seen["addresses"].append(address)
        BUS_LOG.write_text(json.dumps(_seen, ensure_ascii=False), encoding="utf-8")


def provision() -> None:
    """iems 的轮询任务表。空库 = 不轮询,所以这不是可选步骤。"""
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
    conn.execute("INSERT INTO inventory (id, addr, protocol) VALUES (1, ?, 'MODBUS')",
                 (str(SLAVE_ID),))
    conn.executemany("INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (?, 1, ?)",
                     [(BUS_ID, TAG_VOLTAGE), (BUS_ID, TAG_CURRENT)])
    conn.commit()
    conn.close()


async def _serve(reader, writer):
    while True:
        head = await reader.read(256)
        if not head:
            break
        try:
            unit, function, address, count = modbus_rtu.parse_read_request(head)
        except ValueError:
            _record(head, -1)
            continue
        _record(head, address)
        if unit != SLAVE_ID:
            break
        registers = [VOLTAGE_REGISTER if (address + i) == 0 else 0 for i in range(count)]
        writer.write(modbus_rtu.build_read_response(unit, function, registers))
        await writer.drain()
    writer.close()


async def _unix_bus() -> None:
    try:
        Path(SOCK).unlink()
    except FileNotFoundError:
        pass
    server = await asyncio.start_unix_server(_serve, path=SOCK)
    async with server:
        await server.serve_forever()


def _pump(src, dst) -> None:
    try:
        while (chunk := src.recv(4096)):
            dst.sendall(chunk)
    except OSError:
        pass
    finally:
        src.close()
        dst.close()


def _north_bridge() -> None:
    """unix socket → TCP:iems 的北向从站只监听 unix socket,而平台只认 serial/tcp。

    一个连接一对 pump;连不上 iems 的 socket 就直接关掉这条 —— 平台那边看到的是
    "连上了但没应答",由它自己记超时,不需要壳替它编一个应答。
    """
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("127.0.0.1", BRIDGE_PORT))
    server.listen(8)
    while True:
        conn, _ = server.accept()
        try:
            up = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            up.connect(NORTH_SOCK)
        except OSError:
            conn.close()
            continue
        threading.Thread(target=_pump, args=(conn, up), daemon=True).start()
        threading.Thread(target=_pump, args=(up, conn), daemon=True).start()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, required=True)
    args = parser.parse_args()

    RUN_DIR.mkdir(parents=True, exist_ok=True)
    BUS_LOG.write_text(json.dumps(_seen), encoding="utf-8")
    provision()

    loop = asyncio.new_event_loop()
    threading.Thread(target=lambda: loop.run_until_complete(_unix_bus()), daemon=True).start()
    threading.Thread(target=_north_bridge, daemon=True).start()
    time.sleep(0.3)
    # 这个端口不自己占:iems 的 REST 直接绑 EDGE_REST_PORT,平台的就绪探针
    # connect 到的是 iems 自己。壳再起一个假监听会把端口抢走,REST 绑不上,
    # 探针连到的是壳然后被 reset —— 表现为「产品起来了但 REST 全崩」。
    (RUN_DIR / "port.txt").write_text(str(args.port), encoding="utf-8")

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
        "OMNIS_BUS_0_DEV": SOCK,
        "MQTT_BROKER": "tcp://127.0.0.1:1883",
    }
    proc = subprocess.Popen([IEMS_BIN], env=env, cwd=str(RUN_DIR))
    code = proc.wait()
    (RUN_DIR / "iems_exit.json").write_text(json.dumps({"code": code}), encoding="utf-8")
    while True:
        time.sleep(1)


if __name__ == "__main__":
    raise SystemExit(main())
