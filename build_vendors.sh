#!/bin/bash
set -e

# Project root
PROJECT_ROOT=$(pwd)
VENDOR_SRC="$PROJECT_ROOT/vendor"
INSTALL_ROOT="$PROJECT_ROOT/.local"

# Host architecture
HOST_ARCH=$(uname -m)
# Normalize arch name for directory (Consistent with your requirement: linux-x86_64 and linux-arm64)
if [ "$HOST_ARCH" == "aarch64" ]; then
    ARCH_DIR="linux-arm64" 
else
    ARCH_DIR="linux-x86_64"
fi

INSTALL_DIR="$INSTALL_ROOT/$ARCH_DIR"

echo ">>> Building vendors into: $INSTALL_DIR"
mkdir -p "$INSTALL_DIR" "$VENDOR_SRC"

# 1. cJSON
build_cjson() {
    echo "--- Building cJSON ---"
    cd "$VENDOR_SRC"
    # Clean up if exists but empty or broken
    if [ -d "cJSON" ] && [ ! -f "cJSON/CMakeLists.txt" ]; then
        rm -rf cJSON
    fi
    
    if [ ! -d "cJSON" ]; then 
        git clone -b v1.7.15 https://github.com/DaveGamble/cJSON.git
    fi
    
    mkdir -p cJSON/build && cd cJSON/build
    cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
          -DENABLE_CJSON_TEST=OFF \
          -DBUILD_SHARED_AND_STATIC_LIBS=ON \
          -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc) install
}

# 2. Libuv
build_libuv() {
    echo "--- Building libuv ---"
    cd "$VENDOR_SRC"
    if [ -d "libuv" ] && [ ! -f "libuv/CMakeLists.txt" ]; then
        rm -rf libuv
    fi

    if [ ! -d "libuv" ]; then 
        git clone -b v1.44.2 https://github.com/libuv/libuv.git
    fi
    
    mkdir -p libuv/build && cd libuv/build
    cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
          -DBUILD_TESTING=OFF \
          -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc) install
}

# 3. CMocka
build_cmocka() {
    echo "--- Building cmocka ---"
    cd "$VENDOR_SRC"
    if [ -d "cmocka" ] && [ ! -f "cmocka/CMakeLists.txt" ]; then
        rm -rf cmocka
    fi

    if [ ! -d "cmocka" ]; then 
        git clone https://git.cryptomilk.org/projects/cmocka.git
    fi
    
    mkdir -p cmocka/build && cd cmocka/build
    cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
          -DWITH_STATIC_LIB=ON \
          -DWITH_EXAMPLES=OFF \
          -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc) install
}

# 4. SQLite3 (Download Amalgamation)
build_sqlite() {
    echo "--- Building SQLite3 ---"
    cd "$VENDOR_SRC"
    SQLITE_YEAR="2023"
    SQLITE_VER="3420000"
    ZIP_NAME="sqlite-amalgamation-$SQLITE_VER.zip"
    DIR_NAME="sqlite-amalgamation-$SQLITE_VER"

    if [ ! -d "$DIR_NAME" ]; then
        if [ ! -f "$ZIP_NAME" ]; then
            echo "Downloading SQLite..."
            wget "https://www.sqlite.org/$SQLITE_YEAR/$ZIP_NAME"
        fi
        unzip -o "$ZIP_NAME"
    fi
    
    cd "$DIR_NAME"
    
    # Manual compile static lib
    gcc -O2 -c sqlite3.c -o sqlite3.o -DSQLITE_ENABLE_COLUMN_METADATA -fPIC
    ar rcs libsqlite3.a sqlite3.o
    
    # Install
    mkdir -p "$INSTALL_DIR/include" "$INSTALL_DIR/lib"
    cp sqlite3.h sqlite3ext.h "$INSTALL_DIR/include/"
    cp libsqlite3.a "$INSTALL_DIR/lib/"
    echo "SQLite3 installed to $INSTALL_DIR"
}

build_cjson
build_libuv
build_cmocka
build_sqlite

echo "✅ All vendors built successfully!"