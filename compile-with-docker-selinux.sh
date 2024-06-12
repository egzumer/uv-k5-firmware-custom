#!/bin/sh
docker build -t uvk5 .
docker run --rm -v ${PWD}/compiled-firmware:/app/compiled-firmware:z uvk5 /bin/bash -c "cd /app && make && cp firmware* compiled-firmware/"
# Make it easier for the programmer to find the firmware file by adding it to the recently used list.
# See https://github.com/xenomachina/recently_used/tree/master
# recently_used.py -A code compiled-firmware/firmware.packed.bin
