#!/bin/bash
set -e # 遇到错误立即退出

# 项目根目录
PROJECT_ROOT=$(pwd)
INSTALL_ROOT="$PROJECT_ROOT/.local"

# 定义要构建的架构列表
TARGETS=("host" "arm64")

# =========================================================
# 函数：编译 cJSON
# =========================================================
build_cjson() {
    ARCH=$1
    echo ">>> Building cJSON for [$ARCH] ..."
    
    SOURCE_DIR="$PROJECT_ROOT/vendor/cJSON"
    BUILD_DIR="$SOURCE_DIR/build-$ARCH"
    
    # Host 架构归一化
    if [ "$ARCH" == "host" ]; then
        INSTALL_DIR="$INSTALL_ROOT/linux-x86_64"
    else
        INSTALL_DIR="$INSTALL_ROOT/linux-$ARCH"
    fi
    
    rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    CMAKE_ARGS="-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DENABLE_CJSON_TEST=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release"
    
    if [ "$ARCH" == "aarch64" ]; then
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$PROJECT_ROOT/docker/toolchains/aarch64-linux-gnu.cmake"
    fi
    
    cmake $CMAKE_ARGS ..
    make -j$(nproc)
    make install
    
    echo ">>> cJSON for [$ARCH] installed to $INSTALL_DIR"
    cd $PROJECT_ROOT
}

# =========================================================
# 函数：编译 libuv
# =========================================================
build_libuv() {
    ARCH=$1
    echo ">>> Building libuv for [$ARCH] ..."
    
    SOURCE_DIR="$PROJECT_ROOT/vendor/libuv"
    BUILD_DIR="$SOURCE_DIR/build-$ARCH"
    
    if [ "$ARCH" == "host" ]; then
        INSTALL_DIR="$INSTALL_ROOT/linux-x86_64"
    else
        INSTALL_DIR="$INSTALL_ROOT/linux-$ARCH"
    fi

    rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    CMAKE_ARGS="-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DBUILD_TESTING=OFF -DLIBUV_BUILD_SHARED=OFF -DCMAKE_BUILD_TYPE=Release"
    
    if [ "$ARCH" == "aarch64" ]; then
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$PROJECT_ROOT/docker/toolchains/aarch64-linux-gnu.cmake"
    fi
    
    cmake $CMAKE_ARGS ..
    make -j$(nproc)
    make install
    echo ">>> libuv for [$ARCH] installed."
    cd $PROJECT_ROOT
}

# =========================================================
# 函数：编译 cmocka
# =========================================================
build_cmocka() {
    ARCH=$1
    echo ">>> Building cmocka for [$ARCH] ..."
    
    SOURCE_DIR="$PROJECT_ROOT/vendor/cmocka"
    BUILD_DIR="$SOURCE_DIR/build-$ARCH"

    if [ "$ARCH" == "host" ]; then
        INSTALL_DIR="$INSTALL_ROOT/linux-x86_64"
    else
        INSTALL_DIR="$INSTALL_ROOT/linux-$ARCH"
    fi

    rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    CMAKE_ARGS="-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DWITH_STATIC_LIB=ON -DWITH_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release"
    
    if [ "$ARCH" == "aarch64" ]; then
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$PROJECT_ROOT/docker/toolchains/aarch64-linux-gnu.cmake"
    fi
    
    cmake $CMAKE_ARGS ..
    make -j$(nproc)
    make install
    echo ">>> cmocka for [$ARCH] installed."
    cd $PROJECT_ROOT
}


# =========================================================
# 主循环
# =========================================================

HOST_ARCH=$(uname -m)

for TARGET in "${TARGETS[@]}"; do
    if [ "$TARGET" == "host" ]; then
        REAL_ARCH=$HOST_ARCH
    else
        REAL_ARCH=$TARGET
    fi
    
    echo "=========================================="
    echo " Building vendors for Target: $REAL_ARCH"
    echo "=========================================="
    
    build_cjson $TARGET
    build_libuv $TARGET
    build_cmocka $TARGET
done

echo "✅ All vendors built successfully!"
