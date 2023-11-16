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
 * @file margin_read_use_case_example.c
 * @details This use case example shows how to set and execute Margin Read
 *  commands using Ex10TagAccessUseCase. Margin Read is an Impinj proprietary
 *  command that allows the users to check the strength of the bits stored
 *  in NVM.  When the targeted bit does not match the given mask, or if any of
 *  the targeted bit is between the high and low threshold voltages, it will
 *  report a Margin Read command error. Possible error types for Margin Read
 *  are `MemoryOverrun`, `MemoryLocked`, `InsufficientPower`, and `Other`.
 *  Note that Margin Read will not check for ROM and DFF bits.
 *
 *  This is what the example does:
 *  1. Halt on a tag using the Tag Access Use Case
 *  2. Obtain a word in Tag EPC memory bank
 *  3. Execute Margin Read command targeting the EPC bank with a matching mask
 *  4. Report success
 *  5. Execute Margin Read command targeting the EPC bank with a mismatching
 *     mask
 *  6. Report failure with an error type of `Other`
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ex10_use_cases/ex10_tag_access_use_case.h"

#include "board/board_spec.h"
#include "calibration.h"

#include "ex10_api/board_init_core.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_gen2_reply_string.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/gen2_commands.h"
#include "ex10_api/gen2_tx_command_manager.h"
#include "ex10_api/rf_mode_definitions.h"
#include "ex10_regulatory/ex10_default_region_names.h"

#include "utils/ex10_inventory_command_line.h"
#include "utils/ex10_use_case_example_errors.h"

static uint8_t num_tag_read = 0u;

/**
 * Setup gen2 sequence of two margin read commands.
 * The first margin read command will check for the first word of the Tag
 * EPC memory bank with a correct mask. The second margin read command
 * will check for the same bits, but with a wrong mask.
 *
 * @param epc_word An EPC word for the mask
 * @return int Return 0 if both commands were set successfully. Otherwise, -1.
 */
static struct Ex10Result setup_gen2_EPC_margin_read(uint16_t epc_word)
{
    struct Ex10Gen2TxCommandManager const* g2tcm =
        get_ex10_gen2_tx_command_manager();
    g2tcm->clear_local_sequence();
    bool halted_enables[MaxTxCommandCount] = {0u};

    // Setup good margin read args to read correct EPC
    struct BitSpan good_mr_mask = {
        .data   = (uint8_t*)(&epc_word),
        .length = 16u,
    };

    // Note: Check the tag chip memory map to lookup the targeted bit address
    // for the desired margin read command arguments. The first EPC word
    // corresponds to EPC memory bank offset 32.
    struct MarginReadCommandArgs good_mr_args = {
        .memory_bank = EPC,
        .bit_pointer = 32u,
        .bit_length  = 16u,
        .mask        = &good_mr_mask,
    };

    struct Gen2CommandSpec good_mr_cmd = {
        .command = Gen2MarginRead,
        .args    = &good_mr_args,
    };

    size_t            cmd_index = 0;
    struct Ex10Result ex10_result =
        g2tcm->encode_and_append_command(&good_mr_cmd, 0, &cmd_index);
    if (ex10_result.error || cmd_index != 0)
    {
        ex10_ex_eprintf("g2tcm->encode_and_append_command(0) failed:\n");
        print_ex10_result(ex10_result);
        return ex10_result;
    }
    halted_enables[cmd_index] = true;

    // Setup bad margin read args to read wrong EPC
    uint16_t       bad_epc_word = epc_word + 1u;
    struct BitSpan bad_mr_mask  = {
        .data   = (uint8_t*)(&bad_epc_word),
        .length = 16u,
    };

    struct MarginReadCommandArgs bad_mr_args = {
        .memory_bank = EPC,
        .bit_pointer = 32u,
        .bit_length  = 16u,
        .mask        = &bad_mr_mask,
    };

    struct Gen2CommandSpec bad_mr_cmd = {
        .command = Gen2MarginRead,
        .args    = &bad_mr_args,
    };

    ex10_result = g2tcm->encode_and_append_command(&bad_mr_cmd, 1, &cmd_index);
    if (ex10_result.error || cmd_index != 1)
    {
        ex10_ex_eprintf("g2tcm->encode_and_append_command(1) failed:\n");
        return ex10_result;
    }
    halted_enables[cmd_index] = true;

