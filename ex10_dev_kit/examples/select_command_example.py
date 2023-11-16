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
The script below is an example to show how to send a Gen2 Select command using
the Ex10 reader API.
"""

from __future__ import (division, absolute_import, print_function,
                        unicode_literals)
import ctypes

# pylint: disable=locally-disabled, wildcard-import, unused-wildcard-import
from py2c_interface.py2c_python_wrapper import *


# The configuration of the inventory example is set using the definitions below
INVENTORY_DURATION_S = 1            # Duration of inventory operation (seconds)
TRANSMIT_POWER_CDBM = 3000          # 30 dBm
RF_MODE = RfModes.mode_148
REGION = Ex10RegionId.REGION_FCC.value    # Regulatory region
R807_ANTENNA_PORT = 1               # Which R807 antenna port will be used
INITIAL_Q = 4                       # Q field in the Query command
SELECT_ALL = 0                      # SELECT field in the Query command
SELECT_NOT_ASSERT = 2
SELECT_ASSERT = 3
SESSION = 0                         # Session field in the Query command
TARGET = 0                          # Target field in the Query command
DUAL_TARGET = True                  # If True, the Target of subsequent Query
                                    # commands will flip

packet_info = InfoFromPackets(0, 0, 0, 0, TagReadData())


def run_inventory_rounds(helper, select_type):
    """
    Runs an inventory round.
    :param ex10_reader: The reader object
    :param select_type: The type of select to configure the
                        inventory round using.
    """
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
    inventory_config.select = select_type
    inventory_config.target = TARGET
    inventory_config.halt_on_all_tags = False
    inventory_config.fast_id_enable = False
    inventory_config.tag_focus_enable = False

    inventory_config_2 = InventoryRoundControl_2Fields()
    inventory_config_2.max_queries_since_valid_epc = 0

    ihp = InventoryHelperParams()
    ihp.antenna               = R807_ANTENNA_PORT
    ihp.rf_mode               = RF_MODE
    ihp.tx_power_cdbm         = TRANSMIT_POWER_CDBM
    ihp.inventory_config      = pointer(inventory_config)
    ihp.inventory_config_2    = pointer(inventory_config_2)
    ihp.send_selects          = False if select_type == SELECT_ALL else True
    ihp.remain_on             = False
    ihp.dual_target           = DUAL_TARGET
    ihp.inventory_duration_ms = INVENTORY_DURATION_S * 1000
    ihp.packet_info           = pointer(packet_info)
    ihp.enforce_gen2_response = False
    ihp.verbose               = True

    assert 0 == helper.simple_inventory(ihp)


def load_select_commands(tx_command_manager, selected_tag_crc, crc_byte_len):
    """
    Loads select commands into the Gen2 Buffer
    :param selected_tag_crc: The crc to use in building the select command.
    :param crc_byte_len: The length of the crc to use as the mask
    """
    print('Loading commands into Gen2 command buffer')

    crc_bytes = bytearray.fromhex('{:04x}'.format(selected_tag_crc))
    crc_bytes.reverse()
    arraytype = ctypes.c_uint8 * crc_byte_len
    mask_arr = arraytype()
    byte_arr = BitSpan(pointer(c_uint8(addressof(mask_arr))), int(crc_byte_len)*8)
    for ind in range(crc_byte_len):
        byte_arr.data[ind] = crc_bytes[ind]

    select_args = SelectCommandArgs()
    select_args.target = SelectTarget.SelectedFlag
    select_args.action = SelectAction.Action101
    select_args.memory_bank = SelectMemoryBank.SelectEPC
    select_args.bit_pointer = 0
    select_args.truncate = 0
    select_args.mask = pointer(byte_arr)
    select_args.bit_count = 8*crc_byte_len

    cmd = Gen2CommandSpec(Gen2Command.Gen2Select,
                          ctypes.cast(pointer(select_args), c_void_p))

    # Clear any commands before adding new ones
    tx_command_manager.clear_local_sequence()
    enables = [0]*10
    # Add the command
    cmd_index = ctypes.c_size_t(0)
    cmd_index_p = ctypes.cast(ctypes.pointer(cmd_index), ctypes.POINTER(ctypes.c_size_t))
    curr_error = tx_command_manager.encode_and_append_command(cmd, 0, cmd_index_p)
    assert False == curr_error.error
    enables[cmd_index.value] = True

    # Change params for a 001 action
    select_args.action = SelectAction.Action001
    # Add as as new command
    curr_error = tx_command_manager.encode_and_append_command(cmd, 1, cmd_index_p)
    assert False == curr_error.error
    enables[cmd_index.value] = True
    curr_error = tx_command_manager.write_sequence()
    assert False == curr_error.error


def enable_select_command(tx_command_manager, enable_index):
    """
    Enable one select at a time for this scenario via the index.
    :param enable_index: The index of the select to enable
    """
    arraytype = ctypes.c_bool * 10
    enable_me = arraytype(0,0,0,0,0,0,0,0,0,0)
    enable_me[enable_index] = 1
    enable_p = pointer(enable_me)
    cmd_index = ctypes.c_size_t(0)
    cmd_index_p = ctypes.cast(ctypes.pointer(cmd_index), ctypes.POINTER(ctypes.c_size_t))

    tx_command_manager.write_select_enables(ctypes.cast(enable_p, c_void_p), 10, cmd_index_p)


def run_select_script(ex10_ifaces, tx_command_manager):
    """ Run inventory for the specified amount of time """
    ex10_reader = ex10_ifaces.reader
    helper = ex10_ifaces.helpers

    print('Starting Select example')
    try:
        # Perform an inventory round to find a tag that we can
        print('Inventory with Sel=ALL, no Select command')
        run_inventory_rounds(helper, SELECT_ALL)
        if 0 == packet_info.total_singulations:
            raise RuntimeError("No tags found in basic inventory")
        print('Done')

        # We will use the CRC of the EPC as mask for the select commands
        crc_hex_string = '{:04x}'.format(packet_info.access_tag.stored_crc)
        crc_byte_len = int(len(crc_hex_string)/2) # number of nibbles in the string / 2
        tag_epc_list = packet_info.access_tag.epc[:packet_info.access_tag.epc_length]
        tag_epc_hex_list = ('{:02x}'.format(i) for i in tag_epc_list)
        tag_epc_hex_string = ''.join(map(str, tag_epc_hex_list))
        print('The Select mask will be 0x{} (the CRC of tag with EPC=0x{})'.format(
            crc_hex_string, tag_epc_hex_string))

        # Load in select commands based off the received tag crc.
        load_select_commands(tx_command_manager,
                             packet_info.access_tag.stored_crc,
                             crc_byte_len)

        # The command to select on SL is in the buffer under index 1. Enable
        # to run before inventory.
        enable_select_command(tx_command_manager, 1)

        # Run Inventory with SEL=SL(11) and a Select command that asserts SL
        # and check that only the selected tag replies
        print('Inventory with Sel=SL, sending Select with Action001')
        run_inventory_rounds(helper, SELECT_ASSERT)
        if 0 == packet_info.total_singulations:
            raise RuntimeError("Selected tag did not reply")

        print('Done')

        # The command to select on !SL is in the buffer under index 0. Enable
        # to run before inventory.
        enable_select_command(tx_command_manager, 0)

        # Run Inventory with SEL=~SL(10) and a Select command that deasserts SL
        # and check that selected tag replied
        print('Inventory with Sel=~SL, sending Select with Action101')
        run_inventory_rounds(helper, SELECT_NOT_ASSERT)
        if 0 == packet_info.total_singulations:
            raise RuntimeError("Selected tag did not reply")

        print('Done')

        # The command to select on !SL is in the buffer under index 0 which
        # is already enabled.

        print('Inventory with Sel=SL, sending Select with Action101')
        run_inventory_rounds(helper, SELECT_ASSERT)
        if 0 != packet_info.total_singulations:
            tag_epc_list_new = packet_info.access_tag.epc[:packet_info.access_tag.epc_length]
            if tag_epc_list == tag_epc_list_new:
                raise RuntimeError("Tag replied when was not selected")

        print('Done')

    finally:
        ex10_reader.stop_transmitting()
        print('Select example end')


def run_select_example():
    """ Run example using select command and Gen2 Tx buffer """
    try:
        # Init the python to C layer
        py2c = Ex10Py2CWrapper()
        ex10_result = py2c.ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION)
        if ex10_result.error:
            py2c.print_ex10_result(ex10_result)
            py2c.ex10_typical_board_teardown()
            raise RuntimeError('ex10_typical_board_setup() failed')
        ex10_ifaces = py2c.get_ex10_interfaces()

        tx_command_manager = py2c.get_ex10_gen2_tx_command_manager()

        # Run the Select example
        run_select_script(ex10_ifaces, tx_command_manager)
    finally:
        py2c.ex10_typical_board_teardown()


if __name__ == "__main__":
    run_select_example()
