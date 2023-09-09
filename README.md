# Open reimplementation of the Quan Sheng UV K5 v2.1.27 firmware

This repository is a cloned and customized version of DualTachyon's firmware found here ..

https://github.com/DualTachyon/uv-k5-firmware

# User customization

This version you can customize at compile time by making various changes to the makefile.
You can edit those changes by (currently) editing the MakeFile, look for these lines ..

*   # compilation options
*   CFLAGS  += -DDISABLE_NOAA         .. remove NOAA channels option from the firmware
*   CFLAGS  += -DDISABLE_VOICE        .. remove spoken VOICES option from the firmware
*   CFLAGS  += -DDISABLE_AIRCOPY      .. remove AIRCOPY option
*   CFLAGS  += -DKEEP_MEM_NAME        .. don't wipe out the memory channel's name when saving a memory channel
*   #CFLAGS += -DDISABLE_ALARM        .. not yet implemented
*   #CFLAGS += -DBAND_SCOPE           .. not yet implemented

To enable the custom option just uncomment the line by removing the starting '#'.

# Other changes made

* "ABR" menu option now shows extended backlight times
* "MIC" menu option shows actual mic gain in dB's, which now includes the max mic gain setting possible (+15.5dB)
   
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

You can also easily compile this in windows (will add an example shortly) meaning you no longer have to install a linux VM on Windows.

# Credits

Many thanks to various people on Telegram for putting up with me during this effort and helping:

* [Mikhail](https://github.com/fagci/)
* [Andrej](https://github.com/Tunas1337)
* @wagner
* @Lohtse Shar
* [@Matoz](https://github.com/spm81)
* @Davide
* @Ismo OH2FTG
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

