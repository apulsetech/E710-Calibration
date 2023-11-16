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
Test running the Etsi Burst op
"""

from __future__ import (division, absolute_import, print_function,
                        unicode_literals)
from datetime import datetime

from py2c_interface.py2c_python_wrapper import *


ETSI_BURST_TIME_ON = 6              # Duration of ETSI Burst operation (seconds)
TRANSMIT_POWER_CDBM = 3000          # 30 dBm
RF_MODE = RfModes.mode_241
REGION = Ex10RegionId.REGION_FCC.value    # Regulatory region
R807_ANTENNA_PORT = 1               # Which R807 antenna port will be used

INITIAL_Q = 4                       # Q field in the Query command
SELECT_ALL = 0                      # SELECT field in the Query command
SESSION = 0                         # Session field in the Query command
DUAL_TARGET = True                  # If True, the Target of subsequent Query


def run_etsi_burst(ex10_ifaces, region):
    # pylint: disable=missing-docstring
    ex10_reader = ex10_ifaces.reader
    ex10_ops = ex10_ifaces.ops
    helper = ex10_ifaces.helpers

    print('Starting etsi burst test')

    # Put together configurations for the inventory round
    inventory_config = InventoryRoundControlFields()
    inventory_config.initial_q = INITIAL_Q
    inventory_config.max_q = INITIAL_Q
    inventory_config.min_q = INITIAL_Q
    inventory_config.num_min_q_cycles = 1
    inventory_config.fixed_q_mode = True
    inventory_config.q_increase_use_query = False
    inventory_config.q_decrease_use_query = False
    inventory_config.session = SESSION
    inventory_config.select = SELECT_ALL
    inventory_config.target = 0
    inventory_config.halt_on_all_tags = False
    inventory_config.fast_id_enable = False
    inventory_config.tag_focus_enable = False

    inventory_config_2 = InventoryRoundControl_2Fields()
    inventory_config_2.max_queries_since_valid_epc = 0

    # Events to look for to ensure etsi burst is working
    ramp_down_seen = False
    ramp_up_seen = False
    summary_seen = False

    # Params for etsi burst
    on_time_ms = 40
    off_time_ms = 10
    freq_khz = 0

    try:
        # Run ETSI Burst
        ex10_result = ex10_ifaces.ops.wait_op_completion();
        assert ex10_result.error is False
        ex10_result = ex10_reader.etsi_burst_test(
            inventory_config,
            inventory_config_2,
            R807_ANTENNA_PORT,
            RF_MODE,
            TRANSMIT_POWER_CDBM,
            on_time_ms,
            off_time_ms,
            freq_khz)
        assert ex10_result.error is False

        start_time = datetime.now()
        time_delta = datetime.now() - start_time
        while True:
            while ex10_reader.packets_available():
                packet = ex10_reader.packet_peek().contents
                if packet.packet_type == EventPacketType.TxRampDown:
                    ramp_down_seen = True
                elif packet.packet_type == EventPacketType.TxRampUp:
                    ramp_up_seen = True
                elif packet.packet_type == EventPacketType.InventoryRoundSummary:
                    summary_seen = True

                ex10_reader.packet_remove()
            # Wait for the ETSI Burst time to expire
            time_delta = datetime.now() - start_time
            if time_delta.total_seconds() > ETSI_BURST_TIME_ON:
                break

        # Bring the op to a stop since it will continue forever
        ex10_ops.stop_op()
        # Ensure we saw one of everything
        assert ramp_down_seen == True
        assert ramp_up_seen == True
        assert summary_seen == True

    finally:
        ex10_reader.stop_transmitting()
        print('Ending etsi burst test')


def run_etsi_burst_example():
    # pylint: disable=missing-docstring
    try:
        # Init the python to C layer
        py2c = Ex10Py2CWrapper()
        ex10_result = py2c.ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION)
        if ex10_result.error:
            py2c.print_ex10_result(ex10_result)
            py2c.ex10_typical_board_teardown()
            raise RuntimeError('ex10_typical_board_setup() failed')
        ex10_ifaces = py2c.get_ex10_interfaces()

        run_etsi_burst(ex10_ifaces, REGION)
    finally:
        py2c.ex10_typical_board_teardown()


if __name__ == "__main__":
    run_etsi_burst_example()
