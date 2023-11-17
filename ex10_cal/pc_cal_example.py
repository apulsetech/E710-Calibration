#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2022 - 2023 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
"""
This example script runs the calibration procedure for the Ex10 dev kit from
a PC, communicating via a UART interface.
"""

from __future__ import (division, absolute_import, print_function,
                        unicode_literals)
from stopwatch import StopWatch
import ex10_api.mnemonics as mne
import ex10_api.application_address_range as aar
from uart_reader import UartReader
from uart_helper import UartBaud, UartHelper
from sig_gen import KeysightSigGen

# 2023-11-17
# Updated by Kim Young Bong
# replaced from PowerMeter to mini_circuit
import mini_circuit
# from power_meter import PowerMeter

import calib_info as calib_info

import time
import os
import argparse
import sys

import numpy as np
import pandas as pd


# Append this script's parent dir to path so that modules in ex10_api can be
# imported with "ex10_api.module".
parent_dir = os.path.normpath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)), '..'))
sys.path.append(parent_dir)

# Equipment addresses can be GPIB or USB.  For example
# addr="GPIB0::19::INSTR", etc.
power_meter_addr = "USB0::0x2A8D::0x0701::MY59260011::0::INSTR"

# 2023-11-17
# Updated by Kim Young Bong
sig_gen_addr = "USB0::0x0957::0x2018::0115001234::INSTR"


# The serial interface to the device running ex10_fct_wrapper.bin
com_port = "COM5"


