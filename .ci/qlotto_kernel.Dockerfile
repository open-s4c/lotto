FROM rnd-gitlab-eu-c.huawei.com:8443/germany-research-center/dresden-lab/s4c/utils/docker/base:latest

################################################################################
# Essential packages
################################################################################
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        wget \
        cmake \
    && rm -rf /var/lib/apt/lists/*

################################################################################
# Buildroot dependencies
################################################################################
RUN apt-get update && apt-get install -y --no-install-recommends \
        ccache \
        file \
        cpio \
        bc \
        unzip \
        rsync \
    && rm -rf /var/lib/apt/lists/*

################################################################################
# Get Buildroot
################################################################################
RUN wget --tries=10 -O buildroot.tar.gz https://buildroot.org/downloads/buildroot-2023.08.tar.gz
RUN tar xf buildroot.tar.gz
RUN mv buildroot-2023.08/ /buildroot

################################################################################
# Configure Buildroot
################################################################################
ARG GIT_COMMIT
ARG CI_PROJECT_DIR
RUN cp ${CI_PROJECT_DIR}/.ci/buildroot.config /buildroot/.config
RUN cp ${CI_PROJECT_DIR}/.ci/linux-armv8-minimal.config /buildroot/linux-armv8-minimal.config
WORKDIR /buildroot
RUN make source
RUN make
RUN mkdir -p /buildroot_images
RUN cp -a /buildroot/output/images/* /buildroot_images
