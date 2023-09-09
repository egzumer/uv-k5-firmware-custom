CMSIS-DAP v2 firmware for NXP LPC-Link2 debug probe.

CMSIS-DAP v2 uses USB bulk endpoints for the communication with the host PC and is therefore faster.
Optionally, support for streaming SWO trace is provided via an additional USB endpoint.

Following targets are available:
 - LPC-Link2: stand-alone debug probe
 - LPC-Link2 on-board: on-board debug probe (LPC55S69-EVK, MIMXRT1064-EVK, ...)
