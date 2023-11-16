#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2021 - 2022 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
"""
This contains a wrapper for working with a signal generator.
"""

from keysight_N9310A_sig_gen import KeysightN9310ASigGen


class KeysightSigGen(KeysightN9310ASigGen):
    def __init__(self, address=19):
        super().__init__(address)
