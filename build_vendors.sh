#!/bin/bash
set -e

# Project paths
PROJECT_ROOT=$(pwd)
DEPS_DIR="$PROJECT_ROOT/_deps"  # Temporary download dir
INSTALL_ROOT="$PROJECT_ROOT/.local"

# Dependencies versions
CJSON_VER="v1.7.15"
LIBUV_VER="v1.44.2"
CMOCKA_VER="cmocka-1.1.7" # Git tag
LIBMODBUS_VER="v3.1.10"
SQLITE_VER="version-3.43.0" # Using github mirror tag style or similar

# Prepare directories
mkdir -p "$DEPS_DIR"

# Detect host architecture
HOST_ARCH=$(uname -m)
if [ "$HOST_ARCH" == "aarch64" ]; then
    DEFAULT_ARCH="arm64"
else
    DEFAULT_ARCH="x86_64"
fi

TARGET_ARCH=${1:-$DEFAULT_ARCH}

# Determine install path
INSTALL_PREFIX="$INSTALL_ROOT/linux-$TARGET_ARCH"
mkdir -p "$INSTALL_PREFIX"

echo "=== Building Vendors for Target: $TARGET_ARCH ==="
echo "=== Install Prefix: $INSTALL_PREFIX ==="

# Toolchain setup for Cross-Compilation
CMAKE_OPTS="-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -DCMAKE_BUILD_TYPE=Release"

if [ "$TARGET_ARCH" == "arm64" ] && [ "$HOST_ARCH" != "aarch64" ]; then
    echo ">>> Cross-compiling for arm64..."
    # Ensure toolchain file exists
    if [ -f "$PROJECT_ROOT/cmake/toolchains/aarch64-linux-gnu.cmake" ]; then
         CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_TOOLCHAIN_FILE=$PROJECT_ROOT/cmake/toolchains/aarch64-linux-gnu.cmake"
    else
         echo "Error: Toolchain file not found!"
         exit 1
    fi
fi

# -------------------------------------------------------------------------
# 1. cJSON
# -------------------------------------------------------------------------
echo ">>> Processing cJSON ($CJSON_VER)..."
if [ ! -d "$DEPS_DIR/cJSON" ]; then
    git clone --depth 1 --branch $CJSON_VER https://github.com/DaveGamble/cJSON.git "$DEPS_DIR/cJSON"
fi
mkdir -p "$DEPS_DIR/cJSON/build" && cd "$DEPS_DIR/cJSON/build"
cmake .. $CMAKE_OPTS -DENABLE_CJSON_TEST=OFF -DBUILD_SHARED_AND_STATIC_LIBS=ON
make -j$(nproc) && make install

# -------------------------------------------------------------------------
# 2. libuv
# -------------------------------------------------------------------------
echo ">>> Processing libuv ($LIBUV_VER)..."
if [ ! -d "$DEPS_DIR/libuv" ]; then
    git clone --depth 1 --branch $LIBUV_VER https://github.com/libuv/libuv.git "$DEPS_DIR/libuv"
fi
mkdir -p "$DEPS_DIR/libuv/build" && cd "$DEPS_DIR/libuv/build"
cmake .. $CMAKE_OPTS -DBUILD_TESTING=OFF
make -j$(nproc) && make install

# -------------------------------------------------------------------------
# 3. cmocka
# -------------------------------------------------------------------------
echo ">>> Processing cmocka ($CMOCKA_VER)..."
if [ ! -d "$DEPS_DIR/cmocka" ]; then
    git clone --depth 1 --branch $CMOCKA_VER https://git.cryptomilk.org/projects/cmocka.git "$DEPS_DIR/cmocka"
fi
mkdir -p "$DEPS_DIR/cmocka/build" && cd "$DEPS_DIR/cmocka/build"
cmake .. $CMAKE_OPTS -DWITH_STATIC_LIB=ON -DWITH_EXAMPLES=OFF -DWITH_CMOCKERY_SUPPORT=OFF
make -j$(nproc) && make install

