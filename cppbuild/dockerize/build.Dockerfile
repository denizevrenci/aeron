FROM ubuntu:19.04

COPY sources.list /etc/apt

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ninja-build \
        g++-9 \
        clang-8 \
        lld-8 \
        openjdk-11-jdk-headless \
        libbsd-dev \
        uuid-dev

RUN apt-get install -y --no-install-recommends curl

RUN mkdir /tmp/cmake && \
    cd /tmp/cmake && \
    curl -fsSLo install-cmake.sh https://cmake.org/files/v3.15/cmake-3.15.3-Linux-x86_64.sh && \
    bash install-cmake.sh --prefix=/usr --skip-license && \
    rm -rf /tmp/cmake

ENV JAVA_HOME="/usr/lib/jvm/java-11-openjdk-amd64"
