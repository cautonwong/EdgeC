#!/bin/bash
set -e
./scripts/format.sh
mkdir -p build
mkdir -p build/logs
mkdir -p build/tests/logs
mkdir -p build/tests
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j4

# 动态探测架构并设置库路径
ARCH=$(uname -m)
if [ "$ARCH" == "x86_64" ]; then
    LOCAL_ARCH="linux-x86_64"
else
    LOCAL_ARCH="linux-aarch64"
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/../.local/$LOCAL_ARCH/lib

ctest --output-on-failure

# [新增] 打包测试
echo "[CPack] Testing distribution package..."
make package

# [新增] 包运行验证 (验证 RPATH 和 库完整性)
mkdir -p pkg_verify
# 使用通配符匹配版本号，增强脚本鲁棒性
tar -xf EdgeOS-*-${LOCAL_ARCH}-IEMS.tar.gz -C pkg_verify
# 我们不真的运行，只验证 ldd 输出是否由于 RPATH 解决了依赖
ldd pkg_verify/bin/iems | grep "libzlog" || (echo "[FATAL] Package missing zlog dependency!" && exit 1)
echo "[CPack] TGZ Package verified successfully."