    // Write in the access commands and enable them all
    ex10_result = g2tcm->write_sequence();
    if (ex10_result.error)
    {
        ex10_ex_eprintf("g2tcm->write_sequence() failed:\n");
        print_ex10_result(ex10_result);
        return ex10_result;
    }

    ex10_result = g2tcm->write_halted_enables(
        halted_enables, MaxTxCommandCount, &cmd_index);
    if (ex10_result.error)
    {
        ex10_ex_eprintf("g2tcm->write_halted_enables() failed:\n");
        print_ex10_result(ex10_result);
        return ex10_result;
    }

    return make_ex10_success();
}

/**
 * Check if the received packet type is 'Gen2Transaction' and the result of the
 * margin read command is reported as expected.
 *
 * @param success_expected Whether the margin read command is expected to be
 *                         successful or not
 * @return struct Ex10Result indicates whether the packet was successfully
 *         decoded or not.
 */
static struct Ex10Result decode_margin_read_reply(struct Gen2Reply* reply)
{
    enum Verbosity const               verbose = ex10_command_line_verbosity();
    struct Ex10Gen2Commands const*     gen2_commands = get_ex10_gen2_commands();
    struct Ex10TagAccessUseCase const* tauc   = get_ex10_tag_access_use_case();
    struct EventFifoPacket const*      packet = tauc->get_fifo_packet();

    if (verbose >= PRINT_SCOPED_EVENTS)
    {
        get_ex10_event_fifo_printer()->print_packets(packet);
    }

    if (packet->packet_type != Gen2Transaction)
    {
        ex10_ex_eprintf("Got unexpected packet\n");
        tauc->remove_fifo_packet();
        return make_ex10_app_error(Ex10ApplicationUnexpectedPacketType);
    }

    struct Ex10Result ex10_result = make_ex10_success();
    gen2_commands->decode_reply(Gen2MarginRead, packet, reply);
    if (reply->transaction_status != Gen2TransactionStatusOk)
    {
        ex10_ex_eprintf("Gen2Transaction Failed.\n");
        get_ex10_gen2_commands()->print_reply(*reply);
        ex10_result = make_ex10_app_error(Ex10ApplicationGen2ReplyError);
    }

    tauc->remove_fifo_packet();
    return ex10_result;
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
    *ex10_result                               = make_ex10_success();
    struct Ex10TagAccessUseCase const* tauc    = get_ex10_tag_access_use_case();
    enum Verbosity const               verbose = ex10_command_line_verbosity();

    if (verbose >= PRINT_SCOPED_EVENTS)
    {
        ex10_ex_printf("Halted on Tag callback:\n");
        get_ex10_event_fifo_printer()->print_packets(packet);
    }

    if (packet->packet_type != TagRead)
    {
        ex10_ex_eprintf("Got unexpected packet in the callback\n");
        tauc->remove_fifo_packet();
        // Error condition, we return and NAK the tag.
        *cb_result = NakTagAndContinue;
        return;
    }
    num_tag_read += 1u;

    // Read the first word of Tag's EPC
    // This will be used as the margin read command mask
    struct TagReadFields tag_read =
        get_ex10_event_parser()->get_tag_read_fields(
            packet->dynamic_data,
            packet->dynamic_data_length,
            packet->static_data->tag_read.type,
            packet->static_data->tag_read.tid_offset);
    uint16_t first_epc_word = ex10_bytes_to_uint16(tag_read.epc);

    if (verbose >= PRINT_SCOPED_EVENTS)
    {
        ex10_ex_printf("The collected epc word is: %04X\n",
                       ex10_swap_bytes(first_epc_word));
    }

    bool const halted_on_tag = packet->static_data->tag_read.halted_on_tag;
    // Discard the packet from the queue so we can look for some access packets.
    tauc->remove_fifo_packet();

    if (halted_on_tag == false)
    {
        // The LMAC read a tag, but did not successfully halt on it
        // (could have failed CRC in the ReqRN or hit a regulatory timer)
        // so we just return.  Note that the LMAC will have already continued
        // to the next slot on its own.
        ex10_ex_printf("LMAC failed to halt on tag.\n");
        *cb_result = AckTagAndContinue;
        return;
    }

