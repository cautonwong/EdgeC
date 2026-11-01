"""验收用的产品二进制定位，以及"这份二进制是不是当前代码"的判定。

## 为什么需要这一层

验收最隐蔽的失败不是产品坏，而是**验了旧二进制**。结论看起来完整，却和当前
代码无关 —— 比红更难查，因为它是绿的。

本仓库踩过一次：台架把路径写死成 `<repo>/build/products/<p>/bin/<p>`，而开发时
构建产物在 `/tmp/edgec-build`，于是北向用例对着三周前的二进制跑。表现是
`MQTT_BROKER` 生效、`MQTT_PROTOCOL` 与 `MQTT_NODE_ID` 被无视（旧二进制里根本没
这两个字符串）—— 现象像"环境变量没传进去"，实际是文件根本不是那一份。

## 规则

1. 构建根由 `EDGEC_BUILD_DIR` 指定（CI 与开发各自给），缺省 `<repo>/build`
2. 单个产品仍可用 `<PRODUCT>_BIN` 直接指到具体文件（验证别人分支时不换 worktree）
3. 定位结果必须打出来：日志里能看到"验的是哪个文件、什么时候编的"
4. 二进制比源码旧时给警告 —— 不静默。是否升级成错误由调用方决定，
   这里只负责如实报告
"""
from __future__ import annotations

import os
import time
from pathlib import Path
from typing import Iterable

# 源码比二进制新多少才算"过期"。
# 不能取 0：挂载盘与 git checkout 的 mtime 有秒级抖动，等于 0 会天天误报。
_STALE_TOLERANCE_S = 5.0

_SOURCE_SUFFIXES = (".c", ".h")


def repo_root() -> Path:
    """本文件在 `acceptance/` 下，上两级就是仓库根。"""
    return Path(__file__).resolve().parent.parent


def build_root() -> Path:
    """构建根。CI 用 `build/`，开发常放 `/tmp`（挂载盘慢）。"""
    explicit = os.environ.get("EDGEC_BUILD_DIR")
    return Path(explicit) if explicit else repo_root() / "build"


def resolve_product_binary(product: str, *, explicit: str | None = None) -> Path:
    """产品可执行文件的路径。

    不检查存在性：路径不对时应该由调用方在启动那一刻报错，并说清找的是哪里。
    在这里静默回落到别的路径，正是"验错文件"的来源。
    """
    if explicit:
        return Path(explicit)
    return build_root() / "products" / product / "bin" / product


def _newest_source(root: Path) -> tuple[float, Path | None]:
    newest_mtime = 0.0
    newest_path: Path | None = None
    for path in root.rglob("*"):
        if path.suffix not in _SOURCE_SUFFIXES or not path.is_file():
            continue
        try:
            mtime = path.stat().st_mtime
        except OSError:
            continue
        if mtime > newest_mtime:
            newest_mtime, newest_path = mtime, path
    return newest_mtime, newest_path


def staleness(binary: Path | str, source_dirs: Iterable[Path]) -> str | None:
    """二进制比源码旧时返回说明，否则 None。

    只比较 `.c/.h`：改脚本或文档不需要重编，把它们算进来会天天报"过期"，
    报警一多就没人看。
    """
    binary = Path(binary)  # 调用方可能传 str（Popen 用得着）；这里不该因此炸掉
    try:
        binary_mtime = binary.stat().st_mtime
    except OSError:
        return f"找不到产品二进制：{binary}"

    worst_mtime = 0.0
    worst_path: Path | None = None
    for src_dir in source_dirs:
        if not src_dir.is_dir():
            continue
        mtime, path = _newest_source(src_dir)
        if mtime > worst_mtime:
            worst_mtime, worst_path = mtime, path

    if worst_path is None:
        return None
    if worst_mtime <= binary_mtime + _STALE_TOLERANCE_S:
        return None

    return (
        f"产品二进制比源码旧：{binary}\n"
        f"  二进制 {time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(binary_mtime))}\n"
        f"  源码   {worst_path} "
        f"{time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(worst_mtime))}\n"
        f"  验收会对着旧代码给结论。重编，或用 EDGEC_BUILD_DIR 指到正确的构建目录。"
    )


def describe(binary: Path | str) -> str:
    """一行说明"验的是哪份"。日志里必须有，否则事后无法判断结论对应哪次构建。

    诊断函数**绝不能成为启动失败的原因**：曾经因为调用方传的是 str 而在这里
    AttributeError，产品壳直接退出，整轮被判"起不来"。
    """
    binary = Path(binary)
    try:
        stat = binary.stat()
    except OSError:
        return f"产品二进制（缺失）：{binary}"
    stamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(stat.st_mtime))
    return f"产品二进制：{binary}（编译于 {stamp}，{stat.st_size} 字节）"
