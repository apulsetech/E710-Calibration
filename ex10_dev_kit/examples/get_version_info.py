#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2019 - 2023 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
""" Test the VersionInfo class """

from __future__ import (division, absolute_import, print_function,
                        unicode_literals)

import argparse
import sys

from py2c_interface.py2c_python_wrapper import *


def _print_bootloader_version(py2c):
    """
    Print the bootloader version.
    Note that this will reset the Ex10 device to enter the bootloader.
    It will reset the Ex10 device a second time and attempt to start the
    Application.
    """
    ver_info = (c_char * VERSION_STRING_SIZE)()
    ex10_version = py2c.get_ex10_version()
    ex10_version.get_bootloader_info(ver_info, VERSION_STRING_SIZE)
    print(ver_info.value.decode("utf-8"))


def _print_application_version(py2c, ex10_protocol):
    """ Print the application version """
    # Attempt to reset into the Application
    ex10_protocol.reset(Status.Application)

    ver_info = (c_char * VERSION_STRING_SIZE)()
    image_validity = ImageValidityFields(1, 1)
    remain_reason = RemainReasonFields()
    remain_reason.remain_reason = 0
    ex10_version = py2c.get_ex10_version()
    ex10_version.get_application_info(ver_info, VERSION_STRING_SIZE, pointer(image_validity), pointer(remain_reason))
    print(ver_info.value.decode("utf-8"))

    if (image_validity.image_valid_marker) and not (image_validity.image_non_valid_marker):
        print("Application image VALID\n")
    else:
        print("Application image INVALID\n")

    reason_c_string = py2c.get_ex10_helpers().get_remain_reason_string(remain_reason.remain_reason)
    reason_string = (c_char * REMAIN_REASON_STRING_MAX_SIZE)()
    ctypes.memmove(reason_string, reason_c_string, REMAIN_REASON_STRING_MAX_SIZE)
    print("Remain in bootloader reason: {}".format(reason_string.value.decode("utf-8")))


def _print_device_version(py2c):
    """ Print the device version """
    ex10_version = py2c.get_ex10_version()
    dev_info = ex10_version.get_device_info()
    print(dev_info.decode("utf-8"))


def _print_sku(py2c, ex10_protocol):
    """ Print the device SKU """
    # Attempt to reset into the Application
    ex10_protocol.reset(Status.Application)

    ex10_version = py2c.get_ex10_version()
    sku_val = ex10_version.get_sku()
    if sku_val == ProductSku.SkuUnknown:
        print("SKU: Unknown\n")
    elif sku_val == ProductSku.SkuE310:
        print("SKU: E310\n")
    elif sku_val == ProductSku.SkuE510:
        print("SKU: E510\n")
    elif sku_val == ProductSku.SkuE710:
        print("SKU: E710\n")
    else:
        print("SKU: Unknown\n")


def main():
    """ Run the get_version_info() with user supplied parameters """

    parser = argparse.ArgumentParser(description='Get Version example')

    parser.add_argument('-a', '--application', action='store_true',
                        help='Print the application version information')

    parser.add_argument('-b', '--bootloader', action='store_true',
                        help='Print the Bootloader version information '
                        '(this will reset the device into the bootloader to'
                        ' retrieve the bootloader version and reset back to'
                        ' the application)')

    parser.add_argument('-d', '--device', action='store_true',
                        help='Print the Ex10 device version info')

    parser.add_argument('-s', '--sku', action='store_true',
                        help='Print the Ex10 SKU info')

    args = parser.parse_args()

    # If no argument provided, set all to True and print all
    if len(sys.argv) == 1:
        for arg in vars(args):
            setattr(args, arg, True)

    py2c = Ex10Py2CWrapper()
    ex10_result = py2c.ex10_bootloader_board_setup(BOOTLOADER_SPI_CLOCK_HZ)
    if ex10_result.error:
        py2c.print_ex10_result(ex10_result)
        py2c.ex10_bootloader_board_teardown()
        raise RuntimeError('ex10_bootloader_board_setup() failed')
    ex10_protocol = py2c.get_ex10_protocol()

    try:
        if args.application:
            _print_application_version(py2c, ex10_protocol)

        if args.bootloader:
            _print_bootloader_version(py2c)

        if args.device:
            _print_device_version(py2c)

        if args.sku:
            _print_sku(py2c, ex10_protocol)

    finally:
        py2c.ex10_typical_board_teardown()


if __name__ == "__main__":
    main()