# -------------------------------------------------------------------------
# 4. libmodbus
# -------------------------------------------------------------------------
echo ">>> Processing libmodbus ($LIBMODBUS_VER)..."
# libmodbus usually uses autotools, but we can try to use its experimental CMake support or standard autotools.
# For consistency, let's stick to the official repo which often needs autogen.sh
# However, to keep this script simple and dependency-free (no autotools on host), 
# we will use a known CMake-friendly fork or minimal CMake injection if standard fails.
# Actually, libmodbus has had CMake support issues. Let's use autotools if available, or write a tiny CMakeLists.
# Better strategy: Download, if no CMakeLists.txt, use autotools.
if [ ! -d "$DEPS_DIR/libmodbus" ]; then
    git clone --depth 1 --branch $LIBMODBUS_VER https://github.com/stephane/libmodbus.git "$DEPS_DIR/libmodbus"
fi

cd "$DEPS_DIR/libmodbus"
if [ -f "CMakeLists.txt" ]; then
    mkdir -p build && cd build
    cmake .. $CMAKE_OPTS -DBUILD_TESTS=OFF
    make -j$(nproc) && make install
else
    # Fallback: Inject a robust CMakeLists.txt for libmodbus
    echo "   (Injecting CMakeLists.txt for libmodbus)"
    cat <<EOF > CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(modbus C)

set(MODBUS_VERSION "3.1.10")

add_definitions(-D_GNU_SOURCE)
add_definitions(-DLIBMODBUS_VERSION="${MODBUS_VERSION}")

# Generate a minimal config.h for Linux
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/config.h "
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_STRINGS_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_FCNTL_H 1
#define HAVE_LIMITS_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_TIME_H 1
")

add_definitions(-DHAVE_CONFIG_H)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(src)

set(SRC 
    src/modbus.c 
    src/modbus-data.c 
    src/modbus-rtu.c 
    src/modbus-tcp.c
)

add_library(modbus STATIC ${SRC})

install(TARGETS modbus ARCHIVE DESTINATION lib)
install(FILES src/modbus.h src/modbus-version.h src/modbus-rtu.h src/modbus-tcp.h DESTINATION include/modbus)
EOF
    
    mkdir -p build && cd build
    cmake .. $CMAKE_OPTS
    make -j$(nproc) && make install
fi

# -------------------------------------------------------------------------
# 5. SQLite3
# -------------------------------------------------------------------------
echo ">>> Processing SQLite3..."
# SQLite amalgamation is best.
if [ ! -d "$DEPS_DIR/sqlite3" ]; then
    mkdir -p "$DEPS_DIR/sqlite3"
    # Download amalgamation zip (using a fixed version url)
    wget -qO "$DEPS_DIR/sqlite3/sqlite.zip" https://www.sqlite.org/2023/sqlite-amalgamation-3430000.zip
    unzip -q -o "$DEPS_DIR/sqlite3/sqlite.zip" -d "$DEPS_DIR/sqlite3/"
    mv "$DEPS_DIR/sqlite3/sqlite-amalgamation-3430000"/* "$DEPS_DIR/sqlite3/"
fi

# Manually compile SQLite3 static lib
cd "$DEPS_DIR/sqlite3"
CC=gcc
AR=ar
if [ "$TARGET_ARCH" == "arm64" ] && [ "$HOST_ARCH" != "aarch64" ]; then
    CC=aarch64-linux-gnu-gcc
    AR=aarch64-linux-gnu-ar
fi

$CC -O2 -c sqlite3.c -DSQLITE_ENABLE_COLUMN_METADATA -DSQLITE_ENABLE_RTREE
$AR rcs libsqlite3.a sqlite3.o

# Install
mkdir -p "$INSTALL_PREFIX/include" "$INSTALL_PREFIX/lib"
cp sqlite3.h sqlite3ext.h "$INSTALL_PREFIX/include/"
cp libsqlite3.a "$INSTALL_PREFIX/lib/"
echo ">>> SQLite3 installed."

echo "✅ All vendors built and installed to $INSTALL_PREFIX"
