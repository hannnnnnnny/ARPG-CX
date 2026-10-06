# Ndless SDK build environment for Bitling Run.
#
# Builds the official Ndless toolchain (binutils + GCC + newlib, as pinned in
# Ndless' own toolchain/build_toolchain.sh) and the Ndless SDK from source.
# The 2015 image on Docker Hub (ndless/ndless-sdk) predates the lcd_blit API
# required for HW-W+ (rotated LCD) calculators, so we do not use it.
#
# Build (once, ~20-40 min):   docker build -t bitling/ndless-sdk tools/docker
# Use:                        see tools/build_nspire.sh
FROM debian:bookworm

ARG NDLESS_REF=master
ARG JOBS=8

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential git wget ca-certificates xz-utils bzip2 \
        libgmp-dev libmpfr-dev libmpc-dev zlib1g-dev libtinfo-dev \
        libboost-program-options-dev texinfo python3 python3-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --recursive https://github.com/ndless-nspire/Ndless.git /ndless \
    && cd /ndless && git checkout ${NDLESS_REF} && git submodule update --init --recursive

# GDB is not needed for building programs; mark it as built to skip it.
WORKDIR /ndless/ndless-sdk/toolchain
RUN echo -n "gdb-16.2" > .built_gdb \
    && PARALLEL=-j${JOBS} ./build_toolchain.sh \
    && rm -rf download build

ENV PATH=/ndless/ndless-sdk/toolchain/install/bin:/ndless/ndless-sdk/bin:$PATH

WORKDIR /ndless/ndless-sdk
RUN make -j${JOBS}

WORKDIR /src
