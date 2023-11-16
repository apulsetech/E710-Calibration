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
The inventory example below uses 'fixed q' test mode and is not optimized for
 read rate.
"""

from __future__ import (division, absolute_import, print_function,
                        unicode_literals)

# pylint: disable=locally-disabled, wildcard-import, unused-wildcard-import
from py2c_interface.py2c_python_wrapper import *


# The configuration of the inventory example is set using the definitions below
INVENTORY_DURATION_S = 10           # Duration of inventory operation (seconds)

TRANSMIT_POWER_CDBM = 3000          # 30 dBm
RF_MODE = RfModes.mode_148
R807_ANTENNA_PORT = 1               # Which R807 antenna port will be used

INITIAL_Q = 4                       # Q field in the Query command
SELECT_ALL = 0                      # SELECT field in the Query command
SESSION = 0                         # Session field in the Query command
TARGET = 0                          # Target field in the Query command
DUAL_TARGET = True                  # If True, the Target of subsequent Query
                                    # commands will flip
HALT_ON_ALL_TAGS = False            # If True, the Ex10 will halt on each
                                    # inventoried tag
TAG_FOCUS_ENABLE = False            # Tells a tag to be silent after inventoried
                                    # once.
FAST_ID_ENABLE = False              # Tells a tag to backscatter the TID during
                                    # inventory


def run_inventory_example():
    # pylint: disable=missing-docstring
    print('Starting inventory example')
    try:
        # Init the python to C layer
        py2c = Ex10Py2CWrapper()
        ex10_result = py2c.ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, Ex10RegionId.REGION_FCC.value)
        if ex10_result.error:
            py2c.print_ex10_result(ex10_result)
            py2c.ex10_typical_board_teardown()
            raise RuntimeError('ex10_typical_board_setup() failed')
        ex10_ifaces = py2c.get_ex10_interfaces()

        total_singulations = run_inventory(ex10_ifaces, INVENTORY_DURATION_S,
                                           INITIAL_Q, RF_MODE)
        print('Total Singulations: {}'.format(total_singulations))
        if total_singulations <= 0:
            raise RuntimeError('No tags found in inventory')

        print('Ending inventory example')
    finally:
        py2c.ex10_typical_board_teardown()


def run_inventory(ex10_ifaces, duration_s, initial_q, rf_mode):
    """ Run inventory for the specified amount of time """
    helper = ex10_ifaces.helpers

    # Put together configurations for the inventory round
    inventory_config = InventoryRoundControlFields()
    inventory_config.initial_q = initial_q
    inventory_config.max_q = initial_q
    inventory_config.min_q = initial_q
    inventory_config.num_min_q_cycles = 1
    inventory_config.fixed_q_mode = True
    inventory_config.q_increase_use_query = False
    inventory_config.q_decrease_use_query = False
    inventory_config.session = SESSION
    inventory_config.select = SELECT_ALL
    inventory_config.target = 0
    inventory_config.halt_on_all_tags = HALT_ON_ALL_TAGS
    inventory_config.fast_id_enable = FAST_ID_ENABLE
    inventory_config.tag_focus_enable = TAG_FOCUS_ENABLE

    inventory_config_2 = InventoryRoundControl_2Fields()
    inventory_config_2.max_queries_since_valid_epc = 0

    packet_info = InfoFromPackets(0, 0, 0, 0, TagReadData())

    ihp = InventoryHelperParams()
    ihp.antenna               = R807_ANTENNA_PORT
    ihp.rf_mode               = rf_mode
    ihp.tx_power_cdbm         = TRANSMIT_POWER_CDBM
    ihp.inventory_config      = pointer(inventory_config)
    ihp.inventory_config_2    = pointer(inventory_config_2)
    ihp.send_selects          = False
    ihp.remain_on             = False
    ihp.dual_target           = DUAL_TARGET
    ihp.inventory_duration_ms = duration_s * 1000
    ihp.packet_info           = pointer(packet_info)
    ihp.enforce_gen2_response = False
    ihp.verbose               = True

    assert 0 == helper.simple_inventory(ihp)

    return packet_info.total_singulations


if __name__ == "__main__":
    run_inventory_example()
