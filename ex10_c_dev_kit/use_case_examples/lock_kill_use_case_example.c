/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2023 Impinj, Inc. All rights reserved.                      *
 *                                                                           *
 *****************************************************************************/

/**
 * @file lock_kill_use_case_example.c
 * #detail This use-case example shows how to execute a Lock-Kill command using
 *  the Ex10TagAccessUseCase. The command sequence for the Lock-Kill is setup
 *  during the halted callback while the LMAC is halted on the tag. The command
 *  sequence to kill a tag should look something like:
 *
 *   1. Unlock the kill password memory by sending a Gen2Lock command
 *   2. Read and parse the kill password by sending a Gen2Read command
 *   3. To kill a tag, the kill password must be non-zero. So, if the password
 * is set to zero, change it to a non-zero value by sending a Gen2Write command
 *   4. Kill the tag by sending a Gen2Kill command
 *
 *  WARNING: This example will permanently disable tag(s).
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ex10_use_cases/ex10_tag_access_use_case.h"

#include "calibration.h"

#include "ex10_api/board_init_core.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_gen2_reply_string.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/gen2_commands.h"
#include "ex10_api/gen2_tx_command_manager.h"
#include "ex10_api/rf_mode_definitions.h"
#include "ex10_regulatory/ex10_default_region_names.h"

#include "utils/ex10_inventory_command_line.h"
#include "utils/ex10_use_case_example_errors.h"


/**
 * Encode the given Gen2 command to a Gen2 command buffer and writes it to the
 * Ex10 device
 *
 * @param cmd_spec              Gen2 command
 * @return struct Ex10Result    Result of setting up a Gen2 command
 */
static struct Ex10Result setup_gen2_cmd_halted(struct Gen2CommandSpec* cmd_spec)
{
    struct Ex10Gen2TxCommandManager const* g2tcm =
        get_ex10_gen2_tx_command_manager();
    bool halted_enables[MaxTxCommandCount] = {false};

    // Encode the given command and append it to the command buffer
    g2tcm->clear_local_sequence();
    size_t            cmd_index = 0;
    struct Ex10Result gen2_encode_result =
        g2tcm->encode_and_append_command(cmd_spec, 0, &cmd_index);
    if (gen2_encode_result.error || cmd_index != 0u)
    {
        ex10_ex_eprintf("g2tcm->encode_and_append_command() failed:\n");
        print_ex10_result(gen2_encode_result);
        return gen2_encode_result;
    }

    // Write the command buffer to the Ex10 device
    struct Ex10Result const gen2_write_result = g2tcm->write_sequence();
    if (gen2_write_result.error)
    {
        ex10_ex_eprintf("g2tcm->write_sequence() failed:\n");
        print_ex10_result(gen2_encode_result);
        return gen2_encode_result;
    }

    // Enable the command
    halted_enables[cmd_index]                   = true;
    struct Ex10Result const gen2_enables_result = g2tcm->write_halted_enables(
        halted_enables, MaxTxCommandCount, &cmd_index);
    if (gen2_enables_result.error)
    {
        ex10_ex_eprintf("g2tcm->write_select_enables() failed:\n");
        print_ex10_result(gen2_encode_result);
        return gen2_encode_result;
    }

    return make_ex10_success();
}

/**
 * This function verifies that the received Event FIFO packet type is
 * 'Gen2Transaction' and copies the packet information to the given parameter.
 *
 * @param cmd_spec           Gen2 command
 * @param [out] reply        Gen2 command reply
 * @return struct Ex10Result Result of receiving and copying the
 * 'Gen2Transaction' packet
 */
static struct Ex10Result get_gen2_cmd_halted_reply(
    struct Gen2CommandSpec* cmd_spec,
    struct Gen2Reply*       reply)
{
    struct Ex10TagAccessUseCase const* tauc = get_ex10_tag_access_use_case();
    struct Ex10Gen2Commands const*     gen2_commands = get_ex10_gen2_commands();

    // Receive a Gen2Transaction packet
    struct EventFifoPacket const* packet = tauc->get_fifo_packet();
    get_ex10_event_fifo_printer()->print_packets(packet);
    if (packet->packet_type != Gen2Transaction)
    {
        ex10_ex_eprintf(
            "Unexpected packet type. The received packet type after a Gen2 "
            "command is not Gen2Transaction\n");
        return make_ex10_app_error(Ex10ApplicationUnexpectedPacketType);
    }

