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

import sys
import os
import resource as resource
import ctypes
from ctypes import *
from enum import IntEnum
from py2c_interface.py2c_python_auto_enums import *
from py2c_interface.py2c_python_auto_fifo import *
from py2c_interface.py2c_python_auto_regs import *
from py2c_interface.py2c_python_auto_boot_regs import *
from py2c_interface.py2c_python_auto_gen2_commands import *
from py2c_interface.py2c_python_byte_span import *
from py2c_interface.py2c_python_cal import *
from py2c_interface.py2c_python_auto_reg_instances import *
from py2c_interface.py2c_python_ex10_result import *


c_size_t = ctypes.c_uint32
TCXO_FREQ_KHZ = 24000
VERSION_STRING_SIZE = 120
REMAIN_REASON_STRING_MAX_SIZE = 25

BOOTLOADER_SPI_CLOCK_HZ = 1000000
DEFAULT_SPI_CLOCK_HZ = 4000000

# Note: These values must match the C SDK values defined in
# dev_kit/ex10_c_dev_kit/include/ex10_api/gen2_tx_command_manager.h
TxCommandDecodeBufferSize = 40
TxCommandEncodeBufferSize = 40

class Ex10Py2CWrapper(object):
    """
    Takes care of importing the c lib, exchanging python data with
    ctypes, and passing data back and forth.
    """

    def __init__(self, so_path=None):
        """
        Instantiate the py2c object
        """
        if not so_path:
            so_path = os.path.dirname(__file__) + "/lib_py2c.so"
        py2c_so = ctypes.cdll.LoadLibrary(so_path)

        self.py2c_so_path = so_path
        self.py2c_so = py2c_so

        # Set up return types for c defined functions we use in python
        py2c_so.ex10_typical_board_setup.argtypes = c_uint32, c_uint32
        py2c_so.ex10_typical_board_setup.restype = Ex10Result
        py2c_so.ex10_core_board_setup.argtypes = (c_uint32,)
        py2c_so.ex10_core_board_setup.restype = Ex10Result
        py2c_so.ex10_bootloader_board_setup.argtypes = (c_uint32,)
        py2c_so.ex10_bootloader_board_setup.restype = Ex10Result
        py2c_so.get_ex10_board_driver_list.restype = ctypes.POINTER(Ex10DriverList)
        py2c_so.get_ex10_protocol.restype = ctypes.POINTER(Ex10Protocol)
        py2c_so.get_ex10_ops.restype = ctypes.POINTER(Ex10Ops)
        py2c_so.get_ex10_reader.restype = ctypes.POINTER(Ex10Reader)
        py2c_so.get_ex10_power_modes.restype = ctypes.POINTER(Ex10PowerModes)
        py2c_so.get_ex10_autoset_modes.restype = ctypes.POINTER(Ex10AutosetModes)
        py2c_so.get_ex10_power_transactor.restype = ctypes.POINTER(Ex10PowerTransactor)
        py2c_so.get_ex10_gen2_tx_command_manager.restype = ctypes.POINTER(Ex10Gen2TxCommandManager)
        py2c_so.get_ex10_random.restype = ctypes.POINTER(Ex10Random)
        py2c_so.get_ex10_time_helpers.restype = ctypes.POINTER(Ex10TimeHelpers)
        py2c_so.get_ex10_helpers.restype = ctypes.POINTER(Ex10Helpers)
        py2c_so.get_ex10_commands.restype = ctypes.POINTER(Ex10Commands)
        py2c_so.get_ex10_gen2_commands.restype = ctypes.POINTER(Ex10Gen2Commands)
        py2c_so.get_ex10_event_parser.restype = ctypes.POINTER(Ex10EventParser)
        py2c_so.get_ex10_version.restype = ctypes.POINTER(Ex10Version)
        py2c_so.get_ex10_sjc.restype = ctypes.POINTER(Ex10SjcAccessor)
        py2c_so.get_ex10_event_fifo_buffer_pool.restype = ctypes.POINTER(FifoBufferPool)
        py2c_so.get_ex10_fifo_buffer_list.restype = ctypes.POINTER(FifoBufferList)
        py2c_so.get_ex10_active_region.restype = ctypes.POINTER(Ex10ActiveRegion)
        py2c_so.get_ex10_regulatory.restype = ctypes.POINTER(Ex10Regulatory)
        py2c_so.get_ex10_default_region_names.restype = ctypes.POINTER(Ex10DefaultRegionNames)
        py2c_so.get_ex10_board_spec.restype = ctypes.POINTER(Ex10BoardSpec)
        py2c_so.get_ex10_gpio_helpers.restype = ctypes.POINTER(Ex10GpioHelpers)
        py2c_so.get_ex10_rx_baseband_filter.restype = ctypes.POINTER(Ex10RxBasebandFilter)
        py2c_so.get_ex10_aggregate_op_builder.restype = ctypes.POINTER(Ex10AggregateOpBuilder)
        py2c_so.get_ex10_event_fifo_printer.restype = ctypes.POINTER(Ex10EventFifoPrinter)
        py2c_so.get_ex10_rf_power.restype = ctypes.POINTER(Ex10RfPower)
        py2c_so.get_ex10_continuous_inventory_use_case.restype = ctypes.POINTER(Ex10ContinuousInventoryUseCase)
        py2c_so.get_ex10_inventory.restype = ctypes.POINTER(Ex10Inventory)
        py2c_so.get_ex10_inventory_sequence_use_case.restype = ctypes.POINTER(Ex10InventorySequenceUseCase)
        py2c_so.get_ex10_tag_access_use_case.restype = ctypes.POINTER(Ex10TagAccessUseCase)
        py2c_so.get_ex10_event_fifo_queue.restype = ctypes.POINTER(Ex10EventFifoQueue)
        py2c_so.get_ex10_listen_before_talk.restype = ctypes.POINTER(Ex10ListenBeforeTalk)
        py2c_so.get_ex10_antenna_disconnect.restype = ctypes.POINTER(Ex10AntennaDisconnect)
        py2c_so.get_ex10_test.restype = ctypes.POINTER(Ex10Test)

        py2c_so.get_ex10_calibration.restype = POINTER(Ex10Calibration)
        py2c_so.get_ex10_cal_v5.restype = POINTER(Ex10CalibrationV5)

        # functions in ex10_utils
        py2c_so.ex10_set_default_gpio_setup.restype = Ex10Result
        py2c_so.ex10_discard_packets.argtypes = c_bool, c_bool, c_bool
        py2c_so.ex10_discard_packets.restype = ctypes.c_size_t

        # functions in ex10_result
        py2c_so.print_ex10_result.argtype = (Ex10Result,)
        py2c_so.print_ex10_result.restype = ctypes.c_void_p

    def _insert_into_namespace(self, name, value):
        """
        A module's globals() may be different from the caller's so allow the
        caller to add to this module's globals.
        """
        globals()[name] = value

    def __getattr__(self, name):
        try:
            # If the dll contains the function being called, we grab that handle and
            # pass it to the appropriate intercept. If it does not, we contine to
            # the intercepts with the understanding that it may be a purely py2c construct.
            if hasattr(self.py2c_so, name):
                ret_val = getattr(self.py2c_so, name)

            if name == 'get_ex10_protocol':
                ret_val = GetEx10ProtocolIntercept(ret_val)
            elif name == 'get_ex10_ops':
                ret_val = GetEx10OpsIntercept(ret_val)
            elif name == 'get_ex10_reader':
                ret_val = GetEx10ReaderIntercept(ret_val)
            elif name == 'get_ex10_power_modes':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_autoset_modes':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_power_transactor':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_interfaces':
                ret_val = GetEx10InterfacesIntercept(self.py2c_so)
            elif name == 'get_ex10_sjc':
                ret_val = GetEx10SjcAccessorIntercept(ret_val)
            elif name == 'get_ex10_helpers':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_commands':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_gen2_commands':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_event_parser':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_version':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_aggregate_op_builder':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_active_region':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_default_region_names':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_rf_power':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_inventory':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_regulatory':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_board_spec':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_gpio_helpers':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_event_fifo_buffer_pool':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_fifo_buffer_list':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_board_driver_list':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_cal_v4':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_cal_v5':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_gen2_tx_command_manager':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_event_fifo_printer':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_continuous_inventory_use_case':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_inventory_sequence_use_case':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_tag_access_use_case':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_event_fifo_queue':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_listen_before_talk':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_antenna_disconnect':
                ret_val = GetGenericIntercept(ret_val)
            elif name == 'get_ex10_test':
                ret_val = GetGenericIntercept(ret_val)
            return ret_val
        except:
            assert(sys.exc_info()[0])


class Ex10InterfaceIntercept():
    protocol = None
    ops = None
    reader = None
    helpers = None
    gen2_commands = None
    event_parser = None
    version = None
    power_modes = None
    sjc = None
    rf_power = None


class GetEx10InterfacesIntercept():
    def __init__(self, py2c_so):
        """
        Fakes being a function to intercept the return from the c lib
        """
        self.py2c_so_ = py2c_so

    def __call__(self):
        # Return the contents of the call
        interfaces_intercept = Ex10InterfaceIntercept()
        interfaces_intercept.protocol = Ex10ProtocolIntercept(self.py2c_so_.get_ex10_protocol())
        interfaces_intercept.ops = Ex10OpsIntercept(self.py2c_so_.get_ex10_ops())
        interfaces_intercept.reader = Ex10ReaderIntercept(self.py2c_so_.get_ex10_reader())
        interfaces_intercept.power_modes = self.py2c_so_.get_ex10_power_modes().contents
        interfaces_intercept.helpers = self.py2c_so_.get_ex10_helpers().contents
        interfaces_intercept.gen2_commands = self.py2c_so_.get_ex10_gen2_commands().contents
        interfaces_intercept.event_parser = self.py2c_so_.get_ex10_event_parser().contents
        interfaces_intercept.version = self.py2c_so_.get_ex10_version().contents
        interfaces_intercept.power_modes = self.py2c_so_.get_ex10_power_modes().contents
        interfaces_intercept.sjc = self.py2c_so_.get_ex10_sjc().contents
        interfaces_intercept.rf_power = self.py2c_so_.get_ex10_rf_power().contents
        return interfaces_intercept


class GetEx10SjcAccessorIntercept():
    def __init__(self, get_sjc_function):
        """
        Fakes being a function to intercept the return from the c lib
        """
        self.get_sjc_function = get_sjc_function

    def __call__(self):
        # The normal call function
        sjc = self.get_sjc_function()
        return Ex10SjcAccessorIntercept(sjc)


class Ex10SjcAccessorIntercept():
    def __init__(self, sjc_layer):
        """
        Intercepts calls to the reader layer
        """
        self.sjc = sjc_layer.contents

    def __getattr__(self, name):
        attr = getattr(self.sjc, name)
        # Ensure this is a callable function
        if hasattr(attr, '__call__'):
            # Intercept the C function to make 'pythonic'
            def newfunc(*args, **kwargs):
                if name == 'init':
                    # User passes in Ex10ProtocolIntercept, but we want ex10_protocol
                    result = attr(pointer(args[0].ex10_protocol))
                else:
                    result = attr(*args, **kwargs)
                return result
            return newfunc
        else:
            return attr

