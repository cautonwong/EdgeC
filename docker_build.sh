#!/bin/bash
set -e

# 镜像名称
IMAGE_NAME="edgeos-builder:v1.0"
# 项目根目录
PROJECT_ROOT=$(pwd)

# 1. 检查是否需要构建镜像
if [[ "$(docker images -q $IMAGE_NAME 2> /dev/null)" == "" ]]; then
    echo "[EdgeOS] Building Docker image..."
    # 传入当前用户的 UID，防止生成的文件属于 root
    docker build \
        --build-arg USER_ID=$(id -u) \
        --build-arg GROUP_ID=$(id -g) \
        -t $IMAGE_NAME -f docker/Dockerfile .
fi

# 2. 定义编译类型 (默认本机 x86，传入 "arm" 则交叉编译)
BUILD_TYPE=${1:-x86}
BUILD_DIR="build_docker_${BUILD_TYPE}"

echo "[EdgeOS] Compiling for target: $BUILD_TYPE ..."

# 3. 运行容器进行编译
# -v: 挂载当前代码目录到容器内
# --rm: 跑完即删容器
docker run --rm -it \
    -v "$PROJECT_ROOT":/home/builder/src \
    $IMAGE_NAME /bin/bash -c "
        set -e
        
        # Check if vendor libs are already built
        if [ ! -f .local/linux-x86_64/lib/libuv_a.a ] || \
           [ ! -f .local/linux-x86_64/lib/libcjson.a ] || \
           [ ! -f .local/linux-x86_64/lib/libcmocka-static.a ]; then
            echo \"[EdgeOS] Building vendor libraries...\"
            ./build_vendors.sh
        else
            echo \"[EdgeOS] Vendor libraries already built, skipping build_vendors.sh\"
        fi

        mkdir -p $BUILD_DIR && cd $BUILD_DIR
        
        if [ '$BUILD_TYPE' == 'arm' ]; then
            # 交叉编译配置
            cmake -DCMAKE_TOOLCHAIN_FILE=/home/builder/toolchains/aarch64-linux-gnu.cmake \
                  -DCMAKE_BUILD_TYPE=Release ..
        else
            # 本机编译配置 (带 ASan)
            cmake -DCMAKE_BUILD_TYPE=Debug ..
        fi
        
        # 编译
        make -j$(nproc)
        
        # 如果是本机编译，运行测试
        if [ '$BUILD_TYPE' == 'x86' ]; then
            echo '[EdgeOS] Running Tests...'
            ctest --output-on-failure
        fi
    "

echo "[EdgeOS] Build finished. Artifacts are in $BUILD_DIR"
