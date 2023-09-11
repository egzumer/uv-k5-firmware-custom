# Open reimplementation of the Quan Sheng UV K5 v2.1.27 firmware

This repository is a cloned and customized version of DualTachyon's open firmware found here ..

https://github.com/DualTachyon/uv-k5-firmware

Am amazing achievement if you ask me !

# User customization

This version you can customize at compile time by making various changes to the makefile.
You can edit those changes by (currently) editing the MakeFile, look for these lines ..

* CFLAGS  += -DDISABLE_BIG_FREQ     .. show frequencies using smaller font
* CFLAGS  += -DDISABLE_NOAA         .. remove NOAA channels option from the firmware
* CFLAGS  += -DDISABLE_VOICE        .. remove spoken VOICES option from the firmware
* CFLAGS  += -DDISABLE_AIRCOPY      .. remove AIRCOPY option
* CFLAGS  += -DDISABLE_ALARM        .. remove the ALARM transmit option from the firmware
* CFLAGS  += -DKEEP_MEM_NAME        .. keep the memory channels name when re-saving a channel
* CFLAGS  += -DCHAN_NAME_FREQ       .. show the channel frequency (as well as channel number/name)
* #CFLAGS += -DENABLE_TX_WHEN_AM    .. enable TX when radio set to AM (radio does NOT do AM, the adverts are false)
* #CFLAGS += -DSINGLE_VFO_CHAN      .. (not yet implemented) show a single VFO/CHANNEL if dual-watch/cross-band are disabled
* #CFLAGS += -DBAND_SCOPE           .. (not yet implemented) add a band scope/spectrum/panadapter option

To enable the custom option just uncomment the line by removing the starting '#'.

# Other changes made

* Various bugs fixed that the QS firmware had (TX tail, Menu confimation etc)
* Added new bugs
* Battery voltage boot screen now includes percentage
* Slightly less intense menu style
* AM RX now allowed everywhere, although the radio really doesn't do AM, the adverts are a con !
* Finer RSSI bar steps
* Nicer/cleaner big numeric font than original QS big numeric font
*
* "MEM-CH" and "DEL-CH" menus now include channel name
* "STEP" menu, added 1.25kHz option, removed 5kHz option
* "TXP" menu, renamed to "TX-PWR"
* "SAVE" menu, renamed to "B-SAVE"
* "WX" menu, renamed to "CROS-B" ('WX' means weather here in the UK)
* "ABR" menu, renamed to "BAK-LT", extended times, includes always ON option
* "SCR" menu, renamed to "SCRAM"
* "MIC" menu, shows mic gain in dB's, includes max mic gain possible (+15.5dB)
* "VOL" menu, renamed to "BATVOL", added percentage level
* "AM" menu, renamed to "MODE", shows RX modulation mode

Menu renames are to try and reduce 'WTF does that mean ?'

# Compiler

arm-none-eabi GCC version 10.3.1 is recommended, which is the current version on Ubuntu 22.04.03 LTS.
Other versions may generate a flash file that is too big.
You can get an appropriate version from: https://developer.arm.com/downloads/-/gnu-rm

# Building

To build the firmware, you need to fetch the submodules and then run make:
```
git submodule update --init --recursive --depth=1
make
```

To compile directly in windows without the need of a linux virtual machine:

```
* Download and install "gcc-arm-none-eabi-10.3-2021.10-win32.exe" from https://developer.arm.com/downloads/-/gnu-rm
* Download and install "gnu_make-3.81.exe" from https://gnuwin32.sourceforge.net/packages/make.htm
```

You may (or may not) need to reboot your PC after installing the above.

Run 'win_make.bat' from the directory you saved this source code too, you may have to edit the bat file depending on where you installed 'gnu_make' too

# Credits

Many thanks to various people on Telegram for putting up with me during this effort and helping:

* [DualTachyon](https://github.com/DualTachyon)
* [Mikhail](https://github.com/fagci)
* [Andrej](https://github.com/Tunas1337)
* [Manuel](https://github.com/manujedi)
* @wagner
* @Lohtse Shar
* [@Matoz](https://github.com/spm81)
* @Davide
* @Ismo OH2FTG
* [OneOfEleven](https://github.com/OneOfEleven)
* and others I forget

# License

Copyright 2023 Dual Tachyon
https://github.com/DualTachyon

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

