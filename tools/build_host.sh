#!/bin/sh
# Build host programs: headless runner, tests, Windows simulator.
#   sh tools/build_host.sh  -> build/ad_headless(.exe) build/ad_headless_hd(.exe) build/ad_sim(.exe) build/tests(.exe)
#                              build/AshenDepthsDesktop.exe
set -eu
cd "$(dirname "$0")/.."
if [ -z "${CC:-}" ]; then
    if [ -x .tools/Scripts/python.exe ]; then CC=".tools/Scripts/python.exe -m ziglang cc"
    elif [ -x .tools/bin/python ]; then CC=".tools/bin/python -m ziglang cc"
    else CC=gcc; fi
fi
CFLAGS="-std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -g"
mkdir -p build
CORE=$(find src -name '*.c' ! -name runner.c | sort)
EXE=""
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) EXE=".exe";; esac
echo "[host] headless runner"
$CC $CFLAGS -o build/ad_headless$EXE $CORE platform/headless/main_headless.c platform/headless/png.c -lm
echo "[host] headless runner, desktop resolution (640x480, 12px CJK)"
$CC $CFLAGS -DGFX_HD -o build/ad_headless_hd$EXE $CORE platform/headless/main_headless.c platform/headless/png.c -lm
echo "[host] balance simulator"
$CC $CFLAGS -o build/ad_sim$EXE $CORE tools/sim/sim_main.c -lm
echo "[host] unit tests"
$CC $CFLAGS -o build/tests$EXE $CORE $(ls tests/*.c | sort) -lm
if [ -n "$EXE" ]; then
    echo "[host] windows simulator"
    $CC $CFLAGS -DGFX_HD -o build/AshenDepthsDesktop.exe $CORE src/game/runner.c platform/desktop/main_win32.c platform/desktop/audio_win32.c \
        -lgdi32 -luser32 -lwinmm -Wl,--subsystem,windows
fi
echo "[host] done"
