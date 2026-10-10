#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Melodee (Double Rainbow) in the browser: the firmware's sources as the host tests build them (web/emu/melodee_web.c),
# compiled to WebAssembly with Emscripten, plus the page. Ported from Felucca's; after X0X's web/emu/build.sh
# (charlesvestal/fm1-x0x, GPL-3.0).
#   web/emu/build.sh   ->  build/emu/{index.html, worklet.js, melodee.wasm, fonts/}
# Run after one ./build.sh, or build.py's generate() alone (build/gen: the tables, the fonts, the factory banks).
# -ffp-contract=off: no fused multiply-adds, as the host build. EMU_OPT replaces -O2, EMU_CFLAGS adds flags
# (EMU_CFLAGS=-msimd128: let the compiler vectorise; every browser with AudioWorklet has wasm SIMD).
# The page shows the version (MELODEE_VERSION) marked "(preview)" before its release; EMU_PREVIEW=0 drops the mark.
set -e
cd "$(dirname "$0")/../.."
[ -f build/gen/melodee_tables.h ] || { echo "emu: run ./build.sh once first (build/gen)"; exit 1; }
command -v emcc >/dev/null 2>&1 || { echo "emu: no emcc (Emscripten)"; exit 1; }
VERSION=$(sed -n 's/^#define MELODEE_VERSION "\([^"]*\)".*/\1/p' firmware/src/melodee.c)
mkdir -p build/emu/fonts
# shellcheck disable=SC2086
emcc ${EMU_OPT:--O2} -ffp-contract=off -std=gnu99 -w ${EMU_CFLAGS:-} \
    "-DMELODEE_VERSION=\"$VERSION\"" -Ibuild/gen -Ifirmware/src \
    --no-entry -sSTANDALONE_WASM -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=33554432 -sFILESYSTEM=0 \
    -o build/emu/melodee.wasm web/emu/melodee_web.c
[ "${EMU_PREVIEW:-1}" = 0 ] && PREVIEW='' || PREVIEW=' (preview)'
sed -e "s/@VERSION@/$VERSION/g" -e "s/@PREVIEW@/$PREVIEW/g" web/emu/index.html > build/emu/index.html
cp web/emu/worklet.js build/emu/
cp web/emu/fonts/DotGothic16-subset.woff web/emu/fonts/OFL.txt build/emu/fonts/
echo "emu: build/emu ($(wc -c < build/emu/melodee.wasm | tr -d ' ') B wasm, $VERSION)"