    // If this callback got called, the LMAC halted on a tag and
    // should have sent a halted message back after the TagRead packet
    // so we verify that it is there, and discard it (note that the
    // packet pointer is no longer valid as it was removed above).
    if (tauc->remove_halted_packet() == false)
    {
        // Looks like there wasn't a halted packet, which suggests that
        // the LMAC didn't successfully halt on the tag, so we have the
        // same response as the halted_on_tag test above.
        // Actually, this path should not happen.
        *ex10_result = make_ex10_app_error(Ex10ApplicationMissingHaltedPacket);
        *cb_result   = AckTagAndContinue;
        return;
    }

    // Setup MarginRead command targeting the first word of the EPC bank with a
    // matching EPC value
    *ex10_result = setup_gen2_EPC_margin_read(first_epc_word);
    if (ex10_result->error)
    {
        ex10_ex_eprintf("Setting MarginRead command failed\n");
        // Successfully halted on a tag but failed to write a gen2 command.
        // This is an error condition, so Nak the tag and return
        *cb_result = NakTagAndContinue;
        return;
    }

    // Execute the command that was setup.
    enum TagAccessResult const result = tauc->execute_access_commands();
    if (result != TagAccessSuccess)
    {
        // Looks like we might have lost the tag so nothing more to do here
        // and NAK the tag just in case.
        *cb_result = NakTagAndContinue;
        return;
    }

    uint16_t         reply_words[8u] = {0u};
    struct Gen2Reply reply = {.error_code = NoError, .data = reply_words};

    // Margin read with good epc mask
    if (verbose >= PRINT_SCOPED_EVENTS)
    {
        ex10_ex_printf("MarginRead command should report success.\n");
    }

    *ex10_result = decode_margin_read_reply(&reply);
    if (ex10_result->error)
    {
        *cb_result = NakTagAndContinue;
        return;
    }

    if (reply.error_code != NoError)
    {
        ex10_ex_eprintf(
            "Expected MarginRead to success but failed. The data bits are "
            "weakly written.\n");
        get_ex10_gen2_commands()->print_reply(reply);
    }
    else if (verbose >= PRINT_SCOPED_EVENTS)
    {
        get_ex10_gen2_commands()->print_reply(reply);
    }

    // Margin read with bad epc mask
    if (verbose >= PRINT_SCOPED_EVENTS)
    {
        ex10_ex_printf(
            "Passing in a wrong mask. "
            "MarginRead command should report a failure with an error code "
            "'Other'.\n");
    }

    *ex10_result = decode_margin_read_reply(&reply);
    if (ex10_result->error)
    {
        *cb_result = NakTagAndContinue;
        return;
    }

    if (reply.error_code != Other)
    {
        ex10_ex_eprintf("MarginRead command failed due to other reasons.\n");
        get_ex10_gen2_commands()->print_reply(reply);
    }
    else if (verbose >= PRINT_SCOPED_EVENTS)
    {
        get_ex10_gen2_commands()->print_reply(reply);
    }

    // The LMAC will return to the halted state when the Access commands
    // are done, so this consumes that packet
    if (tauc->remove_halted_packet() == false)
    {
        ex10_ex_eprintf("Failed to remove the 'Halted' packet.\n");
        *ex10_result = make_ex10_app_error(Ex10ApplicationMissingHaltedPacket);
        *cb_result   = NakTagAndContinue;
        return;
    }

    // Transactions complete; continue the inventory round
    *cb_result = AckTagAndContinue;
    return;
}

static struct Ex10Result margin_read_use_case_example(
    struct InventoryOptions const* inventory_options)
{
    ex10_ex_printf("Starting margin read use case example\n");

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

    if (num_tag_read == 0u)
    {
        ex10_ex_printf("No tags found in inventory\n");
    }

    ex10_ex_printf("Ending margin read use case example: %s\n",
                   ex10_result.error ? "failed" : "success");
    get_ex10_rf_power()->stop_op_and_ramp_down();
    return ex10_result;
}

int main(int argc, char const* const argv[])
{
    struct InventoryOptions inventory_options = {
        .region_name   = "FCC",
        .read_rate     = 0u,
        .antenna       = 1u,
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
        ex10_result = margin_read_use_case_example(&inventory_options);
    }
    else
    {
        print_ex10_result(ex10_result);
    }

    ex10_core_board_teardown();
    return ex10_result.error ? -1 : 0;
}
