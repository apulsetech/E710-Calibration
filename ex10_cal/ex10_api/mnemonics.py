#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2019 - 2021 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
"""
This module implements methods needed to translate back and forth between raw
bytes and mnemonic dictionaries.  The mnemonic maps are structured according
to the register memory layout.

Mnemonic maps are laid out in 2 basic ways:
  -- a buffer.  The map in this case will look like  {'register_name':bytes}.
  This map represents a simple register without any fields.  The onus is on
  the caller to convert their data type to and from bytes objects.
  -- a register with fields. This will look like
  {'register_name': {'field1_name':<value type>, 'field2_name' 'value_type'},
   ...}
val_types are currently restricted to string, bool, uint and enums(which are
defined in the address map)

Usage:
A call to the Accessor layers write method for InterruptMask:
mne = {
    'InterruptMask':
        {'OpDone': 1, 'Halted': 0, 'EventFifoAboveThresh': 0,
         'EventFifoFull': 1, 'InventoryRoundDone': 0, 'HaltedSequenceDone': 0}
}
accessors.write(nme)

A call to the Accessor layers read method for InterruptMask:
val = accessors.read('InterruptMask')
after the call, val would look like the dictionary above with the fields
populated by the chip.

"""
from __future__ import (
    division,
    absolute_import,
    print_function,
    unicode_literals,
)

import struct
import math
from enum import Enum
import six

from . helpers import unsigned_conversion as convert
from . import application_address_range as aar
from . import bootloader_address_range as bar


class CommandCode(Enum):
    # pylint: disable=locally-disabled, too-few-public-methods
    """ The Ex10 API Command Codes """
    # IPJ_autogen | gen_py_command_codes {

    Read = 0x01
    Write = 0x02
    ReadFifo = 0x03
    StartUpload = 0x04
    ContinueUpload = 0x05
    CompleteUpload = 0x06
    ReValidateMainImage = 0x07
    Reset = 0x08
    TestTransfer = 0x0A
    WriteInfoPage = 0x0B
    TestRead = 0x0C
    InsertFifoEvent = 0x0E
    # IPJ_autogen }


class ResponseCode(Enum):
    # pylint: disable=locally-disabled, too-few-public-methods
    """ The Ex10 API Responses Codes """
    # IPJ_autogen | gen_py_response_codes {

    Success = 0xA5
    CommandInvalid = 0x01
    ArgumentInvalid = 0x02
    ResponseOverflow = 0x06
    CommandMalformed = 0x07
    AddressWriteFailure = 0x08
    ImageInvalid = 0x09
    LengthInvalid = 0x0A
    UploadStateInvalid = 0x0B
    ImageExecFailure = 0x0C
    BadCrc = 0x0E
    FlashInvalidPage = 0x0F
    FlashPageLocked = 0x10
    FlashEraseFailure = 0x11
    FlashProgramFailure = 0x12
    StoredSettingsMalformed = 0x13
    NotEnoughSpace = 0x14
    # IPJ_autogen }


class FlashInfoPage(Enum):
    # pylint: disable=locally-disabled, too-few-public-methods
    """ Enumerate the FLASH Info pages """
    Calibration = 0x03
    StoredSettings = 0x04


class ProductSku(Enum):
    # pylint: disable=locally-disabled, too-few-public-methods, bad-whitespace
    """ Enumerate the SKU Types """
    E310    = 0x0310
    E510    = 0x0510
    E710    = 0x0710
    E910    = 0x0910


def _is_mnem_field_or_buffer(mnemonic_dict):
    """
    Determine if the passed in dict represents a field or a buffer

    This will not work on nodes from the address map!
    :param mnemonic_dict: dict, a mnemonic dict as created by the helper
                          method mnemonics.get_template().
    :return: string, 'field' if the dict represents a field otherwise 'buffer'
    """
    if not isinstance(mnemonic_dict, dict):
        raise ValueError('Input is not a dictionary')
    # register name will be the one and only top level key
    keys = list(mnemonic_dict)
    if len(keys) != 1:
        raise ValueError(
            'This dictionary {} is malformed'.format(mnemonic_dict))
    register_name = keys[0]

    # pylint: disable=locally-disabled, no-else-return
    if isinstance(mnemonic_dict[register_name], dict):
        return 'field'
    else:
        return 'buffer'


