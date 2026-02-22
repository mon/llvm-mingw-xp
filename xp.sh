#!/usr/bin/env bash

set -eu

PREFIX=build
mkdir -p "$PREFIX"

export LLVM_VERSION=llvmorg-21.1.8
export MINGW_W64_VERSION=v13.0.0

export TOOLCHAIN_ARCHS="i686 x86_64"
# don't build the UWP platform, useless on XP
export TOOLCHAIN_TARGET_OSES=mingw32

# It's XP, this might take a few tries to build. Ignore all the insanity if so.
populate_gitignores () {
    ignores=(
        "$PREFIX"
        "mingw-w64/mingw-w64-headers/build"
        "mingw-w64/mingw-w64-tools/gendef/build"
        "mingw-w64/mingw-w64-tools/widl/build"
    )
    for arch in $TOOLCHAIN_ARCHS; do
        ignores+=(
            "llvm-project/compiler-rt/build-$arch"
            "llvm-project/compiler-rt/build-$arch-sanitizers"
            "llvm-project/runtimes/build-$arch"
            "llvm-project/openmp/build-$arch"
            "mingw-w64/mingw-w64-crt/build-$arch"
        )
    done
    for folder in "${ignores[@]}"; do
        gitignore="$folder/.gitignore"
        if [ -d "$folder" ] && [ ! -f "$gitignore" ]; then
            echo '*' > "$gitignore"
        fi
    done
}

populate_gitignores

./build-all.sh $PREFIX \
    --with-default-win32-winnt=0x0501 \
    --with-default-msvcrt=msvcrt \
    --disable-lldb \
    --disable-dylib \
    --disable-cfguard

populate_gitignores
