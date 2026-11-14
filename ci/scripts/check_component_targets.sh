#!/bin/sh
# Build each reusable component target on its own. This catches a component that
# only builds because a sibling target happened to be built first.
set -eu
BUILD_DIR="${1:-build}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

for meta in "$ROOT"/modules/*/component.meta "$ROOT"/adapters/*/component.meta; do
    dir=$(dirname "$meta")
    target=$(sed -n 's/^add_library(\([A-Za-z0-9_]*\).*/\1/p' "$dir/CMakeLists.txt" | head -1)
    if [ -z "$target" ]; then
        echo "[component-target] $dir: no add_library target" >&2
        exit 1
    fi
    echo "[component-target] $target"
    cmake --build "$BUILD_DIR" --target "$target" -j "${JOBS:-4}"
done
echo "[component-target] all targets built"
