#!/bin/bash
set -e

# Load shared environment
source "$(dirname "$0")/scripts/setup_env.sh"

# 参数解析
DO_PACKAGE=false
for arg in "$@"; do
    if [ "$arg" == "--package" ]; then
        DO_PACKAGE=true
    fi
done

echo "=== [1/3] Building Native (x86_64) ==="

# 代码格式化
"$PROJECT_ROOT/scripts/format.sh"

mkdir -p build
cd build

# 自动选择生成器 (优先 Ninja)
GENERATOR=""
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="-GNinja"
fi

cmake .. $GENERATOR -DCMAKE_BUILD_TYPE=Debug -DENABLE_UI=ON
cmake --build . -j$(nproc)

# 设置运行环境并执行 C 单元测试
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/../.local/linux-x86_64/lib
ctest --output-on-failure -j$(nproc)

# ==============================================================================
# 2. 原生打包 (可选)
# ==============================================================================
if [ "$DO_PACKAGE" = true ]; then
    echo "=== [2/3] Packaging Native (x86_64) ==="
    cmake --build . --target package
fi

# ==============================================================================
# 3. 交叉编译打包 (可选 - aarch64)
# ==============================================================================
if [ "$DO_PACKAGE" = true ]; then
    echo "=== [3/3] Cross-Packaging (aarch64) ==="
    cd "$PROJECT_ROOT"
    mkdir -p build_arm64 && cd build_arm64
    
    cmake .. $GENERATOR -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake \
             -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j$(nproc)
    cmake --build . --target package
    echo "=== Multi-platform Packaging Complete ==="
fi

# ==============================================================================
# 4. 包验证
# ==============================================================================
cd "$PROJECT_ROOT/build"
if [ -d "pkg_verify" ]; then rm -rf pkg_verify; fi
mkdir -p pkg_verify
LATEST_PKG=$(ls EdgeOS-*-linux-x86_64-IEMS.tar.gz 2>/dev/null | head -n 1)
if [ -f "$LATEST_PKG" ]; then
    tar -xf "$LATEST_PKG" -C pkg_verify
    ldd pkg_verify/bin/iems | grep "libzlog" || (echo "[FATAL] Package dependency missing!" && exit 1)
    echo "[CPack] Local package verified successfully."
fi
