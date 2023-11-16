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
"""
Script to upload a passed in binary file to the device
"""
from __future__ import division, absolute_import
from __future__ import print_function, unicode_literals
import os
import sys
import argparse
import struct

from py2c_interface.py2c_python_wrapper import *


UPLOAD_FLASH = 1
VERSION_STRING_SIZE = 120

def main():
    """
    Grabs a proper app image from the user and uploads it to the device.
    """
    file_with_image = None

    parser = argparse.ArgumentParser(
        description='Uploads binary apps to Ex10')
    parser.add_argument('-i', '--in_bin', help='Impinj binary file to upload')

    args = parser.parse_args()

    if args.in_bin is not None:
        if os.path.isfile(args.in_bin):
            file_with_image = args.in_bin
        else:
            raise RuntimeError('file {} cannot be opened'.format(args.in_bin))

    if file_with_image is None:
        parser.print_help(sys.stdout)
        exit(1)

    py2c = Ex10Py2CWrapper()
    ex10_result = py2c.ex10_bootloader_board_setup(BOOTLOADER_SPI_CLOCK_HZ)
    if ex10_result.error:
        py2c.print_ex10_result(ex10_result)
        py2c.ex10_bootloader_board_teardown()
        raise RuntimeError('ex10_bootloader_board_setup() failed')
    ex10_protocol = py2c.get_ex10_protocol()

    assert ex10_protocol.get_running_location() == Status.Bootloader
    try:
        if file_with_image is not None:
            with open(file_with_image, 'rb') as read_file:
                image_to_upload = read_file.read()
                assert image_to_upload

            image_array = ctypes.c_uint8 * len(image_to_upload)
            upload_info = ConstByteSpan(
                image_array.from_buffer(bytearray(image_to_upload)),
                len(image_to_upload))
            print("Uploading image...")
            ex10_protocol.upload_image(UPLOAD_FLASH, upload_info)
            print("Done")

            ex10_protocol.reset(Status.Application)

            str_arr = c_char * VERSION_STRING_SIZE
            ver_info = str_arr()
            image_validity = ImageValidityFields(1, 1)
            py2c.get_ex10_version().get_application_info(ver_info, VERSION_STRING_SIZE, pointer(image_validity), None)

            print(ver_info.value.decode("utf-8"))

            if (image_validity.image_valid_marker) and not (image_validity.image_non_valid_marker):
                print("Application image VALID\n")
            else:
                print("Application image INVALID\n")

            # On a successful upload, we should be running from the application here
            assert(ex10_protocol.get_running_location() == Status.Application)
    finally:
        py2c.ex10_bootloader_board_teardown()


if __name__ == "__main__":
    main()
