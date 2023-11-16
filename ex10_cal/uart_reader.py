#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2021 - 2023 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
"""
An Ex10 Reader interface for calibration, expressed in terms of a simple UART
protocol
"""

import binascii
import pathlib
from enum import Enum
import ex10_api.mnemonics as mne


class UartCommand(Enum):
    """
    Serial protocol commands for calibration
    """
    UPG_START = '^ s'
    UPG_CONTINUE = '^ c'
    UPG_COMPLETE = '^ e'
    RXCONFIG = 'a'
    TXATTEN = 'c'
    TXRAMPDOWN = 'd'
    ENABLERADIO = 'e'
    TXFINEGAIN = 'f'
    PRINTHELP = 'h'
    PRINTHELPALT = '?'
    READCALINFO = 'i c'
    COMPRSSI = 'j'
    DEVICESKU = 'k'
    LOCKSYNTH = 'l'
    MEASUREADC = 'm'
    RESET_BOOTLOADER = 'n b'
    RESET_APPLICATION = 'n a'
    LBTRSSI = 'o'
    POWERCNTRL = 'p'
    QUITUARTIF = 'q'
    REGION = 'r'
    READRSSI = 's'
    CWTEST = 't'
    TXRAMPUP = 'u'
    SETVERBOSE = 'v'
    WRITECALINFO = 'w'
    STOPTX = 'x'