    // Decode the packet
    gen2_commands->decode_reply(cmd_spec->command, packet, reply);
    if (reply->error_code != NoError)
    {
        ex10_ex_eprintf("Failed to decode Gen2 command\n");
        get_ex10_gen2_commands()->print_reply(*reply);
        return make_ex10_app_error(Ex10ApplicationGen2ReplyError);
    }

    tauc->remove_fifo_packet();
    return make_ex10_success();
}

/**
 * Sends the given Gen2 command, execute it, and copies the received
 * 'Gen2Transaction' response to the given parameter.
 *
 * @param cmd_spec           Gen2 command
 * @param reply              Gen2 command reply
 * @return struct Ex10Result Result of sending a Gen2 command
 */
static struct Ex10Result send_gen2_cmd_halted(struct Gen2CommandSpec* cmd_spec,
                                              struct Gen2Reply*       reply)
{
    struct Ex10TagAccessUseCase const* tauc = get_ex10_tag_access_use_case();

    struct Ex10Result ex10_result = setup_gen2_cmd_halted(cmd_spec);
    if (ex10_result.error)
    {
        ex10_ex_eprintf("Failed to setup the command\n");
        return ex10_result;
    }

    // Execute Gen2Lock command
    const enum TagAccessResult cmd_execute_result =
        tauc->execute_access_commands();
    if (cmd_execute_result != TagAccessSuccess)
    {
        ex10_ex_eprintf(
            "Failed to execute the Gen2 command. Might have lost the tag\n");
        return make_ex10_app_error(Ex10ApplicationTagLost);
    }

    // Receive Gen2Lock command reply
    ex10_result = get_gen2_cmd_halted_reply(cmd_spec, reply);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Remove packet 'Halted'
    // The LMAC will return to the halted state when the Access commands are
    // done, so this consumes that packet
    if (tauc->remove_halted_packet() == false)
    {
        return make_ex10_app_error(Ex10ApplicationUnexpectedPacketType);
    }

    return make_ex10_success();
}

/**
 * This function is registered as the callback with the Ex10TagAccessUseCase
 * object. When a tag has been inventoried and transitioned into the
 * Acknowledged state, it is considered 'halted'. This callback is used to
 * notify the client with TagRead EventFifo packets. Performing Tag Access
 * commands should be performed within this function.
 *
 * @param packet The TagRead read packet.
 * @return enum HaltedCallbackResult
 *         Whether the Impinj Reader Chip should ACK the tag or NAK the tag.
 */
static void halted_on_tag_callback(struct EventFifoPacket const* packet,
                                   enum HaltedCallbackResult*    cb_result,
                                   struct Ex10Result*            ex10_result)
{
    *ex10_result                            = make_ex10_success();
    struct Ex10TagAccessUseCase const* tauc = get_ex10_tag_access_use_case();
    uint16_t                           reply_words[10u] = {0u};
    struct Gen2Reply reply = {.error_code = NoError, .data = reply_words};

    // Receive a TagRead packet and check if we halted on a tag
    if (packet->packet_type != TagRead)
    {
        ex10_ex_eprintf("Got unexpected packet in the callback\n");
        tauc->remove_fifo_packet();
        // Error condition, we return and NAK the tag.
        *cb_result = NakTagAndContinue;
        return;
    }

    get_ex10_event_fifo_printer()->print_packets(packet);
    bool const halted_on_tag = packet->static_data->tag_read.halted_on_tag;
    // Discard the packet from the queue so we can look for some access packets.
    tauc->remove_fifo_packet();

    if (halted_on_tag == false)
    {
        // The LMAC read a tag, but did not successfully halt on it
        // (could have failed CRC in the ReqRN or hit a regulatory timer)
        // so we just return.  Note that the LMAC will have already continued
        // to the next slot on its own.
        ex10_ex_eprintf("LMAC failed to halt on tag\n");
        *cb_result = AckTagAndContinue;
        return;
    }

    // If this callback got called and the LMAC halted on a tag, the LMAC
    // should have sent a halted message back after the TagRead packet.
    // So we verify that it is there and discard it (note that the
    // packet pointer is no longer valid as it was removed above).
    if (tauc->remove_halted_packet() == false)
    {
        // Looks like there wasn't a halted packet, which suggests that
        // the LMAC didn't successfully halt on the tag, so we have the
        // same response as the halted_on_tag test above.
        // Actually, this path should not happen.
        *cb_result = AckTagAndContinue;
        return;
    }

