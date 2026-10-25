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
      python iems_product.py --port 23456 --expect-boot-fail   # 启动闸门用:不给总线
"""
import argparse
import asyncio
import signal
import json
import socket
import sqlite3
import struct
import subprocess
import threading
import time
from pathlib import Path

from bench import (BRIDGE_PORT, BUS_LOG, BUS_ID, DB_PATH, IEMS_BIN,
                   NORTH_SOCK, RUN_DIR, SOCK, SLAVE_ID, TAG_CURRENT,
                   TAG_VOLTAGE, VOLTAGE_REGISTER)
from testplatform.protocol import modbus_rtu

_lock = threading.Lock()
_fault = {"kind": None, "left": 0}
# 掉链:服务 N 帧后主动断掉这条南向连接,模拟现场断线(产品不许自己重连)
_drop = {"after": 0, "done": False}
_seen = {"requests": 0, "first_request_hex": None, "addresses": []}


def _should_drop() -> bool:
    """第 N 帧之后断链。只在锁内判一次,之后的帧不再更新计数。"""
    with _lock:
        if not _drop["after"] or _drop["done"] or _seen["requests"] < _drop["after"]:
            return False
        _drop["done"] = True
        (RUN_DIR / "dropped.json").write_text(
            json.dumps({"requests": _seen["requests"]}, ensure_ascii=False),
            encoding="utf-8")
        return True


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


def _fail(where: str, exc: BaseException) -> None:
    """台架自己的异常必须留痕。

    这个坑踩过一次:应答帧拼错(把寄存器列表当 payload 传给
    build_read_response)时,协程里抛 TypeError,asyncio 不 retrieve 异常、
    一声不响 —— 从站第 1 帧之后既不回包也不再 read,而验收看到的是
    「产品只发了 1 帧」。台架的 bug 被记成产品的缺陷,是最贵的一种错。
    """
    with _lock:
        _seen.setdefault("errors", []).append(f"{where}: {type(exc).__name__}: {exc}")
        BUS_LOG.write_text(json.dumps(_seen, ensure_ascii=False), encoding="utf-8")


async def _serve(reader, writer):
    try:
        while True:
            head = await reader.read(256)
            if not head:
                break
            try:
                unit, function, address, count = modbus_rtu.parse_read_request(head)
            except ValueError as exc:
                _record(head, -1)
                _fail("parse_read_request", exc)
                continue
            _record(head, address)
            if _should_drop():
                # 不回帧、直接关连接:iems 侧看到的就是链路断了
                break
            if unit != SLAVE_ID:
                break
            payload = b"".join(
                struct.pack(">H", VOLTAGE_REGISTER if (address + i) == 0 else 0)
                for i in range(count)
            )
            frame = modbus_rtu.build_read_response(unit, payload, function)
            frame = await _inject_fault(frame, unit, function, count)
            if frame is not None:
                writer.write(frame)
                await writer.drain()
    except (ConnectionError, OSError) as exc:
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
        faults_path.write_text(json.dumps(served), encoding="utf-8")
    if kind == "slow":
        await asyncio.sleep(3.0)  # 超过一轮轮询周期:iems 必须扛住并恢复
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
        try:
            up = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            up.connect(NORTH_SOCK)
        except OSError:
            conn.close()
            continue
        threading.Thread(target=_pump, args=(conn, up), daemon=True).start()
        threading.Thread(target=_pump, args=(up, conn), daemon=True).start()


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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--fault", choices=["bad-crc", "slow", "silent"],
                        help="对前若干帧应答制造故障(故障注入用例用)")
    parser.add_argument("--fault-frames", type=int, default=2)
    parser.add_argument("--drop-after", type=int, default=0,
                        help="服务 N 帧后主动断开南向连接(链路断开用例用)")
    parser.add_argument("--expect-boot-fail", action="store_true",
                        help="不给任何总线/从站,产品应该启动失败而不是重连")
    args = parser.parse_args()
    if args.fault:
        _fault["kind"], _fault["left"] = args.fault, args.fault_frames
    _drop["after"] = args.drop_after

    RUN_DIR.mkdir(parents=True, exist_ok=True)
    BUS_LOG.write_text(json.dumps(_seen), encoding="utf-8")
    (RUN_DIR / "faults.json").unlink(missing_ok=True)  # 上一轮的注入记录不能留
    (RUN_DIR / "dropped.json").unlink(missing_ok=True)
    provision()

    if not args.expect_boot_fail:
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

    proc = subprocess.Popen([IEMS_BIN], env=env, cwd=str(RUN_DIR))

    # 平台杀的是壳,不是 iems。不转发信号就会留孤儿:iems 继续占着 REST 端口和
    # /tmp/vbus0.sock,下一个任务可能连到它 —— 那时"通过"的是上一个进程(实测踩过)。
    def _forward(signum, _frame):
        # 先给产品机会走它自己的 SIGTERM 收尾(刷 DB/WAL、拆 socket),
        # 3 秒还不走才强杀 —— 直接 kill 等于把"优雅退出"这段路径从没验过。
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        raise SystemExit(128 + signum)

    signal.signal(signal.SIGTERM, _forward)
    signal.signal(signal.SIGINT, _forward)
    try:
        code = proc.wait()
    except BaseException:
        if proc.poll() is None:
            proc.kill()
            proc.wait()
        raise
    (RUN_DIR / "iems_exit.json").write_text(json.dumps({"code": code}), encoding="utf-8")
    # iems 是常驻服务:它自己退出了就等于失败。壳必须跟着退,把退出码交给平台
    # —— 壳若留着自己活,平台会记成 platform_killed,崩了的进程反而没有结论。
    raise SystemExit(0 if code == 0 else (code if code and code > 0 else 1))


if __name__ == "__main__":
    raise SystemExit(main())
