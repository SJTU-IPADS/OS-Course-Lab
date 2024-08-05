# Dockerfile for ipads/chcore_formatter.

FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai
RUN apt-get update

# --- Formatters installed by APT ---

# Install clang-format to format C/C++ code
RUN apt-get install -y clang-format=1:10.0-* && \
        clang-format --version

# --- Formatters installed by CURL ---

RUN apt-get install -y curl

# Install shfmt to format shell scripts
ARG SHFMT_VERSION=3.4.1
RUN curl -L -o /usr/local/bin/shfmt "https://github.com/mvdan/sh/releases/download/v${SHFMT_VERSION}/shfmt_v${SHFMT_VERSION}_linux_amd64" && \
        chmod +x /usr/local/bin/shfmt && \
        shfmt --version

RUN apt-get remove -y --purge curl

# --- Formatters installed by PIP ---

RUN apt-get install -y python3 python3-pip

# Install cmake-format to format CMake scripts
ARG CMAKE_FORMAT_VERSION=0.6.13
RUN pip3 install "cmakelang==$CMAKE_FORMAT_VERSION" && \
        cmake-format --version

# Install black to format Python
ARG BLACK_VERSION=21.10b0
RUN pip3 install "black==$BLACK_VERSION" && \
        black --version

# --- Clean up ---
RUN apt-get clean && rm -rf /var/lib/apt/lists/*
