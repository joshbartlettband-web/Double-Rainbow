#!/bin/sh
# The browser build without Emscripten, for recording the tour where emsdk cannot be installed: plain clang and
# wasi-libc (Debian/Ubuntu: apt install clang lld wasi-libc libclang-rt-18-dev-wasm32). It makes the same
# build/emu/melodee.wasm as web/emu/build.sh (no imports; web/emu/dr_test.mjs passes on it). Run after the
# generate step (build/gen). The site is still built with Emscripten (.github/workflows/double-rainbow.yml).
set -e
cd "$(dirname "$0")/../.."
[ -f build/gen/melodee_tables.h ] || { echo "run the generate step first (build/gen)"; exit 1; }
VERSION=$(sed -n 's/^#define MELODEE_VERSION "\([^"]*\)".*/\1/p' firmware/src/melodee.c)
mkdir -p build/emu build/wasishim/sys
cat > build/wasishim/sys/wait.h <<'H'
#pragma once
/* the browser build never forks (the host test helpers); stubs, as Emscripten has */
static inline int pipe(int *f) { (void)f; return -1; }
static inline int fork(void) { return -1; }
static inline int waitpid(int p, int *s, int o) { (void)p; (void)s; (void)o; return -1; }
#define WIFEXITED(s) 0
#define WEXITSTATUS(s) 0
H
clang --target=wasm32-wasi -isystem build/wasishim -isystem /usr/include/wasm32-wasi -L/usr/lib/wasm32-wasi \
    -O2 -ffp-contract=off -std=gnu99 -w "-DMELODEE_VERSION=\"$VERSION\"" -Ibuild/gen -Ifirmware/src \
    -mexec-model=reactor -Wl,--export-dynamic -Wl,-z,stack-size=1048576 -Wl,--initial-memory=33554432 \
    -o build/emu/melodee.wasm web/emu/melodee_web.c -lm
echo "clang: build/emu/melodee.wasm ($(wc -c < build/emu/melodee.wasm | tr -d ' ') B, $VERSION)"
