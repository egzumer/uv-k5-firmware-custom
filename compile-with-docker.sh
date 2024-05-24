#!/bin/env sh
mkdir -p compiled-firmware
docker build -t egzumer -f Dockerfile.egzumer .
docker run --rm --user `id -u`:`id -g` -v `pwd`:/app egzumer \
/bin/bash -c 'make && mv firmware firmware.bin firmware.packed.bin compiled-firmware && chmod -R 644 compiled-firmware/*'
