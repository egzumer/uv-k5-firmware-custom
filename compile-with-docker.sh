#!/bin/sh
docker build -t uvk5 .
docker run -v $(PWD)/compiled-firmware:/app/compiled-firmware uvk5 /bin/bash -c "cd /app && make && cp firmware* compiled-firmware/"