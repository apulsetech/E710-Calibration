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
 * @file auto_access_read_write_use_case_example.c
 * @details This example shows how to use the continuous inventory use case to
 * do auto access sequences on tags while doing inventory. It does this by
 * setting up a gen2 write and then a gen2 read command. These are set into the
 * device and enabled as autoaccess commands before running continuous
 * inventory. The use case then inventories tags and calls the applications
 * callback when it receives a TagRead or Gen2Transaction event fifo packet (and
 * the ContinuousInventorySummary when it has finished.)  The callback collects
 * the TagRead, Gen2 Access Write packets and stores them. When
 * it receives the Gen2 Access Read packet, it verifies that the previous two
 * packets were received and then prints them out as a group.  This is
 * intended to show how an application might group things together to make
 * sure that the transactions were successful.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "board/board_spec.h"
#include "board/ex10_random.h"

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
#include "ex10_api/gen2_tx_command_manager.h"
#include "ex10_regulatory/ex10_default_region_names.h"

#include "ex10_use_cases/ex10_continuous_inventory_use_case.h"

#include "utils/ex10_inventory_command_line.h"
#include "utils/ex10_select_commands.h"
#include "utils/ex10_use_case_example_errors.h"

// The number of microseconds per second.
#define us_per_s 1000000u

static const uint8_t write_cmd_id = 0u;
static const uint8_t read_cmd_id  = 1u;

static const struct StopConditions stop_conditions = {
    .max_number_of_tags   = 0u,
    .max_duration_us      = 0u * us_per_s,
    .max_number_of_rounds = 2u,
};

static struct ContinuousInventorySummary continuous_inventory_summary = {
    .duration_us                = 0,
    .number_of_inventory_rounds = 0,
    .number_of_tags             = 0,
    .reason                     = SRNone,
    .last_op_id                 = 0,
    .last_op_error              = 0,
    .packet_rfu_1               = 0,
};

static uint16_t write_value = 0;

// EPC Packet to store for later
static union PacketData       tag_read_static_data;
static uint8_t                tag_read_dynamic_data[EPC_BUFFER_BYTE_LENGTH];
static struct EventFifoPacket tag_read_packet = {
    .packet_type         = TagRead,
    .us_counter          = 0u,
    .static_data         = &tag_read_static_data,
    .static_data_length  = 0,
    .dynamic_data        = tag_read_dynamic_data,
    .dynamic_data_length = sizeof(tag_read_dynamic_data),
    .is_valid            = false,
};

// Access Write Packet to store for later
static union PacketData       gen2_write_static_data;
static uint8_t                gen2_write_dynamic_data[6];
static struct EventFifoPacket gen2_write_packet = {
    .packet_type         = Gen2Transaction,
    .us_counter          = 0u,
    .static_data         = &gen2_write_static_data,
    .static_data_length  = 0,
    .dynamic_data        = gen2_write_dynamic_data,
    .dynamic_data_length = 6,
    .is_valid            = false,
};

/**
 * This function is registed as the callback with Ex10ContinuousInventoryUseCase
 * object. The example is tolerant of errors and report them with
 * ex10_ex_eprintf() istead of overwriting the *result_ptr with
 * make_ex10_app_error().
 *
 * @param packet Event Fifo packet
 * @param [out] result_ptr Ex10Result of the function. Note the function will
 * always point make_ex10_success().
 */
static void packet_subscriber_callback(struct EventFifoPacket const* packet,
                                       struct Ex10Result*            result_ptr)
{
    *result_ptr                  = make_ex10_success();
    enum Verbosity const verbose = ex10_command_line_verbosity();

    if (verbose == PRINT_EVERYTHING)
    {
        // this is debug output so we don't worry about
        // printing the events out as a group (note that
        // the scoped events will be duplicated below)
        get_ex10_event_fifo_printer()->print_packets(packet);
    }

