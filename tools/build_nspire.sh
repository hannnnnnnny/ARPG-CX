#!/bin/sh
# Build AshenDepths.tns for the TI-Nspire CX with the Ndless
# SDK inside Docker. First run builds the SDK image (~20-40 min, once).
#   sh tools/build_nspire.sh        -> dist/AshenDepths.tns
set -eu
cd "$(dirname "$0")/.."
IMAGE=bitling/ndless-sdk
if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
    echo "[nspire] building Ndless SDK image (one-time)"
    docker build -t "$IMAGE" tools/docker
fi
# MSYS (Git Bash on Windows) would mangle the container path; disable that.
MSYS_NO_PATHCONV=1 docker run --rm -v "$(pwd -W 2>/dev/null || pwd):/src" -w /src/platform/nspire "$IMAGE" \
    sh -c 'make clean >/dev/null && make'
mkdir -p dist
cp platform/nspire/AshenDepths.tns dist/
ls -l dist/*.tns
