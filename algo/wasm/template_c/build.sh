#!/bin/bash
# 简单的 WASM 编译脚本 (无需 WASI-SDK，仅需 clang)

CLANG=clang
OUTPUT="moving_average.wasm"
SOURCE="moving_average.c"

$CLANG --target=wasm32 \
    -O3 \
    -nostdlib \
    -Wl,--no-entry \
    -Wl,--export-all \
    -Wl,--allow-undefined \
    -o $OUTPUT $SOURCE

echo "✅ Generated $OUTPUT"