class GetGenericIntercept():
    def __init__(self, generic_function_pointer):
        """
        Fakes being a function to intercept the return from the c lib.
        Purely calls the contents to dereference the function pointer.
        """
        self.generic_function_pointer = generic_function_pointer

    def __call__(self):
        # Return the contents of the call
        return self.generic_function_pointer().contents


class GetEx10ReaderIntercept():
    def __init__(self, get_reader_function):
        """
        Fakes being a function to intercept the return from the c lib
        """
        self.get_reader_function = get_reader_function

    def __call__(self):
        # The normal call function
        ex10_reader = self.get_reader_function()
        return Ex10ReaderIntercept(ex10_reader)


class Ex10ReaderIntercept():
    def __init__(self, reader_layer):
        """
        Intercepts calls to the reader layer
        """
        self.ex10_reader = reader_layer.contents

    def __getattr__(self, name):
        attr = getattr(self.ex10_reader, name)
        # Ensure this is a callable function
        if hasattr(attr, '__call__'):
            # Intercept the C function to make 'pythonic'
            def newfunc(*args, **kwargs):
                return attr(*args, **kwargs)
            return newfunc
        else:
            return attr


class GetEx10OpsIntercept():
    def __init__(self, get_ops_function):
        """
        Fakes being a function to intercept the return from the c lib
        """
        self.get_ops_function = get_ops_function

    def __call__(self):
        # The normal call function
        ex10_ops = self.get_ops_function()
        return Ex10OpsIntercept(ex10_ops)


class Ex10OpsIntercept():
    def __init__(self, ops_layer):
        """
        Intercepts calls to the ops layer
        """
        self.ex10_ops = ops_layer.contents

    def __getattr__(self, name):
        attr = getattr(self.ex10_ops, name)
        # Ensure this is a callable function
        if hasattr(attr, '__call__'):
            # Intercept the C function to make 'pythonic'
            def newfunc(*args, **kwargs):
                return attr(*args, **kwargs)
            return newfunc
        else:
            return attr


class GetEx10ProtocolIntercept():
    def __init__(self, get_protocol_function):
        """
        Fakes being a function to intercept the return from the c lib
        """
        self.get_protocol_function = get_protocol_function

    def __call__(self):
        # The normal call function
        ex10_protocol = self.get_protocol_function()
        return Ex10ProtocolIntercept(ex10_protocol)


class Ex10ProtocolIntercept():
    def __init__(self, protocol_layer):
        """
        Intercepts calls to the protocol layer
        """
        self.ex10_protocol = protocol_layer.contents
        self.reg_list = [getattr(RegInstances, attr) for attr in dir(RegInstances) if not callable(getattr(RegInstances, attr)) and not attr.startswith("__")]

    def _get_reg_from_str(self, reg_str):
        return [reg for reg in self.reg_list if reg.name == reg_str][0]

    def _get_return_struct_from_reg(self, reg_to_use):
        # Name passed in plus 'Fields' is the structure to read data into
        # EX: User wants reg 'OpsStatusFields', so we read the reg into a OpsStatusFields() structure
        read_struct_name = reg_to_use.name
        # Deal with special naming cases of return structs versus register naming
        if 'SjcResult' in read_struct_name:
            read_struct_name = read_struct_name[:-1]
        read_struct_name = read_struct_name + 'Fields'
        read_object = globals()[read_struct_name]()
        return read_object

    def _class_contains_bitfield(self, class_obj):
        num_args_to_unpack = len(class_obj._fields_[0])
        contains_bit_pack = True if num_args_to_unpack == 3 else False
        return contains_bit_pack

    def __getattr__(self, name):
        attr = getattr(self.ex10_protocol, name)
        # Ensure this is a callable function
        if hasattr(attr, '__call__'):
            # Intercept the C function to make 'pythonic'
            def newfunc(*args, **kwargs):
                if name == 'write' or name == 'write_index':
                    # User passes in (reg string name, buffer to write)
                    reg_to_use = self._get_reg_from_str(args[0])
                    if len(args) == 3: # Means this is write_index
                        result = attr(reg_to_use, ctypes.cast(pointer(args[1]), c_void_p), args[2])
                    else:
                        result = attr(reg_to_use, ctypes.cast(pointer(args[1]), c_void_p))
                elif name == 'read' or name == 'read_index':
                    # User passes in the reg string name and wants a buffer back
                    reg_to_use = self._get_reg_from_str(args[0])
                    read_object = self._get_return_struct_from_reg(reg_to_use)
                    contains_bit_pack = self._class_contains_bitfield(read_object)

                    # Find out if return type is a pointer - if so, read into a buffer
                    is_pointer = False
                    if not contains_bit_pack:
                        for field_name, field_type in read_object._fields_:
                            is_pointer = isinstance(getattr(read_object, field_name), POINTER(c_uint8))
                    if is_pointer:
                        read_object = (c_uint8 * reg_to_use.length)()

                    # Decide whether to read with read or read_index
                    if len(args) == 3:
                        attr(reg_to_use, ctypes.cast(pointer(read_object), c_void_p), args[2])
                    else:
                        # if reading the whole register, we need to account for the num entries
                        if reg_to_use.num_entries > 1:
                            read_object = (type(read_object) * reg_to_use.num_entries)()
                        attr(reg_to_use, ctypes.cast(pointer(read_object), c_void_p))
                    # Return a bytearray if the class was a buffer
                    result = bytearray(read_object) if is_pointer else read_object
                else:
                    result = attr(*args, **kwargs)
                return result
            return newfunc
        else:
            return attr


class Ex10ListNode(Structure):
    """ Forward declare referencing structs prior to their autogen """
    pass


class Ex10BoardInitStatus(Structure):
    """ Forward declare referencing structs prior to their autogen """
    pass


# Must match the C language symbol TID_LENGTH_BYTES in event_packet_parser.h
TID_LENGTH_BYTES = 12

# The Gen2 maximum possible EPC length, in bytes.
EPC_LENGTH_BYTES = 0x1F * 2

# The Gen2 PC length, in bytes.
PC_LENGTH_BYTES = 2

# The Gen2 XPC length, in bytes.
XPC_LENGTH_BYTES = 4

# The TagReadData epc buffer allocation, which includes room for the PC
# This must match the C language symbol EPC_BUFFER_BYTE_LENGTH in ex10_helpers.h
EPC_BUFFER_BYTE_LENGTH = EPC_LENGTH_BYTES + PC_LENGTH_BYTES + XPC_LENGTH_BYTES

# Note that this is not autogenerated via 'hand_picked structs_of_interest'
# due to C language array allocations. Requires parsing of C preprocessor
# symbols and array bracket sizes.
class TagReadData(Structure):
    _fields_ = [
        ('pc', c_uint16),
        ('xpc_w1', c_uint16),
        ('xpc_w1_is_valid', c_bool),
        ('xpc_w2', c_uint16),
        ('xpc_w2_is_valid', c_bool),
        ('epc', (c_uint8 * EPC_BUFFER_BYTE_LENGTH)),
        ('epc_length', c_size_t),
        ('stored_crc', c_uint16),
        ('stored_crc_is_valid', c_bool),
        ('tid', (c_uint8 * TID_LENGTH_BYTES)),
        ('tid_length', c_size_t),
    ]


# IPJ_autogen | gen_c2python_hand_picked {
# Required enums from C
class AuxAdcControlChannelEnableBits(IntEnum):
    ChannelEnableBitsNone = 0x00
    ChannelEnableBitsPowerLo0 = 0x01
    ChannelEnableBitsPowerLo1 = 0x02
    ChannelEnableBitsPowerLo2 = 0x04
    ChannelEnableBitsPowerLo3 = 0x08
    ChannelEnableBitsPowerRx0 = 0x10
    ChannelEnableBitsPowerRx1 = 0x20
    ChannelEnableBitsPowerRx2 = 0x40
    ChannelEnableBitsPowerRx3 = 0x80
    ChannelEnableBitsTestMux0 = 0x100
    ChannelEnableBitsTestMux1 = 0x200
    ChannelEnableBitsTestMux2 = 0x400
    ChannelEnableBitsTestMux3 = 0x800
    ChannelEnableBitsTemperature = 0x1000
    ChannelEnableBitsPowerLoSum = 0x2000
    ChannelEnableBitsPowerRxSum = 0x4000


class FifoSelection(IntEnum):
    EventFifo = 0


class StopReason(IntEnum):
    SRNone = 0
    SRHost = 1
    SRMaxNumberOfRounds = 2
    SRMaxNumberOfTags = 3
    SRMaxDuration = 4
    SROpError = 5
    SRSdkTimeoutError = 6
    SRDeviceCommandError = 7
    SRDeviceAggregateBufferOverflow = 8
    SRDeviceRampCallbackError = 9
    SRDeviceEventFifoFull = 10
    SRDeviceInventoryInvalidParam = 11
    SRDeviceLmacOverload = 12
    SRDeviceInventorySummaryReasonInvalid = 13
    SRReasonUnknown = 14


class ProductSku(IntEnum):
    SkuUnknown = 0x0
    SkuE310 = 0x0310
    SkuE510 = 0x0510
    SkuE710 = 0x0710
    SkuE910 = 0x0910


class RfFilter(IntEnum):
    UNDEFINED_BAND = 0
    LOWER_BAND = 1
    UPPER_BAND = 2


class RfModes(IntEnum):
    mode_1 = 1
    mode_3 = 3
    mode_5 = 5
    mode_7 = 7
    mode_11 = 11
    mode_12 = 12
    mode_13 = 13
    mode_15 = 15
    mode_102 = 102
    mode_103 = 103
    mode_120 = 120
    mode_123 = 123
    mode_124 = 124
    mode_125 = 125
    mode_141 = 141
    mode_146 = 146
    mode_147 = 147
    mode_148 = 148
    mode_185 = 185
    mode_202 = 202
    mode_203 = 203
    mode_222 = 222
    mode_223 = 223
    mode_225 = 225
    mode_226 = 226
    mode_241 = 241
    mode_244 = 244
    mode_285 = 285
    mode_302 = 302
    mode_323 = 323
    mode_324 = 324
    mode_325 = 325
    mode_342 = 342
    mode_343 = 343
    mode_344 = 344
    mode_345 = 345
    mode_382 = 382


class DrmStatus(IntEnum):
    DrmStatusAuto = 0
    DrmStatusOn = 1
    DrmStatusOff = 2


class AutosetModeId(IntEnum):
    AutosetMode_Invalid = 0
    AutosetMode_1120 = 1120
    AutosetMode_1122 = 1122
    AutosetMode_1123 = 1123
    AutosetMode_1220 = 1220
    AutosetMode_1320 = 1320
    AutosetMode_1322 = 1322
    AutosetMode_1323 = 1323
    AutosetMode_1420 = 1420


