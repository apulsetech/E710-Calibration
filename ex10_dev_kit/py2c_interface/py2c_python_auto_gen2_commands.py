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

from py2c_interface.py2c_python_byte_span import *

from py2c_interface.py2c_python_auto_regs import InventoryRoundControlFields
from py2c_interface.py2c_python_auto_regs import InventoryRoundControl_2Fields
from py2c_interface.py2c_python_auto_regs import DropQueryControlFields


# IPJ_autogen | gen_c2python_gen2_commands {
# Required enums from C
class Gen2Command(IntEnum):
    Gen2Select = 0
    Gen2Read = 1
    Gen2Write = 2
    Gen2Kill_1 = 3
    Gen2Kill_2 = 4
    Gen2Lock = 5
    Gen2Access = 6
    Gen2BlockWrite = 7
    Gen2BlockPermalock = 8
    Gen2Authenticate = 9
    Gen2MarginRead = 10
    _GEN2_COMMAND_MAX = 11


class TagErrorCode(IntEnum):
    Other = 0
    NotSupported = 1
    InsufficientPrivileges = 2
    MemoryOverrun = 3
    MemoryLocked = 4
    CryptoSuite = 5
    CommandNotEncapsulated = 6
    ResponseBufferOverflow = 7
    SecurityTimeout = 8
    InsufficientPower = 11
    NonSpecific = 15
    NoError = 16


class SelectTarget(IntEnum):
    Session0 = 0
    Session1 = 1
    Session2 = 2
    Session3 = 3
    SelectedFlag = 4


class SelectAction(IntEnum):
    Action000 = 0
    Action001 = 1
    Action010 = 2
    Action011 = 3
    Action100 = 4
    Action101 = 5
    Action110 = 6
    Action111 = 7


class MemoryBank(IntEnum):
    Reserved = 0
    EPC = 1
    TID = 2
    User = 3


class SelectMemoryBank(IntEnum):
    SelectFileType = 0
    SelectEPC = 1
    SelectTID = 2
    SelectFile0 = 3


class BlockPermalockReadLock(IntEnum):
    Read = 0
    Permalock = 1


class ResponseType(IntEnum):
    None_ = 0x0
    Immediate = 0x1
    Delayed = 0x2
    InProcess = 0x3


# Required Structs from c code
class BitSpan(Structure):
    _fields_ = [
        ('data', POINTER(c_uint8)),
        ('length', c_size_t),
    ]


class Gen2CommandSpec(Structure):
    _fields_ = [
        ('command', c_uint32),
        ('args', c_void_p),
    ]


class Gen2Reply(Structure):
    _fields_ = [
        ('reply', c_uint32),
        ('error_code', c_uint32),
        ('data', POINTER(c_uint16)),
        ('transaction_status', c_uint32),
    ]


class SelectCommandArgs(Structure):
    _fields_ = [
        ('target', c_uint32),
        ('action', c_uint32),
        ('memory_bank', c_uint32),
        ('bit_pointer', c_uint32),
        ('bit_count', c_uint8),
        ('mask', POINTER(BitSpan)),
        ('truncate', c_bool),
    ]


class ReadCommandArgs(Structure):
    _fields_ = [
        ('memory_bank', c_uint32),
        ('word_pointer', c_uint32),
        ('word_count', c_uint8),
    ]


class WriteCommandArgs(Structure):
    _fields_ = [
        ('memory_bank', c_uint32),
        ('word_pointer', c_uint32),
        ('data', c_uint16),
    ]


class KillCommandArgs(Structure):
    _fields_ = [
        ('password', c_uint16),
    ]


class LockCommandArgs(Structure):
    _fields_ = [
        ('kill_password_read_write_mask', c_bool),
        ('kill_password_permalock_mask', c_bool),
        ('access_password_read_write_mask', c_bool),
        ('access_password_permalock_mask', c_bool),
        ('epc_memory_write_mask', c_bool),
        ('epc_memory_permalock_mask', c_bool),
        ('tid_memory_write_mask', c_bool),
        ('tid_memory_permalock_mask', c_bool),
        ('file_0_memory_write_mask', c_bool),
        ('file_0_memory_permalock_mask', c_bool),
        ('kill_password_read_write_lock', c_bool),
        ('kill_password_permalock', c_bool),
        ('access_password_read_write_lock', c_bool),
        ('access_password_permalock', c_bool),
        ('epc_memory_write_lock', c_bool),
        ('epc_memory_permalock', c_bool),
        ('tid_memory_write_lock', c_bool),
        ('tid_memory_permalock', c_bool),
        ('file_0_memory_write_lock', c_bool),
        ('file_0_memory_permalock', c_bool),
    ]


class AccessCommandArgs(Structure):
    _fields_ = [
        ('password', c_uint16),
    ]


class AccessCommandReply(Structure):
    _fields_ = [
        ('tag_handle', c_uint16),
        ('response_crc[2]', c_uint8),
    ]


class KillCommandReply(Structure):
    _fields_ = [
        ('tag_handle', c_uint16),
        ('response_crc', c_uint16),
    ]


class DelayedReply(Structure):
    _fields_ = [
        ('tag_handle', c_uint16),
        ('response_crc', c_uint16),
    ]


class BlockWriteCommandArgs(Structure):
    _fields_ = [
        ('memory_bank', c_uint32),
        ('word_pointer', c_uint32),
        ('word_count', c_uint8),
        ('data', POINTER(BitSpan)),
    ]


class BlockPermalockCommandArgs(Structure):
    _fields_ = [
        ('read_lock', c_uint32),
        ('memory_bank', c_uint32),
        ('block_pointer', c_uint32),
        ('block_range', c_uint8),
        ('mask', POINTER(BitSpan)),
    ]


class AuthenticateCommandArgs(Structure):
    _fields_ = [
        ('send_rep', c_bool),
        ('inc_rep_len', c_bool),
        ('csi', c_uint8),
        ('length', c_uint16),
        ('message', POINTER(BitSpan)),
        ('rep_len_bits', c_uint16),
    ]


class MarginReadCommandArgs(Structure):
    _fields_ = [
        ('memory_bank', c_uint32),
        ('bit_pointer', c_uint32),
        ('bit_length', c_uint8),
        ('mask', POINTER(BitSpan)),
    ]


class InventoryRoundConfigBasic(Structure):
    _fields_ = [
        ('antenna', c_uint8),
        ('rf_mode', c_uint32),
        ('tx_power_cdbm', c_int16),
        ('inventory_config', InventoryRoundControlFields),
        ('inventory_config_2', InventoryRoundControl_2Fields),
        ('send_selects', c_bool),
    ]
# IPJ_autogen }
