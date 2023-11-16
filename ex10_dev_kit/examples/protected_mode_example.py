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
An example of how to use protected mode and short range functionality.
Note: This example assumes a single tag on antenna port one.
Note: This example assumes a starting password of all 0s
Note: The tag we use for this test is an M775. This is not the only tag which
supports this feature (e.g. M730, M750, M770), but it is one tag which allows
demonstration of all required steps. This example shows usage of password
changing to use the feature, entering protected mode and becoming invisible,
discovering an invisible tag, and leaving protected mode to go back to visible.
"""
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

from collections import namedtuple
from datetime import datetime
import time

# pylint: disable=locally-disabled, wildcard-import, unused-wildcard-import
from py2c_interface.py2c_python_wrapper import *


# The configuration of the inventory round is set using the definitions below
INVENTORY_DURATION_S = 3  # Duration of inventory operation (seconds)
TRANSMIT_POWER_CDBM = 3000  # 30 dBm
RF_MODE = RfModes.mode_148
REGION = Ex10RegionId.REGION_FCC.value  # Regulatory region
R807_ANTENNA_PORT = 1

INITIAL_Q = 4  # Q field in the Query command
SELECT_ALL = 0  # SELECT field in the Query command
SESSION = 0  # Session field in the Query command

# Allows for testing the short range functionality of Impinj tags.
# This is not enabled for general testing, but if enabled, the power
# should be increased.
SET_SHORT_RANGE = False

TagSettings = namedtuple(
    "TagSettings", ["protected_mode_enabled",
                    "short_range_enabled",
                    "data_returned"]
)

ZERO_ARRAY = [0x00, 0x00, 0x00, 0x00]
NON_ZERO_ARRAY = [0x55, 0x55, 0x44, 0x44]

ZERO_ACCESS_PWD = bytes(ZERO_ARRAY)
NON_ZERO_ACCESS_PWD = bytes(NON_ZERO_ARRAY)

# Set up buffers for the tag read data to be written to during a halt
packet_info = InfoFromPackets(0, 0, 0, 0, TagReadData())
fifo_printer = None


def inventory_and_halt(ex10_ifaces, tx_command_manager, select_command=None, expect_tag=True):
    """
    Runs an inventory round until a tag is encounter or 10 rounds pass.
    :params select_command: The select command to send before inventory
    :params expect_tag: Whether or not we expect to see tags in inventory
    """
    global packet_info

    ex10_reader = ex10_ifaces.reader
    helper = ex10_ifaces.helpers

    # Remove all previous packets
    while ex10_reader.packets_available():
        ex10_reader.packet_remove()

    packet_info.total_singulations = 0

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
    inventory_config.halt_on_all_tags = True
    inventory_config.fast_id_enable = False
    inventory_config.tag_focus_enable = False

    inventory_config_2 = InventoryRoundControl_2Fields()
    inventory_config_2.max_queries_since_valid_epc = 0

    send_selects = False
    if select_command:
        send_selects = True

        arraytype = ctypes.c_bool * 10
        select_enables = arraytype(0,0,0,0,0,0,0,0,0,0)
        cmd_index = ctypes.c_size_t(0)
        cmd_index_p = ctypes.cast(ctypes.pointer(cmd_index), ctypes.POINTER(ctypes.c_size_t))

        curr_error = tx_command_manager.encode_and_append_command(select_command, 0, cmd_index_p)
        assert False == curr_error.error
        select_enables[cmd_index.value] = True
        curr_error = tx_command_manager.write_sequence()
        assert False == curr_error.error

        enable_p = pointer(select_enables)
        tx_command_manager.write_select_enables(ctypes.cast(enable_p, c_void_p), 10, cmd_index_p)

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
                                  send_selects,
                                  0,
                                  True)
            if ex10_result.error is True:
                raise Exception("Error when running inventory")
        while ex10_reader.packets_available():
            packet = ex10_reader.packet_peek().contents
            if packet.packet_type == EventPacketType.InventoryRoundSummary:
                inv_done = True
            elif packet.packet_type == EventPacketType.TagRead:
                helper.examine_packets(packet, pointer(packet_info))
            ex10_reader.packet_remove()

    if expect_tag:
        if packet_info.total_singulations == 0:
            raise Exception("No tags found in inventory")
        else:
            # Should be halted on a tag now
            tag_epc = ''.join([str(format(packet_info.access_tag.epc[i], '02x'))
                               for i in range(packet_info.access_tag.epc_length)])
            print('Halted on tag 0x{}'.format(tag_epc))
    else:
        if packet_info.total_singulations > 0:
            raise Exception("Saw tags when not expecting any")


def wait_for_transaction_report(ex10_ifaces, decode_cmd, timeout_s=3):
    """
    Send the Gen2 Access command to the Ex10 reader with password.
    :params decode_cmd: The command structure into which the
                        incoming command will be decoded
    :params timeout_s: The timeout to wait for the incoming command in seconds
    :return: The report from the transaction
    """
    ex10_reader = ex10_ifaces.reader
    helper = ex10_ifaces.helpers

    start_time = datetime.now()

    transactions = []
    while len(transactions) < 1:
        while ex10_reader.packets_available():
            packet = ex10_reader.packet_peek().contents
            helper.examine_packets(packet, packet_info)
            global fifo_printer
            fifo_printer.print_packets(packet)

            if packet.packet_type == EventPacketType.Gen2Transaction:
                tran = Gen2Transaction()
                ctypes.memmove(pointer(tran), packet.static_data, sizeof(tran))
                # Check for bad crc
                if tran.status == 2:
                    raise RuntimeError('Bad crc returned from tag')
                reply_array = c_uint16 * 20
                reply_words = reply_array()
                reply = Gen2Reply(0, 0, reply_words)
                ex10_ifaces.gen2_commands.decode_reply(
                    decode_cmd.command, packet, pointer(reply))
                transactions.append(reply)

            ex10_reader.packet_remove()

        if (datetime.now() - start_time).total_seconds() > timeout_s:
            raise RuntimeError('Timed out waiting for Gen2Transaction')
    return transactions


def ensure_previous_command_finished(ex10_protocol):
    halt_status = ex10_protocol.read('HaltedStatus')
    while halt_status.busy:
        halt_status = ex10_protocol.read('HaltedStatus')


def send_access_command(ex10_ifaces, pwd):
    """
    Send the Gen2 Access command with password.
    :params pwd: One half of the 32 bit Access password
    :return: The report from the transaction
    """
    ex10_helpers = ex10_ifaces.helpers
    ex10_protocol = ex10_ifaces.protocol

    access_params = AccessCommandArgs()
    access_params.password = pwd
    access_command = Gen2CommandSpec(
        Gen2Command.Gen2Access,
        ctypes.cast(pointer(access_params), c_void_p))

    ensure_previous_command_finished(ex10_protocol)

    ex10_helpers.send_single_halted_command(access_command)

    report = wait_for_transaction_report(ex10_ifaces, access_command)
    return report


def write_to_reserved(ex10_ifaces, word_to_write, page_data):
    """
    Write to the reserved memory.
    :param word_to_write: The word number in reserved memory
                          to write to.
    :param page_data: The word to write to specified word
                      offset in reserved memory.
    """
    ex10_protocol = ex10_ifaces.protocol
    ex10_helpers = ex10_ifaces.helpers

    # Send the write command and wait for response
    write_args = WriteCommandArgs()
    write_args.memory_bank = MemoryBank.Reserved
    write_args.word_pointer = word_to_write
    write_args.data = page_data

    ensure_previous_command_finished(ex10_protocol)

    write_command = Gen2CommandSpec(Gen2Command.Gen2Write,
                        ctypes.cast(pointer(write_args), c_void_p))
    ex10_helpers.send_single_halted_command(write_command)

    # Wait for gen2 transaction report
    gen2_transactions = wait_for_transaction_report(ex10_ifaces, write_command)

    # Sent 1 command, should be 1 report
    if len(gen2_transactions) != 1:
        raise Exception(
            "{} reports seen instead of 1".format(len(gen2_transactions)))

    py2c = Ex10Py2CWrapper()
    gen2_commands = py2c.get_ex10_gen2_commands()
    gen2_commands.check_error(gen2_transactions[0])

def read_reserved_memory(ex10_ifaces, word_pointer, word_count):
    """
    Read the reserved memory
    :param word_pointer: The word offset to start the read
    :param word_count: The number of words to read
    :return: Returns the data from the tag reply
    """
    ex10_protocol = ex10_ifaces.protocol
    ex10_helpers = ex10_ifaces.helpers

    # Send the read command and wait for response
    read_args = ReadCommandArgs()
    read_args.memory_bank = MemoryBank.Reserved
    read_args.word_pointer = word_pointer
    read_args.word_count = word_count

    ensure_previous_command_finished(ex10_protocol)

    read_command = Gen2CommandSpec(Gen2Command.Gen2Read,
                        ctypes.cast(pointer(read_args), c_void_p))
    ex10_helpers.send_single_halted_command(read_command)

    # Wait for gen2 transaction report
    gen2_reply = wait_for_transaction_report(ex10_ifaces, read_command)
    # Sent 1 command, should be 1 report
    if len(gen2_reply) != 1:
        raise Exception(
            "{} reports seen instead of 1".format(len(gen2_reply)))
    py2c = Ex10Py2CWrapper()
    gen2_commands = py2c.get_ex10_gen2_commands()
    gen2_commands.check_error(gen2_reply[0])

    return gen2_reply[0].data[:word_count]


def read_settings(ex10_ifaces):
    """
    Read the reserved memory then return whether the device
    is in protected mode, short range mode, as well as the
    entire page so we can do a read modify write.
    :return: Returns a named tuple of info read from the tag
    """
    read_data = read_reserved_memory(ex10_ifaces=ex10_ifaces,
                                     word_pointer=4,
                                     word_count=1)
    read_val = read_data[0]
    print('Read Back: 0x{:04x}'.format(read_val))
    # base 0: bit 1 is P and bit 4 is SR
    protected = (read_val >> 1) & 1
    short_range = (read_val >> 4) & 1
    return TagSettings(protected, short_range, read_val)


def create_protected_select(tag_pass):
    """
    Create the select command to use with protected mode
    :param tag_pass: The protected mode pin to use as the mask in the
    protected mode select.
    :return: The select to use when searching for protected tags.
             The parameters to inspect what is in the command
    """
    protected_params = SelectCommandArgs()
    protected_params.target = SelectTarget.Session0
    protected_params.action = SelectAction.Action001
    protected_params.memory_bank = SelectMemoryBank.SelectFile0
    protected_params.bit_pointer = 0
    protected_params.truncate = False
    protected_params.mask = pointer(tag_pass)
    protected_params.bit_count = 32

    protected_select = Gen2CommandSpec(
        Gen2Command.Gen2Select, ctypes.cast(pointer(protected_params), c_void_p))

    return protected_select, protected_params


def change_access_pwd(ex10_ifaces, ending_pwd):
    """
    Change the access password in reserved memory
    :param ending_pwd: The new password to change to using
    """
    print("Changing access pwd")
    pwd_word_0 = (ending_pwd[1] << 8) | ending_pwd[0]
    pwd_word_1 = (ending_pwd[3] << 8) | ending_pwd[2]

    # Write the new password to memory
    write_to_reserved(ex10_ifaces=ex10_ifaces,
                      word_to_write=2,
                      page_data=pwd_word_0)
    write_to_reserved(ex10_ifaces=ex10_ifaces,
                      word_to_write=3,
                      page_data=pwd_word_1)
    print("Password changed")


def set_protected_mode_state(ex10_ifaces, enable):
    """
    Set the tag memory to change the state of protected
    mode and the short range feature
    :param enable: whether to enable or disable protected mode and
    the short range feature.
    """
    if enable:
        print("Entering protected mode")
    else:
        print("Leaving protected mode")

    # Read reserved memory for modify write
    tag_settings = read_settings(ex10_ifaces)
    page_data = tag_settings.data_returned

    # To enable protected mode
    page_data = page_data & ~(1 << 1)
    page_data = page_data | (int(enable) << 1)
    # To enable short range mode
    page_data = page_data & ~(1 << 4)
    if SET_SHORT_RANGE:
        page_data = page_data | (int(enable) << 4)
    else:
        page_data = page_data | (0 << 4)

    # Write memory controlling protected mode and short range control
    # We change both together here though not necessary
    print("Writing to change protected and sr bit")
    write_to_reserved(ex10_ifaces=ex10_ifaces,
                      word_to_write=4,
                      page_data=page_data)
    print("Settings written")

    # Read back memory to verify the write
    tag_settings = read_settings(ex10_ifaces)
    protected_read = tag_settings.protected_mode_enabled
    short_range_read = tag_settings.short_range_enabled

    # Ensure the read data matches expectations
    if protected_read != int(enable):
        raise Exception(
            "Protected value read is {} instead of {}".format(
                protected_read, int(enable)))
    sr_read = int(enable) and SET_SHORT_RANGE
    if short_range_read != sr_read:
        raise Exception(
            "Short range value read is {} instead of {}".format(
                short_range_read, int(enable)))

    if enable:
        print("Entered protected mode successfully")
    else:
        print("Left protected mode successfully")


def enter_access_mode(ex10_ifaces, access_pwd):
    """
    Send the access password in two steps to enter the access state
    :param access_pwd: The password to use to enter access mdoe
    """
    helper = ex10_ifaces.helpers
    py2c = Ex10Py2CWrapper()
    gen2_commands = py2c.get_ex10_gen2_commands()

    # Send the first word of the password
    report = send_access_command(ex10_ifaces, (access_pwd[1] << 8) | access_pwd[0])
    gen2_commands = py2c.get_ex10_gen2_commands()
    gen2_commands.check_error(report[0])
    print("Access command 1 sent")
    read_reply = report[0]
    # We will use this to ensure that the next command is proper
    handle = read_reply.data

    # Send the second word of the password
    report = send_access_command(ex10_ifaces, (access_pwd[3] << 8) | access_pwd[2])
    gen2_commands.check_error(report[0])
    # Check against the handle from command 1
    if handle[0:1] != read_reply.data[0:1]:
        raise Exception("Handles returned do not match between commands.")
    print("Access command 2 sent")


def run_protected_mode_example():
    # Init the python to C layer
    py2c = Ex10Py2CWrapper()
    ex10_result = py2c.ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION)
    if ex10_result.error:
        py2c.print_ex10_result(ex10_result)
        py2c.ex10_typical_board_teardown()
        raise RuntimeError('ex10_typical_board_setup() failed')
    ex10_ifaces = py2c.get_ex10_interfaces()
    reader = ex10_ifaces.reader
    protocol = ex10_ifaces.protocol
    tx_command_manager = py2c.get_ex10_gen2_tx_command_manager()
    global fifo_printer
    fifo_printer = py2c.get_ex10_event_fifo_printer()

    # Set up to see event fifo activity immediately
    protocol.set_event_fifo_threshold(0)

    data_type = c_uint8 * 4
    msb_non_zero_span_array = [NON_ZERO_ARRAY[3], NON_ZERO_ARRAY[2], NON_ZERO_ARRAY[1], NON_ZERO_ARRAY[0]]
    msb_non_zero_pass  = bytes(msb_non_zero_span_array)
    tag_pass_span = BitSpan(data_type.from_buffer(bytearray(msb_non_zero_span_array)), 32)
    non_zero_select, protected_params = create_protected_select(tag_pass_span)

    # Change the password
    inventory_and_halt(ex10_ifaces, tx_command_manager, None, True)
    change_access_pwd(ex10_ifaces, msb_non_zero_pass)
    reader.stop_transmitting()
    time.sleep(2)

    # Enter protected mode
    inventory_and_halt(ex10_ifaces, tx_command_manager, None, True)
    enter_access_mode(ex10_ifaces, msb_non_zero_pass)
    set_protected_mode_state(ex10_ifaces=ex10_ifaces, enable=True)
    reader.stop_transmitting()
    time.sleep(2)

    # Ensure we can not see the tag
    print("Ensuring the tag can not be seen in inventory")
    inventory_and_halt(ex10_ifaces, tx_command_manager, None, False)
    reader.stop_transmitting()
    time.sleep(2)

    # Now show how to find it again and return to non-protected mode
    print("Now ensure the tag can be found with a select")
    inventory_and_halt(ex10_ifaces, tx_command_manager, non_zero_select, True)
    enter_access_mode(ex10_ifaces, msb_non_zero_pass)
    set_protected_mode_state(ex10_ifaces=ex10_ifaces, enable=False)
    change_access_pwd(ex10_ifaces, ZERO_ACCESS_PWD)
    reader.stop_transmitting()
    time.sleep(2)

    # Ensure we can see it normally now
    inventory_and_halt(ex10_ifaces, tx_command_manager, None, True)
    reader.stop_transmitting()
    time.sleep(2)

    py2c.ex10_typical_board_teardown()


if __name__ == "__main__":
    run_protected_mode_example()
