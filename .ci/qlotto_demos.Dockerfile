FROM rnd-gitlab-eu-c.huawei.com:8443/germany-research-center/dresden-lab/s4c/lotto/qlotto:latest

################################################################################
# Packages for linux demo
################################################################################
RUN apt-get update && apt-get install -y --no-install-recommends \
        bison \
        flex \
        bc \
        cpio \
    && rm -rf /var/lib/apt/lists/*

################################################################################
# Linux demo
################################################################################
ARG LINUX_VERSION
ARG LINUX=linux-${LINUX_VERSION}
ARG LINUX_URL=https://cdn.kernel.org/pub/linux/kernel/v6.x/${LINUX}.tar.xz
RUN curl -LO ${LINUX_URL}

ARG BUSYBOX=busybox-1.36.1
ARG BUSYBOX_URL=https://busybox.net/downloads/${BUSYBOX}.tar.bz2
RUN curl -LO ${BUSYBOX_URL}

RUN cp /etc/apt/sources.list /etc/apt/sources.list.arm64
RUN sed -i 's/mirrors.tools.huawei.com\/ubuntu/ports.ubuntu.com\/ubuntu-ports/' /etc/apt/sources.list.arm64
RUN sed -i 's/^deb /deb [arch=amd64] /' /etc/apt/sources.list
RUN sed -i 's/^deb /deb [arch=arm64] /' /etc/apt/sources.list.arm64
RUN cat /etc/apt/sources.list.arm64 >> /etc/apt/sources.list

RUN dpkg --add-architecture arm64

RUN apt-get update && apt-get install -y --no-install-recommends \
        libelf-dev:arm64 \
        pahole \
        clang \
        llvm \
        binutils-aarch64-linux-gnu \
        sed \
        bash \
    && rm -rf /var/lib/apt/lists/*
