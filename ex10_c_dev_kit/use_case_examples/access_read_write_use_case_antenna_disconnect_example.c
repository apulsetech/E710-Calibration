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

/*****************************************************************************
 * This example uses the Ex10TagAccessUseCase to halt on all the tags in the
 * field of view, and then does a write of random data to user memory, and then
 * reads back that random data.  This sequence is setup during the halted
 * callback while the LMAC is halted on the tag..
 *
 * Then the example runs this sequence 5 times just to show that the inventory
 * round stops after the LMAC decides it is out of tags.
 *****************************************************************************/

#include <errno.h>

#include "ex10_use_cases/ex10_tag_access_use_case.h"

#include "calibration.h"

#include "board/ex10_osal.h"
#include "board/ex10_random.h"

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
#include "utils/ex10_select_commands.h"
#include "utils/ex10_use_case_example_errors.h"

#include "ex10_modules/ex10_antenna_disconnect.h"

// Iterate for Gen2 transaction ids
static uint8_t transaction_id = 0;

/**
 * Setup gen2 sequence to write a random 16-bit value to user memory bank
 * offset 0, and then read back the word at that location
 */
static struct Ex10Result setup_gen2_write_read_sequence(uint16_t data_word)
{
    // a transaction ID to mark the transactions as they
    // are printed out (we increment it every time)
    struct Ex10Gen2TxCommandManager const* g2tcm =
        get_ex10_gen2_tx_command_manager();
    g2tcm->clear_local_sequence();

    bool halted_enables[MaxTxCommandCount] = {false};

    struct WriteCommandArgs write_args = {
        .memory_bank  = User,
        .word_pointer = 0u,
        .data         = data_word,
    };

    struct Gen2CommandSpec write_cmd = {
        .command = Gen2Write,
        .args    = &write_args,
    };

    size_t            cmd_index   = 0;
    struct Ex10Result ex10_result = g2tcm->encode_and_append_command(
        &write_cmd, transaction_id++, &cmd_index);
    if (ex10_result.error || cmd_index != 0u)
    {
        ex10_ex_eprintf("g2tcm->encode_and_append_command(0) failed:\n");
        print_ex10_result(ex10_result);
        return ex10_result;
    }

    halted_enables[cmd_index] = true;

    struct ReadCommandArgs read_args = {
        .memory_bank  = User,
        .word_pointer = 0u,
        .word_count   = 1u,
    };

    struct Gen2CommandSpec read_cmd = {
        .command = Gen2Read,
        .args    = &read_args,
    };

    ex10_result = g2tcm->encode_and_append_command(
        &read_cmd, transaction_id++, &cmd_index);
    if (ex10_result.error || cmd_index != 1u)
    {
        ex10_ex_eprintf("g2tcm->encode_and_append_command(1) failed:\n");
        print_ex10_result(ex10_result);
        return ex10_result;
    }

    halted_enables[cmd_index] = true;

