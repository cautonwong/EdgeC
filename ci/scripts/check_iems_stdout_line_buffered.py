#!/usr/bin/env python3
"""产品 stdout 必须行缓冲：验收读状态行的前提，不能靠台架 stdbuf 掩盖。

2026-11-19 的 TCP 重连验收踩过：产品 stdout 接管道时是块缓冲，
`[PHYSICS-SUCCESS] ... CONNECTED` 迟到二十秒才落盘，用例把「重连成功」
判成「重连没发生」。根因修复是 main 入口的 setvbuf；台架的 stdbuf 只是
掩盖，必须禁止回潮。
"""
import argparse
import os
import re
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

    main_c = _read(os.path.join(args.root, "cmd/iems/main.c"))
    shell = _read(os.path.join(args.root, "acceptance/iems/iems_product.py"))

    errors = []
    if not re.search(r"setvbuf\s*\(\s*stdout\s*,\s*NULL\s*,\s*_IOLBF\s*,\s*0\s*\)", main_c):
        errors.append("cmd/iems/main.c 缺少 setvbuf(stdout, NULL, _IOLBF, 0)")
    if "stdbuf" in shell:
        errors.append("acceptance/iems/iems_product.py 仍依赖 stdbuf，掩盖产品缓冲缺陷")

    for err in errors:
        print(f"❌ [STDOUT_LINE_BUFFERED] {err}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