class InventoryHelperReturns(IntEnum):
    InvHelperSuccess = 0
    InvHelperOpStatusError = 1
    InvHelperStopConditions = 2
    InvHelperTimeout = 3


class BasebandFilterType(IntEnum):
    BasebandFilterHighpass = 0
    BasebandFilterBandpass = 1


class AggregateOpInstructionType(IntEnum):
    InstructionTypeReserved = 0x00
    InstructionTypeWrite = 0x02
    InstructionTypeReset = 0x08
    InstructionTypeInsertFifoEvent = 0x0E
    InstructionTypeRunOp = 0x30
    InstructionTypeGoToIndex = 0x31
    InstructionTypeExitInstruction = 0x32
    InstructionTypeIdentifier = 0x33


class PowerMode(IntEnum):
    PowerModeInvalid = 0
    PowerModeOff = 1
    PowerModeStandby = 2
    PowerModeReadyCold = 3
    PowerModeReady = 4


class InventoryRoundConfigType(IntEnum):
    INVENTORY_ROUND_CONFIG_UNKNOWN = 0
    INVENTORY_ROUND_CONFIG_BASIC = 1


class Ex10RegionId(IntEnum):
    REGION_NOT_DEFINED = 0
    REGION_FCC = 1
    REGION_HK = 2
    REGION_TAIWAN = 3
    REGION_ETSI_LOWER = 4
    REGION_ETSI_UPPER = 5
    REGION_KOREA = 6
    REGION_MALAYSIA = 7
    REGION_CHINA = 8
    REGION_SOUTH_AFRICA = 9
    REGION_BRAZIL = 10
    REGION_THAILAND = 11
    REGION_SINGAPORE = 12
    REGION_AUSTRALIA = 13
    REGION_INDIA = 14
    REGION_URUGUAY = 15
    REGION_VIETNAM = 16
    REGION_ISRAEL = 17
    REGION_PHILIPPINES = 18
    REGION_INDONESIA = 19
    REGION_NEW_ZEALAND = 20
    REGION_JAPAN2 = 21
    REGION_PERU = 22
    REGION_RUSSIA = 23
    REGION_CUSTOM = 24


class Ex10CommandsHostResultCode(IntEnum):
    HostResultSuccess = 0
    HostResultReceivedLengthIncorrect = 1
    HostResultTestTransferVerifyError = 2


class Ex10SdkResultCode(IntEnum):
    Ex10SdkSuccess = 0
    Ex10SdkErrorBadParamValue = 1
    Ex10SdkErrorBadParamLength = 2
    Ex10SdkErrorBadParamAlignment = 3
    Ex10SdkErrorNullPointer = 4
    Ex10SdkErrorTimeout = 5
    Ex10SdkErrorRunLocation = 6
    Ex10SdkErrorAggBufferOverflow = 7
    Ex10SdkErrorOpRunning = 8
    Ex10SdkErrorInvalidState = 9
    Ex10SdkEventFifoFull = 10
    Ex10SdkNoFreeEventFifoBuffers = 11
    Ex10SdkFreeEventFifoBuffersLengthMismatch = 12
    Ex10SdkLmacOverload = 13
    Ex10InventoryInvalidParam = 14
    Ex10InventorySummaryReasonInvalid = 15
    Ex10InvalidEventFifoPacket = 16
    Ex10SdkErrorGpioInterface = 17
    Ex10SdkErrorHostInterface = 18
    Ex10ErrorGen2BufferLength = 19
    Ex10ErrorGen2NumCommands = 20
    Ex10ErrorGen2CommandEncode = 21
    Ex10ErrorGen2CommandDecode = 22
    Ex10ErrorGen2CommandEnableMismatch = 23
    Ex10ErrorGen2EmptyCommand = 24
    Ex10SdkErrorUnexpectedTxLength = 25
    Ex10AboveThreshold = 26
    Ex10BelowThreshold = 27
    Ex10MemcpyFailed = 28
    Ex10MemsetFailed = 29


class Ex10DeviceResultCode(IntEnum):
    Ex10DeviceSuccess = 0
    Ex10DeviceErrorCommandsNoResponse = 1
    Ex10DeviceErrorCommandsWithResponse = 2
    Ex10DeviceErrorOps = 3
    Ex10DeviceErrorOpsTimeout = 4


class Ex10Module(IntEnum):
    Ex10ModuleUndefined = 0
    Ex10ModuleDevice = 1
    Ex10ModuleCommandTransactor = 2
    Ex10ModuleCommands = 3
    Ex10ModuleProtocol = 4
    Ex10ModuleOps = 5
    Ex10ModuleUtils = 6
    Ex10ModuleRfPower = 7
    Ex10ModuleInventory = 8
    Ex10ModuleReader = 9
    Ex10ModuleTest = 10
    Ex10ModulePowerModes = 11
    Ex10ModuleGen2Commands = 12
    Ex10ModuleGen2Response = 13
    Ex10ModuleModuleManager = 14
    Ex10ModuleFifoBufferList = 15
    Ex10ModuleBoardInit = 16
    Ex10AntennaDisconnect = 17
    Ex10ListenBeforeTalk = 18
    Ex10ModuleUseCase = 19
    Ex10ModuleAutosetModes = 20
    Ex10ModuleApplication = 21
    Ex10ModuleRegion = 22
    Ex10ModuleEx10Gpio = 23


class TagAccessResult(IntEnum):
    TagAccessSuccess = 0
    TagAccessTagLost = 1
    TagAccessHaltSequenceWriteError = 2


class HaltedCallbackResult(IntEnum):
    AckTagAndContinue = 0
    NakTagAndContinue = 1


# Required Structs from c code
class Ex10GpioConfig(Structure):
    _fields_ = [
        ('antenna', c_uint8),
        ('baseband_filter', c_uint32),
        ('dio_0', c_bool),
        ('dio_1', c_bool),
        ('dio_6', c_bool),
        ('dio_8', c_bool),
        ('dio_13', c_bool),
        ('pa_bias_enable', c_bool),
        ('power_range', c_uint32),
        ('rf_enable', c_bool),
        ('rf_filter', c_uint32),
    ]


class GpioControlFields(Structure):
    _fields_ = [
        ('output_enable', c_uint32),
        ('output_level', c_uint32),
    ]


class GpioPinsSetClear(Structure):
    _fields_ = [
        ('output_level_set', c_uint32),
        ('output_level_clear', c_uint32),
        ('output_enable_set', c_uint32),
        ('output_enable_clear', c_uint32),
    ]


class EventFifoPacket(Structure):
    _fields_ = [
        ('packet_type', c_uint32),
        ('us_counter', c_uint32),
        ('static_data', POINTER(PacketData)),
        ('static_data_length', c_size_t),
        ('dynamic_data', POINTER(c_uint8)),
        ('dynamic_data_length', c_size_t),
        ('is_valid', c_bool),
    ]


class Ex10ListNode(Structure):
    _fields_ = [
        ('data', c_void_p),
        ('next', POINTER(Ex10ListNode)),
        ('prev', POINTER(Ex10ListNode)),
    ]


class Ex10LinkedList(Structure):
    _fields_ = [
        ('sentinel', Ex10ListNode),
    ]


class FifoBufferNode(Structure):
    _fields_ = [
        ('fifo_data', ConstByteSpan),
        ('raw_buffer', ByteSpan),
        ('list_node', Ex10ListNode),
    ]


class FifoBufferPool(Structure):
    _fields_ = [
        ('fifo_buffer_nodes', POINTER(FifoBufferNode)),
        ('fifo_buffers', POINTER(ByteSpan)),
        ('buffer_count', c_size_t),
    ]


class FifoBufferList(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(Ex10Result, POINTER(FifoBufferNode), POINTER(ByteSpan), c_size_t)),
        ('free_list_put', CFUNCTYPE(c_bool, POINTER(FifoBufferNode))),
        ('free_list_get', CFUNCTYPE(POINTER(FifoBufferNode))),
        ('free_list_size', CFUNCTYPE(c_size_t)),
    ]


class TagReadFields(Structure):
    _fields_ = [
        ('pc', POINTER(c_uint16)),
        ('xpc_w1', POINTER(c_uint16)),
        ('xpc_w2', POINTER(c_uint16)),
        ('epc', POINTER(c_uint8)),
        ('epc_length', c_size_t),
        ('stored_crc', POINTER(c_uint8)),
        ('tid', POINTER(c_uint8)),
        ('tid_length', c_size_t),
    ]


class StopConditions(Structure):
    _fields_ = [
        ('max_number_of_rounds', c_uint32),
        ('max_number_of_tags', c_uint32),
        ('max_duration_us', c_uint32),
    ]


class ContinuousInventorySummary(Structure):
    _fields_ = [
        ('duration_us', c_uint32),
        ('number_of_inventory_rounds', c_uint32),
        ('number_of_tags', c_uint32),
        ('reason', c_uint8),
        ('last_op_id', c_uint8),
        ('last_op_error', c_uint8),
        ('packet_rfu_1', c_uint8),
    ]


class InventoryParams(Structure):
    _fields_ = [
        ('antenna', c_uint8),
        ('rf_mode', c_uint32),
        ('tx_power_cdbm', c_int16),
        ('inventory_config', InventoryRoundControlFields),
        ('inventory_config_2', InventoryRoundControl_2Fields),
        ('send_selects', c_bool),
    ]


class Ex10Inventory(Structure):
    _fields_ = [
        ('run_inventory', CFUNCTYPE(Ex10Result, POINTER(InventoryRoundControlFields), POINTER(InventoryRoundControl_2Fields), c_bool)),
        ('start_inventory', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int16, POINTER(InventoryRoundControlFields), POINTER(InventoryRoundControl_2Fields), c_bool)),
        ('inventory_halted', CFUNCTYPE(c_bool)),
        ('ex10_result_to_continuous_inventory_error', CFUNCTYPE(c_uint32, Ex10Result)),
    ]


class AutosetRfModes(Structure):
    _fields_ = [
        ('autoset_mode_id', c_uint32),
        ('rf_mode_list', POINTER(c_uint32)),
        ('rf_modes_length', c_size_t),
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


class InventoryRoundSequence(Structure):
    _fields_ = [
        ('type_id', c_uint32),
        ('configs', c_void_p),
        ('count', c_size_t),
    ]


class Ex10AutosetModes(Structure):
    _fields_ = [
        ('get_autoset_rf_modes', CFUNCTYPE(POINTER(AutosetRfModes), c_uint32)),
        ('get_autoset_mode_id', CFUNCTYPE(c_uint32, c_uint32, c_uint32)),
        ('init_autoset_basic_inventory_sequence', CFUNCTYPE(Ex10Result, POINTER(InventoryRoundConfigBasic), c_size_t, POINTER(AutosetRfModes), c_uint8, c_int16, c_uint8, c_uint32)),
    ]


class Ex10InventorySequenceUseCase(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(Ex10Result)),
        ('deinit', CFUNCTYPE(Ex10Result)),
        ('register_packet_subscriber_callback', CFUNCTYPE(None, CFUNCTYPE(None, POINTER(EventFifoPacket), POINTER(Ex10Result)))),
        ('enable_packet_filter', CFUNCTYPE(None, c_bool)),
        ('get_inventory_sequence', CFUNCTYPE(POINTER(InventoryRoundSequence))),
        ('get_inventory_round', CFUNCTYPE(POINTER(InventoryRoundConfigBasic))),
        ('run_inventory_sequence', CFUNCTYPE(Ex10Result, POINTER(InventoryRoundSequence))),
    ]


