FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libmpfr-dev libgmp3-dev libmpc-dev \
        git xxd cmake ninja-build clang-tidy \
        python3-pip clang gdb libncurses5-dev \
        libstdc++6 libclang-dev llvm-dev \
        clang-format cmake-format curl make gcc g++ \
    && rm -rf /var/lib/apt/lists/*

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs \
    | sh -s -- -y --default-toolchain 1.82 --profile=minimal \
            --component rustfmt,clippy,cargo
ENV PATH="/root/.cargo/bin:${PATH}"
RUN rustup which cargo
RUN rustup which rustc

COPY requirements.txt /.
RUN pip install --no-cache-dir --prefix /usr -r /requirements.txt

RUN curl \
    -fSL https://download.racket-lang.org/installers/8.10/racket-8.10-x86_64-linux-cs.sh  \
    -o /tmp/racket_installer.sh \
    && chmod +x /tmp/racket_installer.sh \
    && /tmp/racket_installer.sh --unix-style --dest /usr/local \
    && rm /tmp/racket_installer.sh

RUN raco pkg install -i -D --auto pretty-expressive fmt unix-signals doc-coverage quickcheck

RUN git clone https://github.com/db7/mockoto.git
RUN cmake -Smockoto -Bmockoto/build
RUN make -j -C mockoto/build
ENV PATH="${PATH}:/mockoto/build"
