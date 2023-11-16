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


class ByteSpan(Structure):
    _fields_ = [
        ('data', POINTER(c_uint8)),
        ('length', c_size_t),
    ]


class ConstByteSpan(Structure):
    _fields_ = [
        ('data', POINTER(c_uint8)),
        ('length', c_size_t),
    ]