class PowerConfigs(Structure):
    _fields_ = [
        ('tx_atten', c_uint8),
        ('tx_scalar', c_int16),
        ('dc_offset', c_int32),
        ('adc_target', c_uint16),
        ('loop_stop_threshold', c_uint16),
        ('op_error_threshold', c_uint16),
        ('loop_gain_divisor', c_uint16),
        ('max_iterations', c_uint32),
        ('power_detector_adc', c_uint32),
    ]


class Ex10RegulatoryTimers(Structure):
    _fields_ = [
        ('nominal_ms', c_uint16),
        ('extended_ms', c_uint16),
        ('regulatory_ms', c_uint16),
        ('off_same_channel_ms', c_uint16),
    ]


class CwConfig(Structure):
    _fields_ = [
        ('gpio', GpioPinsSetClear),
        ('rf_mode', c_uint32),
        ('power', PowerConfigs),
        ('synth', RfSynthesizerControlFields),
        ('timer', Ex10RegulatoryTimers),
    ]


class InfoFromPackets(Structure):
    _fields_ = [
        ('gen2_transactions', c_size_t),
        ('total_singulations', c_size_t),
        ('total_tid_count', c_size_t),
        ('times_halted', c_size_t),
        ('access_tag', TagReadData),
    ]


class InventoryHelperParams(Structure):
    _fields_ = [
        ('antenna', c_uint8),
        ('rf_mode', c_uint32),
        ('tx_power_cdbm', c_int16),
        ('inventory_config', POINTER(InventoryRoundControlFields)),
        ('inventory_config_2', POINTER(InventoryRoundControl_2Fields)),
        ('send_selects', c_bool),
        ('remain_on', c_bool),
        ('dual_target', c_bool),
        ('inventory_duration_ms', c_uint32),
        ('packet_info', POINTER(InfoFromPackets)),
        ('verbose', c_bool),
    ]


class ContInventoryHelperParams(Structure):
    _fields_ = [
        ('inventory_params', POINTER(InventoryHelperParams)),
        ('stop_conditions', POINTER(StopConditions)),
        ('summary_packet', POINTER(ContinuousInventorySummary)),
    ]


class Ex10GpioInterface(Structure):
    _fields_ = [
        ('initialize', CFUNCTYPE(c_int32, c_bool, c_bool, c_bool)),
        ('cleanup', CFUNCTYPE(None)),
        ('set_board_power', CFUNCTYPE(c_int32, c_bool)),
        ('get_board_power', CFUNCTYPE(c_bool)),
        ('set_ex10_enable', CFUNCTYPE(c_int32, c_bool)),
        ('get_ex10_enable', CFUNCTYPE(c_bool)),
        ('register_irq_callback', CFUNCTYPE(c_int32, CFUNCTYPE(None))),
        ('deregister_irq_callback', CFUNCTYPE(c_int32)),
        ('irq_monitor_callback_enable', CFUNCTYPE(None, c_bool)),
        ('irq_monitor_callback_is_enabled', CFUNCTYPE(c_bool)),
        ('irq_enable', CFUNCTYPE(None, c_bool)),
        ('thread_is_irq_monitor', CFUNCTYPE(c_bool)),
        ('assert_reset_n', CFUNCTYPE(c_int32)),
        ('deassert_reset_n', CFUNCTYPE(c_int32)),
        ('reset_device', CFUNCTYPE(c_int32)),
        ('assert_ready_n', CFUNCTYPE(c_int32)),
        ('release_ready_n', CFUNCTYPE(c_int32)),
        ('busy_wait_ready_n', CFUNCTYPE(c_int32, c_uint32)),
        ('ready_n_pin_get', CFUNCTYPE(c_int32)),
    ]


class HostInterface(Structure):
    _fields_ = [
        ('open', CFUNCTYPE(c_int32, c_uint32)),
        ('close', CFUNCTYPE(None)),
        ('read', CFUNCTYPE(c_int32, c_void_p, c_size_t)),
        ('write', CFUNCTYPE(c_int32, c_void_p, c_size_t)),
    ]


class UartInterface(Structure):
    _fields_ = [
        ('open', CFUNCTYPE(c_int32, c_uint32)),
        ('close', CFUNCTYPE(None)),
        ('read', CFUNCTYPE(c_int32, c_void_p, c_size_t)),
        ('write', CFUNCTYPE(c_int32, c_void_p, c_size_t)),
    ]


class Ex10DriverList(Structure):
    _fields_ = [
        ('gpio_if', Ex10GpioInterface),
        ('host_if', HostInterface),
        ('uart_if', UartInterface),
    ]


class Ex10FirmwareVersion(Structure):
    _fields_ = [
        ('version_string[VERSION_STRING_REG_LENGTH]', c_char),
        ('git_hash_bytes[GIT_HASH_REG_LENGTH]', c_uint8),
        ('build_number', c_uint32),
    ]


class Ex10Protocol(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(None, POINTER(Ex10DriverList))),
        ('init_ex10', CFUNCTYPE(Ex10Result)),
        ('deinit', CFUNCTYPE(Ex10Result)),
        ('register_fifo_data_callback', CFUNCTYPE(Ex10Result, CFUNCTYPE(None, POINTER(FifoBufferNode)))),
        ('register_interrupt_callback', CFUNCTYPE(Ex10Result, InterruptMaskFields, CFUNCTYPE(c_bool, InterruptStatusFields))),
        ('unregister_fifo_data_callback', CFUNCTYPE(None)),
        ('unregister_interrupt_callback', CFUNCTYPE(Ex10Result)),
        ('enable_interrupt_handlers', CFUNCTYPE(None, c_bool)),
        ('read', CFUNCTYPE(Ex10Result, POINTER(RegisterInfo), c_void_p)),
        ('test_read', CFUNCTYPE(Ex10Result, c_uint32, c_uint16, c_void_p)),
        ('read_index', CFUNCTYPE(Ex10Result, POINTER(RegisterInfo), c_void_p, c_uint8)),
        ('write', CFUNCTYPE(Ex10Result, POINTER(RegisterInfo), c_void_p)),
        ('write_index', CFUNCTYPE(Ex10Result, POINTER(RegisterInfo), c_void_p, c_uint8)),
        ('read_partial', CFUNCTYPE(Ex10Result, c_uint16, c_uint16, c_void_p)),
        ('write_partial', CFUNCTYPE(Ex10Result, c_uint16, c_uint16, c_void_p)),
        ('write_multiple', CFUNCTYPE(Ex10Result, POINTER(RegisterInfo), c_void_p, c_size_t)),
        ('get_write_multiple_stored_settings', CFUNCTYPE(Ex10Result, POINTER(RegisterInfo), c_void_p, c_size_t, POINTER(ByteSpan), POINTER(c_size_t))),
        ('read_info_page_buffer', CFUNCTYPE(Ex10Result, c_uint32, POINTER(c_uint8))),
        ('read_multiple', CFUNCTYPE(Ex10Result, POINTER(RegisterInfo), c_void_p, c_size_t)),
        ('start_op', CFUNCTYPE(Ex10Result, c_uint32)),
        ('stop_op', CFUNCTYPE(Ex10Result)),
        ('is_op_currently_running', CFUNCTYPE(c_bool)),
        ('wait_op_completion', CFUNCTYPE(Ex10Result)),
        ('wait_op_completion_with_timeout', CFUNCTYPE(Ex10Result, c_uint32)),
        ('read_ops_status_reg', CFUNCTYPE(Ex10Result, POINTER(OpsStatusFields))),
        ('reset', CFUNCTYPE(Ex10Result, c_uint32)),
        ('set_event_fifo_threshold', CFUNCTYPE(Ex10Result, c_size_t)),
        ('insert_fifo_event', CFUNCTYPE(Ex10Result, c_bool, POINTER(EventFifoPacket))),
        ('get_running_location', CFUNCTYPE(c_uint32)),
        ('get_analog_rx_config', CFUNCTYPE(RxGainControlFields)),
        ('write_info_page', CFUNCTYPE(Ex10Result, c_uint32, c_void_p, c_size_t, c_uint32)),
        ('write_calibration_page', CFUNCTYPE(Ex10Result, POINTER(c_uint8), c_size_t)),
        ('erase_info_page', CFUNCTYPE(Ex10Result, c_uint32, c_uint32)),
        ('erase_calibration_page', CFUNCTYPE(Ex10Result)),
        ('write_stored_settings_page', CFUNCTYPE(Ex10Result, POINTER(c_uint8), c_size_t)),
        ('upload_image', CFUNCTYPE(Ex10Result, c_uint8, ConstByteSpan)),
        ('upload_start', CFUNCTYPE(Ex10Result, c_uint8, c_size_t, ConstByteSpan)),
        ('upload_continue', CFUNCTYPE(Ex10Result, ConstByteSpan)),
        ('upload_complete', CFUNCTYPE(Ex10Result)),
        ('revalidate_image', CFUNCTYPE(ImageValidityFields)),
        ('test_transfer', CFUNCTYPE(Ex10Result, POINTER(ConstByteSpan), POINTER(ByteSpan), c_bool)),
        ('wait_for_event_fifo_empty', CFUNCTYPE(Ex10Result)),
        ('get_device_info', CFUNCTYPE(Ex10Result, POINTER(DeviceInfoFields))),
        ('get_application_version', CFUNCTYPE(Ex10Result, POINTER(Ex10FirmwareVersion))),
        ('get_bootloader_version', CFUNCTYPE(Ex10Result, POINTER(Ex10FirmwareVersion))),
        ('get_image_validity', CFUNCTYPE(ImageValidityFields)),
        ('get_remain_reason', CFUNCTYPE(Ex10Result, POINTER(RemainReasonFields))),
        ('get_sku', CFUNCTYPE(c_uint32)),
    ]


class CdacRange(Structure):
    _fields_ = [
        ('center', c_int8),
        ('limit', c_uint8),
        ('step_size', c_uint8),
    ]


class SjcResult(Structure):
    _fields_ = [
        ('residue', c_int32),
        ('cdac', c_int8),
        ('cdac_limited', c_bool),
    ]


class SjcResultPair(Structure):
    _fields_ = [
        ('i', SjcResult),
        ('q', SjcResult),
    ]


