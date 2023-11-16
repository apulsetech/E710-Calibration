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
The script below is an inventory example. This pass/fail criteria is that
at least 1 tag is inventoried.
"""

from __future__ import (division, absolute_import, print_function,
                        unicode_literals)
from datetime import datetime
import threading

# pylint: disable=locally-disabled, wildcard-import, unused-wildcard-import
from py2c_interface.py2c_python_wrapper import *


# The configuration of the inventory example is set using the definitions below
TRANSMIT_POWER_CDBM = 3000          # Transmit power (dBm)
RF_MODE = RfModes.mode_103
REGION = Ex10RegionId.REGION_FCC.value    # Regulatory region
R807_ANTENNA_PORT = 1               # Which R807 antenna port will be used

INITIAL_Q = 4                       # Q field in the Query command
                                    # (starting Q for dynamic Q algorithm)
MAX_Q = 15                          # Max Q value for dynamic Q
MIN_Q = 0                           # Min Q value for dynamic Q
MIN_Q_CYCLES = 2                    # Number of times we stay on the minimum Q
                                    # before ending the inventory round
MAX_QUERIES_SINCE_VALID_EPC = 16    # Number of queries seen since last good
                                    # EPC before ending the inventory round
SELECT_ALL = 0                      # SELECT field in the Query command
SESSION = 0                         # Session field in the Query command
TARGET = 0                          # Target field in the Query command
HALT_ON_ALL_TAGS = False            # If True, the Ex10 will halt on each
                                    # inventoried tag
TAG_FOCUS_ENABLE = False            # Tells a tag to be silent after inventoried
                                    # once.
FAST_ID_ENABLE = False              # Tells a tag to backscatter the TID during
                                    # inventory

packet_info = InfoFromPackets(0, 0, 0, 0, TagReadData())

# pylint: disable=locally-disabled, invalid-name, global-statement
# Indicates whether new packets are available in Ex10Reader
packets_available = threading.Condition()


def run_continuous_inventory_example():
    # pylint: disable=missing-docstring
    print('Starting continuous inventory example')
    try:
        # Init the python to C layer
        py2c = Ex10Py2CWrapper()
        run_continuous_inventory(py2c, INITIAL_Q)
    finally:
        print('Ending continuous inventory example')
        py2c.ex10_typical_board_teardown()


def run_continuous_inventory(py2c, initial_q):
    """ Run inventory for the specified amount of time """
    ex10_result = py2c.ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION)
    if ex10_result.error:
        py2c.print_ex10_result(ex10_result)
        py2c.ex10_typical_board_teardown()
        raise RuntimeError('ex10_typical_board_setup() failed')
    ex10_ifaces = py2c.get_ex10_interfaces()
    helpers = ex10_ifaces.helpers

    # Put together configurations for the inventory round
    inventory_config = InventoryRoundControlFields()
    inventory_config.initial_q = initial_q
    inventory_config.max_q = MAX_Q
    inventory_config.min_q = MIN_Q
    inventory_config.num_min_q_cycles = MIN_Q_CYCLES
    inventory_config.fixed_q_mode = False
    inventory_config.q_increase_use_query = False
    inventory_config.q_decrease_use_query = False
    inventory_config.session = SESSION
    inventory_config.select = SELECT_ALL
    inventory_config.target = 0
    inventory_config.halt_on_all_tags = HALT_ON_ALL_TAGS
    inventory_config.fast_id_enable = FAST_ID_ENABLE
    inventory_config.tag_focus_enable = TAG_FOCUS_ENABLE

    inventory_config_2 = InventoryRoundControl_2Fields()
    inventory_config_2.max_queries_since_valid_epc = MAX_QUERIES_SINCE_VALID_EPC

    helpers.discard_packets(False, True, False)
    continuous_inventory_stop_on_host_request(py2c,
                                              ex10_ifaces,
                                              inventory_config,
                                              inventory_config_2)

    helpers.discard_packets(False, True, False)
    continuous_inventory_stop_on_max_tags_count(ex10_ifaces,
                                                inventory_config,
                                                inventory_config_2)

    helpers.discard_packets(False, True, False)
    continuous_inventory_stop_on_inventory_round_count(ex10_ifaces,
                                                       inventory_config,
                                                       inventory_config_2)

    helpers.discard_packets(False, True, False)
    continuous_inventory_stop_on_duration(ex10_ifaces,
                                          inventory_config,
                                          inventory_config_2)

    # Cleanup and exit
    ex10_ifaces.reader.stop_transmitting()


def continuous_inventory_stop_on_host_request(
    py2c, ex10_ifaces, inventory_config, inventory_config_2):
    """
    Continuous inventory with host stop requested
    """
    ex10_reader = ex10_ifaces.reader
    helpers = ex10_ifaces.helpers
    # event_parser = ex10_ifaces.event_parser
    event_parser = py2c.get_ex10_event_parser()

    start_time = datetime.now()
    time_delta = datetime.now() - start_time
    inventory_done = False
    timeout_s = 90
    dual_target = True

    stop_conditions = StopConditions()
    stop_conditions.max_number_of_tags   = 0
    stop_conditions.max_duration_us      = 0
    stop_conditions.max_number_of_rounds = 0

    print("Starting continuous inventory with user stop")
    ex10_result = ex10_reader.continuous_inventory(R807_ANTENNA_PORT,
                                     RF_MODE,
                                     TRANSMIT_POWER_CDBM,
                                     inventory_config,
                                     inventory_config_2,
                                     None,
                                     stop_conditions,
                                     dual_target,
                                     0,
                                     False)
    assert ex10_result.error is False

    stop_requested = False
    request_stop_duration_s = 5

    while not inventory_done:
        time_delta = datetime.now() - start_time
        if (time_delta.total_seconds() > request_stop_duration_s and
                not stop_requested):
            print('Requesting to stop')
            ex10_reader.stop_transmitting()
            stop_requested = True

        if ex10_reader.packets_available():
            packet = ex10_reader.packet_peek().contents
            helpers.examine_packets(packet, packet_info)

            if packet.packet_type == EventPacketType.TagRead:
                tag_read = packet.static_data.contents.tag_read

                # Example of how to get back the EPC
                tag_read_fields = event_parser.get_tag_read_fields(packet.dynamic_data,
                                                           packet.dynamic_data_length,
                                                           tag_read.type,
                                                           tag_read.tid_offset)
                epc_buf = [tag_read_fields.epc[i] for i in range(tag_read_fields.epc_length)]
                print('Current tag EPC: {}'.format(epc_buf))
                # Example of how to print the compensated RSSI
                compensated_rssi = ex10_reader.get_current_compensated_rssi(tag_read.rssi)
                print('Compensated RSSI for current tag read: {}'.format(compensated_rssi))
            elif packet.packet_type == EventPacketType.ContinuousInventorySummary:
                inventory_done = True
                cont_summary = ContinuousInventorySummary()
                ctypes.memmove(pointer(cont_summary), packet.static_data, sizeof(cont_summary))
                assert cont_summary.reason == StopReason.SRHost
                print("Inventory stopped")
            ex10_reader.packet_remove()
        if time_delta.total_seconds() > timeout_s:
            break

    assert time_delta.total_seconds() <= timeout_s


def continuous_inventory_stop_on_max_tags_count(
    ex10_ifaces, inventory_config, inventory_config_2):
    """
    Continuous inventory with stop on number of singulated tags
    """

    ex10_reader = ex10_ifaces.reader
    helpers = ex10_ifaces.helpers
    max_tags_count = 20
    stop_conditions = StopConditions()
    stop_conditions.max_number_of_tags   = max_tags_count
    stop_conditions.max_duration_us      = 0
    stop_conditions.max_number_of_rounds = 0

    start_time = datetime.now()
    time_delta = datetime.now() - start_time
    inventory_done = False
    timeout_s = 90
    dual_target = True

    print("Starting Continuous inventory with tag count stop")
    ex10_result = ex10_reader.continuous_inventory(R807_ANTENNA_PORT,
                                     RF_MODE,
                                     TRANSMIT_POWER_CDBM,
                                     inventory_config,
                                     inventory_config_2,
                                     None,
                                     stop_conditions,
                                     dual_target,
                                     0,
                                     False)
    assert ex10_result.error is False

    while not inventory_done:
        if ex10_reader.packets_available():
            packet = ex10_reader.packet_peek().contents
            helpers.examine_packets(packet, packet_info)

            if packet.packet_type == EventPacketType.ContinuousInventorySummary:
                inventory_done = True
                cont_summary = ContinuousInventorySummary()
                ctypes.memmove(pointer(cont_summary), packet.static_data, sizeof(cont_summary))
                print("Inventory stopped")
            ex10_reader.packet_remove()
        time_delta = datetime.now() - start_time
        if time_delta.total_seconds() > timeout_s:
            break

    assert cont_summary.reason == StopReason.SRMaxNumberOfTags
    assert packet_info.total_singulations >= max_tags_count
    assert time_delta.total_seconds() <= timeout_s


def continuous_inventory_stop_on_inventory_round_count(
    ex10_ifaces, inventory_config, inventory_config_2):
    """
    Continuous inventory with stop on number of inventory round
    """
    ex10_reader = ex10_ifaces.reader

    start_time = datetime.now()
    time_delta = datetime.now() - start_time
    inventory_done = False
    timeout_s = 60
    dual_target = True

    max_inventory_round_count = 6
    stop_conditions = StopConditions()
    stop_conditions.max_number_of_tags   = 0
    stop_conditions.max_duration_us      = 0
    stop_conditions.max_number_of_rounds = max_inventory_round_count

    print("Starting continuous inventory with round count stop")
    ex10_result = ex10_reader.continuous_inventory(R807_ANTENNA_PORT,
                                     RF_MODE,
                                     TRANSMIT_POWER_CDBM,
                                     inventory_config,
                                     inventory_config_2,
                                     None,
                                     stop_conditions,
                                     dual_target,
                                     0,
                                     False)
    assert ex10_result.error is False

    while not inventory_done:
        while ex10_reader.packets_available():
            packet = ex10_reader.packet_peek().contents

            if packet.packet_type == EventPacketType.ContinuousInventorySummary:
                inventory_done = True
                cont_summary = ContinuousInventorySummary()
                ctypes.memmove(pointer(cont_summary), packet.static_data, sizeof(cont_summary))
                print("Inventory stopped")
            ex10_reader.packet_remove()
        time_delta = datetime.now() - start_time
        if time_delta.total_seconds() > timeout_s:
            break

    assert cont_summary.reason == StopReason.SRMaxNumberOfRounds
    assert time_delta.total_seconds() <= timeout_s


def continuous_inventory_stop_on_duration(
    ex10_ifaces, inventory_config, inventory_config_2):
    """
    Continuous inventory with stop on inventory duration_us
    """
    ex10_reader = ex10_ifaces.reader

    start_time = datetime.now()
    time_delta = datetime.now() - start_time
    max_duration_s = 3
    inventory_done = False
    timeout_s = 60
    dual_target = True

    stop_conditions = StopConditions()
    stop_conditions.max_number_of_tags   = 0
    stop_conditions.max_duration_us      = max_duration_s * 1000 * 1000
    stop_conditions.max_number_of_rounds = 0

    print("Starting continuous inventory with duration_us stop")
    ex10_result = ex10_reader.continuous_inventory(R807_ANTENNA_PORT,
                                     RF_MODE,
                                     TRANSMIT_POWER_CDBM,
                                     inventory_config,
                                     inventory_config_2,
                                     None,
                                     stop_conditions,
                                     dual_target,
                                     0,
                                     False)
    assert ex10_result.error is False

    while not inventory_done:
        while ex10_reader.packets_available():
            packet = ex10_reader.packet_peek().contents

            if packet.packet_type == EventPacketType.ContinuousInventorySummary:
                inventory_done = True
                cont_summary = ContinuousInventorySummary()
                ctypes.memmove(pointer(cont_summary), packet.static_data, sizeof(cont_summary))
                print("Inventory stopped")
            ex10_reader.packet_remove()
        time_delta = datetime.now() - start_time
        if time_delta.total_seconds() > timeout_s:
            break

    assert cont_summary.reason == StopReason.SRMaxDuration
    assert time_delta.total_seconds() <= timeout_s


if __name__ == "__main__":
    run_continuous_inventory_example()
