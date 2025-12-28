#!/bin/bash
set -e

# 镜像名称
IMAGE_NAME="edgeos-builder:v1.0"
# 项目根目录
PROJECT_ROOT=$(pwd)

# 1. 检查是否需要构建镜像
if [[ "$(docker images -q $IMAGE_NAME 2> /dev/null)" == "" ]]; then
    echo "[EdgeOS] Building Docker image..."
    docker build \
        --build-arg USER_ID=$(id -u) \
        --build-arg GROUP_ID=$(id -g) \
        -t $IMAGE_NAME -f docker/Dockerfile .
fi

# 2. 定义编译类型 (默认本机 x86，传入 "arm" 则交叉编译)
BUILD_TYPE=${1:-x86}
if [ "$BUILD_TYPE" == "arm" ]; then
    ARCH="arm64"
else
    ARCH="x86_64"
fi

BUILD_DIR="build_docker_${ARCH}"

echo "[EdgeOS] Compiling for target: $ARCH ..."

# 3. 运行容器进行编译
docker run --rm -it \
    -v "$PROJECT_ROOT":/home/builder/src \
    $IMAGE_NAME /bin/bash -c "
        set -e

        echo \"[EdgeOS] Ensuring vendor libraries for $ARCH are built...\"
        ./build_vendors.sh $ARCH

        mkdir -p $BUILD_DIR && cd $BUILD_DIR

        if [ '\''$ARCH'\' == 'arm64' ]; then
            # 交叉编译配置
            cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/aarch64-linux-gnu.cmake \
                  -DCMAKE_BUILD_TYPE=Release ..
        else
            # 本机编译配置 (带 ASan)
            cmake -DCMAKE_BUILD_TYPE=Debug ..
        fi

        # 编译
        make -j$(nproc)

        # 如果是本机编译，运行测试
        if [ '\''$ARCH'\' == 'x86_64' ]; then
            echo '[EdgeOS] Running Tests...'
            ctest --output-on-failure
        fi
    "

echo "[EdgeOS] Build finished. Artifacts are in $BUILD_DIR"
