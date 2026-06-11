#!/usr/bin/env bash

set -eu

PREFIX=build
PREFIX_ABS="$(cd "$PREFIX" && pwd)"

export TOOLCHAIN_ARCHS="i686 x86_64"
# don't build the UWP platform, useless on XP
export TOOLCHAIN_TARGET_OSES=mingw32

# With --disable-dylib the C/C++ runtimes are static, but libunwind.dll and
# libc++.dll still import libwinpthread-1.dll, which lives in the toolchain's
# per-arch bin directory rather than alongside the test exes. Point wine at
# those directories so it can resolve the imports at load time.
WINEPATH_ENTRIES=()
for arch in $TOOLCHAIN_ARCHS; do
    WINEPATH_ENTRIES+=("$(winepath -w "$PREFIX_ABS/$arch-w64-mingw32/bin")")
done
export WINEPATH
WINEPATH="$(IFS=';'; echo "${WINEPATH_ENTRIES[*]}")"

./run-tests.sh $PREFIX

# XP-specific tests that exercise the patched codepaths. Kept separate from
# the upstream test/Makefile so llvm-mingw updates don't collide with them.
export PATH="$PREFIX_ABS/bin:$PATH"
for arch in $TOOLCHAIN_ARCHS; do
    case $arch in
        i686|x86_64) RUN=wine ;;
        *)           RUN= ;;
    esac
    test_dir="test/xp-$arch"
    mkdir -p "$test_dir"
    make -C "$test_dir" -f ../xp/Makefile ARCH="$arch" RUN="$RUN" clean
    make -C "$test_dir" -f ../xp/Makefile ARCH="$arch" RUN="$RUN" test
done
echo All XP tests succeeded