class Ex10CalibrationExample(object):
    """
    Calibration procedure example class for Ex10 reader.
    """

    # The calibration procedure configuration is set with the definitions below
    CAL_CFG = {'CAL_VERSION': 5,
               'PM_SLEEP_TIME': 0.15,
               'PD_SLEEP_TIME': 0.02,
               'ANTENNA': 1,
               'RF_MODE': 5,
               'TX_SCALAR': 1152,  # 5 dB backoff from fullscale
               'COARSE_ATTS': list(range(30, -1, -1)),
               'DEFAULT_COARSE_ATT': 8,
               'DEFAULT_UPPER_FREQ_MHZ': 920.25,
               'DEFAULT_UPPER_FREQ_REGION': 'FCC',
               'UPPER_FREQS_MHZ': [902.75, 910.25, 920.25, 927.25],
               'COARSE_ATTS_FREQ': [25, 17, 8],
               'DEFAULT_LOWER_FREQ_MHZ': 866.3,
               'DEFAULT_LOWER_FREQ_REGION': 'ETSI LOWER',
               'LOWER_FREQS_MHZ': [865.7, 866.3, 866.9, 867.5],
               'SOAK_TIME': 3,
               'SOAK_COARSE_ATT': 6,
               'PDET_PWR_PER_TEMP_ADC': [0.02, 0.013, 0.012],
               'DB_PER_ADC_FWD_PWR': -0.039,
               'FWD_PWR_LIMIT_DBM': 32,
               'NUM_PDET_BLOCKS': 3,
               'PDET_ADC_MIN_MAX': [150, 915],
               'ADC_ERROR_THRESHOLD': 10,
               'LOOP_GAIN_DIVISOR': 800,
               'MAX_ITERATIONS': 10,
               'NUM_PDETS': 4,
               }

    # Calibration trade-offs to optimize speed
    CAL_OPTIMIZATIONS = {'PRECISE_COARSE_ATT_THRESH': 10,    # Interpolate COARSE_ATTs larger than this
                         'SKIP_LOWER_BAND_FREQ_CAL': True,  # Skip frequency calibration for LOWER_BAND
                         # Don't change pwr meter freq compensation
                         'SKIP_PWR_METER_FREQ_SET_DURING_FREQ_CAL': True
                         }

    # The DC offset characterization algorithm uses the following definitions
    DC_CFG = {'PWR_TOL': 1,  # Power ripple tolerance in ADC codes
              'MAX_ITERS': 10,  # Max num DC offset algorithm loops
              'INIT_TX_SCALAR': 2047,  # Initial power scalar
              'MAX_FWD_PWR': 550,  # Max power in ADC codes to avoid compression
              'PWR_BACKOFF_SCALAR': 0.8,  # Fwd pwr backoff for Tx scalar
              'INIT_MIN_DC_EXP': 0,     # Initial minimum DC exponent for DCO search
              'INIT_MAX_DC_EXP': 18,    # Initial maximum DC exponent for DCO search
              }

    BLF_KHZ = {  # Used for RSSI calibration
        1: 640,
        3: 320,
        5: 320,
        7: 250,
        11: 640,
        12: 320,
        13: 160,
        15: 640
    }

    RSSI_DEFAULT_CFG = {
        'RX_PWR': -55,  # dBm input power from sig_gen
        'ANTENNA': 1,
        'RF_MODE': 13,
        'FREQ_MHZ': 920.25,
        'UPPER_FREQ_REGION': 'FCC',
        'LOWER_FREQ_MHZ': 866.3,
        'LOWER_FREQ_REGION': 'ETSI LOWER',
        'PGA1_GAIN': 'Gain_6_dB',
        'PGA2_GAIN': 'Gain_6_dB',
        'PGA3_GAIN': 'Gain_18_dB',
        'MIXER_GAIN': 'Gain_11p2_dB',
        'RX_ATT': 'Atten_0_dB',
    }

    def __init__(self, power_meter, sig_gen, ex10_reader, stopwatch, allow_cal_lock, verbose=True):
        self.power_meter = power_meter
        self.sig_gen = sig_gen
        self.ex10_reader = ex10_reader
        self.stopwatch = stopwatch
        self.allow_cal_lock = allow_cal_lock
        self.verbose = verbose
        self.info = self.initialize_cal_info()

    def _print(self, *args):
        if self.verbose:
            print(*args)

    @staticmethod
    def get_board_id(board_id=None, directory='c:/users/lab_user/ex10_board_cal'):
        # pylint: disable=locally-disabled, missing-docstring
        if board_id is None:
            max_id_len = 16
            board_id = int(input('Enter calibrated board ID number (max '
                                 + str(2 ** max_id_len - 1) + '):'))
            if board_id >= 2 ** max_id_len:
                raise ValueError('Board ID number larger than max')
        json_file_to_write = directory + os.sep + str(board_id) + '.json'
        return board_id, json_file_to_write

    def initialize_cal_info(self):
        """
        Instantiates an accessor object to read in and parse calibration
        configurations from YAML.
        :return: calibration accessor object
        """
        info = calib_info.CalibrationInfoPageAccessor()
        info.read_in_yaml(self.CAL_CFG['CAL_VERSION'])
        return info

    def init_pwr_cal_data(self):
        """
        Init power control data structure as nested dict
        """
        coarse_len = len(self.CAL_CFG['COARSE_ATTS'])
        freq_len = len(self.CAL_CFG['UPPER_FREQS_MHZ'])
        freq_att_len = len(self.CAL_CFG['COARSE_ATTS_FREQ'])
        cal_data = {
            'COARSE': {
                'FWD_PWR': [np.nan] * coarse_len,
                'LO_PDET': [[np.nan] * self.CAL_CFG['NUM_PDETS'] for _ in range(coarse_len)],
                'TEMP': [np.nan] * coarse_len,
            },
            'FREQ': {
                'FWD_PWR': [[0.0] * freq_att_len for _ in range(freq_len)],
                'LO_PDET': [[0.0] * freq_att_len for _ in range(freq_len)],
                'TEMP': [[0.0] * freq_att_len for _ in range(freq_len)],
            },
            'DC_OFS': [np.nan] * coarse_len,
            'TEMP': 0,
        }
        return cal_data

    def init_rssi_configs(self):
        """
        Chooses RSSI calibration settings based on product SKU
        """
        _, sku_str = self.ex10_reader.get_sku()
        sku = int(sku_str, 16)
        sku_rssi_cfg = {
            mne.ProductSku.E310.value: {
                'RF_MODES': [3, 5, 7, 12, 13],
            },
            mne.ProductSku.E510.value: {
                'RF_MODES': [1, 3, 5, 7, 12, 13, 15],
            },
            mne.ProductSku.E710.value: {
                'RF_MODES': [1, 3, 5, 7, 11, 12, 13, 15],
            },
            mne.ProductSku.E910.value: {
                'RF_MODES': [1, 3, 5, 7, 11, 12, 13, 15],
            }
        }
        self.RSSI_CFG = {
            'N_RF_MODES': 32,  # Number of max allowed RF modes in cal table
            'N_ANTENNA_PORTS': 8,  # Number of max allowed antennas in cal table
            'ANTENNA_PORTS': [2, 1],  # Antenna ports
            'RF_MODES': sku_rssi_cfg[sku]['RF_MODES'],
            'TEMP_COEFS': [0, 0, 0],  # Units: [RSSI_LOG_2/ADC, ADC]
            'PGA1_GAINS': ['Gain_n6_dB', 'Gain_0_dB', 'Gain_6_dB', 'Gain_12_dB'],
            'PGA2_GAINS': ['Gain_0_dB', 'Gain_6_dB', 'Gain_12_dB', 'Gain_18_dB'],
            'PGA3_GAINS': ['Gain_0_dB', 'Gain_6_dB', 'Gain_12_dB', 'Gain_18_dB'],
            'MIXER_GAINS': ['Gain_1p6_dB', 'Gain_11p2_dB', 'Gain_17p2_dB',
                            'Gain_20p7_dB'],
            'RX_ATTS': ['Atten_0_dB', 'Atten_3_dB', 'Atten_6_dB',
                        'Atten_12_dB', ],
        }

    def init_rssi_data(self):
        rssi_data = {
            'DEFAULT': 0,
            'ANTENNA': [0] * len(self.RSSI_CFG['ANTENNA_PORTS']),
            'MODES': [0] * len(self.RSSI_CFG['RF_MODES']),
            'UPPER_FREQS_MHZ': [0],
            'LOWER_FREQS_MHZ': [0],
            'PGA1': [0] * len(self.RSSI_CFG['PGA1_GAINS']),
            'PGA2': [0] * len(self.RSSI_CFG['PGA2_GAINS']),
            'PGA3': [0] * len(self.RSSI_CFG['PGA3_GAINS']),
            'MIXER': [0] * len(self.RSSI_CFG['MIXER_GAINS']),
            'RX_ATT': [0] * len(self.RSSI_CFG['RX_ATTS']),
        }
        return rssi_data

    def measure_fwd_power(self):
        """
        Reads power from power meter and Ex10 lo power detector
        :return: power meter reading in dBm, list of lo power detector ADC codes
        """
        time.sleep(self.CAL_CFG['PM_SLEEP_TIME'])
        fwd_pwr = self.power_meter.read_power()
        lo_pdet = self.ex10_reader.measure_adc('lo')
        return fwd_pwr, lo_pdet

    def estimate_dc_offset(self):
        """
        Estimates DC offset value by minimizing ripple between positive and
        negative TX ramps
        :return: Optimal DC offset value
        :rtype: int
        """
        # Set appropriate Tx scalar to avoid compression
        tx_scalar = self.DC_CFG['INIT_TX_SCALAR']
        self.ex10_reader.set_tx_fine_gain(tx_scalar)
        # Back off tx scalar until power is in linear range
        while (self.ex10_reader.measure_adc('lo_sum') >
               self.DC_CFG['MAX_FWD_PWR']):
            tx_scalar = int(tx_scalar * self.DC_CFG['PWR_BACKOFF_SCALAR'])
            self.ex10_reader.set_tx_fine_gain(tx_scalar)
            time.sleep(self.CAL_CFG['PD_SLEEP_TIME'])
        # Reassign init_tx_scalar to speed up next DCO estimation
        self.DC_CFG['INIT_TX_SCALAR'] = tx_scalar

        # Reset DC Offset and determine if search range should be negative
        self.ex10_reader.tx_ramp_up(dc_offset=0)
        pwr_diff, pwr_p, pwr_n = self.get_fwd_pwr_diff(tx_scalar)
        dc_sign = -1 if pwr_diff > 0 else 1
        # Initialize binary search loop, with search boundaries as exponents
        # to optimize speed
        n_iters = 0
        dc_exp = 0
        dc_exp_low = self.DC_CFG['INIT_MIN_DC_EXP']
        dc_exp_high = self.DC_CFG['INIT_MAX_DC_EXP']
        while dc_exp_low <= dc_exp_high and n_iters < self.DC_CFG['MAX_ITERS']:
            # Set exponential to be average of bounds
            dc_exp = (dc_exp_low + dc_exp_high) / 2
            self.ex10_reader.tx_ramp_up(dc_offset=dc_sign * int(2 ** dc_exp))
            # Measure power difference between pos and neg tx ramps
            pwr_diff, pwr_p, pwr_n = self.get_fwd_pwr_diff(tx_scalar)
            if abs(pwr_diff) <= self.DC_CFG['PWR_TOL']:
                break
            elif dc_sign * pwr_diff < -1 * self.DC_CFG['PWR_TOL']:
                #  Adjust exponential search bounds for next loop
                dc_exp_low = dc_exp
            else:
                dc_exp_high = dc_exp

            n_iters += 1

        dc_ofs = dc_sign * int(2 ** dc_exp)
        return dc_ofs

    def get_fwd_pwr_diff(self, tx_scalar):
        """
        Gets the power difference between positive and negative Tx-ramps
        :param tx_scalar: Fine gain power scalar
        :return: dB power difference, dBm pos power, dBm neg power
        """
        # Positive ramp
        self.ex10_reader.set_tx_fine_gain(tx_scalar)
        time.sleep(self.CAL_CFG['PD_SLEEP_TIME'])
        pwr_p = self.ex10_reader.measure_adc('lo_sum')

        # Negative ramp
        self.ex10_reader.set_tx_fine_gain(-1 * tx_scalar)
        time.sleep(self.CAL_CFG['PD_SLEEP_TIME'])
        pwr_n = self.ex10_reader.measure_adc('lo_sum')

        pwr_diff = pwr_p - pwr_n
        return pwr_diff, pwr_p, pwr_n

    def set_freq_mhz(self, freq_mhz):
        self.ex10_reader.lock_synthesizer(freq_mhz=freq_mhz)
        self.power_meter.set_frequency(freq_mhz=freq_mhz)

    def reset_dflt_pwr_cal_gains(self):
        self.ex10_reader.set_tx_fine_gain(tx_scalar=self.CAL_CFG['TX_SCALAR'])
        self.ex10_reader.set_coarse_gain(
            tx_atten=self.CAL_CFG['DEFAULT_COARSE_ATT'])

    def acquire_pwr_cal_data(self, rf_filter='UPPER_BAND', cal_dc_offset=False):
        """
        Acquires data used for calibration power control parameters.
        :param rf_filter: 'UPPER_BAND' or 'LOWER_BAND'
        :param cal_dc_offset: Flag to determine if DC offset will be calibrated
        :return: Calibration data structure
        """

        self._print('Acquiring power control calibration data: ' + rf_filter)

        # Initialize data structure
        data = self.init_pwr_cal_data()

        # Set rf filter
        if rf_filter == 'UPPER_BAND':
            self.ex10_reader.set_region(
                self.CAL_CFG['DEFAULT_UPPER_FREQ_REGION'])
            freq_mhz = self.CAL_CFG['DEFAULT_UPPER_FREQ_MHZ']
            freqs = self.CAL_CFG['UPPER_FREQS_MHZ']
        elif rf_filter == 'LOWER_BAND':
            self.ex10_reader.set_region(
                self.CAL_CFG['DEFAULT_LOWER_FREQ_REGION'])
            freq_mhz = self.CAL_CFG['DEFAULT_LOWER_FREQ_MHZ']
            freqs = self.CAL_CFG['LOWER_FREQS_MHZ']
        else:
            raise ValueError('rf_filter not recognized.')

        # Initialize radio settings
        self.ex10_reader.enable_radio(antenna=self.CAL_CFG['ANTENNA'],
                                      rf_mode=self.CAL_CFG['RF_MODE'])
        self.ex10_reader.radio_power_control(True)
        # Set Tx frequency
        self.set_freq_mhz(freq_mhz)
        # Set default gains
        self.reset_dflt_pwr_cal_gains()
        # Enable the transmitter
        self.ex10_reader.tx_ramp_up()
        self.stopwatch.split('PWR CAL', rf_filter + ' init')

        # Heat the board up to operating temp
        self._print('Warming up board')
        self.ex10_reader.set_coarse_gain(self.CAL_CFG['SOAK_COARSE_ATT'])
        time.sleep(self.CAL_CFG['SOAK_TIME'])
        self.stopwatch.split('PWR CAL', rf_filter + ' warmup')

        # Calibrate across coarse attenuations
        self._print('Measuring across coarse att.')
        self._print('{:>3}  {:>5}  {:>20}  {:>3}  {:>7}'.format(
            'att', 'pwr', 'lo_pdet', 'temp', 'dc_ofs'))

        for coarse_idx, coarse_val in enumerate(self.CAL_CFG['COARSE_ATTS']):
            # Skip every odd coarse attenuation up to the specified limit
            # to reduce total calibration runtime.
            if (coarse_val > self.CAL_OPTIMIZATIONS['PRECISE_COARSE_ATT_THRESH']) and (coarse_idx % 2):
                continue
            self.ex10_reader.set_coarse_gain(coarse_val)
            # Measure values
            (data['COARSE']['FWD_PWR'][coarse_val],
             data['COARSE']['LO_PDET'][coarse_val]) = self.measure_fwd_power()
            data['COARSE']['TEMP'][coarse_val] = self.ex10_reader.measure_adc(
                'temp')
            if cal_dc_offset:
                data['DC_OFS'][coarse_val] = self.estimate_dc_offset()
                self.ex10_reader.set_tx_fine_gain(
                    tx_scalar=self.CAL_CFG['TX_SCALAR'])
            to_print = (round(data['COARSE']['FWD_PWR'][coarse_val], 1),
                        data['COARSE']['LO_PDET'][coarse_val],
                        data['COARSE']['TEMP'][coarse_val], data['DC_OFS'][coarse_val])
            self._print(to_print)
            # Stop transmitting above specified limit to protect board
            if (data['COARSE']['FWD_PWR'][coarse_val]
                    > self.CAL_CFG['FWD_PWR_LIMIT_DBM']):
                print("Over FWD_PWR_LIMIT_DBM!")
                break

        self.reset_dflt_pwr_cal_gains()

        data['TEMP'] = self.ex10_reader.measure_adc('temp')
        self.stopwatch.split('PWR CAL', rf_filter + ' coarse')

        # Calibrate across frequency
        if rf_filter == 'LOWER_BAND' and self.CAL_OPTIMIZATIONS['SKIP_LOWER_BAND_FREQ_CAL']:
            self._print('Skipping LOWER_BAND frequency calibration')
        else:
            self._print('Measuring across frequency.')
            self._print('freq         fwd_pwrs                pdet_adcs')
            for freq_idx, freq_val in enumerate(freqs):
                self.set_freq_mhz(freq_mhz=freq_val)
                for coarse_idx, coarse_val in enumerate(self.CAL_CFG['COARSE_ATTS_FREQ']):
                    self.ex10_reader.set_coarse_gain(coarse_val)

                    (pwr, pdet_adc) = self.measure_fwd_power()
                    temp_adc = self.ex10_reader.measure_adc('temp')

                    data['FREQ']['FWD_PWR'][freq_idx][coarse_idx] = pwr
                    data['FREQ']['LO_PDET'][freq_idx][coarse_idx] = pdet_adc
                    data['FREQ']['TEMP'][freq_idx][coarse_idx] = temp_adc
                to_print = (
                    freq_val,
                    [round(x, 1) for x in data['FREQ']['FWD_PWR'][freq_idx]],
                    data['FREQ']['LO_PDET'][freq_idx])
                self._print(to_print)

        self.set_freq_mhz(freq_mhz)
        self.stopwatch.split('PWR CAL', rf_filter + ' freq')

        self.power_off()
        self._print('Finished acquiring cal data ' + rf_filter)
        return data

    def acquire_rssi_cal_data(self, path_loss=0):
        """
        Acquires data used for receiver RSSI calibration/compensation
        :param path_loss: Signal generator to antenna port path loss (dB)
        :return: Calibration data structure
        """

        self._print('Acquiring RSSI calibration data.')

        # Init cal configs based on SKU and initialize data structure
        self.init_rssi_configs()
        data = self.init_rssi_data()

        # Set up defaults
        self.ex10_reader.radio_power_control(True)
        self.sig_gen.set_power_dbm(
            power=self.RSSI_DEFAULT_CFG['RX_PWR']+path_loss)
        self.sig_gen.on()

        # Set RX gains
        self.set_rssi_gains()

        # Set TX RF filter and frequency
        self.ex10_reader.set_region(self.RSSI_DEFAULT_CFG['UPPER_FREQ_REGION'])
        self.ex10_reader.lock_synthesizer(self.RSSI_DEFAULT_CFG['FREQ_MHZ'])

        self.RSSI_CFG['TEMP_COEFS'][1] = self.ex10_reader.measure_adc('temp')

        # Cal across RF Modes and RX gain settings
        self._print('----Sweeping RF modes')
        for m, mode in enumerate(self.RSSI_CFG['RF_MODES']):
            # Set mode
            self.set_mode_antenna(rf_mode=mode,
                                  blf_khz=self.BLF_KHZ[mode])
            data['MODES'][m] = self.ex10_reader.read_rssi()
        self.set_mode_antenna()

        self._print('----Sweeping PGA1 gain')
        for g, pga1 in enumerate(self.RSSI_CFG['PGA1_GAINS']):
            # Set RX Gains
            self.set_rssi_gains(pga1_gain=pga1)
            data['PGA1'][g] = self.ex10_reader.read_rssi()

        self._print('----Sweeping PGA2 gain')
        for g, pga2 in enumerate(self.RSSI_CFG['PGA2_GAINS']):
            # Set RX Gains
            self.set_rssi_gains(pga2_gain=pga2)
            data['PGA2'][g] = self.ex10_reader.read_rssi()

        self._print('----Sweeping PGA3 gain')
        for g, pga3 in enumerate(self.RSSI_CFG['PGA3_GAINS']):
            # Set RX Gains
            self.set_rssi_gains(pga3_gain=pga3)
            data['PGA3'][g] = self.ex10_reader.read_rssi()

        self._print('----Sweeping TIA gain')
        for g, tia in enumerate(self.RSSI_CFG['MIXER_GAINS']):
            # Set RX Gains
            self.set_rssi_gains(mixer_gain=tia)
            data['MIXER'][g] = self.ex10_reader.read_rssi()

        self._print('----Sweeping RX ATT gain')
        for a, rx_atten in enumerate(self.RSSI_CFG['RX_ATTS']):
            # Set RX Gains
            self.set_rssi_gains(rx_att=rx_atten)
            data['RX_ATT'][a] = self.ex10_reader.read_rssi()
        self.set_rssi_gains()

        data['DEFAULT'] = self.ex10_reader.read_rssi()

        self._print('----Sweeping FREQS')
        self.ex10_reader.set_region(self.RSSI_DEFAULT_CFG['LOWER_FREQ_REGION'])
        self.ex10_reader.lock_synthesizer(
            self.RSSI_DEFAULT_CFG['LOWER_FREQ_MHZ'])
        self.set_mode_antenna(freq_mhz=self.RSSI_DEFAULT_CFG['LOWER_FREQ_MHZ'])
        data['LOWER_FREQS_MHZ'] = self.ex10_reader.read_rssi()
        self.ex10_reader.set_region(self.RSSI_DEFAULT_CFG['UPPER_FREQ_REGION'])
        self.ex10_reader.lock_synthesizer(self.RSSI_DEFAULT_CFG['FREQ_MHZ'])
        self.set_mode_antenna(freq_mhz=self.RSSI_DEFAULT_CFG['FREQ_MHZ'])
        data['UPPER_FREQS_MHZ'] = self.ex10_reader.read_rssi()

        # Check antenna path losses
        self._print('----Sweeping ANTENNA paths')
        for a, antenna in enumerate(self.RSSI_CFG['ANTENNA_PORTS']):
            input('Change to antenna port {} and press ENTER'.format(
                antenna))
            self.set_mode_antenna(antenna=antenna)
            data['ANTENNA'][a] = self.ex10_reader.read_rssi()

        self.sig_gen.off()
        return data

    def set_rssi_gains(self,
                       rx_att=None,
                       pga1_gain=None,
                       pga2_gain=None,
                       pga3_gain=None,
                       mixer_gain=None,
                       ):

        analog_rx = aar.APPLICATION_ADDRESS_RANGE['RxGainControl']['fields']
        rx_atten_enums = analog_rx['RxAtten']['enums']
        pga1_enums = analog_rx['Pga1Gain']['enums']
        pga2_enums = analog_rx['Pga2Gain']['enums']
        pga3_enums = analog_rx['Pga3Gain']['enums']
        mixer_enums = analog_rx['MixerGain']['enums']

        rx_att = rx_att if rx_att is not None else self.RSSI_DEFAULT_CFG['RX_ATT']
        pga1_gain = pga1_gain if pga1_gain is not None else self.RSSI_DEFAULT_CFG[
            'PGA1_GAIN']
        pga2_gain = pga2_gain if pga2_gain is not None else self.RSSI_DEFAULT_CFG[
            'PGA2_GAIN']
        pga3_gain = pga3_gain if pga3_gain is not None else self.RSSI_DEFAULT_CFG[
            'PGA3_GAIN']
        mixer_gain = (mixer_gain if mixer_gain is not None else
                      self.RSSI_DEFAULT_CFG['MIXER_GAIN'])

        rx_config = {
            'RxAtten': rx_atten_enums[rx_att],
            'Pga1Gain': pga1_enums[pga1_gain],
            'Pga2Gain': pga2_enums[pga2_gain],
            'Pga3Gain': pga3_enums[pga3_gain],
            'MixerGain': mixer_enums[mixer_gain],
            'MixerBandwidth': True,
            'Pga1RinSelect': False
        }
        self.ex10_reader.set_analog_rx_config(rx_config)

    def set_mode_antenna(self,
                         antenna=None,
                         rf_mode=None,
                         freq_mhz=None,
                         blf_khz=None):
        """
        Set the signal generator frequency and device radio. If a param is not specified, use the default value
        :param antenna : device antenna setting
        :param rf_mode : device RF mode number
        :param freq_mhz: Signal generator center frequency in MHz
        :param blf_khz : Signal generator offset frequency in kHz
        """
        antenna = antenna if antenna is not None else self.RSSI_DEFAULT_CFG[
            'ANTENNA']
        freq_mhz = freq_mhz if freq_mhz is not None else self.RSSI_DEFAULT_CFG[
            'FREQ_MHZ']
        rf_mode = rf_mode if rf_mode is not None else self.RSSI_DEFAULT_CFG[
            'RF_MODE']
        blf_khz = blf_khz if blf_khz is not None else self.BLF_KHZ[
            self.RSSI_DEFAULT_CFG['RF_MODE']]

        self.sig_gen.set_freq_mhz(freq_mhz + blf_khz / 1e3)
        self.ex10_reader.enable_radio(antenna=antenna,
                                      rf_mode=rf_mode)
        self.set_rssi_gains()

    @staticmethod
    def argmin_2(num_list):
        """
        Calculates indices of a number list with the two lowest values
        :param num_list: Python list or tuple of numbers
        """
        array = list(num_list)
        index1 = array.index(min(array))
        array.pop(index1)
        index2 = array.index(min(array))
        index2 = index2 if index2 < index1 else index2 + 1  # compensate for pop
        return index1, index2

    def _valid_pdet_adc(self, adc):
        valid_min_adc, valid_max_adc = self.CAL_CFG['PDET_ADC_MIN_MAX']
        return (adc >= valid_min_adc) and adc <= valid_max_adc

    def _adc_to_power(self, adc, powers, adcs):
        if not self._valid_pdet_adc(adc):
            return -0xFF, -0xFF
        i1, i2 = self.argmin_2([abs(i - adc) for i in adcs])
        power = powers[i1] + (adc - adcs[i1]) * (
            powers[i2] - powers[i1]) / (adcs[i2] - adcs[i1])
        return round(power, 2), i1

    def store_pwr_cal_params(self, data, rf_filter):
        """
        Calculates calibration parameters from acquired data and stores them
        :param data     : calibration data from acquire_pwr_cal_data()
        :param rf_filter: 'UPPER_BAND' or 'LOWER_BAND'
        """
        self._print('Calculating cal params ' + rf_filter)

        if rf_filter == 'UPPER_BAND':
            rf_cal_prefix = 'UpperBand'
            freqs = self.CAL_CFG['UPPER_FREQS_MHZ']
        elif rf_filter == 'LOWER_BAND':
            rf_cal_prefix = 'LowerBand'
            freqs = self.CAL_CFG['LOWER_FREQS_MHZ']
        else:
            raise ValueError('rf_filter not recognized.')

        # Save cal bands for each rf_filter
        self.info.set_parameter(
            'RfFilter' + rf_cal_prefix,
            tuple([min(freqs), max(freqs)]))

        # Set temperature compensation params
        self.info.set_parameter(rf_cal_prefix + 'LoPdetTempSlope',
                                tuple(self.CAL_CFG['PDET_PWR_PER_TEMP_ADC']))

        self.info.set_parameter(rf_cal_prefix + 'FwdPowerTempSlope',
                                tuple([self.CAL_CFG['DB_PER_ADC_FWD_PWR']]))

        self.info.set_parameter(rf_cal_prefix + 'CalTemp',
                                tuple([data['TEMP']]))

        # Interpolate coarse attenuation data
        # PDET vals from op MUST be in same order
        pdet_names = [f'PDET{i}' for i in range(self.CAL_CFG['NUM_PDETS'])]
        pdet_values = np.array(data['COARSE'].pop('LO_PDET')).transpose()
        pdet_data = {pdet: list(adc)
                     for pdet, adc in zip(pdet_names, pdet_values)}

        df_coarse_data = pd.DataFrame({**data['COARSE'],
                                       **pdet_data,
                                       **{'DC_OFS': data['DC_OFS']}})
        df_interp_data = df_coarse_data.interpolate()

        default_vals = {
            'FWD_PWR': 0xFF,
            'TEMP': 0,
            'DC_OFS': 0,
            'PDET0': 0xFFF,
            'PDET1': 0xFFF,
            'PDET2': 0xFFF,
            'PDET3': 0xFFF,
        }

        df_interp_data = df_interp_data.fillna(value=default_vals)
        df_interp_data[['TEMP', 'DC_OFS'] +
                       pdet_names] = df_interp_data[['TEMP', 'DC_OFS'] + pdet_names].apply(round)
        df_interp_data = df_interp_data.convert_dtypes()
        self._print('\nInterpolated Coarse Power\n', df_interp_data)

        data['COARSE']['FWD_PWR'] = df_interp_data.FWD_PWR.tolist()
        data['COARSE']['LO_PDET'] = [
            list(map(int, pdet_adcs)) for pdet_adcs in df_interp_data[pdet_names].to_numpy()]
        data['COARSE']['TEMP'] = list(map(int, df_interp_data.TEMP.to_list()))
        data['DC_OFS'] = list(map(int, df_interp_data.DC_OFS.to_list()))

        # Store PDET ADC values as LUT
        pdet_lut = tuple(
            zip(*data['COARSE']['LO_PDET']))[0:self.CAL_CFG['NUM_PDET_BLOCKS']]
        self.info.set_parameter(
            rf_cal_prefix + 'PdetAdcLut',
            sum(pdet_lut, ()))

        # Calc freq power offsets in PDETs
        r_coarse_atts = range(len(self.CAL_CFG['COARSE_ATTS_FREQ']))
        r_pdet_blocks = range(self.CAL_CFG['NUM_PDET_BLOCKS'])
        pwr_diff_over_freq = [[0] * len(freqs) for j in r_pdet_blocks]
        if not (rf_filter == 'LOWER_BAND' and self.CAL_OPTIMIZATIONS['SKIP_LOWER_BAND_FREQ_CAL']):
            for p_ind in r_pdet_blocks:
                for c_ind in r_coarse_atts:
                    for f_ind, freq in enumerate(freqs):
                        # Calculate what the power should be from initial cal
                        adc = data['FREQ']['LO_PDET'][f_ind][c_ind][p_ind]
                        pwr, i = self._adc_to_power(adc,
                                                    data['COARSE']['FWD_PWR'],
                                                    pdet_lut[p_ind])

                        # Compensate for temperature
                        if pwr != -0xFF:  # not invalid power
                            temp_delta = data['COARSE']['TEMP'][i] - \
                                data['FREQ']['TEMP'][f_ind][c_ind]
                            pwr += self.CAL_CFG['PDET_PWR_PER_TEMP_ADC'][p_ind] * temp_delta
                            pwr_diff_over_freq[p_ind][f_ind] = int((pwr - data[
                                'FREQ']['FWD_PWR'][f_ind][c_ind]) * 100)

        pwr_diff_over_freq = tuple([f for p in pwr_diff_over_freq for f in p])
        self.info.set_parameter(
            rf_cal_prefix + 'LoPdetFreqLut',
            pwr_diff_over_freq)

        self.info.set_parameter(
            rf_cal_prefix + 'LoPdetFreqs',
            tuple(freqs))

        # Store forward power data
        self.info.set_parameter(
            rf_cal_prefix + 'FwdPowerCoarsePwrCal',
            tuple(data['COARSE']['FWD_PWR']))

        # Store forward power freq shifts
        dflt_pwr = data['COARSE']['FWD_PWR'][
            self.CAL_CFG['COARSE_ATTS_FREQ'][0]]

        if rf_filter == 'LOWER_BAND' and self.CAL_OPTIMIZATIONS['SKIP_LOWER_BAND_FREQ_CAL']:
            # Set shifts to zero for skipped lower band cal
            power_shifts_over_freq = [0 for i in data['FREQ']['FWD_PWR']]
        else:
            power_shifts_over_freq = [i[0] - dflt_pwr for i in data['FREQ'][
                'FWD_PWR']]
        self.info.set_parameter(rf_cal_prefix + 'FwdPwrFreqLut',
                                tuple(power_shifts_over_freq))

        self._print('Calibration parameters done ' + rf_filter)

    def store_dc_cal_params(self, data):
        """
        Store DC offset estimates
        :param data: calibration data from acquire_pwr_cal_data()
        """
        self.info.set_parameter('DcOffsetCal', tuple(data['DC_OFS']))

    def store_rssi_cal_params(self, rssi_data):
        """
        Store RSSI data
        :param rssi_data: calibration data from acquire_rssi_cal_data()
        """
        baseline_rssi = rssi_data['DEFAULT']  # RSSI value at default settings

        # Store baseline RSSI value and input power
        self.info.set_parameter('RssiRxDefaultPwr',
                                tuple([self.RSSI_DEFAULT_CFG['RX_PWR']]))
        self.info.set_parameter('RssiRxDefaultLog2', tuple([baseline_rssi]))

        # Store RF modes and RF mode RSSI offsets
        rf_mode_cal_pad = [0] * (self.RSSI_CFG['N_RF_MODES'] - len(
            self.RSSI_CFG['RF_MODES']))
        self.info.set_parameter('RssiRfModes',
                                tuple(self.RSSI_CFG['RF_MODES']
                                      + rf_mode_cal_pad))
        mode_offsets = [i - baseline_rssi for i in rssi_data['MODES']]
        self.info.set_parameter('RssiRfModeLut',
                                tuple(mode_offsets + rf_mode_cal_pad))

        # Store analog RX gain offsets
        pga1_offsets = [i - baseline_rssi for i in rssi_data['PGA1']]
        pga2_offsets = [i - baseline_rssi for i in rssi_data['PGA2']]
        pga3_offsets = [i - baseline_rssi for i in rssi_data['PGA3']]
        mixer_gain_offsets = [i - baseline_rssi for i in rssi_data['MIXER']]
        rx_att_offsets = [i - baseline_rssi for i in rssi_data['RX_ATT']]
        self.info.set_parameter('RssiPga1Lut', tuple(pga1_offsets))
        self.info.set_parameter('RssiPga2Lut', tuple(pga2_offsets))
        self.info.set_parameter('RssiPga3Lut', tuple(pga3_offsets))
        self.info.set_parameter('RssiMixerGainLut', tuple(mixer_gain_offsets))
        self.info.set_parameter('RssiRxAttLut', tuple(rx_att_offsets))

        # Store Antenna offsets
        antenna_cal_pad = [0] * (self.RSSI_CFG['N_ANTENNA_PORTS']
                                 - len(self.RSSI_CFG['ANTENNA_PORTS']))
        self.info.set_parameter('RssiAntennas',
                                tuple(self.RSSI_CFG['ANTENNA_PORTS']
                                      + antenna_cal_pad))
        antenna_offsets = [i - baseline_rssi for i in rssi_data['ANTENNA']]
        self.info.set_parameter('RssiAntennaLut',
                                tuple(antenna_offsets + antenna_cal_pad))

        # Store Upper + Lower Band RSSI offsets across frequency
        self.info.set_parameter('UpperBandRssiFreqOffset',
                                tuple([rssi_data['UPPER_FREQS_MHZ'] -
                                      baseline_rssi]))
        self.info.set_parameter('LowerBandRssiFreqOffset',
                                tuple([rssi_data['LOWER_FREQS_MHZ'] -
                                      baseline_rssi]))

        # Store RSSI Temp Coefficients
        self.info.set_parameter('RssiTempSlope',
                                tuple([self.RSSI_CFG['TEMP_COEFS'][0]]))
        self.info.set_parameter('RssiTempIntercept',
                                tuple([self.RSSI_CFG['TEMP_COEFS'][1]]))

    def store_cal_version_params(self, board_id):
        """
        Store calibration version number, board id, calibration tx_scalar,
        PDET limits, and power control loop params
        :param board_id: device id number
        """
        self.info.set_parameter(
            'CalibrationVersion', tuple([self.CAL_CFG['CAL_VERSION']]))

        # User board ID
        self.info.set_parameter('UserBoardId', tuple([board_id]))

        # Tx scalar
        self.info.set_parameter(
            'TxScalarCal', tuple([self.CAL_CFG['TX_SCALAR']]))

        # Valid PDET ADC range for each PDET block
        self.info.set_parameter('ValidPdetAdcs', tuple(
            self.CAL_CFG['PDET_ADC_MIN_MAX']))

        # Control loop params
        self.info.set_parameter('ControlLoopParams', tuple([self.CAL_CFG['LOOP_GAIN_DIVISOR'],
                                                            self.CAL_CFG['ADC_ERROR_THRESHOLD'],
                                                            self.CAL_CFG['MAX_ITERATIONS']]))

    def write_calibration_info_page(self, allow_cal_lock):
        """
        Create bytestring of calibration data and write to device
        """
        bytestream = self.info.to_info_page_string()
        if not allow_cal_lock:
            print('Defaulting to unlocked cal info page')
            print('by setting bytes at offset 0x7F8 to 0xFF.')
            print('See <CalibrationInfo> in the "Application Address Map" document')
            print('and the "-L" option in this script to allow locking.')

            # Set the last 8 bytes in bytestream to 0xFF to prevent
            # locking the calibration info page.
            print(f'Cal bytestream length is {len(bytestream)}')
            # 0x7F8 -> 0x7FF = 2040 -> 2047
            max_len = 0x800
            num_ffs_to_append = 0
            if len(bytestream) <= max_len and len(bytestream) > (max_len-8):
                num_ffs_to_append = len(bytestream) - (max_len-8)
                bs2 = bytestream[:len(
                    bytestream) - num_ffs_to_append] + b'\xff' * num_ffs_to_append
                bytestream = bs2
        self.ex10_reader.write_cal_info_page(bytestream)

    def read_calibration_info_page(self):
        """
        This method reads the calibration data from the info page on
        Ex10 and puts it into a calib_info object and returns it
        """
        data = self.ex10_reader.read_cal_info_page()
        self.info.from_info_page_string(data)

    def power_off(self):
        """
        Stop and ramp down
        """
        self.ex10_reader.stop_transmitting()


