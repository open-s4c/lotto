FROM rnd-gitlab-eu-c.huawei.com:8443/germany-research-center/dresden-lab/s4c/utils/docker/clang-format

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential libmpfr-dev libgmp3-dev libmpc-dev
RUN wget http://ftp.gnu.org/gnu/gcc/gcc-14.2.0/gcc-14.2.0.tar.gz
RUN tar -xf gcc-14.2.0.tar.gz
RUN cd gcc-14.2.0 && ./configure -v \
    --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu        \
    --prefix=/usr/local/gcc-14.2.0 --enable-checking=release --enable-languages=c,c++ \
    --disable-multilib --program-suffix=-14.2.0 >/dev/null 2>/dev/null
RUN cd gcc-14.2.0 && make -j 4 >/dev/null 2>/dev/null
RUN cd gcc-14.2.0 && make install -j 4
RUN update-alternatives --install /usr/bin/g++ g++ /usr/local/gcc-14.2.0/bin/g++-14.2.0 14
RUN update-alternatives --install /usr/bin/gcc gcc /usr/local/gcc-14.2.0/bin/gcc-14.2.0 14
RUN rm gcc-14.2.0* -rf
RUN echo /usr/local/gcc-14.2.0/lib64 > /etc/ld.so.conf.d/gcc-14.conf
RUN apt-get update                                \
    && apt-get install -y --no-install-recommends \
        git xxd cmake ninja-build clang-tidy      \
        python3-pip clang gdb libncurses5-dev     \
        libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs \
    | sh -s -- -y --default-toolchain 1.82 --profile=minimal \
            --component rustfmt clippy cargo
ENV PATH="/root/.cargo/bin:${PATH}"
RUN rustc && cargo

COPY requirements.txt /.
RUN pip install --trusted-host mirrors.tools.huawei.com -i https://mirrors.tools.huawei.com/pypi/simple \
    --no-cache-dir --prefix /usr -r /requirements.txt

RUN curl \
    -fSL https://download.racket-lang.org/installers/8.10/racket-8.10-x86_64-linux-cs.sh  \
    -o /tmp/racket_installer.sh \
    && chmod +x /tmp/racket_installer.sh \
    && /tmp/racket_installer.sh --unix-style --dest /usr/local \
    && rm /tmp/racket_installer.sh

RUN  curl -LO https://github.com/sorawee/pretty-expressive/archive/refs/heads/main.tar.gz
RUN  raco pkg install -i -D -n pretty-expressive main.tar.gz
RUN  curl -LO https://github.com/sorawee/fmt/archive/refs/heads/master.tar.gz
RUN  raco pkg install -i -D -n fmt master.tar.gz
RUN  curl -LO https://github.com/tonyg/racket-unix-signals/archive/refs/heads/master.tar.gz
RUN  raco pkg install -i -D -n unix-signals master.tar.gz
RUN  curl -LO https://github.com/jackfirth/doc-coverage/archive/refs/heads/master.tar.gz
RUN  raco pkg install -i -D -n doc-coverage master.tar.gz || true
RUN  curl -LO https://github.com/ifigueroap/racket-quickcheck/archive/refs/heads/master.tar.gz
RUN  raco pkg install -i -D -n quickcheck master.tar.gz || true
