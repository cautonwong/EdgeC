#!/bin/bash
set -e

# 参数解析
DO_PACKAGE=false
for arg in "$@"; do
    if [ "$arg" == "--package" ]; then
        DO_PACKAGE=true
    fi
done

./scripts/format.sh

# 1. 本地原生构建与测试 (x86_64)
echo "=== [1/3] Building Native (x86_64) ==="
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# 设置本地库路径并运行测试
LOCAL_ARCH="linux-x86_64"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/../.local/$LOCAL_ARCH/lib
ctest --output-on-failure

# 2. 原生打包 (可选)
if [ "$DO_PACKAGE" = true ]; then
    echo "=== [2/3] Packaging Native (x86_64) ==="
    make package
fi

# 3. 交叉编译打包 (可选 - aarch64)
if [ "$DO_PACKAGE" = true ]; then
    echo "=== [3/3] Cross-Packaging (aarch64) ==="
    cd ..
    mkdir -p build_arm64
    cd build_arm64
    
    # 使用交叉编译工具链
    cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake \
             -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    make package
    echo "=== Multi-platform Packaging Complete ==="
fi

# 4. 包验证 (对最近生成的产品包进行 ldd 检查)
# 如果刚跑完交叉编译，目录在 build_arm64，需要回退
if [[ "$PWD" == *"build_arm64"* ]]; then
    cd ../build
fi

if [ -d "pkg_verify" ]; then rm -rf pkg_verify; fi
mkdir -p pkg_verify
LATEST_PKG=$(ls EdgeOS-*-${LOCAL_ARCH}-IEMS.tar.gz 2>/dev/null | head -n 1)
if [ -f "$LATEST_PKG" ]; then
    tar -xf "$LATEST_PKG" -C pkg_verify
    ldd pkg_verify/bin/iems | grep "libzlog" || (echo "[FATAL] Package dependency missing!" && exit 1)
    echo "[CPack] Local package verified successfully."
fi
