# CMSIS-Core Validation

This folder contains a test suite that validates CMSIS-Core implementations. It uses [**Fixed Virtual Platforms**](https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms) to run tests to verify correct operation of the CMSIS-Core functionality on various Arm Cortex based processors.

## Folder structure

```txt
    📂 CoreValidation
    ┣ 📂 Include        Include files for test cases etc.
    ┣ 📂 Layer          Layers for creating the projects.
    ┣ 📂 Project        Solution and project files to build tests for various configurations.
    ┗ 📂 Source         Test case source code.
```

## Test matrix

Currently, the following build configurations are provided:

1. Compiler
   - Arm Compiler 6 (AC6)
   - GNU Compiler (GCC)
   - IAR Compiler (IAR)
2. Devices
   - Cortex-M0
   - Cortex-M0+
   - Cortex-M3
   - Cortex-M4
     - w/o FPU
     - with FPU
   - Cortex-M7
     - w/o FPU
     - with SP FPU
     - with DP FPU
   - Cortex-M23
     - w/o security extensions (TrustZone)
     - in secure mode
     - in non-secure mode
   - Cortex-M33 (with FPU and DSP extensions)
     - w/o security extensions (TrustZone)
     - in secure mode
     - in non-secure mode
   - Cortex-M35P (with FPU and DSP extensions)
     - w/o security extensions (TrustZone)
     - in secure mode
     - in non-secure mode
   - Cortex-M55 (with FPU and DSP extensions)
     - in secure mode
     - in non-secure mode
   - Cortex-M85 (with FPU and DSP extensions)
     - in secure mode
     - in non-secure mode
   - Cortex-A5
     - w/o NEON extensions
   - Cortex-A7
     - w/o NEON extensions
   - Cortex-A9
     - w/o NEON extensions
3. Optimization Levels
   - Low
     - AC6: `-O1`
     - GCC: `-O1`
     - IAR: `-Ol`
   - Mid
     - AC6: `-O2`
     - GCC: `-O2`
     - IAR: `-Om`
   - High
     - AC6: `-O3`
     - GCC: `-O3`
     - IAR: `-Oh`
   - Size
     - AC6: `-Os`
     - GCC: `-Os`
     - IAR: `-Ohz`
   - Tiny
     - AC6: `-Oz`
     - GCC: `-Ofast`
     - IAR: `-Ohs`

## Prerequisites

The following tools are required to build and run the CoreValidation tests:

- [CMSIS-Toolbox](https://github.com/Open-CMSIS-Pack/cmsis-toolbox/releases) 1.3.0 or higher
- CMake
- Ninja build
- Arm Compiler 6
- GNU Compiler
- IAR Compiler
- Python 3.8 or higher
- Arm Virtual Hardware Models

The executables need to be present on the `PATH`.

Install the Python packages required by `build.py`:

```bash
CMSIS_5/CMSIS/CoreValidation/Project $ pip install -r requirements.txt
```

## Build and run

To build and run the CoreValidation tests for one or more configurations use the following command line.
Select the `<compiler>`, `<device>`, and `optimize` level to `build` and `run` for.

```bash
CMSIS_5/CMSIS/CoreValidation/Project $ ./build.py -c <compiler> -d <device> -o <optimize> [build] [run]
```

For example, build and run the tests using GCC for Cortex-M3 with low optimization, execute:

```bash
CMSIS_5/CMSIS/CoreValidation/Project $ ./build.py -c GCC -d CM3 -o low build run
[GCC][Cortex-M3][low](build:csolution) csolution convert -s Validation.csolution.yml -c Validation.GCC_low+CM3
[GCC][Cortex-M3][low](build:csolution) csolution succeeded with exit code 0
[GCC][Cortex-M3][low](build:cbuild) cbuild Validation.GCC_low+CM3/Validation.GCC_low+CM3.cprj
[GCC][Cortex-M3][low](build:cbuild) cbuild succeeded with exit code 0
[GCC][Cortex-M3][low](run:model_exec) VHT_MPS2_Cortex-M3 -q --simlimit 100 -f ../Layer/Target/CM3/model_config.txt -a Validation.GCC_low+CM3/Validation.GCC_low+CM3_outdir/Validation.GCC_low+CM3.elf
[GCC][Cortex-M3][low](run:model_exec) VHT_MPS2_Cortex-M3 succeeded with exit code 0

Matrix Summary
==============

compiler    device     optimize    build    clean    extract    run
----------  ---------  ----------  -------  -------  ---------  -----
GCC         Cortex-M3  low         success  (skip)   (skip)     35/35
```

The full test report is written to `Core_Validation-GCC-low-CM3-<timestamp>.junit` file.

## License

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
