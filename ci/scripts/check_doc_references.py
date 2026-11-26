#!/usr/bin/env python3
"""已实现态设计文档引用的文件与 dcu_* 符号必须真实存在。

目标态/设计态/不存在的列举行用同行的“目标态/设计态/不存在/未实现/计划/旧稿/另立”
标记豁免；豁免只针对该行，不针对整篇。
"""
import argparse
import os
import re
import sys

TARGET_DOCS = ("docs/design/DCU_698_OBJECT_MODEL.md",)
EXEMPT = ("目标态", "设计态", "不存在", "未实现", "计划", "旧稿", "另立", "后续")
PATH_RE = re.compile(
    r"(?<![\w/])((?:cmd|adapters|foundation|modules|tests|ci|scripts|acceptance|docs)/"
    r"[A-Za-z0-9_./-]+?\.(?:c|h|md|py|txt|json|cmake))(?::\d+)?")
SYM_RE = re.compile(r"\b(dcu_[a-z0-9_]+)\s*\(")


def _read(path):
    try:
        with open(path, encoding="utf-8") as f:
            return f.read()
    except OSError:
        return ""


def _symbols(root):
    found = set()
    for base in ("cmd", "adapters", "foundation", "modules", "tests"):
        for dirpath, _dirs, files in os.walk(os.path.join(root, base)):
            for name in files:
                if name.endswith((".c", ".h")):
                    found.update(
                        re.findall(r"\b(dcu_[a-z0-9_]+)\b", _read(os.path.join(dirpath, name))))
    return found


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    args = parser.parse_args()
    errors = []
    symbols = None
    for rel in TARGET_DOCS:
        path = os.path.join(args.root, rel)
        if not os.path.isfile(path):
            continue
        for lineno, line in enumerate(_read(path).splitlines(), 1):
            if any(word in line for word in EXEMPT):
                continue
            for match in PATH_RE.finditer(line):
                token = match.group(1)
                if not os.path.exists(os.path.join(args.root, token)):
                    errors.append(f"{rel}:{lineno}: 文件不存在 `{token}`")
            if symbols is None:
                symbols = _symbols(args.root)
            for match in SYM_RE.finditer(line):
                name = match.group(1)
                if name not in symbols:
                    errors.append(f"{rel}:{lineno}: 函数/符号不存在 `{name}`")
    for err in errors:
        print(f"❌ [DOC_REFERENCES] {err}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
