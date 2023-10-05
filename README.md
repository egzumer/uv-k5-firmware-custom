# Main features:
 - OneOfEleven mods, including AM fix
 - fagci spectrum analyzer (**F+5** to turn on)
 - better battery percentage calculation (based on real measurement, battery calibration recommended)
 - battery calibration option available available in the menu (instructions below)

<img src="images/main.jpg" width=300 /><img src="images/spectrum.jpg" width=300 /><img src="images/audiobar.jpg" width=300 /><img src="images/rssibar.jpg" width=300 />

❗❗❗You can now calibrate the battery voltage reading in the radio menu. 

<img src="images/batcal.jpg" width=300 />

To enter the calibration:

1. turn the radio on while holding PTT and the first side button 
2. go to BATCAL option and adjust the voltage with UP/DOWN buttons
3. confirm the setting, calibration will be saved to EEPROM


# Open reimplementation of the Quan Sheng UV-K5 v2.1.27 firmware

This repository is a merge of OneOfEleven custom firmware with fagci spectrum analizer plus my few changes.

https://github.com/OneOfEleven/uv-k5-firmware-custom<br>
https://github.com/fagci/uv-k5-firmware-fagci-mod/tree/refactor

All is a cloned and customized version of DualTachyon's open firmware found here ..

https://github.com/DualTachyon/uv-k5-firmware .. a cool achievement !

Use this firmware at your own risk (entirely). There is absolutely no guarantee that it will work in any way shape or form on your radio(s), it may even brick your radio(s), in which case, you'd need to buy another radio.
Anyway, have fun.

# Radio performance

Please note that the Quansheng uv-k radios are not professional quality transceivers, their
performance is strictly limited, somewhat below that of a decent transceiver. The RX front
end has no track-tuned band pass filtering at all, and so are wide band/wide open to any
and all signals over a wide frequency range.
Using the radio in high intensity RF environments will nearly always destroy your reception,
the receiver simply doesn't have a great dynamic range, which means distorted AM audio with
strong received signals, there is nothing more anyone can do in firmware/software to stop that
happening once the RX gain adjustment I do (AM fix) reaches the hardwares limit.

Saying that, they are nice toys for the price, fun to play with.

# User customization

You can customize the firmware by enabling/disabling various compile options.
You'll find the options at the top of "Makefile" ('0' = disable, '1' = enable) ..

