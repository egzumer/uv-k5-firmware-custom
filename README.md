# Open reimplementation of the Quan Sheng UV K5 v2.1.27 firmware

This repository is a cloned and customized version of DualTachyon's open firmware found here ..

https://github.com/DualTachyon/uv-k5-firmware

Am amazing achievement if you ask me !

# User customization

This version you can customize at compile time by making various changes to the makefile.
You can edit those changes by (currently) editing the MakeFile, look for these lines at the top of the file ..

* ENABLE_AIRCOPY                := 0
* ENABLE_FMRADIO                := 0       FM band 2 radio
* ENABLE_OVERLAY                := 1
* ENABLE_UART                   := 1       without this you can't configure the radio with your PC
* ENABLE_NOAA                   := 0       NOAA channels
* ENABLE_VOICE                  := 0       strange voices
* ENABLE_ALARM                  := 0       TX alarms
* ENABLE_BIG_FREQ               := 0       big font for the frequencies
* ENABLE_KEEP_MEM_NAME          := 1       maintain the channel name when (re)saving a memory channel
* ENABLE_CHAN_NAME_FREQ         := 1       show the channel frequency below the channel name/number
* ENABLE_WIDE_RX                := 1       enable the RX in the full 18MHz to 1300MHz (though frontend is not tuned for full range)
* ENABLE_TX_WHEN_AM             := 0       allow TX when RX set to AM
* ENABLE_TAIL_CTCSS_PHASE_SHIFT := 1       use CTCSS tail phase shift rather than QS's 55Hz tone method 
* #ENABLE_SINGLE_VFO_CHAN       := 1       not yet implemented
* #ENABLE_BAND_SCOPE            := 1       not yet implemented

To enable the custom option, set the above option to '1'

# Other changes made

* Various bugs fixed that the QS firmware had (TX tail, Menu confimation etc)
* Added new bugs
* Battery voltage boot screen now includes percentage
* Slightly less intense menu style
* AM RX now allowed everywhere, although the radio really doesn't do AM, the adverts are a con !
* Finer RSSI bar steps
* Nicer/cleaner big numeric font than original QS big numeric font
* Various menu re-wordings - to try and reduce 'WTH does that mean ?'

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
1. Download and install "gcc-arm-none-eabi-10.3-2021.10-win32.exe" from https://developer.arm.com/downloads/-/gnu-rm
2. Download and install "gnu_make-3.81.exe" from https://gnuwin32.sourceforge.net/packages/make.htm
3. You may (or may not) need to reboot your PC after installing the above
```

Then you can run 'win_make.bat' from the directory you saved this source code too.
You may need to edit the bat file (path to make.exe) depending on where you installed 'gnu_make' too.

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