    ex10_result = g2tcm->write_sequence();
    if (ex10_result.error)
    {
        ex10_ex_eprintf("g2tcm->write_sequence failed:\n");
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
    struct Ex10EventFifoPrinter const* event_fifo_printer =
        get_ex10_event_fifo_printer();

    event_fifo_printer->print_packets(packet);
    if (packet->packet_type != TagRead)
    {
        ex10_ex_eprintf("Got unexpected packet in the callback\n");
        tauc->remove_fifo_packet();
        *ex10_result = make_ex10_app_error(Ex10ApplicationUnwelcomeTag);
        *cb_result   = NakTagAndContinue;
        return;
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
        ex10_ex_eprintf("LMAC failed to halt on tag\n");
        *cb_result = AckTagAndContinue;
        return;
    }

    // If this callback got called the LMAC halted on a tag and
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

    // Set up the LMAC to write some random data
    uint16_t const write_value = (uint16_t)get_ex10_random()->get_random();
    *ex10_result               = setup_gen2_write_read_sequence(write_value);

    // Now send the read-write-read access commands that was setup.
    enum TagAccessResult const result = tauc->execute_access_commands();
    if (result != TagAccessSuccess)
    {
        // looks like we might have lost the tag so nothing more to do here
        // and NAK the tag just in case.
        *cb_result = NakTagAndContinue;
        return;
    }

    // A big enough data array to handle all of the responses.
    // The write command reply can be at most 6 bytes (error condition).
    // The read command reply is at most 7 bytes (with 1 word being read).
    // Choose 8 bytes here to have a little exta buffer.
    uint16_t         reply_words[8];
    struct Gen2Reply reply = {.error_code = NoError, .data = reply_words};
    ex10_memzero(reply_words, sizeof(reply_words));

    // Expect to get 2 Gen2Transactions back. If anything else comes out
    // of the event fifo, something bad happened and we should give up and
    // NAK the tag. (could possibly be a regulatory ramp down which means
    // the tag is gone anyway).
    struct Ex10Gen2Commands const* gen2_commands = get_ex10_gen2_commands();

    // Get the first packet, which is the Write command reply.
    packet = tauc->get_fifo_packet();
    event_fifo_printer->print_packets(packet);
    if (packet->packet_type != Gen2Transaction)
    {
        ex10_ex_eprintf("Got unexpected packet\n");
        *ex10_result = make_ex10_app_error(Ex10ApplicationGen2ReplyExpected);
        tauc->remove_fifo_packet();
        *cb_result = NakTagAndContinue;
        return;
    }

    // Decode the write packet and check for error.
    gen2_commands->decode_reply(Gen2Write, packet, &reply);
    if (reply.error_code != NoError)
    {
        ex10_ex_eprintf("Write command failed\n");
        get_ex10_gen2_commands()->print_reply(reply);
        // If the tag memory is locked, then move on to the next tag.
        // All other errors will be handled as an error.
        if (reply.error_code != MemoryLocked)
        {
            *ex10_result = make_ex10_app_error(Ex10ApplicationGen2ReplyError);
        }
        tauc->remove_fifo_packet();
        *cb_result = AckTagAndContinue;
        return;
    }
    tauc->remove_fifo_packet();

    // Second packet should be the Read command reply.
    packet = tauc->get_fifo_packet();
    event_fifo_printer->print_packets(packet);
    if (packet->packet_type != Gen2Transaction)
    {
        ex10_ex_eprintf("Got unexpected packet\n");
        *ex10_result = make_ex10_app_error(Ex10ApplicationGen2ReplyExpected);
        *cb_result   = NakTagAndContinue;
        return;
    }

    // Decode the packet and check for error and
    // the read value is the first word of the data.
    ex10_memzero(reply_words, sizeof(reply_words));
    gen2_commands->decode_reply(Gen2Read, packet, &reply);
    if (reply.error_code != NoError)
    {
        ex10_ex_eprintf("Read command failed\n");
        get_ex10_gen2_commands()->print_reply(reply);
        *ex10_result = make_ex10_app_error(Ex10ApplicationGen2ReplyError);
        tauc->remove_fifo_packet();
        *cb_result = NakTagAndContinue;
        return;
    }
    tauc->remove_fifo_packet();

    // The first word of the read reply above is the data read back,
    // which should match the random number that was written.
    // Last gen2 reply has result of the Read command.
    enum HaltedCallbackResult nak_tag = AckTagAndContinue;
    if (reply.data[0] == write_value)
    {
        ex10_ex_printf(
            "Response 0x%04x from Read command matched what was written\n",
            reply.data[0]);
    }
    else
    {
        ex10_ex_eprintf(
            "Expected: 0x%04x, read: 0x%04x\n", write_value, reply.data[0]);

        get_ex10_gen2_commands()->print_reply(reply);
        *ex10_result = make_ex10_app_error(Ex10ApplicationGen2ReplyError);
        nak_tag      = NakTagAndContinue;
    }

    // The LMAC will return to the halted state when the Access commands
    // are done, so this consumes that packet
    if (tauc->remove_halted_packet() == false)
    {
        *ex10_result = make_ex10_app_error(Ex10ApplicationMissingHaltedPacket);
        *cb_result   = NakTagAndContinue;  // missing halted packet
        return;
    }

    // Transactions complete; continue the inventory round
    *cb_result = nak_tag;
    return;
}

static struct Ex10Result access_read_write_use_case_example(
    struct InventoryOptions const* inventory_options)
{
    ex10_ex_printf("Starting access read write use case example\n");

    get_ex10_antenna_disconnect()->init();

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

    // arbitrary number of rounds
    const size_t      num_rounds  = 5;
    struct Ex10Result ex10_result = make_ex10_success();
    // Run inventory 5 times just to show that it runs through only
    // one inventory round and stops.  So if multiple inventory rounds
    // or some kind of continuous inventory behavior is desired it needs
    // to be implemented at this example level.
    for (size_t iter = 1; iter <= num_rounds && ex10_result.error == false;
         iter++)
    {
        ex10_ex_printf("Inventory round %zu\n", iter);
        ex10_result = tauc->run_inventory(&params);

        get_ex10_rf_power()->stop_op_and_ramp_down();
    }

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
        .session       = SessionS0,
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
        ex10_result = access_read_write_use_case_example(&inventory_options);
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
