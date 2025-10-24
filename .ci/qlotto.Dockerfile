FROM rnd-gitlab-eu-c.huawei.com:8443/germany-research-center/dresden-lab/s4c/utils/docker/base:latest

################################################################################
# Essential packages
################################################################################
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        wget \
        openssh-client \
        git \
        ninja-build \
        cmake \
    && rm -rf /var/lib/apt/lists/*

################################################################################
# Packages for qlotto
################################################################################
RUN apt-get update && apt-get install -y --no-install-recommends \
        ccache \
        python3-pip \
        pkg-config \
        libglib2.0-dev \
        libpixman-1-dev \
        libelf-dev \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --upgrade tomli # Required to build QEMUv9

################################################################################
# Packages for lotto
################################################################################
RUN apt-get update && apt-get install -y --no-install-recommends \
        xxd \
        g++-arm-linux-gnueabi \
        g++-aarch64-linux-gnu \
        gcc-aarch64-linux-gnu \
        libc-dev-arm64-cross \
    	gcc-arm-linux-gnueabi \
    	libc6-dev-armel-cross \
        linux-libc-dev-armel-cross \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --upgrade meson

################################################################################
# Packages for buildroot
################################################################################
RUN apt-get update && apt-get install -y --no-install-recommends \
        sudo \
        genext2fs \
    && rm -rf /var/lib/apt/lists/*

################################################################################
# Copy over linux aarch64 linux kernel
################################################################################

RUN mkdir -p /buildroot_images
COPY --from=rnd-gitlab-eu-c.huawei.com:8443/germany-research-center/dresden-lab/s4c/lotto/qlotto_kernel:latest /buildroot_images /buildroot_images
RUN mkdir -p /buildroot_extracted
RUN sudo tar -C /buildroot_extracted -xf /buildroot_images/rootfs.tar
