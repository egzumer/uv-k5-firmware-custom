#!/usr/bin/python3
# -*- coding: utf-8 -*-

from enum import Enum

from matrix_runner import main, matrix_axis, matrix_action, matrix_command


@matrix_axis("device", "d", "Device(s) to be considered.")
class Device(Enum):
    CM0 = ('CM0', 'CM0_LE')
    CM3 = ('CM3', 'CM3_LE')
    CM4F = ('CM4F', 'CM4F_LE')
    V8MB = ('V8MB', 'ARMv8MBL_LE')
    V8MBN = ('V8MBN', 'ARMv8MBL_NS_LE')
    V8MM = ('V8MM', 'ARMv8MML_LE')
    V8MMF = ('V8MMF', 'ARMv8MML_SP_LE')
    V8MMFN = ('V8MMFN', 'ARMv8MML_SP_NS_LE')
    V8MMN = ('V8MMN', 'ARMv8MML_NS_LE')


@matrix_axis("compiler", "c", "Compiler(s) to be considered.")
class CompilerAxis(Enum):
    AC6 = ('AC6', 'ArmCompiler6', 'armclang')
    GCC = ('GCC',)

    @property
    def project(self):
        return {
            CompilerAxis.AC6: "ARM/MDK/RTX_CM.uvprojx",
            CompilerAxis.GCC: "GCC/MDK/RTX_CM.uvprojx"
        }[self]


@matrix_action
def build(config, results):
    """Build the selected configurations."""
    yield uvision(config)


@matrix_command()
def uvision(config):
    return ['uvision.com',
            '-r', config.compiler.project,
            '-t', config.device[1],
            '-j0']


if __name__ == "__main__":
    main()
