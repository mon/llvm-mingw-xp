#!/usr/bin/env bash

set -e

cd llvm-project
git diff > ../patches/llvm-project.patch
cd ..
