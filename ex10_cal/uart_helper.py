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
A collection of UART helper functions, wrapped in a class
"""

from enum import Enum
import serial
import serial.tools.list_ports


class UartBaud(Enum):
    """
    Supported serial port bit rates
    """
    RATE115200 = 115200
    RATE56700  = 57600
    RATE28800  = 28800
    RATE19200  = 19200


class UartHelper():
    """
    Helper class for serial interface
    """

    def __init__(self):
        self.port    = None
        self.uart_if = None
        self.page_buffer       = bytearray(2048)
        self.page_buffer_empty = True
        self.debug_dump = False

    def choose_serial_port(self, desired_serial_port=None):
        """
        Enumerate available serial ports and request user to select one
        """

        serial_ports = []

        interactive = True
        found_desired_serial_port = False
        if desired_serial_port:
            interactive = False

        if interactive:
            print('Device #: Device Name - Description - Hardware ID')
            print('----------------------------------------------------------------')
        for idx, (device, description, hwid) in enumerate(serial.tools.list_ports.comports()):
            if device == desired_serial_port:
                found_desired_serial_port = True
            if interactive:
                print(str(idx)+':       ', device, '-', description, '-', hwid)
            serial_ports.append(device)

        if not interactive:
            if found_desired_serial_port:
                self.port = desired_serial_port
                return
            else:
                raise Exception('Specified serial port device ' + desired_serial_port +
                    ' not found in ' + str(serial_ports) )

        if not serial_ports:
            raise Exception('No serial ports found.')

        selected_port = None
        while selected_port is None:
            selected_port = input('Enter device number: ')
            try:
                idx = int(selected_port)
                if not 0 <= idx < len(serial_ports):
                    print('Device number out of range')
                    selected_port = None
                    continue
            except ValueError:
                print('Use device number')
                selected_port = None
                continue
            else:
                selected_port = serial_ports[idx]
        self.port = selected_port

    def open_port(self, baud_rate=UartBaud.RATE115200):
        """
        Open previously selected serial port with requested speed
        :param baud_rate: enum for baud rate desired

        :returns: a serial port object for the opened port
        """
        print("Opening port {} at speed {}".format(self.port, baud_rate.value))
        try:
            self.uart_if = serial.Serial(self.port, baud_rate.value, timeout = 1)
        except serial.SerialException as ser_except:
            raise Exception('Could not open port {} at speed {}'.format
                            (self.port, baud_rate.value)) from ser_except

    def _parse_hexdump_line(self, line):
        """
        After receiving a line of data over the serial interface, check if it
        is part of a hex dump, and then parse the ascii string to hex bytes,
        and copy them to correct offset in page_buffer

        Expected line format:
            '00000000: FF FF FF FF FF FF FF FF    FF FF FF FF FF FF FF FF \r\n'
        """
        if line[8:9] == b':':
            # Get the base address from the first line
            if self.page_buffer_empty:
                self.base_addr = int(line[:8], 16)
            try:
                ofs = int(line[:8], 16) - self.base_addr
                if 0 <= ofs <= 2032:
                    cnt = 0
                    for _, val in enumerate(line[9:].strip().split(b' ')):
                        if val != b'':
                            self.page_buffer[ofs + cnt: ofs + cnt + 1] = \
                                int(val, 16).to_bytes(1, 'little')
                            self.page_buffer_empty = False
                            cnt += 1
            except ValueError:
                # Not a hex dump line - continue
                pass

    def send_and_receive(self, uart_command):
        """
        Send command bytes to Ex10 via UART, terminated with newline. Reads
        responses from Ex10/R807, printing each line. Response is complete when
        'OK' or 'ERROR' is received.  Returns error and also the last message,
        or parsed result, from the returned messages.
        :param uart_command: the string to be sent
        :returns: success status, last message string
        """
        self.uart_if.write(uart_command.encode('ascii'))
        self.uart_if.write(b'\n')
        if (self.debug_dump):
            print("TX: {}".format(uart_command))

        response = b''
        message = ""
        count = 0
        result = False
        result_value = ""
        while message != 'OK':
            response = self.uart_if.readline()
            message = str(response.decode('ascii')).strip()
            if(self.debug_dump):
                print("RX:",message)
            if response == b'':
                count +=1
            if count > 5:
                raise Exception('Device is not responding. Is ex10_wrapper running?')
            if message == 'OK':
                return True, result_value
            if message == 'ERROR':
                return False, result_value
            if not result:
                if message.startswith('Result: '):
                    result = True
                    result_value = message[8:]
            if not result:
                # Check for hexdump
                self._parse_hexdump_line(response)

    def set_verbose(self, verbose):
        """
        Set verbose mode.
        """
        self.debug_dump = verbose

    def clear_page_buffer(self):
        """
        Reinitialize the contents of uart_helper's info page buffer to 0
        """
        self.page_buffer.__init__(2048)
        self.page_buffer_empty = True

    def dump_page_buffer(self):
        """
        Print contents of uart_helper's info page buffer
        """
        if self.page_buffer_empty is False:
            print(self.page_buffer)
        else:
            print("page_buffer is empty")

    def get_page_buffer(self):
        """
        Return contents of info page buffer at immutable bytes
        """
        return bytes(self.page_buffer)

    def shutdown(self):
        """
        Close serial port
        """
        if self.uart_if:
            self.uart_if.close()
        self.uart_if = None
