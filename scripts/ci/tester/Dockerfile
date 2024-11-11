# Dockerfile for ipads/chcore_tester.

FROM ubuntu:20.04

ARG FBINFER_VERSION=1.1.0

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai
RUN apt-get update && \
        \
        apt-get install -y \
        sudo \
        curl \
        screen \
        expect \
        qemu-system-arm \
        qemu-system-x86 \
        qemu-system-misc \
        libncurses5 && \
        \
        apt-get clean && \
        rm -rf /var/lib/apt/lists/* && \
        \
        curl -L "https://github.com/facebook/infer/releases/download/v$FBINFER_VERSION/infer-linux64-v$FBINFER_VERSION.tar.xz" | tar -C /opt -xJ && \
        ln -s "/opt/infer-linux64-v$FBINFER_VERSION/bin/infer" /usr/local/bin/infer
