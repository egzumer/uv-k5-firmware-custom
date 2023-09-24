# Open reimplementation of the Quan Sheng UV-K5 v2.1.27 firmware

This repository is a cloned and customized version of DualTachyon's open firmware found here ..

https://github.com/DualTachyon/uv-k5-firmware

A cool achievement

# User customization

You can customize the firmware by enabling/disabling various compile options.
You'll find the options at the top of "Makefile" ('0' = disable, '1' = enable) ..

```
ENABLE_SWD                    := 0       only needed if using CPU's SWD port (debugging/programming)
ENABLE_OVERLAY                := 1       cpu FLASH stuff
ENABLE_UART                   := 1       without this you can't configure radio via PC
ENABLE_AIRCOPY                := 0       easier to just enter frequency
ENABLE_FMRADIO                := 0       WBFM VHF band 2 RX
ENABLE_NOAA                   := 0       Everything NOAA
ENABLE_VOICE                  := 0       want to hear voices ?
ENABLE_ALARM                  := 0       TX alarms
ENABLE_BIG_FREQ               := 0       big font frequencies
ENABLE_SMALL_BOLD             := 1       bold channel name/no. (name + freq channel display mode)
ENABLE_KEEP_MEM_NAME          := 1       maintain channel name when (re)saving memory channel
ENABLE_WIDE_RX                := 1       full 18MHz to 1300MHz RX (though frontend not tuned over full range)
ENABLE_TX_WHEN_AM             := 0       allow TX (always FM) when RX is set to AM
ENABLE_CTCSS_TAIL_PHASE_SHIFT := 1       standard CTCSS tail phase shift rather than QS's own 55Hz tone method
ENABLE_MAIN_KEY_HOLD          := 1       initial F-key press not needed, instead hold down keys 0-9 to access the functions
ENABLE_BOOT_BEEPS             := 0       give user audio feedback on volume knob position at boot-up
ENABLE_COMPANDER              := 1       compander option (per channel)
ENABLE_SHOW_CHARGE_LEVEL      := 1       show the charge level when the radio is on charge
ENABLE_REVERSE_BAT_SYMBOL     := 1       mirror the battery symbol on the status bar (+ pole on the right)
ENABLE_AM_FIX                 := 1       dynamically adjust the front end gains when in AM mode to helo prevent AM demodulator saturation - ignore the on-screen RSSI (for now)
ENABLE_AM_FIX_SHOW_DATA       := 1       show debug data for the AM fix
ENABLE_SQUELCH1_LOWER         := 1       adjusts squelch setting '1' to be more sensitive - I plan to let user adjust it in the menu
ENABLE_AUDIO_BAR              := 0       experimental, display an audo bar level when TX'ing
#ENABLE_SINGLE_VFO_CHAN       := 1       not yet implemented - single VFO on display when possible
#ENABLE_BAND_SCOPE            := 1       not yet implemented - spectrum/pan-adapter
```

# Some changes made from the Quansheng firmware

* Various Quansheng firmware bugs fixed
* Added new bugs
* Mic menu includes max gain possible
* AM RX everywhere (left the TX as is)
* An attempt to improve the AM RX audio (demodulator getting saturated/overloaded)
* keypad-5/NOAA button now toggles scanlist-1 on/off for current channel when held down - IF NOAA not used
* Better backlight times (inc always on)
* Live DTMF decoder option, though the decoder needs some coeff tuning changes to decode other radios it seems
* Various menu re-wordings (trying to reduce 'WTH does that mean ?')
* ..

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
3. You may (or not) need to manualy add gcc path to you OS environment PATH, ie C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin
4. You may (or not) need to reboot your PC after installing the above
```

Then you can run 'win_make.bat' from the directory you saved this source code too.
You may need to edit the bat file (path to make.exe) depending on where you installed the above two packages too.

I've left some notes in the win_make.bat file to maybe help with stuff.

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
* @d1ced95
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

# Example changes/updates

<p float="left">
  <img src="/image1.png" width="300" />
  <img src="/image2.png" width="300" />
  <img src="/image3.png" width="300" />
</p>
