@echo off
docker build -t uvk5 .
docker run -v %CD%\compiled-firmware:/app/compiled-firmware uvk5 /bin/bash -c "cd /app && make clean && make && cp firmware* compiled-firmware/"
pause