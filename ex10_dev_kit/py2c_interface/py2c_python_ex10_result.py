#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2023 Impinj, Inc. All rights reserved.                      #
#                                                                           #
#############################################################################

import sys
import os
import resource as resource
import ctypes
from ctypes import *
from py2c_interface.py2c_python_auto_regs import *

# Ex10Result-related structures seperated out to be used in multiple
# locations. Used for many functions in the SDK as well as part of event
# FIFO packets.

# IPJ_autogen | gen_c2python_ex10_result {
# Required enums from C
# Required Structs from c code
class Ex10CommandsHostResult(Structure):
    _pack_ = 1
    _fields_ = [
        ('failed_result_code', c_uint8, 8),
        ('failed_command_code', c_uint8, 8),
        ('failed_host_result_code', c_uint8, 8),
        ('rfu', c_uint8, 8),
    ]


class Ex10ResultCode(Union):
    _pack_ = 1
    _fields_ = [
        ('sdk', c_uint8, 8),
        ('device', c_uint8, 8),
        ('raw', c_uint8, 8),
    ]


class Ex10DeviceStatus(Union):
    _pack_ = 1
    _fields_ = [
        ('cmd_result', CommandResultFields),
        ('cmd_host_result', Ex10CommandsHostResult),
        ('ops_status', OpsStatusFields),
        ('raw', c_uint32, 32),
    ]


class Ex10Result(Structure):
    _pack_ = 1
    _fields_ = [
        ('error', c_bool, 1),
        ('customer', c_bool, 1),
        ('rfu', c_uint16, 14),
        ('module', c_uint8, 8),
        ('result_code', Ex10ResultCode),
        ('device_status', Ex10DeviceStatus),
    ]
# IPJ_autogen }