def run_pc_cal(uart_helper, power_meter, sig_gen, path_loss, board_id, verbose, debug_serial, stopwatch, allow_cal_lock):
    """
    Executes calibration procedure: acquires calibration data, calculates
    calibration params, and writes cal table into flash.
    :param uart_helper: helper object for serial interface
    :param power_meter: power meter VISA object
    :param sig_gen: signal generator VISA object
    :param path_loss: Signal generator to antenna port path loss (dB)
    :param board_id: R807 board ID number
    :param verbose: verbose flag for printing
    :param debug_serial: flag for dumping all serial data to/from device
    :param stopwatch: object to measure time
    :param allow_cal_lock: Allow setting cal page lock bytes if True
    """
    uart_helper.open_port(UartBaud.RATE115200)
    ex10_reader = UartReader(uart_helper)
    ex10_reader.dump_serial(debug_serial)
    stopwatch.split('READER SDK', 'setup')

    cal_example = Ex10CalibrationExample(power_meter=power_meter,
                                         sig_gen=sig_gen,
                                         ex10_reader=ex10_reader,
                                         stopwatch=stopwatch,
                                         allow_cal_lock=allow_cal_lock,
                                         verbose=verbose)
    stopwatch.split('CAL', 'setup')

    board_id, json_file_to_write = cal_example.get_board_id(board_id)
    rssi_data = cal_example.acquire_rssi_cal_data(path_loss=path_loss)
    stopwatch.split('RSSI CAL', 'stop')
    upper_data = cal_example.acquire_pwr_cal_data(rf_filter='UPPER_BAND',
                                                  cal_dc_offset=True)
    stopwatch.split('PWR CAL', 'UPPER_BAND stop')
    lower_data = cal_example.acquire_pwr_cal_data(rf_filter='LOWER_BAND')
    stopwatch.split('PWR CAL', 'LOWER_BAND stop')

    cal_example.store_cal_version_params(board_id=board_id)
    cal_example.store_pwr_cal_params(data=upper_data, rf_filter='UPPER_BAND')
    cal_example.store_pwr_cal_params(data=lower_data, rf_filter='LOWER_BAND')
    cal_example.store_dc_cal_params(data=upper_data)
    cal_example.store_rssi_cal_params(rssi_data)
    stopwatch.split('CAL', 'store')
    cal_example.info.dump_params()
    cal_example.info.to_json(json_file_to_write)
    cal_example.power_off()

    #  Writing cal info page
    cal_example.write_calibration_info_page(allow_cal_lock)
    stopwatch.split('CAL', 'write')

    # Test cal info page written correctly
    cal_example.read_calibration_info_page()
    print(*cal_example.info.dump_params(), sep=os.linesep)
    stopwatch.split('CAL', 'readback')

    print('Calibration complete.')


