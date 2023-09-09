CMSIS-DAP v2 firmware for NXP MCU-LINK debug probe.

CMSIS-DAP v2 uses USB bulk endpoints for the communication with the host PC and is therefore faster.
Optionally, support for streaming SWO trace is provided via an additional USB endpoint.

Instructions for programing CMSIS_DAP firmware on MCU-LINK:
- download and install MCU-LINK_installer from https://www.nxp.com/design/microcontrollers-developer-resources/mcu-link-debug-probe:MCU-LINK
- disconnect MCU-LINK from USB (J1), set "firmware update" jumper (J3), connect MCU-LINK to USB (J1)
- open a Command Window
- navigate to the MCU-LINK_installer installation (default C:\nxp\MCU-LINK_installer\) and go to the scripts sub-directory
- copy pre-built firmware hex file ..\CMSIS\DAP\Firmware\Examples\MCU-LINK\Objects\CMSIS_DAP.hex to scripts directory
- run the command: programm_CMSIS.cmd CMSIS_DAP.hex
- follow the instructions in command window
