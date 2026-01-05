# 使用 Debian 12 (Bookworm) Slim 版本，体积小且稳定
FROM debian:bookworm-slim

# 设置元数据
LABEL maintainer="EdgeOS Team"
LABEL description="Build environment for EdgeOS (x86_64 & aarch64)"

# 避免 apt 安装时的交互提示
ENV DEBIAN_FRONTEND=noninteractive

# Phase 1: Heavy Dependencies (Stable)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
    libssl-dev \
    cppcheck \
    valgrind \
    clang-format \
    clang-tidy \
    lcov \
    ccache \
    golang \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Ccache configuration
ENV CCACHE_DIR=/home/builder/.ccache
ENV CCACHE_MAXSIZE=5G

# Phase 2: Cross-compile Toolchains
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    libc6-dev-arm64-cross \
    && rm -rf /var/lib/apt/lists/*

# 3. 创建构建用户 (避免生成的产物是 root 权限)
ARG USER_ID=1000
ARG GROUP_ID=1000
RUN groupadd -g ${GROUP_ID} builder && \
    useradd -m -u ${USER_ID} -g builder -s /bin/bash builder

# 4. 配置工作目录
USER builder
WORKDIR /home/builder/src

# 5. 复制工具链文件 (将在下一步创建)
COPY --chown=builder:builder docker/toolchains/ /home/builder/toolchains/

# 默认指令
CMD ["/bin/bash"]
