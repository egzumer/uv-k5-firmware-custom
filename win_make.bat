
:: Cmpile directly in windows without the need of a linux virtual machine:
:: 
:: 1. Download and install "gcc-arm-none-eabi-10.3-2021.10-win32.exe" from https://developer.arm.com/downloads/-/gnu-rm
:: 2. Download and install "gnu_make-3.81.exe" from https://gnuwin32.sourceforge.net/packages/make.htm
:: 3. You may (or not) need to manualy add gcc path to you OS environment PATH, ie ..
::    C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin
:: 4. You may (or not) need to reboot your PC after installing the above
:: 
:: Then you can run this bat from the directory you saved this source code too.
::
:: You will need to edit the line below that runs the make.exe file to point to it's actual location if
:: it's different from mine.

:: delete any left over files from any previous compile
del /S /Q *.o >nul 2>nul
del /S /Q *.d >nul 2>nul
del /Q *.bin >nul 2>nul

:: do the compile !
"C:\Program Files (x86)\GnuWin32\bin\make"

:: delete the unused created when compiling files
del /S /Q *.o >nul 2>nul
del /S /Q *.d >nul 2>nul

:: If you have python installed, you can create a 'packed' .bin from the compiled firmware.bin file.
:: The Quansheng windows upload-to-radio program requires a 'packed' .bin file.
::
:: if you don't have python installed, then you can still upload the standard unpacked firmware.bin
:: file another way ...
::
:: I wrote a GUI version of k5prog to do this easily in windows ..
::    https://github.com/OneOfEleven/k5prog-win

:: these just install the required initial python module if you want to create the packed firmware bin file, either
:: only needs running once, ever.
::
::python  -m pip install --upgrade pip crcmod
::python3 -m pip install --upgrade pip crcmod

:: show the compiled .bin file size
::arm-none-eabi-size firmware

pause
