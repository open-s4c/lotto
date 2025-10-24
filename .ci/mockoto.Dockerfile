FROM rnd-gitlab-eu-c.huawei.com:8443/germany-research-center/dresden-lab/s4c/utils/docker/racket:latest

RUN apt-get update && apt-get install -y --no-install-recommends \
        libclang-dev \
        llvm-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/db7/mockoto.git
RUN cmake -Smockoto -Bmockoto/build
RUN make -j -C mockoto/build
ENV PATH="${PATH}:/mockoto/build"