class Ex10SjcAccessor(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(None, POINTER(Ex10Protocol))),
        ('set_sjc_control', CFUNCTYPE(None, c_uint8, c_uint8, c_bool, c_bool, c_uint8)),
        ('set_analog_rx_config', CFUNCTYPE(None)),
        ('set_settling_time', CFUNCTYPE(None, c_uint16, c_uint16)),
        ('set_cdac_range', CFUNCTYPE(None, CdacRange, CdacRange)),
        ('set_residue_threshold', CFUNCTYPE(None, c_uint16)),
        ('set_cdac_to_find_solution', CFUNCTYPE(None)),
        ('get_sjc_results', CFUNCTYPE(SjcResultPair)),
    ]


class SynthesizerParams(Structure):
    _fields_ = [
        ('freq_khz', c_uint32),
        ('r_divider_index', c_uint8),
        ('n_divider', c_uint16),
    ]


class AggregateRunOpFormat(Structure):
    _fields_ = [
        ('op_to_run', c_uint8),
    ]


class AggregateIdentifierFormat(Structure):
    _fields_ = [
        ('identifier', c_uint16),
    ]


class AggregateGoToIndexFormat(Structure):
    _fields_ = [
        ('jump_index', c_uint16),
        ('repeat_counter', c_uint8),
    ]


class TxCommandInfo(Structure):
    _fields_ = [
        ('decoded_buffer[TxCommandDecodeBufferSize]', c_uint8),
        ('encoded_buffer[TxCommandEncodeBufferSize]', c_uint8),
        ('encoded_command', BitSpan),
        ('decoded_command', Gen2CommandSpec),
        ('valid', c_bool),
        ('transaction_id', c_uint8),
    ]


class ContinuousInventoryState(Structure):
    _fields_ = [
        ('state', c_uint32),
        ('done_reason', c_uint32),
        ('initial_inventory_config', InventoryRoundControlFields),
        ('previous_q', c_uint8),
        ('min_q_count', c_uint8),
        ('queries_since_valid_epc_count', c_uint8),
        ('stop_reason', c_uint32),
        ('round_count', c_size_t),
        ('tag_count', c_size_t),
        ('target', c_uint8),
        ('inventory_round_iter', c_size_t),
    ]


class Ex10WriteFormat(Structure):
    _pack_ = 1
    _fields_ = [
        ('address', c_uint16),
        ('length', c_uint16),
        ('data', POINTER(c_uint8)),
    ]


class Ex10InsertFifoEventFormat(Structure):
    _pack_ = 1
    _fields_ = [
        ('trigger_irq', c_uint8),
        ('packet', POINTER(c_uint8)),
    ]


class Ex10ResetFormat(Structure):
    _pack_ = 1
    _fields_ = [
        ('destination', c_uint8),
    ]


class AggregateInstructionData(Union):
    _fields_ = [
        ('write_format', Ex10WriteFormat),
        ('reset_format', Ex10ResetFormat),
        ('insert_fifo_event_format', Ex10InsertFifoEventFormat),
        ('run_op_format', AggregateRunOpFormat),
        ('go_to_index_format', AggregateGoToIndexFormat),
        ('identifier_format', AggregateIdentifierFormat),
    ]


class AggregateOpInstruction(Structure):
    _fields_ = [
        ('instruction_type', c_uint32),
        ('instruction_data', POINTER(AggregateInstructionData)),
    ]


class Ex10AggregateOpBuilder(Structure):
    _fields_ = [
        ('append_instruction', CFUNCTYPE(c_bool, AggregateOpInstruction, POINTER(ByteSpan))),
        ('clear_buffer', CFUNCTYPE(c_bool)),
        ('set_buffer', CFUNCTYPE(c_bool, POINTER(ByteSpan))),
        ('get_instruction_from_index', CFUNCTYPE(c_size_t, c_size_t, POINTER(ByteSpan), POINTER(AggregateOpInstruction))),
        ('print_buffer', CFUNCTYPE(None, POINTER(ByteSpan))),
        ('print_aggregate_op_errors', CFUNCTYPE(None, POINTER(AggregateOpSummary))),
        ('append_reg_write', CFUNCTYPE(c_bool, POINTER(RegisterInfo), POINTER(ConstByteSpan), POINTER(ByteSpan))),
        ('append_reset', CFUNCTYPE(c_bool, c_uint8, POINTER(ByteSpan))),
        ('append_insert_fifo_event', CFUNCTYPE(c_bool, c_bool, POINTER(EventFifoPacket), POINTER(ByteSpan))),
        ('append_op_run', CFUNCTYPE(c_bool, c_uint32, POINTER(ByteSpan))),
        ('append_go_to_instruction', CFUNCTYPE(c_bool, c_uint16, c_uint8, POINTER(ByteSpan))),
        ('append_identifier', CFUNCTYPE(c_bool, c_uint16, POINTER(ByteSpan))),
        ('append_exit_instruction', CFUNCTYPE(c_bool, POINTER(ByteSpan))),
        ('append_set_rf_mode', CFUNCTYPE(c_bool, c_uint16, POINTER(ByteSpan))),
        ('append_measure_aux_adc', CFUNCTYPE(c_bool, c_uint32, c_uint8, POINTER(ByteSpan))),
        ('append_set_gpio', CFUNCTYPE(c_bool, c_uint32, c_uint32, POINTER(ByteSpan))),
        ('append_set_clear_gpio_pins', CFUNCTYPE(c_bool, POINTER(GpioPinsSetClear), POINTER(ByteSpan))),
        ('append_lock_synthesizer', CFUNCTYPE(c_bool, c_uint8, c_uint16, POINTER(ByteSpan))),
        ('append_sjc_settings', CFUNCTYPE(c_bool, POINTER(SjcControlFields), POINTER(SjcGainControlFields), POINTER(SjcInitialSettlingTimeFields), POINTER(SjcResidueSettlingTimeFields), POINTER(SjcCdacIFields), POINTER(SjcResidueThresholdFields), POINTER(ByteSpan))),
        ('append_run_sjc', CFUNCTYPE(c_bool, POINTER(ByteSpan))),
        ('append_set_tx_coarse_gain', CFUNCTYPE(c_bool, c_uint8, POINTER(ByteSpan))),
        ('append_set_tx_fine_gain', CFUNCTYPE(c_bool, c_int16, POINTER(ByteSpan))),
        ('append_set_regulatory_timers', CFUNCTYPE(c_bool, POINTER(Ex10RegulatoryTimers), POINTER(ByteSpan))),
        ('append_tx_ramp_up', CFUNCTYPE(c_bool, c_int32, POINTER(ByteSpan))),
        ('append_power_control', CFUNCTYPE(c_bool, POINTER(PowerConfigs), POINTER(ByteSpan))),
        ('append_start_log_test', CFUNCTYPE(c_bool, c_uint32, c_uint16, POINTER(ByteSpan))),
        ('append_set_atest_mux', CFUNCTYPE(c_bool, c_uint32, c_uint32, c_uint32, c_uint32, POINTER(ByteSpan))),
        ('append_set_aux_dac', CFUNCTYPE(c_bool, c_uint8, c_uint8, POINTER(c_uint16), POINTER(ByteSpan))),
        ('append_tx_ramp_down', CFUNCTYPE(c_bool, POINTER(ByteSpan))),
        ('append_radio_power_control', CFUNCTYPE(c_bool, c_bool, POINTER(ByteSpan))),
        ('append_set_analog_rx_config', CFUNCTYPE(c_bool, POINTER(RxGainControlFields), POINTER(ByteSpan))),
        ('append_measure_rssi', CFUNCTYPE(c_bool, POINTER(ByteSpan), c_uint8)),
        ('append_hpf_override_test', CFUNCTYPE(c_bool, POINTER(ByteSpan), c_uint8)),
        ('append_listen_before_talk', CFUNCTYPE(c_bool, POINTER(ByteSpan), c_uint8, c_uint16, c_int32, c_uint8)),
        ('append_start_timer_op', CFUNCTYPE(c_bool, c_uint32, POINTER(ByteSpan))),
        ('append_wait_timer_op', CFUNCTYPE(c_bool, POINTER(ByteSpan))),
        ('append_start_event_fifo_test', CFUNCTYPE(c_bool, c_uint32, c_uint8, POINTER(ByteSpan))),
        ('append_enable_sdd_logs', CFUNCTYPE(c_bool, LogEnablesFields, c_uint8, POINTER(ByteSpan))),
        ('append_start_inventory_round', CFUNCTYPE(c_bool, POINTER(InventoryRoundControlFields), POINTER(InventoryRoundControl_2Fields), POINTER(ByteSpan))),
        ('append_start_prbs', CFUNCTYPE(c_bool, POINTER(ByteSpan))),
        ('append_start_ber_test', CFUNCTYPE(c_bool, c_uint16, c_uint16, c_bool, POINTER(ByteSpan))),
        ('append_ramp_transmit_power', CFUNCTYPE(c_bool, POINTER(PowerConfigs), POINTER(Ex10RegulatoryTimers), POINTER(ByteSpan))),
        ('append_droop_compensation', CFUNCTYPE(c_bool, POINTER(PowerDroopCompensationFields), POINTER(ByteSpan))),
    ]


class Ex10Helpers(Structure):
    _fields_ = [
        ('print_command_result_fields', CFUNCTYPE(None, POINTER(CommandResultFields))),
        ('check_gen2_error', CFUNCTYPE(c_bool, POINTER(Gen2Reply))),
        ('print_aggregate_op_errors', CFUNCTYPE(None, AggregateOpSummary)),
        ('discard_packets', CFUNCTYPE(c_size_t, c_bool, c_bool, c_bool)),
        ('inventory_halted', CFUNCTYPE(c_bool)),
        ('clear_info_from_packets', CFUNCTYPE(None, POINTER(InfoFromPackets))),
        ('examine_packets', CFUNCTYPE(None, POINTER(EventFifoPacket), POINTER(InfoFromPackets))),
        ('deep_copy_packet', CFUNCTYPE(c_bool, POINTER(EventFifoPacket), POINTER(EventFifoPacket))),
        ('simple_inventory', CFUNCTYPE(c_uint32, POINTER(InventoryHelperParams))),
        ('continuous_inventory', CFUNCTYPE(c_uint32, POINTER(ContInventoryHelperParams))),
        ('copy_tag_read_data', CFUNCTYPE(c_bool, POINTER(TagReadData), POINTER(TagReadFields))),
        ('get_remain_reason_string', CFUNCTYPE(c_char_p, c_uint32)),
        ('swap_bytes', CFUNCTYPE(c_uint16, c_uint16)),
        ('read_rssi_value_from_op', CFUNCTYPE(c_uint16, c_uint8)),
        ('send_single_halted_command', CFUNCTYPE(Ex10Result, POINTER(Gen2CommandSpec))),
        ('fill_u32', CFUNCTYPE(None, POINTER(c_uint32), c_uint32, c_size_t)),
    ]


class Ex10LbtHelpers(Structure):
    _fields_ = [
        ('continuous_inventory_lbt', CFUNCTYPE(c_uint32, POINTER(ContInventoryHelperParams))),
    ]