    // Send Gen2Lock command to unlock the kill password memory
    ex10_ex_printf(
        "Sending Gen2Lock command to unlock the kill password memory\n");
    // Mask and action fields correspond to
    // Kill pwd mask =   '11b' - All other mask bits indicate 'skip'
    // Kill pwd action = '00b'
    // Kill pwd action description: Associated password location is readable and
    //                              writable from either the open or secured
    //                              states.
    struct LockCommandArgs kill_pwd_unlock_args = {
        .kill_password_read_write_mask   = true,
        .kill_password_permalock_mask    = true,
        .access_password_read_write_mask = false,
        .access_password_permalock_mask  = false,
        .epc_memory_write_mask           = false,
        .epc_memory_permalock_mask       = false,
        .tid_memory_write_mask           = false,
        .tid_memory_permalock_mask       = false,
        .file_0_memory_write_mask        = false,
        .file_0_memory_permalock_mask    = false,
        .kill_password_read_write_lock   = false,
        .kill_password_permalock         = false,
        .access_password_read_write_lock = false,
        .access_password_permalock       = false,
        .epc_memory_write_lock           = false,
        .epc_memory_permalock            = false,
        .tid_memory_write_lock           = false,
        .tid_memory_permalock            = false,
        .file_0_memory_write_lock        = false,
        .file_0_memory_permalock         = false};

    struct Gen2CommandSpec unlock_kill_pwd_command = {
        .command = Gen2Lock, .args = &kill_pwd_unlock_args};

    *ex10_result = send_gen2_cmd_halted(&unlock_kill_pwd_command, &reply);
    if (ex10_result->error)
    {
        // An error has occurred. Nak the tag and continue
        *cb_result = NakTagAndContinue;
        return;
    }

    // Send Gen2Read command to read the kill password
    ex10_ex_printf("Sending Gen2Read command to read the kill password\n");
    struct ReadCommandArgs read_kill_pwd_args = {
        .memory_bank  = Reserved,
        .word_pointer = 0u,
        .word_count   = 2u,
    };

    struct Gen2CommandSpec read_kill_pwd_command = {
        .command = Gen2Read, .args = &read_kill_pwd_args};

    *ex10_result = send_gen2_cmd_halted(&read_kill_pwd_command, &reply);
    if (ex10_result->error)
    {
        // An error has occurred. Nak the tag and continue
        *cb_result = NakTagAndContinue;
        return;
    }

    // Parse the tag kill password from Gen2Read command
    uint32_t kill_pwd = reply.data[0];
    kill_pwd <<= 16;
    kill_pwd |= reply.data[1];
    ex10_ex_printf("The kill password is %08X\n", kill_pwd);

    // Ensure a non-zero kill password, as required to kill a tag
    if (kill_pwd == 0)
    {
        struct WriteCommandArgs write_kill_pwd_args = {
            .memory_bank  = Reserved,
            .word_pointer = 0u,  // Modify value before use
            .data         = 0u,  // Modify value before use
        };

        struct Gen2CommandSpec write_kill_pwd_command = {
            .command = Gen2Write, .args = &write_kill_pwd_args};

        kill_pwd = NON_ZERO_ACCESS_PWD;
        ex10_ex_printf(
            "The kill password is set to zero. Password will be changed to a "
            "default non-zero password value %08X\n",
            kill_pwd);

        // Send Gen2Write command to write the first half of a non-zero
        // kill password
        ex10_ex_printf(
            "Sending Gen2Write command to write the kill password\n");
        write_kill_pwd_args.data         = (kill_pwd >> 16u) & 0xFFFF;
        write_kill_pwd_args.word_pointer = 0u;

        *ex10_result = send_gen2_cmd_halted(&write_kill_pwd_command, &reply);
        if (ex10_result->error)
        {
            // An error has occurred. Nak the tag and continue
            *cb_result = NakTagAndContinue;
            return;
        }

        // Send Gen2Write command to write the second half of a non-zero
        // kill password
        write_kill_pwd_args.data         = kill_pwd & 0xFFFF;
        write_kill_pwd_args.word_pointer = 1u;

        *ex10_result = send_gen2_cmd_halted(&write_kill_pwd_command, &reply);
        if (ex10_result->error)
        {
            // An error has occurred. Nak the tag and continue
            *cb_result = NakTagAndContinue;
            return;
        }
    }

