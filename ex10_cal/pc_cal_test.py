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

import time
import argparse
import os
import sys

# Append this script's parent dir to path so that modules in ex10_api can be
# imported with "ex10_api.module".
parent_dir = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
sys.path.append(parent_dir)

# Equipment addresses can be GPIB or USB.  For example
# addr="GPIB0::19::INSTR", etc.
power_meter_addr = "USB0::0x2A8D::0x0701::MY59260011::0::INSTR"
sig_gen_addr = "USB0::0x0957::0x2018::0116C599::INSTR"


from power_meter import PowerMeter
from sig_gen import KeysightSigGen
from uart_helper import UartBaud, UartHelper
from uart_reader import UartReader
import calib_info as calib_info
import ex10_api.mnemonics as mne
import ex10_api.application_address_range as aar


class Ex10CalibrationTest:
    """
    Calibration test class for Ex10 reader.
    """
    # The configuration of the cal test is set using the definitions below
    SLEEP_TIME = 0.2  # Time to wait for power meter read
    RF_MODE = 5  # RF mode
    R807_ANTENNA_PORT = 1  # Antenna select
    UPPER_REGION_EXAMPLE = 'FCC'  # Example of upper band region
    LOWER_REGION_EXAMPLE = 'ETSI LOWER'  # Example of lower band region
    FREQUENCIES = [865.7, 867.5, 903.25, 915.75, 927.25]  # Test frequencies
    CW_TOL_HIGH_PWR = 0.5  # CW accuracy tolerance >20 dBm
    CW_TOL_LOW_PWR = 1.0  # CW accuracy tolerance <20 dBm
    DC_OFFSET_TOL = 0.2  # DC offset accuracy tolerance
    DC_OFFSET_TOL_LOW_PWR = 50000 # No DC offset tolerance <10 dBm
    MIN_TX_PWR = 10.0  # 10 dBm is the min power of the supported Tx range
    CW_RANGE_HIGH = [20, 25, 30, 31]  # Tx output >20 dBm
    CW_RANGE_LOW = [5, 10, 15]  # Tx output <20 dBm
    DC_OFFSET_TEST_FREQ = 903.25  # Frequency in MHz to test DC offset
    DC_COARSE_ATTS = [7, 10, 15, 20, 25, 30]  # Coarse gains for DC offset test

    # The configuration of the RSSI test is set using the definitions below
    RSSI_ANTENNAS = [1]  # RSSI test antenna
    RSSI_REGION_FREQ_MHZ_PAIRS = [  # RSSI test region/frequency pairs
        ("ETSI LOWER", 866.3),
        ("FCC", 915.25)
    ]
    RSSI_RX_PWRS = [-60, -50]  # dB Rx input power
    RSSI_ERROR_TOL_DB = 1.0  # dB error tolerance
    BLF_KHZ = {
        1: 640,
        3: 320,
        5: 320,
        7: 250,
        11: 640,
        12: 320,
        13: 160,
        15: 640
    }
    RSSI_MODES = [5, 13]
    RSSI_GAINS = [{
        'RX_ATTS': 'Atten_0_dB',
        'MIXER_GAINS': 'Gain_11p2_dB',
        'PGA1_GAINS': 'Gain_12_dB',
        'PGA2_GAINS': 'Gain_0_dB',
        'PGA3_GAINS': 'Gain_18_dB',
    }]

    # RSSI LBT Configurations
    RSSI_LBT_ANTENNAS = [1]  # RSSI test antenna
    RSSI_LBT_FREQS_MHZ = [916.8, 918.0, 919.2, 920.4]  # RSSI test frequencies
    RSSI_LBT_RX_PWRS = [-74]
    RSSI_LBT_LOWER_BOUND_DB = -0.5  # dB lower bound tolerance
    RSSI_LBT_UPPER_BOUND_DB = 1.5  # dB upper bound tolerance
    NUM_LBT_RSSI_READS = 5

    def __init__(self, power_meter, sig_gen, path_loss, ex10_reader, verbose=True):
        self.power_meter = power_meter
        self.sig_gen = sig_gen
        self.path_loss = path_loss
        self.ex10_reader = ex10_reader
        self.cal_info = calib_info.CalibrationInfoPageAccessor()
        cal_data = self.ex10_reader.read_cal_info_page()
        self.cal_info.from_info_page_string(cal_data)
        self.verbose = verbose

    def _print(self, *args):
        if self.verbose:
            print(*args)

    def test_cw_on(self,
                   freq_mhz,
                   power_level):
        """
        Tests power ramp accuracy
        :param freq_mhz: Tx LO frequency
        :param power_level: 'high' or 'low' input power
        :return: A list of cw power-specific tuples.
        """
        if power_level == 'high':
            power_targets = self.CW_RANGE_HIGH
        else:
            power_targets = self.CW_RANGE_LOW
        results = [(0,)] * len(power_targets)
        print('Testing power ramp at {} MHz at {} power'.format(freq_mhz,
                                                                power_level))
        # Loop through power values
        for power_ind, power_target in enumerate(power_targets):
            self.ex10_reader.cw_test(antenna=self.R807_ANTENNA_PORT,
                                     rf_mode=self.RF_MODE,
                                     tx_power_dbm=power_target,
                                     freq_mhz=freq_mhz,
                                     remain_on=True)
            time.sleep(self.SLEEP_TIME)  # Delay to allow power meter to settle
            power_pm = self.power_meter.read_power()

            results[power_ind] = (power_target,
                                  round(power_pm, 1),
                                  round(power_pm - power_target, 2))
            print(results)
        return results

    def test_dc_offset(self):
        """"
        Tests DC offset compenation accuracy
        :return: A list of DC offset specific tuples.
        """
        results = [(0,)] * len(self.DC_COARSE_ATTS)
        # Set up hardware
        print('Testing DC offsets')
        # Loop through coarse gain values
        for c_ind, c_val in enumerate(self.DC_COARSE_ATTS):
            dc_offset = self.cal_info.get_parameter('DcOffsetCal')[c_val]
            self.ex10_reader.set_coarse_gain(c_val)
            self.ex10_reader.set_tx_fine_gain(
                self.cal_info.get_parameter('TxScalarCal')[0])
            self.ex10_reader.tx_ramp_up(dc_offset=dc_offset)

            pwr_diff, pwr_p, pwr_n = self.get_fwd_pwr_diff(
                (c_val + 3) * 50)  # Set Tx_scalar roughly

            results[c_ind] = (c_val,
                              dc_offset,
                              round(pwr_p, 1),
                              round(pwr_n, 1),
                              round(pwr_diff, 2))
        return results

    def get_fwd_pwr_diff(self, tx_scalar):
        """
        Gets the power difference between positive and negative Tx-ramps
        :param tx_scalar: Fine gain power scalar
        :return: dB power difference, dBm pos power, dBm neg power
        """
        self.ex10_reader.set_tx_fine_gain(tx_scalar)
        time.sleep(self.SLEEP_TIME)
        pwr_p = self.power_meter.read_power()
        self.ex10_reader.set_tx_fine_gain(-1 * tx_scalar)
        time.sleep(self.SLEEP_TIME)
        pwr_n = self.power_meter.read_power()
        pwr_diff = pwr_p - pwr_n
        return pwr_diff, pwr_p, pwr_n

    def set_frequency(self, freq_mhz):
        """
        Sets up RF filter based on the Tx frequency
        :param freq_mhz: Target frequency in MHz
        """
        lower_rf_filter_bands = self.cal_info.get_parameter(
            'RfFilterLowerBand')
        upper_rf_filter_bands = self.cal_info.get_parameter(
            'RfFilterUpperBand')

        if round(lower_rf_filter_bands[0], 2) <= freq_mhz <= round(
                lower_rf_filter_bands[1], 2):
            region = self.LOWER_REGION_EXAMPLE
            self.ex10_reader.set_region(region)
            rf_cal_prefix = 'LowerBand'
        elif round(upper_rf_filter_bands[0], 2) <= freq_mhz <= round(
                upper_rf_filter_bands[1], 2):
            region = self.UPPER_REGION_EXAMPLE
            self.ex10_reader.set_region(region)
            rf_cal_prefix = 'UpperBand'
        else:
            raise ValueError('Frequency {} MHz not in calibration range'.
                             format(freq_mhz))

        self.ex10_reader.enable_radio(antenna=self.R807_ANTENNA_PORT,
                                      rf_mode=self.RF_MODE)
        self.ex10_reader.radio_power_control(True)
        self.ex10_reader.lock_synthesizer(freq_mhz)
        self.power_meter.set_frequency(freq_mhz)
        return region, rf_cal_prefix

    def power_off(self):
        """
        Shuts down CW and radio power, and closes SPI connection.
        """
        # Disable transmitter
        self.ex10_reader.stop_transmitting()

    def set_rssi_gains(self, rx_att, pga1_gain, pga2_gain, pga3_gain, mixer_gain):
        analog_rx = aar.APPLICATION_ADDRESS_RANGE['RxGainControl']['fields']
        rx_atten_enums = analog_rx['RxAtten']['enums']
        pga1_enums = analog_rx['Pga1Gain']['enums']
        pga2_enums = analog_rx['Pga2Gain']['enums']
        pga3_enums = analog_rx['Pga3Gain']['enums']
        mixer_enums = analog_rx['MixerGain']['enums']

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

    def set_mode_antenna(self, antenna, rf_mode, freq_mhz, blf_khz):
        """
        Set the signal generator frequency and device radio. If a param is not specified, use the default value
        :param antenna : device antenna setting
        :param rf_mode : device RF mode number
        :param freq_mhz: Signal generator center frequency in MHz
        :param blf_khz : Signal generator offset frequency in kHz
        """
        self.sig_gen.set_freq_mhz(freq_mhz + blf_khz / 1e3)
        self.ex10_reader.enable_radio(antenna=antenna,
                                      rf_mode=rf_mode)

    def run_power_control_tests(self):
        start_time = time.time()

        # warm board
        region, _ = self.set_frequency(915)
        self.ex10_reader.cw_test(self.R807_ANTENNA_PORT,
                                 self.RF_MODE,
                                 30,
                                 915,
                                 True)
        time.sleep(self.SLEEP_TIME)

        # Test CW_on accuracy
        n_cw_high_errs = 0
        n_cw_low_errs = 0
        for freq_mhz in self.FREQUENCIES:
            self.set_frequency(freq_mhz)
            cw_pwr_returns_high = self.test_cw_on(freq_mhz, 'high')
            cw_pwr_returns_low = self.test_cw_on(freq_mhz, 'low')

            # Compare power desired to power measured
            for cw_pwr_return_high in cw_pwr_returns_high:
                self._print(cw_pwr_return_high)
            for cw_pwr_return_low in cw_pwr_returns_low:
                self._print(cw_pwr_return_low)

            n_cw_high_array = [abs(r[2]) > self.CW_TOL_HIGH_PWR for r in
                               cw_pwr_returns_high]
            n_cw_low_array = [abs(r[2]) > self.CW_TOL_LOW_PWR for r in
                              cw_pwr_returns_low]
            n_cw_high_errs += sum(n_cw_high_array)
            n_cw_low_errs += sum(n_cw_low_array)

        # Test DC offset estimation accuracy
        self.set_frequency(self.DC_OFFSET_TEST_FREQ)
        dc_offset_returns = self.test_dc_offset()

        for dc_offset_return in dc_offset_returns:
            self._print(dc_offset_return)
            if dc_offset_return[2] < self.MIN_TX_PWR:
                tolerance = self.DC_OFFSET_TOL_LOW_PWR
            else:
                tolerance = self.DC_OFFSET_TOL
        n_dc_array = [abs(r[4]) > tolerance for r in dc_offset_returns]
        n_dc_errs = sum(n_dc_array)
        self.power_off()

        test_time = time.time()

        print('Test time: {}'.format(round(test_time - start_time)))

        self._print('{} high-pwr CW errors'.format(
            n_cw_high_errs)) if n_cw_high_errs else None
        self._print('{} low-pwr CW errors'.format(
            n_cw_low_errs)) if n_cw_low_errs else None
        self._print('{} DC-offset errors'.format(
            n_dc_errs)) if n_dc_errs else None

        # RSSI Rx default
        rssi_rx_default = int(self.cal_info.get_parameter('RssiRxDefaultLog2')[0])
        self._print(f'RSSI Rx default: {rssi_rx_default}')

        rssi_rx_errs = 0

        try:
            num_rssi_accuracy_errors = self.run_rssi_tests()
            self._print('{} rssi accuracy errors'.format(
                num_rssi_accuracy_errors
            )) if num_rssi_accuracy_errors else None
            num_lbt_rssi_accuracy_errors = self.run_lbt_rssi_tests()
            self._print('{} lbt rssi accuracy errors'.format(
                num_lbt_rssi_accuracy_errors
            )) if num_lbt_rssi_accuracy_errors else None
            rssi_rx_errs = rssi_rx_errs + num_rssi_accuracy_errors + num_lbt_rssi_accuracy_errors
        finally:
            self.sig_gen.off()

        return (n_cw_high_errs,
                n_cw_low_errs,
                n_dc_errs,
                rssi_rx_errs)

    def run_rssi_tests(self):
        self.sig_gen.on()
        self.ex10_reader.radio_power_control(True)
        n_rssi_errors = 0
        for region, freq_mhz in self.RSSI_REGION_FREQ_MHZ_PAIRS:
            self.sig_gen.set_freq_mhz(freq_mhz)
            self.ex10_reader.set_region(region)
            self.ex10_reader.lock_synthesizer(freq_mhz)
            for rx_pwr in self.RSSI_RX_PWRS:
                self.sig_gen.set_power_dbm(rx_pwr + self.path_loss)
                for antenna in self.RSSI_ANTENNAS:
                    for rf_mode in self.RSSI_MODES:
                        for rx_cfg in self.RSSI_GAINS:

                            self.set_mode_antenna(
                                antenna=antenna,
                                rf_mode=rf_mode,
                                freq_mhz=freq_mhz,
                                blf_khz=self.BLF_KHZ[rf_mode])
                            self.set_rssi_gains(
                                rx_att=rx_cfg['RX_ATTS'],
                                mixer_gain=rx_cfg['MIXER_GAINS'],
                                pga1_gain=rx_cfg['PGA1_GAINS'],
                                pga2_gain=rx_cfg['PGA2_GAINS'],
                                pga3_gain=rx_cfg['PGA3_GAINS'],
                            )

                            rssi_compensated = int(self.ex10_reader.compensated_rssi(
                                rf_mode=rf_mode,
                                antenna=antenna,
                            ))
                            rssi_dbm_compensated = rssi_compensated / 100
                            rssi_err = rssi_dbm_compensated - rx_pwr

                            print(
                                '| MODE {}'.format(rf_mode)
                                + '| FREQ {}'.format(freq_mhz)
                                + '| PWR {}'.format(rx_pwr)
                                + '| RSSI_DBM {:6.2f}'.format(rssi_dbm_compensated)
                                + '| RSSI_ERR {:6.2f}'.format(rssi_err)
                            )

                            if abs(rssi_err) > self.RSSI_ERROR_TOL_DB:
                                n_rssi_errors += 1

        self.sig_gen.off()
        return n_rssi_errors

    def run_lbt_rssi_tests(self):
        self.sig_gen.on()
        self.ex10_reader.radio_power_control(True)
        n_rssi_errors = 0
        for antenna in self.RSSI_LBT_ANTENNAS:
            for freq_mhz in self.RSSI_LBT_FREQS_MHZ:
                self.sig_gen.set_freq_mhz(freq_mhz)
                self.ex10_reader.set_region('JAPAN')
                self.ex10_reader.lock_synthesizer(freq_mhz)
                for rx_pwr in self.RSSI_LBT_RX_PWRS:
                    max_rx_rssi = -100
                    for i in range(self.NUM_LBT_RSSI_READS):
                        self.sig_gen.set_power_dbm(rx_pwr + self.path_loss)

                        self.set_mode_antenna(
                            antenna=antenna,
                            rf_mode=13,
                            freq_mhz=freq_mhz,
                            blf_khz=0)
                        self.set_rssi_gains(
                            rx_att=self.RSSI_GAINS[0]['RX_ATTS'],
                            mixer_gain=self.RSSI_GAINS[0]['MIXER_GAINS'],
                            pga1_gain=self.RSSI_GAINS[0]['PGA1_GAINS'],
                            pga2_gain=self.RSSI_GAINS[0]['PGA2_GAINS'],
                            pga3_gain=self.RSSI_GAINS[0]['PGA3_GAINS'],
                        )

                        rssi_compensated = int(self.ex10_reader.read_lbt_rssi(
                            antenna=antenna,
                            frequency_khz=freq_mhz*1e3,
                            lbt_offset=-200,
                            rssi_count=11,
                            override_used=False
                        ))
                        rssi_dbm_compensated = rssi_compensated / 100
                        if rssi_dbm_compensated > max_rx_rssi:
                            max_rx_rssi = rssi_dbm_compensated

                    rssi_err = max_rx_rssi - rx_pwr

                    print(
                        '| FREQ {}'.format(freq_mhz)
                        + '| PWR {}'.format(rx_pwr)
                        + '| RSSI_DBM {:6.2f}'.format(max_rx_rssi)
                        + '| RSSI_ERR {:6.2f}'.format(rssi_err)
                    )

                    if not (self.RSSI_LBT_LOWER_BOUND_DB < rssi_err < self.RSSI_LBT_UPPER_BOUND_DB):
                        n_rssi_errors += 1

        self.sig_gen.off()
        return n_rssi_errors