class Ex10Ops(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(None)),
        ('release', CFUNCTYPE(None)),
        ('read_ops_status', CFUNCTYPE(OpsStatusFields)),
        ('start_log_test', CFUNCTYPE(Ex10Result, c_uint32, c_uint16)),
        ('set_atest_mux', CFUNCTYPE(Ex10Result, c_uint32, c_uint32, c_uint32, c_uint32)),
        ('route_atest_pga3', CFUNCTYPE(Ex10Result)),
        ('measure_aux_adc', CFUNCTYPE(Ex10Result, c_uint32, c_uint8)),
        ('set_aux_dac', CFUNCTYPE(Ex10Result, c_uint8, c_uint8, POINTER(c_uint16))),
        ('set_rf_mode', CFUNCTYPE(Ex10Result, c_uint32)),
        ('tx_ramp_up', CFUNCTYPE(Ex10Result, c_int32)),
        ('tx_ramp_down', CFUNCTYPE(Ex10Result)),
        ('set_tx_coarse_gain', CFUNCTYPE(Ex10Result, c_uint8)),
        ('set_tx_fine_gain', CFUNCTYPE(Ex10Result, c_int16)),
        ('radio_power_control', CFUNCTYPE(Ex10Result, c_bool)),
        ('set_analog_rx_config', CFUNCTYPE(Ex10Result, POINTER(RxGainControlFields))),
        ('measure_rssi', CFUNCTYPE(Ex10Result, c_uint8)),
        ('run_listen_before_talk', CFUNCTYPE(Ex10Result, POINTER(LbtControlFields), POINTER(RxGainControlFields), POINTER(LbtOffsetFields), POINTER(RfSynthesizerControlFields), c_uint8)),
        ('start_timer_op', CFUNCTYPE(Ex10Result, c_uint32)),
        ('wait_timer_op', CFUNCTYPE(Ex10Result)),
        ('lock_synthesizer', CFUNCTYPE(Ex10Result, c_uint8, c_uint16)),
        ('start_event_fifo_test', CFUNCTYPE(Ex10Result, c_uint32, c_uint8)),
        ('enable_sdd_logs', CFUNCTYPE(Ex10Result, LogEnablesFields, c_uint8)),
        ('send_gen2_halted_sequence', CFUNCTYPE(Ex10Result)),
        ('continue_from_halted', CFUNCTYPE(Ex10Result, c_bool)),
        ('run_sjc', CFUNCTYPE(Ex10Result)),
        ('wait_op_completion', CFUNCTYPE(Ex10Result)),
        ('wait_op_completion_with_timeout', CFUNCTYPE(Ex10Result, c_uint32)),
        ('run_aggregate_op', CFUNCTYPE(Ex10Result)),
        ('stop_op', CFUNCTYPE(Ex10Result)),
        ('get_gpio', CFUNCTYPE(GpioControlFields)),
        ('set_gpio', CFUNCTYPE(Ex10Result, c_uint32, c_uint32)),
        ('set_clear_gpio_pins', CFUNCTYPE(Ex10Result, POINTER(GpioPinsSetClear))),
        ('start_inventory_round', CFUNCTYPE(Ex10Result, POINTER(InventoryRoundControlFields), POINTER(InventoryRoundControl_2Fields))),
        ('start_prbs', CFUNCTYPE(Ex10Result)),
        ('start_hpf_override_test_op', CFUNCTYPE(Ex10Result, POINTER(HpfOverrideSettingsFields))),
        ('run_etsi_burst', CFUNCTYPE(Ex10Result)),
        ('start_ber_test', CFUNCTYPE(Ex10Result, c_uint16, c_uint16, c_bool)),
        ('send_select', CFUNCTYPE(Ex10Result)),
        ('get_device_time', CFUNCTYPE(c_uint32)),
        ('run_power_control_loop', CFUNCTYPE(Ex10Result, POINTER(PowerConfigs))),
    ]


class Ex10Reader(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(None, c_uint32)),
        ('init_ex10', CFUNCTYPE(Ex10Result)),
        ('read_calibration', CFUNCTYPE(None)),
        ('deinit', CFUNCTYPE(Ex10Result)),
        ('continuous_inventory', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int16, POINTER(InventoryRoundControlFields), POINTER(InventoryRoundControl_2Fields), c_bool, POINTER(StopConditions), c_bool, c_bool)),
        ('inventory', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int16, POINTER(InventoryRoundControlFields), POINTER(InventoryRoundControl_2Fields), c_bool, c_bool)),
        ('fifo_data_handler', CFUNCTYPE(None, POINTER(FifoBufferNode))),
        ('interrupt_handler', CFUNCTYPE(c_bool, InterruptStatusFields)),
        ('packet_peek', CFUNCTYPE(POINTER(EventFifoPacket))),
        ('packet_remove', CFUNCTYPE(None)),
        ('packets_available', CFUNCTYPE(c_bool)),
        ('continue_from_halted', CFUNCTYPE(Ex10Result, c_bool)),
        ('cw_test', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int16, c_uint32, c_bool)),
        ('prbs_test', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int16, c_uint32, c_bool)),
        ('ber_test', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int16, c_uint32, c_uint16, c_uint16, c_bool)),
        ('etsi_burst_test', CFUNCTYPE(Ex10Result, POINTER(InventoryRoundControlFields), POINTER(InventoryRoundControl_2Fields), c_uint8, c_uint32, c_int16, c_uint16, c_uint16, c_uint32)),
        ('insert_fifo_event', CFUNCTYPE(Ex10Result, c_bool, POINTER(EventFifoPacket))),
        ('enable_sdd_logs', CFUNCTYPE(Ex10Result, LogEnablesFields, c_uint8)),
        ('stop_transmitting', CFUNCTYPE(Ex10Result)),
        ('build_cw_configs', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int16, c_uint32, c_bool, POINTER(CwConfig))),
        ('get_current_compensated_rssi', CFUNCTYPE(c_int16, c_uint16)),
        ('get_current_rssi_log2', CFUNCTYPE(c_uint16, c_int16)),
        ('get_listen_before_talk_rssi', CFUNCTYPE(c_int16, c_uint8, c_uint32, c_int32, c_uint8, c_bool)),
        ('listen_before_talk_multi', CFUNCTYPE(Ex10Result, c_uint8, c_uint8, LbtControlFields, POINTER(c_uint32), POINTER(c_int32), POINTER(c_int16))),
        ('get_current_analog_rx_fields', CFUNCTYPE(POINTER(RxGainControlFields))),
        ('get_continuous_inventory_state', CFUNCTYPE(POINTER(ContinuousInventoryState))),
    ]


class Ex10PowerModes(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(None)),
        ('deinit', CFUNCTYPE(None)),
        ('set_power_mode', CFUNCTYPE(Ex10Result, c_uint32)),
        ('get_power_mode', CFUNCTYPE(c_uint32)),
    ]


class Ex10PowerTransactor(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(None)),
        ('deinit', CFUNCTYPE(None)),
        ('power_up_to_application', CFUNCTYPE(c_int)),
        ('power_up_to_bootloader', CFUNCTYPE(None)),
        ('power_down', CFUNCTYPE(None)),
    ]


class Ex10Gen2TxCommandManager(Structure):
    _fields_ = [
        ('clear_local_sequence', CFUNCTYPE(None)),
        ('clear_command_in_local_sequence', CFUNCTYPE(Ex10Result, c_uint8, POINTER(c_size_t))),
        ('clear_sequence', CFUNCTYPE(None)),
        ('init', CFUNCTYPE(None)),
        ('write_sequence', CFUNCTYPE(Ex10Result)),
        ('write_select_enables', CFUNCTYPE(Ex10Result, c_void_p, c_uint8, POINTER(c_size_t))),
        ('write_halted_enables', CFUNCTYPE(Ex10Result, c_void_p, c_uint8, POINTER(c_size_t))),
        ('write_auto_access_enables', CFUNCTYPE(Ex10Result, c_void_p, c_uint8, POINTER(c_size_t))),
        ('append_encoded_command', CFUNCTYPE(Ex10Result, POINTER(BitSpan), c_uint8, POINTER(c_size_t))),
        ('encode_and_append_command', CFUNCTYPE(Ex10Result, POINTER(Gen2CommandSpec), c_uint8, POINTER(c_size_t))),
        ('read_device_to_local_sequence', CFUNCTYPE(Ex10Result)),
        ('print_local_sequence', CFUNCTYPE(None)),
        ('dump_control_registers', CFUNCTYPE(None)),
        ('get_local_sequence', CFUNCTYPE(POINTER(TxCommandInfo))),
    ]


class Ex10Random(Structure):
    _fields_ = [
        ('setup_random', CFUNCTYPE(None)),
        ('get_random', CFUNCTYPE(c_int)),
    ]


class Ex10TimeHelpers(Structure):
    _fields_ = [
        ('time_now', CFUNCTYPE(c_uint32)),
        ('time_elapsed', CFUNCTYPE(c_uint32, c_uint32)),
        ('busy_wait_ms', CFUNCTYPE(None, c_uint32)),
        ('wait_ms', CFUNCTYPE(None, c_uint32)),
    ]


class Ex10EventFifoPrinter(Structure):
    _fields_ = [
        ('print_packets', CFUNCTYPE(None, POINTER(EventFifoPacket))),
        ('print_event_tag_read_compensated_rssi', CFUNCTYPE(None, POINTER(EventFifoPacket), c_uint32, c_uint8, c_uint32, c_uint16)),
        ('print_tag_read_data', CFUNCTYPE(None, POINTER(TagReadData))),
    ]


class Ex10Gen2Commands(Structure):
    _fields_ = [
        ('encode_gen2_command', CFUNCTYPE(Ex10Result, POINTER(Gen2CommandSpec), POINTER(BitSpan))),
        ('decode_gen2_command', CFUNCTYPE(Ex10Result, POINTER(Gen2CommandSpec), POINTER(BitSpan))),
        ('decode_reply', CFUNCTYPE(c_bool, c_uint32, POINTER(EventFifoPacket), POINTER(Gen2Reply))),
        ('check_error', CFUNCTYPE(c_bool, Gen2Reply)),
        ('print_reply', CFUNCTYPE(None, Gen2Reply)),
        ('get_gen2_tx_control_config', CFUNCTYPE(Ex10Result, POINTER(Gen2CommandSpec), POINTER(Gen2TxnControlsFields))),
        ('get_ebv_bit_len', CFUNCTYPE(c_size_t, c_size_t)),
        ('bit_pack', CFUNCTYPE(c_size_t, POINTER(c_uint8), c_size_t, c_uint32, c_size_t)),
        ('bit_pack_ebv', CFUNCTYPE(c_size_t, POINTER(c_uint8), c_size_t, c_size_t)),
        ('bit_unpack', CFUNCTYPE(POINTER(c_uint8), POINTER(c_uint8), c_size_t, c_size_t)),
        ('bit_unpack_ebv', CFUNCTYPE(c_uint32, POINTER(c_uint8), c_size_t)),
        ('bit_unpack_msb', CFUNCTYPE(POINTER(c_uint8), POINTER(c_uint8), c_size_t, c_size_t)),
        ('ebv_length_decode', CFUNCTYPE(c_uint32, POINTER(c_uint8), c_uint32)),
        ('le_bytes_to_uint16', CFUNCTYPE(c_uint16, c_void_p)),
    ]


