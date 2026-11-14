#!/usr/bin/env python3
"""End-to-end check of the app platform on the real product process.

Installs a signed `.eapp` through appctl, kills the app-host child up to the
fault cap, checks isolation, manual start, snapshot generation and boot
reactivation. Exits non-zero on the first failed step and prints the iems log.
"""

import argparse
import json
import os
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

RESTART_CAP = 5
APP_ID = "abi-probe"


class Failure(RuntimeError):
    pass


def appctl_json(appctl, sock, *args):
    proc = subprocess.run(
        [str(appctl), *args, "--sock", sock],
        capture_output=True,
        text=True,
        timeout=10,
    )
    body = proc.stdout.strip()
    try:
        payload = json.loads(body)
    except json.JSONDecodeError:
        raise Failure(f"appctl {' '.join(args)} returned non-json: {body or proc.stderr}")
    if proc.returncode != 0:
        raise Failure(f"appctl {' '.join(args)} failed: {body}")
    return payload


def wait_for(predicate, timeout, what):
    deadline = time.time() + timeout
    last = None
    while time.time() < deadline:
        try:
            value = predicate()
        except (Failure, OSError, ValueError) as exc:
            last = exc
            value = None
        if value:
            return value
        time.sleep(0.05)
    raise Failure(f"timed out waiting for {what}: {last}")


def app_host_status(pid, key):
    for line in Path(f"/proc/{pid}/status").read_text().splitlines():
        if line.startswith(key):
            return line.split()[1]
    raise Failure(f"no {key} for {pid}")


def app_host_uid(pid):
    return int(app_host_status(pid, "Uid:"))


def app_host_children(iems_pid):
    try:
        children = Path(f"/proc/{iems_pid}/task/{iems_pid}/children").read_text().split()
    except FileNotFoundError:
        return []
    pids = []
    for child in children:
        try:
            cmdline = Path(f"/proc/{child}/cmdline").read_bytes().replace(b"\0", b" ")
        except OSError:
            continue
        if b"app-host" in cmdline:
            pids.append(int(child))
    return pids