def run_pc_cal_test(uart_helper: UartHelper, power_meter, sig_gen, path_loss, verbose, debug_serial):

    uart_helper.open_port(UartBaud.RATE115200)

    ex10_reader = UartReader(uart_helper)
    ex10_reader.dump_serial(debug_serial)
    cal_test = Ex10CalibrationTest(power_meter=power_meter,
                                   sig_gen=sig_gen,
                                   path_loss=path_loss,
                                   ex10_reader=ex10_reader,
                                   verbose=verbose)

    n_cw_high_errs, n_cw_low_errs, n_dc_errs, rssi_rx_errs = cal_test.run_power_control_tests()
    num_power_accuracy_errors = sum([n_cw_high_errs, n_cw_low_errs, n_dc_errs])
    print('Total number of power accuracy errors: {}'.format(
        num_power_accuracy_errors))
    print('Total number of rssi errors: {}'.format(rssi_rx_errs))

    print('Calibration test complete.')

    return n_cw_high_errs, n_cw_low_errs, n_dc_errs, rssi_rx_errs


if __name__ == "__main__":
    print('Ex10 Development Kit PC Calibration Test')

    parser = argparse.ArgumentParser(
        description='Tests Ex10 R807 development board calibration from PC')
    parser.add_argument('-d', '--debug_serial', default=False, action='store_true',
                        help='Flag to print out all RX serial data')
    parser.add_argument('-v', '--verbose', default=False, action='store_true',
                        help='Flag to print out cal and test data')
    parser.add_argument('-o', '--power_offset', type=float,
                        help='Power offset setting for power meter')
    parser.add_argument('-l', '--path_loss', type=float,
                        help='Path loss in sig-gen to R807 for RSSI cal')
    args = parser.parse_args()

    # Use selected port with speed 115200, 8n1
    uart_helper = UartHelper()
    uart_helper.choose_serial_port()

    power_meter = PowerMeter(addr=power_meter_addr, ofs=args.power_offset)
    sig_gen = KeysightSigGen(address=sig_gen_addr)

    try:
        run_pc_cal_test(uart_helper, power_meter, sig_gen, args.path_loss, args.verbose, args.debug_serial)
    finally:
        sig_gen.off()
        uart_helper.shutdown()