class Ex10EventParser(Structure):
    _fields_ = [
        ('get_tag_read_fields', CFUNCTYPE(TagReadFields, c_void_p, c_size_t, c_uint32, c_uint8)),
        ('get_static_payload_length', CFUNCTYPE(c_size_t, c_uint32)),
        ('get_packet_type_valid', CFUNCTYPE(c_bool, c_uint32)),
        ('parse_event_packet', CFUNCTYPE(EventFifoPacket, POINTER(ConstByteSpan))),
        ('make_packet_header', CFUNCTYPE(PacketHeader, c_uint32)),
    ]


class Ex10Version(Structure):
    _fields_ = [
        ('get_bootloader_info', CFUNCTYPE(c_size_t, c_char_p, c_size_t)),
        ('get_application_info', CFUNCTYPE(c_size_t, c_char_p, c_size_t, POINTER(ImageValidityFields), POINTER(RemainReasonFields))),
        ('get_sku', CFUNCTYPE(c_uint32)),
        ('get_device_info', CFUNCTYPE(c_char_p)),
    ]


class Ex10RegulatoryChannels(Structure):
    _fields_ = [
        ('start_freq_khz', c_uint32),
        ('spacing_khz', c_uint32),
        ('count', c_uint16),
        ('usable', POINTER(c_uint16)),
        ('usable_count', c_uint16),
        ('random_hop', c_bool),
    ]


class Ex10Region(Structure):
    _fields_ = [
        ('region_id', c_uint32),
        ('regulatory_timers', Ex10RegulatoryTimers),
        ('regulatory_channels', Ex10RegulatoryChannels),
        ('pll_divider', c_uint32),
        ('rf_filter', c_uint32),
        ('max_power_cdbm', c_int32),
    ]


class Ex10RegionRegulatory(Structure):
    _fields_ = [
        ('set_region', CFUNCTYPE(None, POINTER(Ex10Region))),
        ('get_region', CFUNCTYPE(POINTER(Ex10Region))),
        ('get_regulatory_timers', CFUNCTYPE(None, c_uint16, c_uint32, POINTER(Ex10RegulatoryTimers))),
        ('regulatory_timer_set_start', CFUNCTYPE(None, c_uint16, c_uint32)),
        ('regulatory_timer_set_end', CFUNCTYPE(None, c_uint16, c_uint32)),
        ('regulatory_timer_clear', CFUNCTYPE(None)),
    ]


class Ex10ActiveRegion(Structure):
    _fields_ = [
        ('set_region', CFUNCTYPE(Ex10Result, c_uint32, c_uint32)),
        ('get_region_id', CFUNCTYPE(c_uint32)),
        ('update_active_channel', CFUNCTYPE(None)),
        ('get_channel_table_size', CFUNCTYPE(c_uint16)),
        ('get_active_channel_khz', CFUNCTYPE(c_uint32)),
        ('get_next_channel_khz', CFUNCTYPE(c_uint32)),
        ('get_adjacent_channel_khz', CFUNCTYPE(Ex10Result, c_uint16, c_int16, POINTER(c_uint32))),
        ('get_channel_spacing', CFUNCTYPE(c_uint32)),
        ('get_active_channel_index', CFUNCTYPE(c_uint16)),
        ('get_next_channel_index', CFUNCTYPE(c_uint16)),
        ('get_channel_index', CFUNCTYPE(c_uint16, c_uint32)),
        ('get_next_channel_regulatory_timers', CFUNCTYPE(Ex10Result, POINTER(Ex10RegulatoryTimers))),
        ('get_regulatory_timers', CFUNCTYPE(Ex10Result, POINTER(Ex10RegulatoryTimers))),
        ('get_synthesizer_params', CFUNCTYPE(Ex10Result, c_uint32, POINTER(SynthesizerParams))),
        ('get_synthesizer_frequency_khz', CFUNCTYPE(Ex10Result, c_uint8, c_uint16, POINTER(c_uint32))),
        ('get_rf_filter', CFUNCTYPE(c_uint32)),
        ('get_pll_r_divider', CFUNCTYPE(c_uint32)),
        ('calculate_n_divider', CFUNCTYPE(c_uint16, c_uint32, c_uint32)),
        ('calculate_r_divider_index', CFUNCTYPE(c_uint8, c_uint32)),
        ('build_channel_table', CFUNCTYPE(Ex10Result, POINTER(Ex10RegulatoryChannels), POINTER(c_uint16), POINTER(c_uint16))),
        ('set_single_frequency', CFUNCTYPE(None, c_uint32)),
        ('disable_regulatory_timers', CFUNCTYPE(None)),
        ('reenable_regulatory_timers', CFUNCTYPE(None)),
        ('regulatory_timer_set_start', CFUNCTYPE(None, c_uint32)),
        ('regulatory_timer_set_end', CFUNCTYPE(None, c_uint32)),
        ('update_channel_time_tracking', CFUNCTYPE(Ex10Result)),
    ]


class Ex10Regulatory(Structure):
    _fields_ = [
        ('set_region', CFUNCTYPE(None, c_uint32, POINTER(Ex10Region))),
        ('get_region', CFUNCTYPE(POINTER(Ex10Region), c_uint32)),
        ('get_regulatory_timers', CFUNCTYPE(None, c_uint32, c_uint16, c_uint32, POINTER(Ex10RegulatoryTimers))),
        ('regulatory_timer_set_start', CFUNCTYPE(None, c_uint32, c_uint16, c_uint32)),
        ('regulatory_timer_set_end', CFUNCTYPE(None, c_uint32, c_uint16, c_uint32)),
        ('calculate_channel_khz', CFUNCTYPE(c_uint32, c_uint32, c_uint16)),
        ('calculate_channel_index', CFUNCTYPE(c_uint16, c_uint32, c_uint32)),
    ]


class Ex10DefaultRegionNames(Structure):
    _fields_ = [
        ('get_region_id', CFUNCTYPE(c_uint32, c_char_p)),
    ]


class Ex10RfPower(Structure):
    _fields_ = [
        ('init_ex10', CFUNCTYPE(Ex10Result)),
        ('get_cw_is_on', CFUNCTYPE(c_bool)),
        ('stop_op_and_ramp_down', CFUNCTYPE(Ex10Result)),
        ('cw_off', CFUNCTYPE(Ex10Result)),
        ('measure_and_read_aux_adc', CFUNCTYPE(Ex10Result, c_uint32, c_uint8, POINTER(c_uint16))),
        ('measure_and_read_adc_temperature', CFUNCTYPE(Ex10Result, POINTER(c_uint16))),
        ('set_rf_mode', CFUNCTYPE(Ex10Result, c_uint32)),
        ('build_cw_configs', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int16, c_uint16, c_bool, POINTER(CwConfig))),
        ('cw_on', CFUNCTYPE(Ex10Result, POINTER(GpioPinsSetClear), POINTER(PowerConfigs), POINTER(RfSynthesizerControlFields), POINTER(Ex10RegulatoryTimers), POINTER(PowerDroopCompensationFields))),
        ('ramp_transmit_power', CFUNCTYPE(Ex10Result, POINTER(PowerConfigs), POINTER(Ex10RegulatoryTimers))),
        ('get_droop_compensation_defaults', CFUNCTYPE(PowerDroopCompensationFields)),
        ('set_regulatory_timers', CFUNCTYPE(None, POINTER(Ex10RegulatoryTimers))),
        ('set_analog_rx_config', CFUNCTYPE(Ex10Result, POINTER(RxGainControlFields))),
        ('enable_droop_compensation', CFUNCTYPE(Ex10Result, POINTER(PowerDroopCompensationFields))),
        ('disable_droop_compensation', CFUNCTYPE(None)),
    ]


class Ex10RxBasebandFilter(Structure):
    _fields_ = [
        ('set_drm_status', CFUNCTYPE(None, c_uint32)),
        ('get_drm_status', CFUNCTYPE(c_uint32)),
        ('rf_mode_is_drm', CFUNCTYPE(c_bool, c_uint32)),
        ('choose_rx_baseband_filter', CFUNCTYPE(c_uint32, c_uint32)),
    ]


class Ex10Commands(Structure):
    _fields_ = [
        ('read', CFUNCTYPE(Ex10Result, POINTER(RegisterInfo), c_void_p, c_size_t, c_uint32)),
        ('test_read', CFUNCTYPE(Ex10Result, c_uint32, c_uint16, c_void_p)),
        ('write', CFUNCTYPE(Ex10Result, POINTER(RegisterInfo), c_void_p, c_size_t, c_uint32)),
        ('read_fifo', CFUNCTYPE(Ex10Result, c_uint32, POINTER(ByteSpan))),
        ('write_info_page', CFUNCTYPE(Ex10Result, c_uint8, POINTER(ConstByteSpan), c_uint16)),
        ('start_upload', CFUNCTYPE(Ex10Result, c_uint8, POINTER(ConstByteSpan))),
        ('continue_upload', CFUNCTYPE(Ex10Result, POINTER(ConstByteSpan))),
        ('complete_upload', CFUNCTYPE(Ex10Result)),
        ('revalidate_main_image', CFUNCTYPE(Ex10Result)),
        ('reset', CFUNCTYPE(Ex10Result, c_uint32)),
        ('test_transfer', CFUNCTYPE(Ex10Result, POINTER(ConstByteSpan), POINTER(ByteSpan), c_bool)),
        ('create_fifo_event', CFUNCTYPE(Ex10Result, POINTER(EventFifoPacket), POINTER(c_uint8), c_size_t, c_size_t)),
        ('insert_fifo_event', CFUNCTYPE(Ex10Result, c_bool, POINTER(EventFifoPacket))),
    ]


class Ex10BoardSpec(Structure):
    _fields_ = [
        ('get_default_gpio_output_levels', CFUNCTYPE(c_uint32)),
        ('get_gpio_output_levels', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_uint32, POINTER(c_uint32))),
        ('get_gpio_output_enables', CFUNCTYPE(c_uint32)),
        ('get_gpio_output_pins_set_clear', CFUNCTYPE(Ex10Result, POINTER(GpioPinsSetClear), c_uint8, c_int16, c_uint32, c_uint32)),
        ('get_default_rx_analog_config', CFUNCTYPE(POINTER(RxGainControlFields))),
        ('get_sjc_residue_threshold', CFUNCTYPE(c_uint16)),
        ('get_pa_bias_power_on_delay_ms', CFUNCTYPE(c_uint32)),
        ('get_default_gpio_setup', CFUNCTYPE(GpioPinsSetClear)),
        ('temperature_compensation_enabled', CFUNCTYPE(c_bool, c_uint16)),
    ]


