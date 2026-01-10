#!/bin/bash
set -e

# 参数解析
DO_PACKAGE=false
for arg in "$@"; do
    if [ "$arg" == "--package" ]; then
        DO_PACKAGE=true
    fi
done

# ==============================================================================
# 0. 开发环境初始化 (Python venv & Dependencies)
# ==============================================================================
echo "=== [0/3] Preparing Development Environment ==="

if [ ! -d ".venv" ]; then
    echo "[Python] Creating virtual environment..."
    python3 -m venv .venv
fi

# 自动激活并安装依赖
# 注意：在 Bash 脚本中 source 需要在当前进程生效，或者后续直接使用 .venv/bin/python
export PATH="$(pwd)/.venv/bin:$PATH"

if [ -f "requirements.txt" ]; then
    echo "[Python] Installing/Updating dependencies..."
    pip install -q --upgrade pip
    pip install -q -r requirements.txt
fi

# 确保第三方二进制文件有执行权限
chmod +x .local/linux-x86_64/bin/contract-compiler 2>/dev/null || true
chmod +x .local/linux-aarch64/bin/contract-compiler 2>/dev/null || true

# 代码格式化
./scripts/format.sh

# ==============================================================================
# 1. 本地原生构建与测试 (x86_64)
# ==============================================================================
echo "=== [1/3] Building Native (x86_64) ==="
mkdir -p build
cd build

# 自动选择生成器 (优先 Ninja)
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="-GNinja"
else
    GENERATOR=""
fi

cmake .. $GENERATOR -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)

# 设置本地库路径并运行测试
LOCAL_ARCH="linux-x86_64"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/../.local/$LOCAL_ARCH/lib
ctest --output-on-failure

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
    cd ..
    mkdir -p build_arm64
    cd build_arm64
    
    cmake .. $GENERATOR -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake \
             -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j$(nproc)
    cmake --build . --target package
    echo "=== Multi-platform Packaging Complete ==="
fi

# ==============================================================================
# 4. 包验证 (对最近生成的产品包进行 ldd 检查)
# ==============================================================================
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