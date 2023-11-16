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
Halt on the first tag seen, write to it, then read back the value.
"""

from __future__ import (division, absolute_import, print_function,
                        unicode_literals)
from datetime import datetime

# pylint: disable=locally-disabled, wildcard-import, unused-wildcard-import
from py2c_interface.py2c_python_wrapper import *


# The configuration of the inventory example is set using the definitions below
INVENTORY_DURATION_S = 5            # Duration of inventory operation (seconds)
TRANSMIT_POWER_CDBM = 3000          # 30 dBm
RF_MODE = RfModes.mode_148
R807_ANTENNA_PORT = 1               # Which R807 antenna port will be used
INITIAL_Q = 2                       # Q field in the Query command
SELECT_ALL = 0                      # SELECT field in the Query command
SESSION = 0                         # Session field in the Query command
TARGET = 0                          # Target field in the Query command
HALT_ON_ALL_TAGS = True             # If True, the Ex10 will halt on each
                                    # inventoried tag
TAG_FOCUS_ENABLE = False            # Tells a tag to be silent after inventoried
                                    # once.
FAST_ID_ENABLE = False              # Tells a tag to backscatter the TID during
                                    # inventory

# Words to be written and read back from tag
TEST_WORD_0 = 0x55AA

packet_info = InfoFromPackets(0, 0, 0, 0, TagReadData())

cmds = []

def run_access_rw_example():
    # pylint: disable=missing-docstring
    print('Starting read/write example')
    # Init the python to C layer
    py2c = Ex10Py2CWrapper()
    try:
        run_access_read_write(py2c, INITIAL_Q)
    finally:
        print('Ending read/write example')
        py2c.ex10_typical_board_teardown()


def enable_commands(tx_command_manager, enable_index):
    """
    Enable one select at a time for this scenario via the index.
    :param enable_index: The index of the select to enable
    """
    arraytype = ctypes.c_bool * 10
    enable_me = arraytype(0,0,0,0,0,0,0,0,0,0)

    for ind in range(len(enable_index)):
        enable_me[ind] = enable_index[ind]
    enable_p = pointer(enable_me)
    cmd_index = ctypes.c_size_t(0)
    cmd_index_p = ctypes.cast(ctypes.pointer(cmd_index), ctypes.POINTER(ctypes.c_size_t))

    tx_command_manager.write_halted_enables(ctypes.cast(enable_p, c_void_p), 10, cmd_index_p)


def load_access_commands(tx_command_manager):
    """ Load Gen2 commands into Ex10 buffer so they can be sent later. """
    print('Loading commands into Gen2 command buffer')
    write_args = WriteCommandArgs()
    write_args.memory_bank = MemoryBank.User
    write_args.word_pointer = 0
    write_args.data = TEST_WORD_0

    read_args = ReadCommandArgs()
    read_args.memory_bank = MemoryBank.User
    read_args.word_pointer = 0
    read_args.word_count = 1

    cmds.append(Gen2CommandSpec(Gen2Command.Gen2Write,
                                ctypes.cast(pointer(write_args), c_void_p)))
    cmds.append(Gen2CommandSpec(Gen2Command.Gen2Read,
                                ctypes.cast(pointer(read_args), c_void_p)))

    # Clear any commands before adding new ones
    tx_command_manager.clear_local_sequence()
    enables = [0]*10
    # Add the command
    cmd_index = ctypes.c_size_t(0)
    cmd_index_p = ctypes.cast(ctypes.pointer(cmd_index), ctypes.POINTER(ctypes.c_size_t))
    curr_error = tx_command_manager.encode_and_append_command(cmds[0], 0, cmd_index_p)
    assert False == curr_error.error
    enables[cmd_index.value] = True
    curr_error = tx_command_manager.encode_and_append_command(cmds[1], 1, cmd_index_p)
    assert False == curr_error.error
    enables[cmd_index.value] = True
    curr_error = tx_command_manager.write_sequence()
    assert False == curr_error.error

    # Enable the command we added
    enable_commands(tx_command_manager, enables)


def run_until_halted(ex10_reader, helper, inventory_config):
    """ Run until halted or timeout trying to find a tag. """
    inventory_config_2 = InventoryRoundControl_2Fields()
    inventory_config_2.max_queries_since_valid_epc = 0

    inv_done = True
    start_time = datetime.now()
    while not packet_info.total_singulations:
        time_delta = (datetime.now() - start_time).total_seconds()
        if time_delta > 5:
            break
        if inv_done:
            inv_done = False
            ex10_result = ex10_reader.inventory(R807_ANTENNA_PORT,
                                             RF_MODE,
                                             TRANSMIT_POWER_CDBM,
                                             pointer(inventory_config),
                                             pointer(inventory_config_2),
                                             False,
                                             True)
            assert ex10_result.error is False
        while ex10_reader.packets_available():
            packet = ex10_reader.packet_peek().contents
            if packet.packet_type == EventPacketType.InventoryRoundSummary:
                inv_done = True
            elif packet.packet_type == EventPacketType.TagRead:
                helper.examine_packets(packet, pointer(packet_info))
            ex10_reader.packet_remove()

    # Should be halted on a tag now
    assert packet_info.total_singulations == 1  # Should only be one when halted
    tag_epc = ''.join([str(format(packet_info.access_tag.epc[i], '02x'))
                       for i in range(packet_info.access_tag.epc_length)])
    print('Halted on tag 0x{}'.format(tag_epc))


def wait_for_transaction_report(ex10_ifaces, fifo_printer, expected_reports=1, timeout_s=3):
    ex10_reader = ex10_ifaces.reader
    helper = ex10_ifaces.helpers

    start_time = datetime.now()

    transactions = []
    while len(transactions) < expected_reports:
        while ex10_reader.packets_available():
            packet = ex10_reader.packet_peek().contents
            helper.examine_packets(packet, packet_info)
            fifo_printer.print_packets(packet)

            if packet.packet_type == EventPacketType.TxRampDown:
                raise RuntimeError(
                    'Modem ramped down while waiting for Gen2 transaction')
            elif packet.packet_type == EventPacketType.Gen2Transaction:
                reply_array = c_uint16 * 10
                reply_words = reply_array()
                reply = Gen2Reply(0, 0, reply_words)
                ex10_ifaces.gen2_commands.decode_reply(
                    cmds[len(transactions)].command, packet, pointer(reply))
                transactions.append(reply)

            ex10_reader.packet_remove()

        if (datetime.now() - start_time).total_seconds() > timeout_s:
            raise RuntimeError('Timed out waiting for Gen2Transaction')
    return transactions


def run_access_read_write(py2c, initial_q):
    # pylint: disable=too-many-locals,too-many-statements
    """ Run inventory for the specified amount of time """
    ex10_result = py2c.ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ,
                                                Ex10RegionId.REGION_FCC.value)
    if ex10_result.error:
        py2c.print_ex10_result(ex10_result)
        py2c.ex10_typical_board_teardown()
        raise RuntimeError('ex10_typical_board_setup() failed')
    ex10_ifaces = py2c.get_ex10_interfaces()
    ex10_reader = ex10_ifaces.reader
    ex10_ops = ex10_ifaces.ops
    helper = ex10_ifaces.helpers
    tx_command_manager = py2c.get_ex10_gen2_tx_command_manager()
    fifo_printer = py2c.get_ex10_event_fifo_printer()
    gen2_commands = py2c.get_ex10_gen2_commands()

    ex10_ifaces.protocol.set_event_fifo_threshold(0)

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
    inventory_config.target = TARGET
    inventory_config.halt_on_all_tags = HALT_ON_ALL_TAGS
    inventory_config.fast_id_enable = FAST_ID_ENABLE
    inventory_config.tag_focus_enable = TAG_FOCUS_ENABLE

    print('########## Delayed Write - User Data Test ##########')

    load_access_commands(tx_command_manager)

    ex10_ops.send_gen2_halted_sequence()

    run_until_halted(ex10_reader, helper, inventory_config)

    transactions = wait_for_transaction_report(ex10_ifaces=ex10_ifaces,
                                               fifo_printer = fifo_printer,
                                               expected_reports=2)
    assert len(transactions) == 2

    # Transaction reports are received in the same order commands are sent out.
    # Verify the 2 write commands worked.
    write_reply = transactions[0]
    gen2_commands.check_error(write_reply)

    read_reply = transactions[1]
    gen2_commands.check_error(read_reply)
    print('Read Back: 0x{:04x}'.format(read_reply.data[0]))
    assert read_reply.data[0] == TEST_WORD_0

    # Demonstrate continuing to next tag, not used here.
    ex10_reader.continue_from_halted(False)
    time.sleep(0.25)  # Wait a small amount of time for reports

    while ex10_reader.packets_available():
        packet = ex10_reader.packet_peek().contents
        helper.examine_packets(packet, packet_info)
        fifo_printer.print_packets(packet)
        ex10_reader.packet_remove()

        if packet.packet_type == EventPacketType.TxRampDown:
            inventory_config.target = 0
        elif packet.packet_type == EventPacketType.InventoryRoundSummary:
            print('Round complete')

    ex10_reader.stop_transmitting()


if __name__ == "__main__":
    run_access_rw_example()