    if (packet->packet_type == TagRead)
    {
        // store the Tag Read away for later
        ex10_deep_copy_packet(&tag_read_packet, packet);
    }
    else if (packet->packet_type == Gen2Transaction)
    {
        if (packet->static_data->gen2_transaction.transaction_id ==
            write_cmd_id)
        {
            // It is the write packet. Check to make sure there wasn't an error.
            uint16_t         reply_words[10u] = {0};
            struct Gen2Reply reply            = {
                .error_code = NoError,
                .data       = reply_words,
            };
            get_ex10_gen2_commands()->decode_reply(Gen2Write, packet, &reply);
            if (reply.error_code == NoError)
            {
                // This is the write access command response
                // save it for later verification
                ex10_deep_copy_packet(&gen2_write_packet, packet);
            }
            else
            {
                ex10_ex_eprintf("Error received during Gen2 Write command\n");
                get_ex10_gen2_commands()->print_reply(reply);
                return;
            }
        }
        else if (packet->static_data->gen2_transaction.transaction_id ==
                 read_cmd_id)
        {
            // decode the packet
            uint16_t         reply_words[10u] = {0};
            struct Gen2Reply reply            = {
                .error_code = NoError,
                .data       = reply_words,
            };
            get_ex10_gen2_commands()->decode_reply(Gen2Read, packet, &reply);

            if (reply.error_code != NoError)
            {
                ex10_ex_eprintf("Gen2Transaction Error %s\n",
                                get_ex10_gen2_error_string(reply.error_code));
            }
            else
            {
                // The sequence is now complete, so verify that we got all the
                // pieces: the TagRead, the Gen2Transaction for the write, and
                // the Gen2Transaction for the read.
                if (tag_read_packet.is_valid == false ||
                    gen2_write_packet.is_valid == false)
                {
                    ex10_ex_eprintf(
                        "Invalid Packet sequence, didn't get TagRead and Gen2 "
                        "Write packet before Gen2 Read packet was received!\n");
                    return;
                }
                else
                {
                    // check the result of the read for the return code
                    if (reply_words[0] != write_value)
                    {
                        ex10_ex_eprintf(
                            "The read value does not match the write value "
                            "Expected: %u Received: %u",
                            reply_words[0],
                            write_value);
                    }

                    // Now that all the transactions that we are expecting have
                    // been received, we will report them as a group.
                    if (verbose >= PRINT_SCOPED_EVENTS)
                    {
                        struct Ex10EventFifoPrinter const* printer =
                            get_ex10_event_fifo_printer();
                        printer->print_packets(&tag_read_packet);
                        printer->print_packets(&gen2_write_packet);
                        printer->print_packets(packet);


                        /* Last gen2 reply has result of read command */
                        if (reply_words[0] == write_value)
                        {
                            ex10_ex_printf(
                                "Response 0x%04x from Read command matched "
                                "what was written\n",
                                reply_words[0]);
                        }
                        else
                        {
                            ex10_ex_eprintf("Expected: 0x%04x, read: 0x%04x\n",
                                            write_value,
                                            reply_words[0]);
                            get_ex10_gen2_commands()->print_reply(reply);
                        }
                    }
                }
            }
            // Now that we have consumed the group of events, we clear
            // the validity of the stored events (and in the event of an erro
            // we clear them too)
            tag_read_packet.is_valid   = false;
            gen2_write_packet.is_valid = false;
        }
        else
        {
            ex10_ex_eprintf(
                "Unexpected Gen2 transaction id %d\n",
                packet->static_data->gen2_transaction.transaction_id);
            *result_ptr =
                make_ex10_app_error(Ex10ApplicationUnexpectedPacketType);
        }
    }
    else if (packet->packet_type == ContinuousInventorySummary)
    {
        continuous_inventory_summary =
            packet->static_data->continuous_inventory_summary;
    }
}

/**
 * Before starting inventory, setup gen2 sequence to write a random 16-bit
 * value to user memory bank offset 0, and then read back the word at that
 * location
 */
static struct Ex10Result setup_gen2_write_read_sequence(uint16_t data_word)
{
    struct WriteCommandArgs write_args = {
        .memory_bank  = User,
        .word_pointer = 0u,
        .data         = data_word,
    };

    struct Gen2CommandSpec write_cmd = {
        .command = Gen2Write,
        .args    = &write_args,
    };

    struct Ex10Gen2TxCommandManager const* g2tcm =
        get_ex10_gen2_tx_command_manager();
    g2tcm->clear_local_sequence();

    size_t            cmd_index = 0;
    struct Ex10Result ex10_result =
        g2tcm->encode_and_append_command(&write_cmd, write_cmd_id, &cmd_index);
    if (ex10_result.error || cmd_index != 0u)
    {
        /* First command added must have index 0 */
        ex10_ex_eprintf("Command Manager index is not 0 it is %d\n", cmd_index);
        exit(-1);
    }

    bool auto_access_enables[MaxTxCommandCount] = {0u};

    auto_access_enables[cmd_index] = true;

    struct ReadCommandArgs read_args = {
        .memory_bank  = User,
        .word_pointer = 0u,
        .word_count   = 1u,
    };

    struct Gen2CommandSpec read_cmd = {
        .command = Gen2Read,
        .args    = &read_args,
    };