class Ex10GpioHelpers(Structure):
    _fields_ = [
        ('get_levels', CFUNCTYPE(Ex10Result, POINTER(Ex10GpioConfig), POINTER(c_uint32))),
        ('get_config', CFUNCTYPE(None, c_uint32, POINTER(Ex10GpioConfig))),
        ('get_output_enables', CFUNCTYPE(c_uint32)),
        ('set_antenna_port', CFUNCTYPE(Ex10Result, POINTER(GpioPinsSetClear), c_uint8)),
        ('set_rx_baseband_filter', CFUNCTYPE(Ex10Result, POINTER(GpioPinsSetClear), c_uint32)),
        ('set_pa_bias_enable', CFUNCTYPE(Ex10Result, POINTER(GpioPinsSetClear), c_bool)),
        ('set_pa_power_range', CFUNCTYPE(Ex10Result, POINTER(GpioPinsSetClear), c_uint32)),
        ('set_rf_power_supply_enable', CFUNCTYPE(Ex10Result, POINTER(GpioPinsSetClear), c_bool)),
        ('set_tx_rf_filter', CFUNCTYPE(Ex10Result, POINTER(GpioPinsSetClear), c_uint32)),
        ('set_dio_unused_pins', CFUNCTYPE(Ex10Result, POINTER(GpioPinsSetClear), c_uint32)),
        ('print_pin_bits', CFUNCTYPE(None, c_uint32)),
    ]


class Ex10CalibrationV5(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(None, POINTER(Ex10Protocol))),
        ('get_params', CFUNCTYPE(POINTER(Ex10CalibrationParamsV5))),
    ]


class Ex10Calibration(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(c_int16, POINTER(Ex10Protocol))),
        ('deinit', CFUNCTYPE(None)),
        ('power_to_adc', CFUNCTYPE(c_uint16, c_int16, c_uint32, c_uint16, c_bool, c_uint32, POINTER(c_uint32))),
        ('reverse_power_to_adc', CFUNCTYPE(c_uint16, c_int16, c_uint32, c_uint16, c_bool, c_uint32, POINTER(c_uint32))),
        ('get_power_control_params', CFUNCTYPE(PowerConfigs, c_int16, c_uint32, c_uint16, c_bool, c_uint32)),
        ('get_compensated_rssi', CFUNCTYPE(c_int16, c_uint16, c_uint32, POINTER(RxGainControlFields), c_uint8, c_uint32, c_uint16)),
        ('get_rssi_log2', CFUNCTYPE(c_uint16, c_int16, c_uint32, POINTER(RxGainControlFields), c_uint8, c_uint32, c_uint16)),
        ('get_compensated_lbt_rssi', CFUNCTYPE(c_int16, c_uint16, POINTER(RxGainControlFields), c_uint8, c_uint32, c_uint16)),
        ('get_cal_version', CFUNCTYPE(c_uint8)),
        ('get_customer_cal_version', CFUNCTYPE(c_uint8)),
    ]


class Ex10ContinuousInventoryUseCaseParameters(Structure):
    _fields_ = [
        ('antenna', c_uint8),
        ('rf_mode', c_uint32),
        ('tx_power_cdbm', c_int16),
        ('initial_q', c_uint8),
        ('session', c_uint8),
        ('target', c_uint8),
        ('select', c_uint8),
        ('send_selects', c_bool),
        ('stop_conditions', POINTER(StopConditions)),
        ('dual_target', c_bool),
    ]


class Ex10ContinuousInventoryUseCase(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(Ex10Result)),
        ('deinit', CFUNCTYPE(Ex10Result)),
        ('register_packet_subscriber_callback', CFUNCTYPE(None, CFUNCTYPE(None, POINTER(EventFifoPacket), POINTER(Ex10Result)))),
        ('enable_packet_filter', CFUNCTYPE(None, c_bool)),
        ('enable_auto_access', CFUNCTYPE(None, c_bool)),
        ('enable_abort_on_fail', CFUNCTYPE(None, c_bool)),
        ('get_continuous_inventory_stop_reason', CFUNCTYPE(c_uint32)),
        ('continuous_inventory', CFUNCTYPE(Ex10Result, POINTER(Ex10ContinuousInventoryUseCaseParameters))),
    ]


class Ex10TagAccessUseCaseParameters(Structure):
    _fields_ = [
        ('antenna', c_uint8),
        ('rf_mode', c_uint32),
        ('tx_power_cdbm', c_int16),
        ('initial_q', c_uint8),
        ('session', c_uint8),
        ('target', c_uint8),
        ('select', c_uint8),
        ('send_selects', c_bool),
    ]


class Ex10TagAccessUseCase(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(Ex10Result)),
        ('deinit', CFUNCTYPE(Ex10Result)),
        ('register_halted_callback', CFUNCTYPE(None, CFUNCTYPE(None, POINTER(EventFifoPacket), POINTER(c_uint32), POINTER(Ex10Result)))),
        ('run_inventory', CFUNCTYPE(Ex10Result, POINTER(Ex10TagAccessUseCaseParameters))),
        ('execute_access_commands', CFUNCTYPE(c_uint32)),
        ('get_fifo_packet', CFUNCTYPE(POINTER(EventFifoPacket))),
        ('remove_fifo_packet', CFUNCTYPE(None)),
        ('remove_halted_packet', CFUNCTYPE(c_bool)),
    ]


class Ex10EventFifoQueue(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(None)),
        ('list_node_push_back', CFUNCTYPE(None, POINTER(FifoBufferNode))),
        ('packet_peek', CFUNCTYPE(POINTER(EventFifoPacket))),
        ('packet_remove', CFUNCTYPE(None)),
        ('packet_wait', CFUNCTYPE(None)),
        ('packet_wait_with_timeout', CFUNCTYPE(c_bool, c_uint32)),
        ('packet_unwait', CFUNCTYPE(None)),
    ]


class Ex10RampModuleManager(Structure):
    _fields_ = [
        ('store_pre_ramp_variables', CFUNCTYPE(None, c_uint8)),
        ('store_post_ramp_variables', CFUNCTYPE(None, c_int16, c_uint32)),
        ('store_adc_temperature', CFUNCTYPE(None, c_uint16)),
        ('retrieve_adc_temperature', CFUNCTYPE(c_uint16)),
        ('retrieve_pre_ramp_antenna', CFUNCTYPE(c_uint8)),
        ('retrieve_post_ramp_frequency_khz', CFUNCTYPE(c_uint32)),
        ('retrieve_post_ramp_tx_power_cdbm', CFUNCTYPE(c_int16)),
        ('call_pre_ramp_callback', CFUNCTYPE(Ex10Result)),
        ('call_post_ramp_callback', CFUNCTYPE(Ex10Result)),
        ('register_ramp_callbacks', CFUNCTYPE(Ex10Result, CFUNCTYPE(None, POINTER(Ex10Result)), CFUNCTYPE(None, POINTER(Ex10Result)))),
        ('unregister_ramp_callbacks', CFUNCTYPE(None)),
    ]


class Ex10ListenBeforeTalk(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(Ex10Result)),
        ('deinit', CFUNCTYPE(Ex10Result)),
        ('set_rssi_count', CFUNCTYPE(None, c_uint8)),
        ('set_passes_required', CFUNCTYPE(None, c_uint8)),
        ('set_lbt_pass_threshold_cdbm', CFUNCTYPE(None, c_int32)),
        ('set_max_rssi_measurements', CFUNCTYPE(None, c_uint32)),
        ('set_measurement_delay_us', CFUNCTYPE(None, c_uint16)),
        ('get_last_rssi_measurement', CFUNCTYPE(c_int16)),
        ('get_last_frequency_khz', CFUNCTYPE(c_uint32)),
        ('get_total_num_rssi_measurements', CFUNCTYPE(c_uint32)),
        ('get_listen_before_talk_rssi', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int32, c_uint8, c_bool, POINTER(RxGainControlFields), POINTER(c_int16))),
        ('listen_before_talk_multi', CFUNCTYPE(Ex10Result, c_uint8, c_uint8, LbtControlFields, POINTER(c_uint32), POINTER(c_int32), POINTER(c_int16), POINTER(RxGainControlFields))),
        ('multi_listen_before_talk_rssi', CFUNCTYPE(c_int16, c_uint8)),
        ('get_default_lbt_rx_analog_configs', CFUNCTYPE(RxGainControlFields)),
        ('lbt_pre_ramp_callback', CFUNCTYPE(None, POINTER(Ex10Result))),
    ]


class Ex10AntennaDisconnect(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(Ex10Result)),
        ('deinit', CFUNCTYPE(Ex10Result)),
        ('set_return_loss_cdb', CFUNCTYPE(None, c_uint16)),
        ('set_max_margin_cdb', CFUNCTYPE(None, c_int16)),
        ('get_last_reverse_power_adc_threshold', CFUNCTYPE(c_uint16)),
        ('get_last_reverse_power_adc', CFUNCTYPE(c_uint16)),
        ('get_return_loss_threshold_exceeded', CFUNCTYPE(c_bool)),
        ('antenna_disconnect_post_ramp_callback', CFUNCTYPE(None, POINTER(Ex10Result))),
    ]


class Ex10AntennaDisconnectListenBeforeTalk(Structure):
    _fields_ = [
        ('init', CFUNCTYPE(Ex10Result)),
        ('deinit', CFUNCTYPE(Ex10Result)),
        ('set_return_loss_cdb', CFUNCTYPE(None, c_uint16)),
        ('set_max_margin_cdb', CFUNCTYPE(None, c_int16)),
        ('get_last_reverse_power_adc_threshold', CFUNCTYPE(c_uint16)),
        ('get_last_reverse_power_adc', CFUNCTYPE(c_uint16)),
        ('set_rssi_count', CFUNCTYPE(None, c_uint8)),
        ('set_passes_required', CFUNCTYPE(None, c_uint8)),
        ('set_lbt_pass_threshold_cdbm', CFUNCTYPE(None, c_int32)),
        ('set_max_rssi_measurements', CFUNCTYPE(None, c_uint32)),
        ('set_measurement_delay_us', CFUNCTYPE(None, c_uint16)),
        ('get_last_rssi_measurement', CFUNCTYPE(c_int16)),
        ('get_last_frequency_khz', CFUNCTYPE(c_uint32)),
        ('get_total_num_rssi_measurements', CFUNCTYPE(c_uint32)),
    ]


class Ex10Test(Structure):
    _fields_ = [
        ('cw_test', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int16, c_uint32, POINTER(PowerDroopCompensationFields), c_uint16, c_bool)),
        ('prbs_test', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int16, c_uint32, c_uint16, c_bool)),
        ('ber_test', CFUNCTYPE(Ex10Result, c_uint8, c_uint32, c_int16, c_uint32, c_uint16, c_uint16, c_bool, c_uint16, c_bool)),
        ('etsi_burst_test', CFUNCTYPE(Ex10Result, POINTER(InventoryRoundControlFields), POINTER(InventoryRoundControl_2Fields), c_uint8, c_uint32, c_int16, c_uint16, c_uint16, c_uint32, c_uint16, c_bool)),
    ]
# IPJ_autogen }