def _is_addr_map_field_or_buffer(address_map_node):
    # pylint: disable=locally-disabled, no-else-return
    if not isinstance(address_map_node, dict):
        raise ValueError('Input is not a dictionary')
    if 'fields' in address_map_node.keys():
        return 'field'
    else:
        return 'buffer'


def _create_field_bytes(register_name, address, offset, length, mnemonic_dict):
    """
    Create a bytes object from the passed in dictionary

    Note the bytes object will not contain a write command

    In order to ensure correctness, the dict passed in should be created using
    the helper method on this module.
    :param register_name: string
    :param address: uint
    :param offset: uint
    :param length: uint
    :param mnemonic_dict: python dict, the structure of this dict mirrors the
    structure of the field registers memory
    :return: bytes,
    """
    # pack in the address and length
    b_add_and_len = struct.pack(b'<HH', address+offset, length)
    ret_val = bytearray(b_add_and_len)

    u32_field_values = _create_bit_field(register_name, mnemonic_dict)
    data_arr = bytearray()
    for u32_value in u32_field_values:
        data_arr += bytearray(struct.pack(b'<I', u32_value))
    ret_val += data_arr[offset:offset+length]

    return bytes(ret_val)


def _create_buffer_bytes(address, offset, length, bytes_):
    """
    Create a bytes object from the passed in params.

    This method is used to create bytes objects for writing to simple buffer
    style registers. Note, the bytes object will not include the write command.

    :param address: uint
    :param length: uint
    :param offset: uint
    :param bytes_: bytes object
    :return: bytes
    """
    if not isinstance(bytes_, (bytearray, bytes)):
        raise ValueError('bytes_ param must be a bytes or bytearray object.')

    # pack in address and length
    b_add_len = struct.pack(b'<HH', address+offset, length)
    ret_val = bytearray(b_add_len)
    ret_val += bytes_[offset:offset+length]

    # return immutable object
    return bytes(ret_val)


