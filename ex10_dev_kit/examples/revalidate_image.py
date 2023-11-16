#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2020 - 2023 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
"""
Run the ReValidateImage command and print the results.
"""
from __future__ import division, absolute_import
from __future__ import print_function, unicode_literals

from py2c_interface.py2c_python_wrapper import *


def main():
    # pylint: disable=missing-docstring
    """
    Run the ReValidateImage command and print the results.
    """
    try:
        # Init the python to C layer
        py2c = Ex10Py2CWrapper()
        ex10_result = py2c.ex10_bootloader_board_setup(BOOTLOADER_SPI_CLOCK_HZ)
        if ex10_result.error:
            py2c.print_ex10_result(ex10_result)
            py2c.ex10_bootloader_board_teardown()
            raise RuntimeError('ex10_bootloader_board_setup() failed')
        ex10_protocol = py2c.get_ex10_protocol()

        assert ex10_protocol.get_running_location() == Status.Bootloader
        image_validity = ex10_protocol.revalidate_image()

        valid = bool(image_validity.image_valid_marker)
        non_valid = bool(image_validity.image_non_valid_marker)
        image_valid = valid and not non_valid

        print('Image marked   valid: {}'.format(valid))
        print('Image marked invalid: {}'.format(non_valid))
        print('Image is       valid: {}'.format(image_valid))
        
    finally:
        py2c.ex10_bootloader_board_teardown()


if __name__ == "__main__":
    main()
