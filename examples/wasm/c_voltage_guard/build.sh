#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

CLANG=clang
OUTPUT="voltage_guard.wasm"
SOURCE="voltage_guard.c"

echo "🔨 Compiling EdgeOS WASM module: $SOURCE..."
$CLANG --target=wasm32 \
    -O3 \
    -nostdlib \
    -Wl,--no-entry \
    -Wl,--export-all \
    -Wl,--allow-undefined \
    -o $OUTPUT $SOURCE

echo "✅ Generated $OUTPUT ($(du -h $OUTPUT | cut -f1))"
