#!/usr/bin/env bash

set -e

cd mingw-w64
git diff > ../patches/mingw-w64.patch
cd ..
