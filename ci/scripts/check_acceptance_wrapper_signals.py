#!/usr/bin/env python3
"""验收产品壳必须把 SIGTERM/SIGINT 转发给子进程，否则平台杀壳后留下孤儿产品。"""
import argparse
import glob
import os
import sys


def _read(path):
    try:
        with open(path, encoding="utf-8") as f:
            return f.read()
    except OSError:
        return ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    args = parser.parse_args()

    errors = []
    for path in sorted(glob.glob(os.path.join(args.root, "acceptance", "*", "*_product.py"))):
        rel = os.path.relpath(path, args.root)
        text = _read(path)
        if "subprocess.Popen" not in text:
            continue
        missing = []
        if "signal.signal(signal.SIGTERM" not in text:
            missing.append("SIGTERM")
        if "signal.signal(signal.SIGINT" not in text:
            missing.append("SIGINT")
        if ".terminate()" not in text:
            missing.append("terminate()")
        if missing:
            errors.append(f"{rel} 未转发 {'/'.join(missing)} 给子进程")

    for err in errors:
        print(f"❌ [ACCEPTANCE_WRAPPER_SIGNALS] {err}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
