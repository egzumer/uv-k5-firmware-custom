#!/usr/bin/python
# -*- coding: utf-8 -*-

import logging

from datetime import datetime
from enum import Enum
from glob import glob, iglob
from pathlib import Path

from lxml.etree import XMLSyntaxError
from zipfile import ZipFile

from matrix_runner import main, matrix_axis, matrix_action, matrix_command, matrix_filter, \
    ConsoleReport, CropReport, TransformReport, JUnitReport


@matrix_axis("device", "d", "Device(s) to be considered.")
class DeviceAxis(Enum):
    CM0 = ('Cortex-M0', 'CM0')
    CM0plus = ('Cortex-M0plus', 'CM0plus')
    CM3 = ('Cortex-M3', 'CM3')
    CM4 = ('Cortex-M4', 'CM4')
    CM4FP = ('Cortex-M4FP', 'CM4FP')
    CM7 = ('Cortex-M7', 'CM7')
    CM7SP = ('Cortex-M7SP', 'CM7SP')
    CM7DP = ('Cortex-M7DP', 'CM7DP')
    CM23 = ('Cortex-M23', 'CM23')
    CM23S = ('Cortex-M23S', 'CM23S')
    CM23NS = ('Cortex-M23NS', 'CM23NS')
    CM33 = ('Cortex-M33', 'CM33')
    CM33S = ('Cortex-M33S', 'CM33S')
    CM33NS = ('Cortex-M33NS', 'CM33NS')
    CM35P = ('Cortex-M35P', 'CM35P')
    CM35PS = ('Cortex-M35PS', 'CM35PS')
    CM35PNS = ('Cortex-M35PNS', 'CM35PNS')
    CM55S = ('Cortex-M55S', 'CM55S')
    CM55NS = ('Cortex-M55NS', 'CM55NS')
    CM85S = ('Cortex-M85S', 'CM85S')
    CM85NS = ('Cortex-M85NS', 'CM85NS')
    CA5 = ('Cortex-A5', 'CA5')
    CA7 = ('Cortex-A7', 'CA7')
    CA9 = ('Cortex-A9', 'CA9')
#    CA5NEON = ('Cortex-A5neon', 'CA5neon')
#    CA7NEON = ('Cortex-A7neon', 'CA7neon')
#    CA9NEON = ('Cortex-A9neon', 'CA9neon')

    def has_bl(self):
        return self in [
            DeviceAxis.CM23NS,
            DeviceAxis.CM33NS,
            DeviceAxis.CM35PNS,
            DeviceAxis.CM55NS,
            DeviceAxis.CM85NS
        ]

    @property
    def bl_device(self):
        bld = {
            DeviceAxis.CM23NS: 'CM23S',
            DeviceAxis.CM33NS: 'CM33S',
            DeviceAxis.CM35PNS: 'CM35PS',
            DeviceAxis.CM55NS: 'CM55S',
            DeviceAxis.CM85NS: 'CM85S'
        }
        return bld[self]


@matrix_axis("compiler", "c", "Compiler(s) to be considered.")
class CompilerAxis(Enum):
    AC6 = ('AC6')
    AC6LTM = ('AC6LTM')
    GCC = ('GCC')
    IAR = ('IAR')

    @property
    def image_ext(self):
        ext = {
            CompilerAxis.AC6: 'axf',
            CompilerAxis.AC6LTM: 'axf',
            CompilerAxis.GCC: 'elf',
            CompilerAxis.IAR: 'elf'
        }
        return ext[self]


@matrix_axis("optimize", "o", "Optimization level(s) to be considered.")
class OptimizationAxis(Enum):
    LOW = ('low', 'O1')
    MID = ('mid', 'O2')
    HIGH = ('high', 'Ofast')
    SIZE = ('size', 'Os')
    TINY = ('tiny', 'Oz')


MODEL_EXECUTABLE = {
    DeviceAxis.CM0: ("VHT_MPS2_Cortex-M0", []),
    DeviceAxis.CM0plus: ("VHT_MPS2_Cortex-M0plus", []),
    DeviceAxis.CM3: ("VHT_MPS2_Cortex-M3", []),
    DeviceAxis.CM4: ("VHT_MPS2_Cortex-M4", []),
    DeviceAxis.CM4FP: ("VHT_MPS2_Cortex-M4", []),
    DeviceAxis.CM7: ("VHT_MPS2_Cortex-M7", []),
    DeviceAxis.CM7DP: ("VHT_MPS2_Cortex-M7", []),
    DeviceAxis.CM7SP: ("VHT_MPS2_Cortex-M7", []),
    DeviceAxis.CM23: ("VHT_MPS2_Cortex-M23", []),
    DeviceAxis.CM23S: ("VHT_MPS2_Cortex-M23", []),
    DeviceAxis.CM23NS: ("VHT_MPS2_Cortex-M23", []),
    DeviceAxis.CM33: ("VHT_MPS2_Cortex-M33", []),
    DeviceAxis.CM33S: ("VHT_MPS2_Cortex-M33", []),
    DeviceAxis.CM33NS: ("VHT_MPS2_Cortex-M33", []),
    DeviceAxis.CM35P: ("VHT_MPS2_Cortex-M35P", []),
    DeviceAxis.CM35PS: ("VHT_MPS2_Cortex-M35P", []),
    DeviceAxis.CM35PNS: ("VHT_MPS2_Cortex-M35P", []),
    DeviceAxis.CM55S: ("VHT_MPS2_Cortex-M55", []),
    DeviceAxis.CM55NS: ("VHT_MPS2_Cortex-M55", []),
    DeviceAxis.CM85S: ("VHT_MPS2_Cortex-M85", []),
    DeviceAxis.CM85NS: ("VHT_MPS2_Cortex-M85", []),
    DeviceAxis.CA5: ("FVP_VE_Cortex-A5x1", []),
    DeviceAxis.CA7: ("FVP_VE_Cortex-A7x1", []),
    DeviceAxis.CA9: ("FVP_VE_Cortex-A9x1", []),
#    DeviceAxis.CA5NEON: ("FVP_VE_Cortex-A5x1", []),
#    DeviceAxis.CA7NEON: ("FVP_VE_Cortex-A7x1", []),
#    DeviceAxis.CA9NEON: ("FVP_VE_Cortex-A9x1", [])
}

