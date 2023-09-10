
:: download "gcc-arm-none-eabi-10.3-2021.10-win32.exe" for windows ..
:: https://developer.arm.com/downloads/-/gnu-rm
::
:: download "gnu_make-3.81.exe" for windows ..
:: https://gnuwin32.sourceforge.net/packages/make.htm

del /S /Q *.o >nul 2>nul
del /S /Q *.d >nul 2>nul
del /S /Q *.bin >nul 2>nul
"C:\Program Files (x86)\GnuWin32\bin\make"
del /S /Q *.o >nul 2>nul
del /S /Q *.d >nul 2>nul

::python -m pip install --upgrade pip crcmod
fw-pack.py firmware.bin 230911 firmware.packed.bin

::arm-none-eabi-size firmware

pause
