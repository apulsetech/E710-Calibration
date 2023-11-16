#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2020 - 2021 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################

from ctypes import *
from enum import IntEnum

# There is a bug in ctypes based on machine
# in which this is run where a packed bit field
# of c_bool will not be set properly. Instead,
# we use c_uint8 with a bit limit of 1

# IPJ_autogen | gen_c2python_regs_boot {
# Structs which break down the fields and sizing within boot regs
class RamImageReturnValueFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('data', c_uint32, 32),
    ]

class FrefFreqFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('fref_freq_khz', c_uint32, 32),
    ]

class RemainReasonFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('remain_reason', c_uint8, 8), # enum RemainReason
    ]

class ImageValidityFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('image_valid_marker', c_uint8, 1),
        ('image_non_valid_marker', c_uint8, 1),
        ('rfu', c_int8, 6),
    ]

class BootloaderVersionStringFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('data', POINTER(c_uint8)),
    ]

class BootloaderBuildNumberFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('data', POINTER(c_uint8)),
    ]

class BootloaderGitHashFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('data', POINTER(c_uint8)),
    ]

class CrashInfoFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('data', POINTER(c_uint8)),
    ]
# IPJ_autogen }
