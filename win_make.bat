
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

:: If you have python installed, you can create a 'packed' .bin from the compiled firmware.bin file.
:: The Quangsheng windows upload-to-radio program requires a 'packed' .bin file.
::
:: if you don't have python installed, then comment out the python line below, in which case you'll need
:: to upload the standard firmware.bin file another way.
:: I wrote a windows version of k5prog to do this easily in windows ..
::    https://github.com/OneOfEleven/k5prog-win
::
::python -m pip install --upgrade pip crcmod
fw-pack.py firmware.bin 230919 firmware.packed.bin

::arm-none-eabi-size firmware

pause
