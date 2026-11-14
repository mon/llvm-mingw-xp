#!/usr/bin/env bash

set -eu

# Hey! This assumes you're me (mon). Committed as a reference, not because it's
# going to work to push into my dockerhub account.

docker login docker.io

docker build -f Dockerfile.xp \
    -t montymintypie/llvm-mingw-xp:23 \
    .

docker push montymintypie/llvm-mingw-xp:23
