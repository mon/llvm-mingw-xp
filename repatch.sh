#!/usr/bin/env bash

set -e

cd llvm-project
git diff > ../patches/llvm-project.patch
cd ..

cd mingw-w64
git diff > ../patches/mingw-w64.patch
cd ..
