#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2020 - 2022 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
"""
This contains a wrapper for working with a power meter.
"""

from __future__ import (division, absolute_import, print_function,
                        unicode_literals)

import os
import pyvisa as visa


class PowerMeter(object):
    """
    Controller class for Keysight E4416A EPM-P Series Power Meter
    Object that takes in a VISA Resource Manager and returns an object for
    controlling power meter
    """
    def __init__(self, addr, freq_mhz=915.25, ofs=None):
        rm = visa.ResourceManager()
        self.instr = rm.open_resource(addr)
        self.instr.write('SYSTem:PRESet')
        self.instr.write('INIT:CONT ON')
        self.instr.write('TRIG:SOUR IMM')
        self.set_frequency(freq_mhz)
        if ofs is not None:
            self.set_offset_value(ofs)

    def read_power(self):
        """ Reads measured power
        :return: Measured power in dBm
        """
        return float(self.instr.query('FETCH?'))

    def set_rate(self, rate=1):
        """ Sets power meter sampling rate
        :param rate: Sampling rate multiplier as either 1 or 2; default is 1
        :type rate: int
        """
        rate_strings = {
            1: 'NORM',
            2: 'DOUB',
        }
        self.instr.write('SENS:MRAT ' + rate_strings[rate])
        rate_string_read = self.instr.read('SENS:MRAT?').strip()
        if rate_strings[rate] != rate_string_read:
            raise Exception('Rate error.  Rate was set to ' + rate_string_read)

    def set_frequency(self, freq_mhz):
        """ Sets power meter expected frequency
        :param freq_mhz: Frequency in MHz
        :type freq_mhz: float
        """
        self.instr.write('SENS:FREQ ' + str(freq_mhz) + ' MHZ')
        freq_mhz_read = self.get_frequency()
        if freq_mhz != freq_mhz_read:
            raise Exception('Frequency error. Frequency was set to: {} '
                            'MHz'.format(freq_mhz_read))

    def get_frequency(self):
        """ Gets power meter expected frequency setting
        :return: Frequency in MHz"""
        return float(self.instr.query('SENS:FREQ?'))/1e6

    def set_offset_value(self, ofs=0):
        """ This sets an offset for display only
        :param ofs: Offset in dB to be added to measurement (to compensate
        for line loss, attenuators, etc)
        :type ofs: float
        """
        self.instr.write('CALC:GAIN ' + str(ofs))
        ofs_read = self.get_offset_value()
        if abs(ofs_read - ofs) > 0.01:
            raise Exception(f'Offset error.  Offset read back was: {ofs_read} dB but should have been {ofs}')

    def get_offset_value(self):
        """ Gets display offset (to compensate for line loss, attenuators, etc)
        :return: Display offset in dB
        """
        return float(self.instr.query('CALC:GAIN?'))

    def close(self):
        """
        Closes current session
        """
        self.instr.close()
