# 基础镜像
FROM ubuntu:latest

# 运行环境
# C环境
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    gcc

# java环境
ENV JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
RUN apt-get update && apt-get install -y \
    openjdk-8-jdk
RUN java -version

# python环境
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    python3-dev
# 设置Python 3作为默认的python版本
RUN update-alternatives --install /usr/bin/python python /usr/bin/python3 10

# verilog
RUN apt-get update && apt-get install -y \
    iverilog \
    gtkwave

# lua
RUN apt-get update && apt-get install -y \
    lua5.3


# 依赖环境
# boost
RUN apt-get update && apt-get install -y \
    cmake \
    make \
    git \
    libboost-filesystem-dev \
    libboost-system-dev \
    libboost-chrono-dev \
    librabbitmq-dev

WORKDIR /usr/src
# SimpleAmqpClient
RUN git clone https://github.com/alanxz/SimpleAmqpClient.git && \
    cd SimpleAmqpClient && \
    mkdir -p build && \
    cd build && \
    cmake .. && \
    make && \
    make install

# workspace
RUN git clone https://github.com/CodingHanYa/workspace.git && \
    cd workspace && \
    mkdir -p build && \
    cd build && \
    cmake .. && \
    make && \
    make install
# 纠正workspace的头文件路径
RUN mv /usr/local/workspace /usr/local/include/workspace

# 清理缓存以减小镜像大小
RUN apt-get clean
RUN rm -rf /var/lib/apt/lists/*
RUN ldconfig

# 工作目录
WORKDIR /app

# 复制源代码
COPY . .
RUN chmod +x ./scripts
# 编译
RUN make build

# 设置启动命令
CMD ["./build/JudgeCompile"]