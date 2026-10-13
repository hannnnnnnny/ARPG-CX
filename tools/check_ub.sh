#!/bin/sh
# Checked build: tests, the balance simulator and the headless runner with
# undefined-behaviour checks (out-of-bounds indexes, overflow, bad shifts,
# null pointers). Any hit stops with the file and line. Run before a release.
#   sh tools/check_ub.sh            -> tests + 6 classes x 3 builds x 4 h + headless menus
set -eu
cd "$(dirname "$0")/.."
CC="${CC:-.tools/Scripts/python.exe -m ziglang cc}"
F="-std=c99 -O1 -g -mcpu=baseline -fsanitize=undefined -fno-omit-frame-pointer"
CORE=$(find src -name '*.c' ! -name runner.c | sort)
mkdir -p build/chk
echo "[check] building"
$CC $F -o build/chk/tests.exe $CORE $(ls tests/*.c | sort) -lm
$CC $F -o build/chk/ad_sim.exe $CORE tools/sim/sim_main.c -lm
$CC $F -DGFX_HD -o build/chk/ad_headless_hd.exe $CORE platform/headless/main_headless.c platform/headless/png.c -lm
echo "[check] tests"
build/chk/tests.exe | tail -1
echo "[check] simulator"
for c in 0 1 2 3 4 5; do
    for p in 0 1 2; do
        build/chk/ad_sim.exe 4 "$c" "$p" > /dev/null
    done
done
echo "[check] headless: every page, a door, the trial"
build/chk/ad_headless_hd.exe --new --class 2 --fast 2 --doors \
    --script "-:30 O:2 -:30 T:2 -:6 T:2 -:6 T:2 -:6 T:2 -:6 T:2 -:6 T:2 -:6 A:2 -:10 O:2 -:600 B:2 -:30" > /dev/null
echo "[check] clean"