def run_pc_cal_example(stopwatch):
    print('Ex10 Development Kit Calibration example')

    try:
        # Set up
        parser = argparse.ArgumentParser(
            description='Calibrates Ex10 R807 development board from PC')
        parser.add_argument('-v', '--verbose',
                            default=False,
                            action='store_true',
                            help='Verbose flag')
        parser.add_argument('-b', '--board_id', type=int,
                            help='Serial number of R807 board, '
                                 'between 0 and 65535')
        parser.add_argument('-d', '--debug_serial', default=False, action='store_true',
                            help='Flag to print out all RX serial data')
        parser.add_argument('-o', '--power_offset', type=float,
                            help='Power offset setting for power meter')
        parser.add_argument('-l', '--path_loss', type=float,
                            help='Path loss in sig-gen to R807 for RSSI cal')
        parser.add_argument('-L', '--allow-cal-lock', type=bool,
                            help='By default, cal info page lock is set to 0xFF '
                                 'to prevent locking the page by mistake. '
                                 'When this option is used, all data is written '
                                 'as-is, including the lock bytes.')
        args = parser.parse_args()

        stopwatch.start()

        # Use selected port with speed 115200, 8n1
        uart_helper = UartHelper()
        uart_helper.choose_serial_port(com_port)
        stopwatch.split('READER HW', 'init')

        # 2023-11-17
        # Updated by Kim Young Bong
        # power_meter class replaced from PowerMeter to mini_circuit.PowerMeter()
        # power_meter = PowerMeter(addr=power_meter_addr, ofs=args.power_offset)
        power_meter = mini_circuit.PowerMeter()

        stopwatch.split('INST', 'power meter init')
        sig_gen = KeysightSigGen(address=sig_gen_addr)
        stopwatch.split('INST', 'signal generator init')

        run_pc_cal(uart_helper, power_meter, sig_gen,
                   args.path_loss, args.board_id, args.verbose, args.debug_serial, stopwatch, args.allow_cal_lock)
        stopwatch.stop()

    finally:
        uart_helper.shutdown()

        if args.verbose:
            print('\nCalibration timeline:')
            for label, time_s, category in zip(stopwatch.labels, stopwatch.times, stopwatch.categories):
                print(f'{time_s: 7.1f} || {category:10} || {label}')

            print('\nCalibration time breakdown:')
            for label, time_s, category in zip(stopwatch.lap_labels, stopwatch.lap_times, stopwatch.lap_categories):
                if time_s <= 5e-2:
                    continue
                print(f'{time_s: 7.1f} || {category:10} || {label}')

            print(f'\n{stopwatch.times[-1]:7.1f} || Total')


if __name__ == "__main__":
    stopwatch = StopWatch()
    stopwatch.start()
    run_pc_cal_example(stopwatch)
