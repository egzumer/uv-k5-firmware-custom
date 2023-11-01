
@echo off




:: Compile directly in windows without the need of a linux virtual machine:
:: 
:: 1. Download and install "gcc-arm-none-eabi-10.3-2021.10-win32.exe" from https://developer.arm.com/downloads/-/gnu-rm
:: 2. Download and install "gnu_make-3.81.exe" from https://gnuwin32.sourceforge.net/packages/make.htm
::
:: 3. You may (or may not) need to manualy add a path to you OS environment PATH, ie ..
::    C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin
::
:: 4. You may (or may not) need to reboot windows after installing the above
:: 
:: You can then run this bat from the directory you saved the firmware source code too.



:: You may need to edit/change these three paths to suit your setup
::
:: Temporarily add the compiler and make program directories to the system PATH ..
::
@set PATH="C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin";%PATH%
@set PATH="C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\arm-none-eabi\bin";%PATH%
@set PATH="C:\Program Files (x86)\GnuWin32\bin\";%PATH%

:: Do the compile
::
make clean
make


:: If you have python installed, you can create a 'packed' .bin from the compiled firmware.bin file.
:: The Quansheng windows upload-to-radio program requires a 'packed' .bin file.
::
:: if you don't have python installed, then you can still upload the standard unpacked firmware.bin
:: file another way ...
::
:: I wrote a GUI version of k5prog to do this easily in windows ..
::    https://github.com/OneOfEleven/k5prog-win



:: One of these two lines simply install the required python module if you want to create the packed
:: firmware bin file, either only needs running once, ever.
::
::python  -m pip install --upgrade pip crcmod
::python3 -m pip install --upgrade pip crcmod



:: show the compiled .bin file size
::
::arm-none-eabi-size firmware




pause
@echo on
