#!/bin/bash
set -e

# --- Configuration ---
PROJECT_ROOT=$(pwd)
VENDOR_SRC="$PROJECT_ROOT/vendor"
INSTALL_ROOT="$PROJECT_ROOT/.local"
TOOLCHAIN_DIR="$PROJECT_ROOT/docker/toolchains"

# --- Argument Parsing ---
usage() {
    echo "Usage: $0 [TARGET_ARCH]"
    echo "Builds all vendor dependencies for a specific architecture."
    echo ""
    echo "TARGET_ARCH:"
    echo "  linux-x86_64    (Default) Build for x86_64"
    echo "  linux-aarch64   Cross-compile for aarch64"
    exit 1
}

TARGET_ARCH=${1:-"linux-x86_64"}

# --- Environment Setup ---
HOST_ARCH=$(uname -m)
if [[ "$HOST_ARCH" == "aarch64" ]]; then
    HOST_ARCH_NAME="linux-aarch64"
else
    HOST_ARCH_NAME="linux-x86_64"
fi

INSTALL_DIR="$INSTALL_ROOT/$TARGET_ARCH"
CMAKE_ARGS=()

# Setup for cross-compiling
if [ "$TARGET_ARCH" != "$HOST_ARCH_NAME" ]; then
    echo "--- Cross-compiling for $TARGET_ARCH on $HOST_ARCH_NAME ---"
    if [ "$TARGET_ARCH" == "linux-aarch64" ]; then
        export CC=aarch64-linux-gnu-gcc
        export AR=aarch64-linux-gnu-ar
        CMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_DIR/aarch64-linux-gnu.cmake"
        CMAKE_ARGS+=("-DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE")
    else
        echo "ERROR: Unsupported cross-compilation target: $TARGET_ARCH"
        exit 1
    fi
else
    echo "--- Native build for $TARGET_ARCH ---"
    export CC=gcc
    export AR=ar
fi

echo ">>> Initializing submodules..."
git submodule update --init --recursive

echo ">>> Building vendors into: $INSTALL_DIR"
mkdir -p "$INSTALL_DIR"

# --- Build Functions ---

# 1. cJSON
build_cjson() {
    echo "--- Building cJSON ---"
    BUILD_DIR="$VENDOR_SRC/cJSON/build"
    mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
    
    cmake "${CMAKE_ARGS[@]}" \
          -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
          -DENABLE_CJSON_TEST=OFF \
          -DBUILD_SHARED_AND_STATIC_LIBS=ON \
          -DCMAKE_BUILD_TYPE=Release \
          ..
    make -j$(nproc) install
}

# 2. Libuv
build_libuv() {
    echo "--- Building libuv ---"
    BUILD_DIR="$VENDOR_SRC/libuv/build"
    mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"

    cmake "${CMAKE_ARGS[@]}" \
          -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
          -DBUILD_TESTING=OFF \
          -DCMAKE_BUILD_TYPE=Release \
          ..
    make -j$(nproc) install
}

# 3. CMocka
build_cmocka() {
    echo "--- Building cmocka ---"
    BUILD_DIR="$VENDOR_SRC/cmocka/build"
    mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"

    cmake "${CMAKE_ARGS[@]}" \
          -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
          -DWITH_STATIC_LIB=ON \
          -DWITH_EXAMPLES=OFF \
          -DCMAKE_BUILD_TYPE=Release \
          ..
    make -j$(nproc) install
}

# 4. SQLite3 (from submodule)
build_sqlite() {
    echo "--- Building SQLite3 ---"
    cd "$VENDOR_SRC/sqlite"
    
    # Manual compile static lib using the configured compiler
    "$CC" -O2 -c sqlite3.c -o sqlite3.o -DSQLITE_ENABLE_COLUMN_METADATA -fPIC
    "$AR" rcs libsqlite3.a sqlite3.o
    
    # Install
    mkdir -p "$INSTALL_DIR/include" "$INSTALL_DIR/lib"
    cp sqlite3.h sqlite3ext.h "$INSTALL_DIR/include/"
    cp libsqlite3.a "$INSTALL_DIR/lib/"
    echo "SQLite3 installed to $INSTALL_DIR"
}

# 5. Libmodbus
build_libmodbus() {
    echo "--- Building libmodbus ---"
    cd "$VENDOR_SRC/libmodbus"

    CONFIGURE_ARGS=("--prefix=$INSTALL_DIR" --enable-static --disable-shared)
    if [ "$TARGET_ARCH" == "linux-aarch64" ]; then
        CONFIGURE_ARGS+=("--host=aarch64-linux-gnu")
    fi
    
    ./autogen.sh
    ./configure "${CONFIGURE_ARGS[@]}"
    make -j$(nproc) install
}

# --- Execution ---
build_cjson
build_libuv
build_cmocka
build_sqlite
build_libmodbus

echo "✅ All vendors built successfully for $TARGET_ARCH!"