class BusSockets:
    def __init__(self, paths):
        self.sockets = []
        self.connections = []
        self.stop = False
        for path in paths:
            sock = socket.socket(socket.AF_UNIX)
            sock.bind(str(path))
            sock.listen(4)
            sock.settimeout(0.05)
            self.sockets.append(sock)
        self.thread = threading.Thread(target=self._accept, daemon=True)
        self.thread.start()

    def _accept(self):
        while not self.stop:
            for sock in self.sockets:
                try:
                    conn, _ = sock.accept()
                    self.connections.append(conn)
                except socket.timeout:
                    pass

    def close(self):
        self.stop = True
        self.thread.join(timeout=1)
        for conn in self.connections:
            conn.close()
        for sock in self.sockets:
            sock.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", default="build")
    parser.add_argument("--keep", action="store_true")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[2]
    build = Path(args.build).resolve()
    iems = build / "products/iems/bin/iems"
    appctl = build / "tools/appctl/appctl"
    pack = build / "tools/edgeos-pack/edgeos-pack"
    app_host = build / "cmd/app-host/app-host"
    launcher = build / "cmd/edgeos-launcher/edgeos-launcher"
    probe = repo / "tests/fixtures/abi_probe.wasm"
    for path in (iems, appctl, pack, app_host, launcher, probe):
        if not path.exists():
            raise Failure(f"missing {path}")

    root = Path(tempfile.mkdtemp(prefix="edgeos-e2e-"))
    log_path = root / "iems.log"
    log = open(log_path, "ab", buffering=0)
    bus = None
    proc = None
    shm_name = f"edgeos_e2e_{os.getpid()}"

    def start_iems(extra=None):
        env = os.environ.copy()
        env.update(extra or {})
        env.update(
            EDGE_DB_PATH=str(root / "iems.db"),
            EDGE_SHM_NAME=shm_name,
            EDGEC_APPCTL_SOCK=str(root / "appctl.sock"),
            EDGE_REST_PORT=str(free_port()),
            EDGE_APPS_DIR=str(root / "apps"),
            EDGEC_REGISTRY_DIR=str(root / "registry"),
            OMNIS_BUS_0_DEV=str(root / "vbus0.sock"),
            OMNIS_BUS_1_DEV=str(root / "vbus1.sock"),
            EDGEC_APP_HOST_PATH=str(app_host),
            EDGEC_LAUNCHER_PATH=str(launcher),
            EDGEC_APPS_ROOT=str(root / "apps"),
            EDGEC_PACKAGE_KEYRING_FILE=str(root / "keyring"),
            EDGEC_APP_RESTART_DELAY_MS="20",
            EDGEC_RECONCILE_INTERVAL_MS="300",
            EDGEC_TIME_SYNC_FILE=str(root / "time_sync"),
        )
        command = [str(iems)]
        if shutil.which("stdbuf"):
            command = ["stdbuf", "-oL", "-eL"] + command
        return subprocess.Popen(
            command, stdout=log, stderr=subprocess.STDOUT, env=env
        )

    def stop_iems(process):
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)

    def installed():
        apps = appctl_json(appctl, str(root / "appctl.sock"), "list")
        return apps[0] if apps else None

    try:
        (root / "apps").mkdir()
        (root / "registry").mkdir()
        (root / "keys").mkdir()
        os.chmod(root, 0o711)
        subprocess.run(
            [str(pack), "keygen", "--out-dir", str(root / "keys_b")],
            check=True,
            capture_output=True,
            text=True,
        )
        subprocess.run(
            [str(pack), "keygen", "--out-dir", str(root / "keys")],
            check=True,
            capture_output=True,
            text=True,
        )
        manifest = root / "manifest.json"
        manifest.write_text(
            json.dumps(
                {
                    "abi": 1,
                    "id": APP_ID,
                    "version": "1.0.0",
                    "language": "c",
                    "execution_profile": "wasm",
                    "entry": "app.wasm",
                    "limits": {
                        "heapKb": 64,
                        "storageKb": 64,
                        "cpuMsPerSec": 50,
                        "callsPerSec": 100,
                        "publishPerSec": 10,
                        "pidsMax": 4,
                    },
                }
            )
        )
        package = root / "abi-probe.eapp"
        subprocess.run(
            [
                str(pack),
                "build",
                "--manifest",
                str(manifest),
                "--wasm",
                str(probe),
                "--seed",
                str(root / "keys/edgec-eapp.seed"),
                "--out",
                str(package),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        pub_a = (root / "keys/edgec-eapp.pub").read_text().strip()
        pub_b = (root / "keys_b/edgec-eapp.pub").read_text().strip()
        (root / "keyring").write_text(f"acme 2 1 {pub_a}\nbeta 1 0 {pub_b}\n")
        package_b = root / "abi-probe-b.eapp"
        subprocess.run(
            [
                str(pack),
                "build",
                "--manifest",
                str(manifest),
                "--wasm",
                str(probe),
                "--seed",
                str(root / "keys_b/edgec-eapp.seed"),
                "--out",
                str(package_b),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        subprocess.run(
            [str(pack), "verify", "--package", str(package),
             "--pubkey", str(root / "keys/edgec-eapp.pub")],
            check=True,
            capture_output=True,
            text=True,
        )

        bus = BusSockets([root / "vbus0.sock", root / "vbus1.sock"])
        # Crash-consistency: the product dies after the package write and
        # before the registry commit. A restart must show no half-installed app.
        proc = start_iems({"EDGEC_TEST_CRASH_AFTER_PACKAGE_WRITE": "1"})
        wait_for(lambda: (root / "appctl.sock").exists(), 10, "appctl socket")
        wait_for(lambda: appctl_json(appctl, str(root / "appctl.sock"), "status"), 10, "status")
        (root / "time_sync").write_text(f"{int(time.time())}\n")
        wait_for(
            lambda: appctl_json(appctl, str(root / "appctl.sock"), "time")["state"] == "trusted",
            10,
            "time sync reporter",
        )
        try:
            subprocess.run(
                [str(appctl), "install", str(package), "--key",
                 str(root / "keys/edgec-eapp.pub"), "--sock", str(root / "appctl.sock")],
                capture_output=True,
                text=True,
                timeout=10,
            )
        except subprocess.SubprocessError:
            pass
        wait_for(lambda: proc.poll() is not None, 10, "crash after package write")
        stop_iems(proc)
        proc = start_iems()
        wait_for(lambda: (root / "appctl.sock").exists(), 10, "appctl socket")
        wait_for(lambda: appctl_json(appctl, str(root / "appctl.sock"), "status"), 10, "status")
        if installed():
            raise Failure("crash left a registry row before the commit")
        wait_for(
            lambda: appctl_json(appctl, str(root / "appctl.sock"), "time")["state"] == "trusted",
            10,
            "trusted clock after restart",
        )

        rejected = subprocess.run(
            [str(appctl), "install", str(package_b), "--key",
             str(root / "keys/edgec-eapp.pub"), "--sock", str(root / "appctl.sock")],
            capture_output=True,
            text=True,
            timeout=10,
        )
        if rejected.returncode == 0:
            raise Failure("install accepted a package from an unexpected signer")

        appctl_json(appctl, str(root / "appctl.sock"), "install", str(package),
                    "--key", str(root / "keys/edgec-eapp.pub"))
        def running():
            app = installed()
            if app and app["id"] == APP_ID and app["state"] == 2 and app["restart_count"] == 0:
                return app
            return None

        app = wait_for(running, 10, "installed app running")
        if app["id"] != APP_ID:
            raise Failure(f"unexpected install state: {app}")
        if not (root / "apps" / APP_ID / "app.wasm").exists():
            raise Failure("installed wasm file missing")
        import sqlite3

        con = sqlite3.connect(str(root / "iems.db"), timeout=5)
        try:
            trust = con.execute(
                "SELECT trust_level, verified FROM publisher_trust WHERE publisher_id=?",
                (APP_ID,),
            ).fetchone()
        finally:
            con.close()
        if trust != (2, 1):
            raise Failure(f"publisher trust not recorded: {trust}")
        child = wait_for(lambda: app_host_children(proc.pid), 10, "app-host child")
        uid = app_host_uid(child[0])
        if uid < 20000 or uid > 29999:
            raise Failure(f"app-host runs as uid {uid}, expected the sandbox range")
        if app_host_status(child[0], "NoNewPrivs:") != "1":
            raise Failure("app-host did not get no_new_privs")
        if int(app_host_status(child[0], "CapEff:"), 16) != 0:
            raise Failure("app-host kept capabilities")

        for attempt in range(1, RESTART_CAP + 2):
            old_pid = child[0]
            os.kill(old_pid, signal.SIGKILL)
            if attempt <= RESTART_CAP:
                def restarted():
                    app = installed()
                    kids = app_host_children(proc.pid)
                    if kids and kids[0] != old_pid and app["state"] == 2 and app[
                            "restart_count"] >= attempt:
                        return kids
                    return None
                child = wait_for(restarted, 20, f"restart {attempt}")
            else:
                def isolated():
                    app = installed()
                    return app["state"] == 4 and app["restart_count"] >= attempt and not app_host_children(
                        proc.pid)
                wait_for(isolated, 20, "isolation after the fault cap")

        appctl_json(appctl, str(root / "appctl.sock"), "start", APP_ID)
        child = wait_for(
            lambda: app_host_children(proc.pid) if installed()["state"] == 2 else None,
            15,
            "manual start",
        )
        if installed()["restart_count"] < RESTART_CAP + 1:
            raise Failure(f"manual start reset the streak: {installed()}")

        snapshot = appctl_json(appctl, str(root / "appctl.sock"), "snapshot")
        if snapshot.get("status") != "ok" or not Path(snapshot["path"]).exists():
            raise Failure(f"snapshot failed: {snapshot}")
        generation = int(snapshot["generation"])
        if generation <= 0:
            raise Failure(f"snapshot generation did not advance: {generation}")

        last_child = child[0]
        stop_iems(proc)
        wait_for(lambda: not Path(f"/proc/{last_child}").exists(), 5,
                 "app-host exit with the product process")
        proc = start_iems()
        wait_for(lambda: (root / "appctl.sock").exists(), 10, "appctl socket after restart")
        app = wait_for(lambda: installed(), 10, "registry after restart")
        if app["id"] != APP_ID or app["state"] != 2 or app["restart_count"] < RESTART_CAP + 1:
            raise Failure(f"registry did not survive the restart: {app}")
        wait_for(lambda: app_host_children(proc.pid), 15, "boot reactivation")

        # Desired-state reconcile: a registry edit that no command path writes
        # must converge within one period.
        set_registry(root / "iems.db", "UPDATE app_registry SET desired_state=0 WHERE id=?", (APP_ID,))
        wait_for(lambda: not app_host_children(proc.pid) and installed()["state"] == 3, 10,
                 "reconcile stopped the app")
        set_registry(root / "iems.db",
                     "UPDATE app_registry SET desired_state=1, state=2 WHERE id=?", (APP_ID,))
        wait_for(lambda: app_host_children(proc.pid) and installed()["state"] == 2, 10,
                 "reconcile restarted the app")

        print(
            "ok: install, {} restarts, isolation, manual start, snapshot gen {}, boot reactivation, reconcile".format(
                RESTART_CAP, generation
            )
        )
        return 0
    except Failure as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        try:
            if proc is not None:
                print(f"children: {app_host_children(proc.pid)}", file=sys.stderr)
            if (root / "appctl.sock").exists():
                printed = subprocess.run(
                    [str(appctl), "list", "--sock", str(root / "appctl.sock")],
                    capture_output=True,
                    text=True,
                    timeout=5,
                )
                print(f"state: {printed.stdout.strip()} {printed.stderr.strip()}", file=sys.stderr)
        except (OSError, subprocess.SubprocessError) as diag:
            print(f"diag failed: {diag}", file=sys.stderr)
        try:
            tail = log_path.read_text().splitlines()[-40:]
            print("\n".join(tail), file=sys.stderr)
        except OSError:
            pass
        return 1
    finally:
        if proc is not None:
            stop_iems(proc)
        if bus is not None:
            bus.close()
        log.close()
        for pattern in (f"{shm_name}*", f"edgeos_shadow_{shm_name}*"):
            for path in Path("/dev/shm").glob(pattern):
                try:
                    path.unlink()
                except OSError:
                    pass
        if not args.keep:
            shutil.rmtree(root, ignore_errors=True)
        else:
            print(f"kept {root}")


def set_registry(db_path, sql, args):
    import sqlite3

    con = sqlite3.connect(str(db_path), timeout=5)
    try:
        con.execute("PRAGMA busy_timeout=3000")
        con.execute(sql, args)
        con.commit()
    finally:
        con.close()


def free_port():
    sock = socket.socket()
    sock.bind(("127.0.0.1", 0))
    port = sock.getsockname()[1]
    sock.close()
    return port


if __name__ == "__main__":
    sys.exit(main())
