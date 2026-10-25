"""smart_box 产品壳:把 go 网关架到平台分配的那个端口上。

为什么需要壳:
  - 平台要求 launch 里有 ${PORT},并且就绪探针要 TCP connect 成功;
    网关自己认 GATEWAY_PORT / EDGE_DB_PATH 两个环境变量,不认 --port。
  - 平台给产品子进程的环境只有调用方的环境(CHILD_ENV 继承 os.environ),
    壳负责把这两个变量摆正,并且把 db 放到固定的运行目录里,用例才找得到。

这个壳不做断言、不改数据 —— 数据全由被测的网关自己写。
"""
import argparse
import os

import subprocess
import sys
import time
from pathlib import Path

from bench import DB_PATH, PORT_FILE, RUN_DIR

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[1]  # acceptance/smart_box -> 仓库根
GO_DIR = REPO_ROOT / "go"
BIN = REPO_ROOT / "build" / "go-bin" / "smart-box-gateway"


def build_if_missing() -> None:
    if os.environ.get("SMART_BOX_GATEWAY_BIN"):
        return
    if BIN.exists():
        return
    BIN.parent.mkdir(parents=True, exist_ok=True)
    print(f"[壳] 没有网关二进制,先 go build → {BIN}", flush=True)
    subprocess.run(
        ["go", "build", "-o", str(BIN), "./cmd/smart-box-gateway"],
        cwd=str(GO_DIR),
        check=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, required=True)
    args = parser.parse_args()

    RUN_DIR.mkdir(parents=True, exist_ok=True)
    for path in (DB_PATH, PORT_FILE):
        if path.exists():
            path.unlink()

    build_if_missing()
    binary = os.environ.get("SMART_BOX_GATEWAY_BIN") or str(BIN)
    if not Path(binary).exists():
        print(f"[壳] 网关二进制不存在:{binary}", file=sys.stderr, flush=True)
        return 2

    PORT_FILE.write_text(str(args.port), encoding="utf-8")
    PORT_FILE.chmod(0o644)

    env = {
        "PATH": "/usr/local/bin:/usr/bin:/bin",
        "GATEWAY_PORT": str(args.port),
        "EDGE_DB_PATH": str(DB_PATH),
        "HOME": os.environ.get("HOME", "/tmp"),
    }
    print(f"[壳] 起网关:{binary} GATEWAY_PORT={args.port} EDGE_DB_PATH={DB_PATH}", flush=True)
    proc = subprocess.Popen([binary], env=env, cwd=str(RUN_DIR))
    code = proc.wait()
    # 网关不退就不走这里;退了(崩了/被平台杀)就照实记下退出码,让用例去断言。
    print(f"[壳] 网关退出,code={code}", flush=True)
    return code


if __name__ == "__main__":

    raise SystemExit(main())