    // Send Gen2Kill command 1/2
    ex10_ex_printf("Sending Gen2Kill commands to kill the tag permanently\n");
    struct KillCommandArgs kill_1_args = {
        .password = (kill_pwd >> 16) & 0xFFFF,
    };

    struct Gen2CommandSpec kill_1_cmd = {.command = Gen2Kill_1,
                                         .args    = &kill_1_args};

    *ex10_result = send_gen2_cmd_halted(&kill_1_cmd, &reply);
    if (ex10_result->error)
    {
        // An error has occurred. Nak the tag and continue
        *cb_result = NakTagAndContinue;
        return;
    }

    // Send Gen2Kill command 2/2
    struct KillCommandArgs kill_2_args = {
        .password = kill_pwd & 0xFFFF,
    };

    struct Gen2CommandSpec kill_2_cmd = {.command = Gen2Kill_2,
                                         .args    = &kill_2_args};

    *ex10_result = send_gen2_cmd_halted(&kill_2_cmd, &reply);
    if (ex10_result->error)
    {
        // An error has occurred. Nak the tag and continue
        *cb_result = NakTagAndContinue;
        return;
    }

    // We completed the transactions we want.. continue the inventory round
    *cb_result = AckTagAndContinue;
    return;
}

static struct Ex10Result lock_kill_use_case_example(
    struct InventoryOptions const* inventory_options)
{
    ex10_ex_printf("Starting access lock-kill use case example\n");

    struct Ex10TagAccessUseCase const* tauc = get_ex10_tag_access_use_case();
    tauc->init();
    tauc->register_halted_callback(halted_on_tag_callback);

    uint8_t const target =
        (inventory_options->target_spec == 'B') ? target_B : target_A;
    struct Ex10TagAccessUseCaseParameters params = {
        .antenna       = inventory_options->antenna,
        .rf_mode       = inventory_options->mode.rf_mode_id,
        .tx_power_cdbm = inventory_options->tx_power_cdbm,
        .initial_q     = inventory_options->initial_q,
        .session       = (uint8_t)inventory_options->session,
        .target        = target,
        .select        = (uint8_t)SelectAll,
        .send_selects  = false};

    if (inventory_options->frequency_khz != 0)
    {
        get_ex10_active_region()->set_single_frequency(
            inventory_options->frequency_khz);
    }

    if (inventory_options->remain_on)
    {
        get_ex10_active_region()->disable_regulatory_timers();
    }

    struct Ex10Result ex10_result = tauc->run_inventory(&params);
    get_ex10_rf_power()->stop_op_and_ramp_down();

    return ex10_result;
}

int main(int argc, char const* const argv[])
{
    struct InventoryOptions inventory_options = {
        .region_name   = "FCC",
        .read_rate     = 0u,
        .antenna       = 2u,
        .frequency_khz = 0u,
        .remain_on     = false,
        .tx_power_cdbm = 3000,
        .mode          = {.rf_mode_id = mode_103},
        .target_spec   = 'D',
        .initial_q     = 8,
        .session       = SessionS2,
    };

    struct Ex10Result const ex10_result_command_line =
        ex10_inventory_parse_command_line(&inventory_options, argv, argc);
    ex10_print_inventory_command_line_settings(&inventory_options);
    if (ex10_result_command_line.error || ex10_command_line_help_requested())
    {
        return ex10_result_command_line.error ? EINVAL : 0;
    }

    enum Ex10RegionId const region_id =
        get_ex10_default_region_names()->get_region_id(
            inventory_options.region_name);

    struct Ex10Result ex10_result =
        ex10_core_board_setup(region_id, DEFAULT_SPI_CLOCK_HZ);
    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_core_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_core_board_teardown();
        return -1;
    }

    ex10_result = ex10_set_default_gpio_setup();
    get_ex10_calibration()->init(get_ex10_protocol());

    if (ex10_result.error == false)
    {
        ex10_result = lock_kill_use_case_example(&inventory_options);
        if (ex10_result.error == true)
        {
            print_ex10_app_result(ex10_result);
        }
    }
    else
    {
        print_ex10_result(ex10_result);
    }


    ex10_core_board_teardown();
    return ex10_result.error ? -1 : 0;
}