def _create_bit_field(register_name, mnemonic_dict):
    """
    Create a list of 32-bit fields from the field values for 'register_name'
    :param register_name: A string identifying the register that will be packed
                          with a list of integer data by this function.
    :param mnemonic_dict: A dictionary of associated field names and their
                          values within the register.
    :return: A list if integers representing unsigned 32-bit packed values.
    """
    register_node = aar.APPLICATION_ADDRESS_RANGE[register_name]
    register_fields = register_node['fields']
    byte_length = register_node['length']
    u32_length = (byte_length // 4) + (1 if (byte_length % 4) else 0)

    u32_list = [0] * u32_length
    for field_name, _ in register_fields.items():
        field_pos = register_fields[field_name]['pos']
        field_bits = register_fields[field_name]['bits']

        # get the value from the passed in map
        field_value = mnemonic_dict[register_name][field_name]
        if field_value > (2 ** field_bits) - 1:
            raise ValueError(
                'register {}.{} value 0x{:08x} does not fit into {}-bits'
                .format(register_name, field_name, field_value, field_bits))

        if field_bits == 0:
            raise ValueError('register {}.{} has zero bit length'
                             .format(register_name, field_name))

        while field_bits > 0:
            u32_bits = field_bits % 32
            u32_bits = 32 if u32_bits == 0 else u32_bits
            u32_mask = (1 << u32_bits) - 1
            u32_value = (field_value & u32_mask) << (field_pos % 32)
            u32_list[field_pos // 32] |= u32_value

            field_bits -= 32
            field_pos -= 32

    return u32_list


def _get_packing_length(sorted_packing_lengths, length):
    """
    Return an acceptable packing length based on the allowed packing lengths
    passed in. This function finds the value in sorted_packing_lengths which
    is >= the length parameter required.
    :param sorted_packing_lengths: A sorted iterable of acceptable packing
                                   lengths.
    :param length:                 The byte length of the object to unpack.
    """
    prev_unpack_length = 0
    for unpack_length in sorted_packing_lengths:
        if prev_unpack_length < length <= unpack_length:
            return unpack_length
        prev_unpack_length = unpack_length

    raise RuntimeError('Invalid packing length {}'.format(length))


def _resolve_field_from_bytes(position, length_bits, bytes_, is_signed):
    """
    Resolve one field value.

    :param position:    number of bits to advance bytes_ to find the start of
                        this field
    :param length_bits: number of bits fo this field
    :param bytes_:      a bytes object, typically returned from the device in a
                        read operation
    :param is_signed:   boolean indicating whether the value parsed should be
                        handled as a signed value (True) or unsigned (False)

    :return the field's value
    """
    length_in_bytes = int(math.ceil(length_bits / 8))
    first_byte = math.floor(position / 8)

    # int cast maintains python 2 and 3 compatibility
    first_byte = int(first_byte)

    byte_total = 0
    for i in range(length_in_bytes):
        curr_byte = bytes_[first_byte+i]
        if isinstance(curr_byte, str):
            curr_byte = struct.unpack('B', bytes_[first_byte+i])[0]
        byte_total += curr_byte << (i * 8)
    # shift to a byte boundary if starting not at a mod 8 position
    byte_total = byte_total >> (position % 8)
    # create a mask for the top since it may not use all bytes grabbed
    top_mask = (0x1 << length_bits) - 1
    ret_val = top_mask & byte_total

    if is_signed:
        ret_val = convert.unsigned_to_signed_int(length_bits, ret_val)

    return ret_val


#######################
# public interface
#######################

def get_template(register_name):
    """
    Return a template for a register's mnemonic dictionary.

    The output of this call can be used as a template for a write call.
    Note: The value part of the dictionary is a placeholder string that is
    expected to be overwritten with integer data value(s).
    These values have no meaning other than being informative during debug.

    :param register_name: string
    :returns: dict, the mnemonic dictionary
    """
    # get the address_map node
    try:
        register_node = aar.APPLICATION_ADDRESS_RANGE[register_name]
    except KeyError:
        register_node = bar.BOOTLOADER_ADDRESS_RANGE[register_name]
    length = register_node['length']

    field_or_buffer = _is_addr_map_field_or_buffer(register_node)
    # pylint: disable=locally-disabled, no-else-return, invalid-name
    if field_or_buffer == 'buffer':
        # return a simple dict like this: {registername: <bytes>}
        return {register_name: '<{}-byte buffer>'.format(length)}
    else:
        assert field_or_buffer == 'field'
        output = {register_name: {}}
        register_fields = register_node['fields']
        for k, v in register_fields.items():
            output[register_name][k] = '<{}-bit value.>'.format(v['bits'])

    return output


def show_template(register_name):
    """
    Print out a template for a register's mnemonic dictionary.

    The output of this call can be used as a template for a write call.

    :param register_name: string
    :returns: string, the mnemonic dictionary
    """
    print(get_template(register_name))


def register_address_and_length(register_info):
    """
    Return the address and length for a register.

    :param register_info: string or tuple(for partial register access)
    :return: tuple(<address>, <register>)
    """
    read_len = 0
    reg_start = 0
    if not isinstance(register_info, six.string_types):
        # Contains offset and length
        register_name = register_info[0]
        reg_start = register_info[1]
        read_len = register_info[2] - reg_start
    else:
        register_name = register_info
    try:
        register_node = aar.APPLICATION_ADDRESS_RANGE[register_name]
    except KeyError:
        register_node = bar.BOOTLOADER_ADDRESS_RANGE[register_name]
    if read_len == 0:
        read_len = register_node['length']
        if 'num_entries' in register_node.keys():
            read_len *= register_node['num_entries']

    return (register_node['address'] + reg_start, read_len)


def mnemonic_to_bytes(mnemonic_dict, index=0):
    """
    Convert a mnemonic dictionary to a bytes object.

    Packs the information from the dictionary into a bytes object suitable for
    sending across the wire.  The mnemonics.get_template() can be used to
    create a template for the dictionary.

    :param mnemonic_dict: A dictionary that represents the memory of a register.
    :param index:         An index into a field if it is an array
    :return: a bytes object
    """
    # Check if dictionary is partial accessor. A partial accessor performs
    # reads/writes using offset, length pairs rather than register mnemonics
    # and may span multiple registers or parts of a register or both.
    reg_len = None
    offset = 0
    if isinstance(mnemonic_dict, tuple):
        offset = mnemonic_dict[1]
        reg_len = mnemonic_dict[2]
        mnemonic_dict = mnemonic_dict[0]

    # register name will be the one and only top level key
    keys = list(mnemonic_dict)
    if len(keys) != 1:
        raise ValueError(
            'This dictionary {} is malformed'.format(mnemonic_dict))
    register_name = keys[0]

    register_node = aar.APPLICATION_ADDRESS_RANGE[register_name]
    address = register_node['address']
    if reg_len is None:
        length = register_node['length']
    else:
        length = reg_len

    field_or_buffer = _is_mnem_field_or_buffer(mnemonic_dict)

    # pylint: disable=locally-disabled, no-else-return
    if field_or_buffer == 'field':
        if index + 1 > register_node['num_entries']:
            raise ValueError(
                "Index is greater than the number of entries {} {}".
                format(index + 1, register_node['num_entries']))

        # index must be ok, so we apply it (note we use the real length of the
        # register, if reg_len is not equal, then we start their write at that
        # point.  (the offset will still be applied)
        address += index * register_node['length']
        return _create_field_bytes(register_name, address, offset,
                                   length, mnemonic_dict)
    else:
        # this register is a buffer
        assert field_or_buffer == 'buffer'
        value = mnemonic_dict[register_name]
        return _create_buffer_bytes(address, offset, length, value)


def bytes_to_mnemonic(register_info, bytes_):
    """
    Convert a bytestring to a mnemonic dictionary.

    :param register_info: string or tuple(for partial register access)
    :param bytes_: a bytes object, typically returned from the chip
    :return:dict, a mnemonic laden dictionary representation of the bytes
    object
    """
    if not isinstance(register_info, six.string_types):
        register_name = register_info[0]
    else:
        register_name = register_info

    try:
        register_node = aar.APPLICATION_ADDRESS_RANGE[register_name]
    except KeyError:
        register_node = bar.BOOTLOADER_ADDRESS_RANGE[register_name]

    # Fill rest of buffer with zeros to use with the mnemonics
    if not isinstance(register_info, six.string_types):
        # Prepend before start address
        filler = bytearray(register_info[1])
        filler += bytes_
        # Fill after stop address
        filler += bytearray(register_node['length']-len(filler))
        bytes_ = filler

    field_or_buffer = _is_addr_map_field_or_buffer(register_node)
    if field_or_buffer == 'field':
        mnemonic_dict = get_template(register_name)
        register_fields = mnemonic_dict[register_name]
        for field_name, _ in register_fields.items():
            pos = register_node['fields'][field_name]['pos']
            bits = register_node['fields'][field_name]['bits']
            num_entries = 1
            if 'num_entries' in register_node.keys():
                num_entries = register_node['num_entries']

            is_signed = False
            try:
                is_signed = (
                    register_node['fields'][field_name]['resolve_as'] == 'int')
            except KeyError:
                # If the key 'resolve_as' does not exist then parsing will be
                # handled as an unsigned integer.
                pass

            # resolve each field value
            if bits > 64:  # Treat field value as buffer
                field_val = bytes_[pos:pos+int(bits/8)]
            elif num_entries == 1:
                field_val = _resolve_field_from_bytes(position=pos,
                                                      length_bits=bits,
                                                      bytes_=bytes_,
                                                      is_signed=is_signed)
            else:   # num_entries > 1
                field_val = []
                field_length = register_node['length']
                for idx in range(num_entries):
                    slice_beg = idx * field_length
                    slice_end = slice_beg + field_length
                    field_val.append(_resolve_field_from_bytes(
                        position=pos,
                        length_bits=bits,
                        bytes_=bytes_[slice_beg:slice_end],
                        is_signed=is_signed))

            register_fields[field_name] = field_val

    else:
        # this register is a buffer
        assert field_or_buffer == 'buffer'
        if len(bytes_) != register_node['length']:
            raise ValueError('Length of bytes_ is incorrect')

        # get the template, and populate it with the bytes value
        mnemonic_dict = get_template(register_name)
        mnemonic_dict[register_name] = bytes_
        print()

    return mnemonic_dict
