#!/usr/bin/env bash
set -e

ROOT_DIR=$(pwd)
TP_DIR=${ROOT_DIR}/third_party
SRC_DIR=${TP_DIR}
BUILD_DIR=${TP_DIR}/build
INSTALL_DIR=${ROOT_DIR}/.local

build_target() {
    local TARGET=$1
    local HOST=$2
    local CC=$3

    local PREFIX=${INSTALL_DIR}/${TARGET}
    local BDIR=${BUILD_DIR}/${TARGET}

    mkdir -p "${BDIR}" "${PREFIX}"

    export CC=${CC}
    export CFLAGS="-O2 -fPIC"
    export LDFLAGS=""

    echo "=============================="
    echo " Building for ${TARGET}"
    echo "=============================="

    ############################
    # libcurl
    ############################
    cd ${SRC_DIR}/libcurl
    [ -f configure ] || ./buildconf

    mkdir -p ${BDIR}/libcurl && cd ${BDIR}/libcurl
    ${SRC_DIR}/libcurl/configure \
        --prefix=${PREFIX} \
        ${HOST:+--host=${HOST}} \
        --disable-shared \
        --enable-static \
        --without-libpsl \
        --without-zlib \
        --without-ssl \
        --disable-ldap \
        --disable-rtsp \
        --disable-dict \
        --disable-telnet \
        --disable-pop3 \
        --disable-imap \
        --disable-smtp \
        --disable-gopher

    make -j$(nproc)
    make install

    ############################
    # paho.mqtt.c
    ############################
    cd ${SRC_DIR}/paho.mqtt.c
    mkdir -p ${BDIR}/paho && cd ${BDIR}/paho

    cmake ${SRC_DIR}/paho.mqtt.c \
        -DCMAKE_INSTALL_PREFIX=${PREFIX} \
        -DCMAKE_C_COMPILER=${CC} \
        -DPAHO_BUILD_STATIC=TRUE \
        -DPAHO_BUILD_SHARED=FALSE \
        -DPAHO_WITH_SSL=FALSE

    make -j$(nproc)
    make install

    ############################
    # quickjs
    ############################
    cd ${SRC_DIR}/quickjs

    make clean || true
    make libquickjs.a \
        CC=${CC} \
        AR=ar \
        CFLAGS="-O2 -fPIC -D_GNU_SOURCE -DCONFIG_VERSION=\"$(shell cat VERSION)\" "

    mkdir -p ${PREFIX}/lib ${PREFIX}/include/quickjs
    cp libquickjs.a ${PREFIX}/lib/
    cp quickjs.h quickjs-libc.h ${PREFIX}/include/quickjs/
}

############################
# native x86_64
############################
build_target linux-x86_64 "" gcc

############################
# cross aarch64
############################
build_target linux-aarch64 aarch64-linux-gnu aarch64-linux-gnu-gcc

echo "All third-party libraries built successfully."
