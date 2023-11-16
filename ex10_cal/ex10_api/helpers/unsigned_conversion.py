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
Convert unsigned specific width values into python signed integer values
and vice-versa.
"""

def unsigned_to_signed_int(bit_width, value):
    """
    Convert an arbitray unsigned bit value into a signed integer value.
    :bit_width: The bit width of a 2's complement value, where bit bit_width
                locates the sign bit of the value.
    Example:    To sign extend a 20 bit value the bit_width is 20.
                The sign bit is bit 19.
    :value:     The python integer value that is to be sign extended and
                converted to a signed value. This parameter is often read from
                a hardware register.
    """
    mask = (1 << bit_width) - 1
    max_signed_int_value = mask >> 1

    value &= mask
    if value > max_signed_int_value:
        value = value - mask - 1

    return value

def signed_to_unsigned_int(bit_width, value):
    """
    Convert an arbitray signed bit value into a unsigned integer value.
    Convert a signed integer into a bit-wise equivalent unsigned value.
    """
    mask = (1 << bit_width) - 1
    max_signed_int_value = mask >> 1

    value &= max_signed_int_value
    if value < 0:
        value += max_signed_int_value + 1

    return value & max_signed_int_value