    ex10_result =
        g2tcm->encode_and_append_command(&read_cmd, read_cmd_id, &cmd_index);
    if (ex10_result.error)
    {
        ex10_ex_eprintf("g2tcm->encode_and_append_command failed:\n");
        print_ex10_result(ex10_result);
        return ex10_result;
    }
    auto_access_enables[cmd_index] = true;

    /* Second command added must have index 1 */
    if (cmd_index != 1u)
    {
        /* First command added must have index 0 */
        ex10_ex_eprintf("Command Manager index is not 1 it is %d\n", cmd_index);
        exit(-1);
    }

    /* Enable the two access commands as auto access to be sent for every
     * singulated tag if auto-access is enabled in the inventory control
     * register.
     */
    ex10_result = g2tcm->write_sequence();
    if (ex10_result.error)
    {
        ex10_ex_eprintf("g2tcm->write_sequence failed:\n");
        print_ex10_result(ex10_result);
        return ex10_result;
    }
    ex10_result = g2tcm->write_auto_access_enables(
        auto_access_enables, MaxTxCommandCount, &cmd_index);

    return ex10_result;
}

static struct Ex10Result continuous_inventory_use_case_example(
    struct InventoryOptions const* inventory_options)
{
    ex10_ex_printf("Starting auto access read write use case example\n");

    struct Ex10ContinuousInventoryUseCase const* ciuc =
        get_ex10_continuous_inventory_use_case();

    ciuc->init();
    // Clear out any left over packets
    ex10_discard_packets(false, true, false);

    // set the value to be written to a random number
    write_value                   = (uint16_t)get_ex10_random()->get_random();
    struct Ex10Result ex10_result = setup_gen2_write_read_sequence(write_value);
    if (ex10_result.error)
    {
        return ex10_result;
    }
    ciuc->enable_auto_access(true);

    ciuc->register_packet_subscriber_callback(packet_subscriber_callback);
    ciuc->enable_packet_filter(ex10_command_line_verbosity() <
                               PRINT_EVERYTHING);

    uint8_t const target =
        (inventory_options->target_spec == 'B') ? target_B : target_A;
    bool const dual_target = (inventory_options->target_spec == 'D');

    struct Ex10ContinuousInventoryUseCaseParameters params = {
        .antenna         = inventory_options->antenna,
        .rf_mode         = inventory_options->mode.rf_mode_id,
        .tx_power_cdbm   = inventory_options->tx_power_cdbm,
        .initial_q       = inventory_options->initial_q,
        .session         = (uint8_t)inventory_options->session,
        .target          = target,
        .select          = (uint8_t)SelectAll,
        .send_selects    = false,
        .stop_conditions = &stop_conditions,
        .dual_target     = dual_target};

    if (inventory_options->frequency_khz != 0)
    {
        get_ex10_active_region()->set_single_frequency(
            inventory_options->frequency_khz);
    }

    if (inventory_options->remain_on)
    {
        get_ex10_active_region()->disable_regulatory_timers();
    }

    ex10_result = ciuc->continuous_inventory(&params);
    if (ex10_result.error)
    {
        // Something bad happened so we exit with an error
        // (we assume that the user was notified by whatever set the error)
        return ex10_result;
    }

    if (continuous_inventory_summary.reason != SRMaxNumberOfRounds)
    {
        // Unexpected stop reason so we exit with an error
        ex10_ex_eprintf("Unexpected stop reason: %u\n",
                        continuous_inventory_summary.reason);
        return make_ex10_app_error(Ex10StopReasonUnexpected);
    }

    uint32_t const read_rate =
        ex10_calculate_read_rate(continuous_inventory_summary.number_of_tags,
                                 continuous_inventory_summary.duration_us);

    ex10_ex_printf(
        "Read rate = %u - tags: %u / seconds %u.%03u (Mode %u)\n",
        read_rate,
        continuous_inventory_summary.number_of_tags,
        continuous_inventory_summary.duration_us / us_per_s,
        (continuous_inventory_summary.duration_us % us_per_s) / 1000u,
        params.rf_mode);

    if (continuous_inventory_summary.number_of_tags == 0)
    {
        ex10_ex_eprintf("No tags found in inventory\n");
        return make_ex10_app_error(Ex10ApplicationTagCount);
    }

    if (read_rate < inventory_options->read_rate)
    {
        ex10_ex_printf("Read rate of %u below minimal threshold of %u\n",
                       read_rate,
                       inventory_options->read_rate);
        return make_ex10_app_error(Ex10ApplicationReadRate);
    }

    return make_ex10_success();
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
        ex10_result = continuous_inventory_use_case_example(&inventory_options);
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