class UartReader():
    """
    Ex10 reader interface class over UART, for PC calibration example
    """

    ADC_ENUMS = {'enums' : {
        'PowerLo0'    : 0,
        'PowerLo1'    : 1,
        'PowerLo2'    : 2,
        'PowerLo3'    : 3,
        'PowerRx0'    : 4,
        'PowerRx1'    : 5,
        'PowerRx2'    : 6,
        'PowerRx3'    : 7,
        'TestMux0'    : 8,
        'TestMux1'    : 9,
        'TestMux2'    : 10,
        'TestMux3'    : 11,
        'Temperature' : 12,
        'PowerLoSum'  : 13,
        'PowerRxSum'  : 14,
        }}

    def __init__(self, uart_helper):
        self.uart_helper = uart_helper
        self.debug_dump = False
        self._adc_config = {
            'lo': ['PowerLo0', 'PowerLo1', 'PowerLo2', 'PowerLo3'],
            'rx': ['PowerRx0', 'PowerRx1', 'PowerRx2', 'PowerRx3'],
            'temp': ['Temperature'],
            'lo_sum': ['PowerLoSum'],
            'rx_sum': ['PowerRxSum'], }

    @staticmethod
    def _hex_padded_bytes(value, length):
        hex_string = str(hex(value))[2:]
        if length > len(hex_string):
            padding = '0' * (length - len(hex_string))
            hex_string = padding + hex_string
        return hex_string

    def cw_test(self,
                antenna,
                rf_mode,
                tx_power_dbm,
                freq_mhz,
                remain_on=False):
        """
        Ramp up power and transmit CW using the C SDK over serial.
        """
        tx_ramp_down_cmd = UartCommand.TXRAMPDOWN.value
        self.uart_helper.send_and_receive(tx_ramp_down_cmd)

        cw_test_cmd = (UartCommand.CWTEST.value + ' ' + str(antenna) + ' ' +
                      str(rf_mode) + ' ' + str(int(tx_power_dbm * 100)) +
                      ' ' + str(int(freq_mhz * 1000)))
        if remain_on is True:
            cw_test_cmd += ' 1'
        else:
            cw_test_cmd += ' 0'

        self.uart_helper.send_and_receive(cw_test_cmd)

    def dump_serial(self, enable=None):
        """
        Set/toggle serial dump mode for debug.
        """
        dump_serial_cmd = UartCommand.SETVERBOSE.value
        if enable is True:
            dump_serial_cmd += ' 1'
            self.debug_dump = True
        elif enable is False:
            dump_serial_cmd += ' 0'
            self.debug_dump = False
        elif enable is None:
            self.debug_dump = not self.debug_dump
        self.uart_helper.set_verbose(self.debug_dump)
        self.uart_helper.send_and_receive(dump_serial_cmd)

    def enable_radio(self, antenna, rf_mode):
        """
        Enable radio. Sets GPIO, calls RadioPowerControl and SetRfMode ops
        :param antenna: antenna port to enable
        :param rf_mode: Rf mode to use
        """
        enable_radio_cmd = (UartCommand.ENABLERADIO.value + ' ' + str(antenna) + ' ' + str(rf_mode))
        self.uart_helper.send_and_receive(enable_radio_cmd)

    def get_sku(self):
        """
        Ask device to return SKU (examples: '0310', '0510', '0710')
		"""
        sku_cmd = UartCommand.DEVICESKU.value
        sku = self.uart_helper.send_and_receive(sku_cmd)
        return sku

    def lock_synthesizer(self, freq_mhz):
        """
        Convert requested frequency to kHz, then call lock_synthesizer op
        :param freq_mhz: Channel frequency in MHz
        """
        lock_synth_cmd = UartCommand.LOCKSYNTH.value + ' ' + str(int(freq_mhz * 1000))
        self.uart_helper.send_and_receive(lock_synth_cmd)

    def measure_adc(self, select):
        """
        Measure ADC channel
        :param select: Channel to measure. Call MeasureAdc op to configure
                       ADC, and then call GetAdcMeasurement op for result
        """
        cmd_base = UartCommand.MEASUREADC.value

        responses = []
        for field in self._adc_config[select]:
            request = self.ADC_ENUMS['enums'][field]
            measure_adc_cmd = cmd_base + ' ' + str(request)

            value_received, adc_value = self.uart_helper.send_and_receive(measure_adc_cmd)
            if value_received is False or adc_value == '':
                #Perhaps an exception would be helpful here
                print("ERROR reading ADC request {}".format(select))
                adc_value = -1
            responses.append(int(adc_value))

        if len(self._adc_config[select]) != 1:
            return responses

        return responses[0]

    def radio_power_control(self, enable):
        """
        Enable or disable radio power control.
        """
        radio_power_cmd = UartCommand.POWERCNTRL.value
        if enable is True:
            radio_power_cmd += ' 1'
        elif enable is False:
            radio_power_cmd += ' 0'
        self.uart_helper.send_and_receive(radio_power_cmd)

    def read_cal_info_page(self):
        """
        Read cal info page from device flash memory, and return hex dump
        """
        self.uart_helper.clear_page_buffer()
        read_info_page_cmd = UartCommand.READCALINFO.value
        self.uart_helper.send_and_receive(read_info_page_cmd)
        return self.uart_helper.get_page_buffer()

    def read_rssi(self):
        """
        Read the RSSI using the MeasureRssiOp
        """
        read_rssi_cmd = UartCommand.READRSSI.value

        value_received, rssi_value = self.uart_helper.send_and_receive(read_rssi_cmd)
        if value_received is False or rssi_value == '':
            #Perhaps an exception would be helpful here
            print("ERROR reading RSSI")
            rssi_value = -1
        return int(rssi_value)

    def compensated_rssi(self, antenna, rf_mode):
        """
        Get the compensated RSSI using reader->get_current_compensated_rssi
        """
        read_comp_rssi_cmd = UartCommand.COMPRSSI.value + ' ' + str(antenna) + ' ' + str(rf_mode)

        value_recieved, comp_rssi_value = self.uart_helper.send_and_receive(read_comp_rssi_cmd)
        if value_recieved is False or comp_rssi_value == '':
            print("ERROR reading compensated RSSI")
            comp_rssi_value = -1
        return comp_rssi_value

    def read_lbt_rssi(self, antenna, frequency_khz, lbt_offset, rssi_count, override_used):
        """
        Get the compensated LBT RSSI using reader->get_listen_before_talk_rssi
        """
        read_lbt_rssi_cmd = (UartCommand.LBTRSSI.value + ' ' + str(antenna) + ' ' + str(int(frequency_khz)) + ' '
                             + str(lbt_offset) + ' ' + str(rssi_count) + ' ' + str(int(override_used)))

        value_recieved, lbt_rssi_value = self.uart_helper.send_and_receive(read_lbt_rssi_cmd)
        if value_recieved is False or lbt_rssi_value == '':
            # Perhaps an exception would be helpful here
            print("ERROR reading lbt RSSI")
            lbt_rssi_value = -1
        return lbt_rssi_value

    def set_analog_rx_config(self, analog_rx_dict):
        """
        Set gain for the individual blocks of the receiver.
        """
        register = 'RxGainControl'
        config = mne.get_template(register)
        for field, setting in analog_rx_dict.items():
            config[register][field] = setting

        config = mne.mnemonic_to_bytes(config, 0)[4:6]
        val = int.from_bytes(config, byteorder='little', signed=False)
        reg_val_hex = str(hex(val))[2:]
        set_rx_config_cmd = UartCommand.RXCONFIG.value + ' ' + str(reg_val_hex)
        self.uart_helper.send_and_receive(set_rx_config_cmd)

    def set_coarse_gain(self, tx_atten):
        """
        Set tx coarse gain (tx_atten) value
        :param tx_atten: Attenuation to use [0-30]
        """
        tx_atten_cmd = UartCommand.TXATTEN.value + ' ' + str(tx_atten)
        self.uart_helper.send_and_receive(tx_atten_cmd)

    def set_region(self, region):
        """
        Initialize (or reinitialize) region channel table
        :param region: FCC or ETSI_LOWER
        """
        set_region_cmd = UartCommand.REGION.value + ' ' + region
        self.uart_helper.send_and_receive(set_region_cmd)

    def set_tx_fine_gain(self, tx_scalar):
        """
        Set tx_scalar fine gain value
        :param tx_scalar: Scalar to use - signed 12 bit value
        """
        tx_fine_gain_cmd = UartCommand.TXFINEGAIN.value + ' ' + str(tx_scalar)
        self.uart_helper.send_and_receive(tx_fine_gain_cmd)

    def stop_transmitting(self):
        """
        TxRampDown/Stop transmitting. Calls Stop op and TxRampDown op.
        """
        stop_tx_cmd = UartCommand.STOPTX.value
        self.uart_helper.send_and_receive(stop_tx_cmd)

    def tx_ramp_down(self):
        """
        TX ramp down op
        """
        tx_ramp_down_cmd = UartCommand.TXRAMPDOWN.value
        self.uart_helper.send_and_receive(tx_ramp_down_cmd)

    def tx_ramp_up(self, dc_offset=0):
        """
        TX ramp with no regulatory timers, using TxRampUp Op
        :param dc_offset: DC offset for transmitter (32-bit value)
        """
        tx_ramp_up_cmd = UartCommand.TXRAMPUP.value + ' ' + str(dc_offset)
        self.uart_helper.send_and_receive(tx_ramp_up_cmd)

    def upgrade_image(self, image_filename):
        image_len = pathlib.Path(image_filename).stat().st_size

        with open(image_filename, mode='rb') as image_file:
            image_data = image_file.read()

        # Must be in bootloader
        self.uart_helper.send_and_receive(UartCommand.RESET_BOOTLOADER.value)

        # Use the maximum bootloader xfer size less 2 bytes (one byte for command code
        # and one byte for the destination), less 2 bytes (start_upload).
        BOOTLOADER_MAX_COMMAND_SIZE = 2048 + 4
        MAX_CHUNK_SIZE = BOOTLOADER_MAX_COMMAND_SIZE - 2 - 2

        chunk_size = MAX_CHUNK_SIZE
        bytes_left = len(image_data)
        chunk_start = 0
        chunk_end = chunk_size

        while bytes_left > 0:
            if chunk_start == 0:
                # Start upload
                cmd = UartCommand.UPG_START.value + " " + str(image_len)
            else:
                cmd = UartCommand.UPG_CONTINUE.value

            chunk_data = image_data[chunk_start:chunk_end]
            chunk_crc16 = binascii.crc_hqx(chunk_data, 0xFFFF)
            cmd += ' ' + str(chunk_crc16)

            for byte in image_data[chunk_start:chunk_end]:
                cmd += ' {:02x}'.format(byte)
            self.uart_helper.send_and_receive(cmd)

            if (bytes_left < chunk_size):
                chunk_size = bytes_left
            bytes_left -= chunk_size

            chunk_start += chunk_size
            chunk_end += chunk_size

        # Complete upload
        self.uart_helper.send_and_receive(UartCommand.UPG_COMPLETE.value)

        # Back to application
        self.uart_helper.send_and_receive(UartCommand.RESET_APPLICATION.value)


    def write_cal_info_page(self, bytestream):
        """
        Write cal bytes to device flash memory. Iterate over bytestream,
        sending data as hex dump which is stored by the microcontroller.
        Then send write command to write stored data to calibration page.

        Note: echoed characters are received as hex dump lines, altering
        page buffer. This is harmless.
        """
        write_info_page_cmd = UartCommand.WRITECALINFO.value
        self.uart_helper.send_and_receive(write_info_page_cmd)

        offset = 0
        while offset < len(bytestream):
            if offset + 16 < len(bytestream):
                line = bytestream[offset:offset + 16]
            else:
                line = bytestream[offset:]
            line = self._hex_padded_bytes(offset, 4) + \
                   str(': ') + binascii.hexlify(line, ' ', 1).decode('ascii')
            self.uart_helper.send_and_receive(line)
            offset += 16

        write_info_page_cmd = UartCommand.WRITECALINFO.value
        self.uart_helper.send_and_receive(write_info_page_cmd)
