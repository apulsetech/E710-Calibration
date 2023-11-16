#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2022 Impinj, Inc. All rights reserved.                      #
#                                                                           #
#############################################################################
"""
This contains a wrapper for working with the Keysight N9310A signal generator
"""

import pyvisa as visa
import time

FREQ_TOL = 0.01
PWR_TOL = 0.1


class KeysightN9310ASigGen(object):
    def __init__(self, address=19):
        self.address = address
        self.freq = None
        self.power = None
        rm = visa.ResourceManager()
        self.KeysightSigGen = rm.open_resource(address)

    def initialize(self, freq, power):
        self.freq = freq
        self.power = power
        self.set_freq_mhz(freq)
        self.set_power_dbm(power)
        self.on()

    def set_freq_mhz(self, freq):
        """
        :param freq: This value should be in MHz
        :return:
        """
        self.KeysightSigGen.write(":FREQuency:CW {} MHz".format(freq))
        freq_set = float(self.KeysightSigGen.query(":FREQuency:CW?")) / 1e6
        if abs(freq - freq_set) > FREQ_TOL:
            print("Error setting signal generator frequency\n")
            print("freq: {}\n".format(freq))
            print("freq_set: {}\n".format(freq_set))
            raise RuntimeError("Wrong signal generator frequency")

    def set_power_dbm(self, power):
        """
        :param power: This value should be in dBm
        :return:
        """
        self.power = power
        self.KeysightSigGen.write("AMPL:CW {} dBm\n".format(power))
        power_set = float(self.KeysightSigGen.query(":AMPL:CW?"))
        if abs(power - power_set) > PWR_TOL:
            print("Error setting signal generator power\n")
            print("power: {}\n".format(power))
            print("power_set: {}\n".format(power_set))
            raise RuntimeError("Wrong signal generator power")

    def on(self):
        self.KeysightSigGen.write("RFO:STAT ON\n")
        self.KeysightSigGen.write("MOD:STAT OFF\n")

    def off(self):
        self.KeysightSigGen.write("RFO:STAT OFF\n")
        self.KeysightSigGen.write("MOD:STAT OFF\n")