```
ENABLE_CLANG                  := 0       experimental, builds with clang instead of gcc (LTO will be disabled if you enable this)
ENABLE_SWD                    := 0       only needed if using CPU's SWD port (debugging/programming)
ENABLE_OVERLAY                := 0       cpu FLASH stuff, not needed
ENABLE_LTO                    := 0     **experimental, reduces size of compiled firmware but might break EEPROM reads (overlay will be disabled if you enable this)
ENABLE_UART                   := 1       without this you can't configure radio via PC !
ENABLE_AIRCOPY                := 0       easier to just enter frequency with butts
ENABLE_FMRADIO                := 0       WBFM VHF broadcast band receiver
ENABLE_NOAA                   := 0       everything NOAA (only of any use in the USA)
ENABLE_VOICE                  := 0       want to hear voices ?
ENABLE_VOX                    := 0
ENABLE_ALARM                  := 0       TX alarms
ENABLE_1750HZ                 := 0       side key 1750Hz TX tone (older style repeater access)
ENABLE_BIG_FREQ               := 0       big font frequencies (like original QS firmware)
ENABLE_SMALL_BOLD             := 1       bold channel name/no. (when name + freq channel display mode)
ENABLE_KEEP_MEM_NAME          := 1       maintain channel name when (re)saving memory channel
ENABLE_WIDE_RX                := 1       full 18MHz to 1300MHz RX (though front-end/PA not designed for full range)
ENABLE_TX_WHEN_AM             := 0       allow TX (always FM) when RX is set to AM
ENABLE_F_CAL_MENU             := 0       enable/disable the radios hidden frequency calibration menu
ENABLE_CTCSS_TAIL_PHASE_SHIFT := 1       standard CTCSS tail phase shift rather than QS's own 55Hz tone method
ENABLE_BOOT_BEEPS             := 0       gives user audio feedback on volume knob position at boot-up
ENABLE_SHOW_CHARGE_LEVEL      := 0       show the charge level when the radio is on charge
ENABLE_REVERSE_BAT_SYMBOL     := 1       mirror the battery symbol on the status bar (+ pole on the right)
ENABLE_CODE_SCAN_TIMEOUT      := 0       enable/disable 32-sec CTCSS/DCS scan timeout (press exit butt instead of time-out to end scan)
ENABLE_AM_FIX                 := 1       dynamically adjust the front end gains when in AM mode to helo prevent AM demodulator saturation, ignore the on-screen RSSI level (for now)
ENABLE_AM_FIX_SHOW_DATA       := 1       show debug data for the AM fix (still tweaking it)
ENABLE_SQUELCH_MORE_SENSITIVE := 0       make squelch levels a little bit more sensitive - I plan to let user adjust the values themselves
#ENABLE_FASTER_CHANNEL_SCAN   := 0       don't use (for now) .. increases the channel scan speed, but the squelch is also made more twitchy
ENABLE_RSSI_BAR               := 1       enable a dBm/Sn RSSI bar graph level inplace of the little antenna symbols
ENABLE_AUDIO_BAR              := 0       experimental, display an audo bar level when TX'ing
ENABLE_COPY_CHAN_TO_VFO       := 1       copy current channel into the other VFO. Long press Menu key ('M')
#ENABLE_SINGLE_VFO_CHAN       := 1       not yet implemented - single VFO on display when possible
#ENABLE_BAND_SCOPE            := 1       not yet implemented - spectrum/pan-adapter
```

# New/modified function keys

* Long-press 'M' .. Copy selected channel into same VFO, then switch VFO to frequency mode
*
* Long-press '7' .. Toggle selected channel scanlist setting .. if VOX  is disabled in Makefile
*       or
* Long-press '5' .. Toggle selected channel scanlist setting .. if NOAA is disabled in Makefile
*
* Long-press '*' .. Start scanning, then toggles scanlist scan 1, 2 or ALL channel scanning

# Some changes made from the Quansheng firmware

* Various Quansheng firmware bugs fixed
* Added new bugs
* Mic menu includes max gain possible
* AM RX everywhere (left the TX as is)
* An attempt to improve the AM RX audio (demodulator getting saturated/overloaded in Quansheng firmware)
* keypad-5/NOAA button now toggles scanlist-1 on/off for current channel when held down - IF NOAA not used
* Better backlight times (inc always on)
* Live DTMF decoder option, though the decoder needs some coeff tuning changes to decode other radios it seems
* Various menu re-wordings (trying to reduce 'WTH does that mean ?')
* ..

# Compiler

arm-none-eabi GCC version 10.3.1 is recommended, which is the current version on Ubuntu 22.04.03 LTS.
Other versions may generate a flash file that is too big.
You can get an appropriate version from: https://developer.arm.com/downloads/-/gnu-rm

clang may be used but isn't fully supported. Resulting binaries may also be bigger.
You can get it from: https://releases.llvm.org/download.html

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

3. You may need to (I didn't) manualy add gcc path to your OS environment PATH.
   ie add C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin

4. You may need to reboot your PC after installing the above
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
  <img src="/image1.png" width=300 />
  <img src="/image2.png" width=300 />
  <img src="/image3.png" width=300 />
</p>

Video showing the AM fix working ..

<video src="/AM_fix.mp4"></video>

<video src="https://github.com/OneOfEleven/uv-k5-firmware-custom/assets/51590168/2a3a9cdc-97da-4966-bf0d-1ce6ad09779c"></video>