def config_suffix(config, timestamp=True):
    suffix = f"{config.compiler[0]}-{config.optimize[0]}-{config.device[1]}"
    if timestamp:
        suffix += f"-{datetime.now().strftime('%Y%m%d%H%M%S')}"
    return suffix


def image_name(config):
    return f"Validation"


def project_name(config):
    return f"Validation.{config.compiler}_{config.optimize}+{config.device[1]}"


def bl_image_name(config):
    return f"Bootloader"


def bl_project_name(config):
    return f"Bootloader.{config.compiler}_{config.optimize}+{config.device.bl_device}"


def output_dir(config):
    return "outdir"


def bl_output_dir(config):
    return "outdir"


def model_config(config):
    return f"../Layer/Target/{config.device[1]}/model_config.txt"


@matrix_action
def clean(config):
    """Build the selected configurations using CMSIS-Build."""
    yield cbuild_clean(f"{project_name(config)}/{project_name(config)}.cprj")


@matrix_action
def build(config, results):
    """Build the selected configurations using CMSIS-Build."""

    if config.device.has_bl():
        logging.info("Compiling Bootloader...")
        yield csolution(f"{bl_project_name(config)}")
        yield cbuild(f"{bl_project_name(config)}/{bl_project_name(config)}.cprj")

    logging.info("Compiling Tests...")

    if config.compiler == CompilerAxis.GCC and  config.device.match("CA*"):
        ldfile = Path(f"{project_name(config)}/RTE/Device/ARM{config.device[1]}/ARM{config.device[1]}.ld")
        infile = ldfile.replace(ldfile.with_suffix('.ld.in'))
        yield preprocess(infile, ldfile)

    yield csolution(f"{project_name(config)}")
    yield cbuild(f"{project_name(config)}/{project_name(config)}.cprj")

    if not all(r.success for r in results):
        return

    file = f"Core_Validation-{config_suffix(config)}.zip"
    logging.info(f"Archiving build output to {file}...")
    with ZipFile(file, "w") as archive:
        for content in iglob(f"{project_name(config)}/**/*", recursive=True):
            if Path(content).is_file():
                archive.write(content)


@matrix_action
def extract(config):
    """Extract the latest build archive."""
    archives = sorted(glob(f"RTOS2_Validation-{config_suffix(config, timestamp=False)}-*.zip"), reverse=True)
    yield unzip(archives[0])


@matrix_action
def run(config, results):
    """Run the selected configurations."""
    logging.info("Running Core Validation on Arm model ...")
    yield model_exec(config)

    try:
        results[0].test_report.write(f"Core_Validation-{config_suffix(config)}.junit")
    except RuntimeError as e:
        if isinstance(e.__cause__, XMLSyntaxError):
            logging.error("No valid test report found in model output!")
        else:
            logging.exception(e)


@matrix_command()
def cbuild_clean(project):
    return ["cbuild", "-c", project]


@matrix_command()
def unzip(archive):
    return ["bash", "-c", f"unzip {archive}"]


@matrix_command()
def preprocess(infile, outfile):
    return ["arm-none-eabi-gcc", "-xc", "-E", infile, "-P", "-o", outfile]

@matrix_command()
def csolution(project):
    return ["csolution", "convert", "-s", "Validation.csolution.yml", "-c", project]

@matrix_command()
def cbuild(project):
    return ["cbuild", project]


@matrix_command(test_report=ConsoleReport() |
                            CropReport('<\?xml version="1.0"\?>', '</report>') |
                            TransformReport('validation.xsl') |
                            JUnitReport(title=lambda title, result: f"{result.command.config.compiler}."
                                                                    f"{result.command.config.optimize}."
                                                                    f"{result.command.config.device}."
                                                                    f"{title}"))
def model_exec(config):
    cmdline = [MODEL_EXECUTABLE[config.device][0], "-q", "--simlimit", 100, "-f", model_config(config)]
    cmdline += MODEL_EXECUTABLE[config.device][1]
    cmdline += ["-a", f"{project_name(config)}/{output_dir(config)}/{image_name(config)}.{config.compiler.image_ext}"]
    if config.device.has_bl():
        cmdline += ["-a", f"{bl_project_name(config)}/{bl_output_dir(config)}/{bl_image_name(config)}.{config.compiler.image_ext}"]
    return cmdline


if __name__ == "__main__":
    main()
