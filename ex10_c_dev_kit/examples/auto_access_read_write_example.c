/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2020 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

/**
 * @file auto_access_read_write_example.c
 * @details The auto access read write example shows how to do an auto access
 *  sequence using the Ex10Reader
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "board/ex10_random.h"
#include "board/time_helpers.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/gen2_tx_command_manager.h"
#include "ex10_api/rf_mode_definitions.h"

/* Settings used when running this example */
static uint8_t const  antenna             = 1;
static uint16_t const rf_mode             = mode_148;
static uint16_t const transmit_power_cdbm = 3000;
static uint8_t const  initial_q           = 2;
static bool const     dual_target         = false;
static uint8_t const  session             = 0;
static bool const     tag_focus_enable    = false;
static bool const     fast_id_enable      = false;

/* Global state */
static struct Gen2CommandSpec access_cmds[MaxTxCommandCount]         = {0u};
static bool                   auto_access_enables[MaxTxCommandCount] = {0u};


/**
 * Before starting inventory, setup gen2 sequence to write a random 16-bit
 * value to user memory bank offset 0, and then read back the word at that
 * location
 *
 * @return int  0 On success.
 * @return int -1 On error. An error can happen when the command was not
 *                successfully encoded and appended to the command buffer
 */
static int setup_gen2_write_read_sequence(uint16_t data_word)
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
        g2tcm->encode_and_append_command(&write_cmd, 0, &cmd_index);

    /* First command added must have index 0 */
    if (ex10_result.error || cmd_index != 0u)
    {
        ex10_ex_eprintf("Encoding and appending the write command failed\n");
        print_ex10_result(ex10_result);
        return -1;
    }

    auto_access_enables[cmd_index] = true;
    access_cmds[cmd_index]         = write_cmd;

    struct ReadCommandArgs read_args = {
        .memory_bank  = User,
        .word_pointer = 0u,
        .word_count   = 1u,
    };

    struct Gen2CommandSpec read_cmd = {
        .command = Gen2Read,
        .args    = &read_args,
    };

    ex10_result = g2tcm->encode_and_append_command(&read_cmd, 0, &cmd_index);
    auto_access_enables[cmd_index] = true;
    access_cmds[cmd_index]         = read_cmd;

    /* Second command added must have index 1 */
    if (ex10_result.error || cmd_index != 1u)
    {
        ex10_ex_eprintf("Encoding and appending the read command failed\n");
        print_ex10_result(ex10_result);
        return -1;
    }

    /* Enable the two access commands as auto access to be sent for every
     * singulated tag if auto-access is enabled in the inventory control
     * register.
     */
    ex10_result = g2tcm->write_sequence();
    if (ex10_result.error)
    {
        ex10_ex_eprintf("Gen2 write sequence failed\n");
        print_ex10_result(ex10_result);
        return -1;
    }

    g2tcm->write_auto_access_enables(
        auto_access_enables, MaxTxCommandCount, &cmd_index);

    return 0;
}

static int run_inventory_round(enum SelectType const         select_type,
                               struct Gen2CommandSpec const* select_config)
{
    struct InventoryRoundControlFields inventory_config = {
        .initial_q            = initial_q,
        .max_q                = initial_q,
        .min_q                = initial_q,
        .num_min_q_cycles     = 1,
        .fixed_q_mode         = true,
        .q_increase_use_query = false,
        .q_decrease_use_query = false,
        .session              = session,
        .select               = (uint8_t)select_type,
        .target               = 0,
        .halt_on_all_tags     = false,
        .tag_focus_enable     = tag_focus_enable,
        .fast_id_enable       = fast_id_enable,
        .auto_access          = true,
        .abort_on_fail        = false,
        .halt_on_fail         = false,
        .rfu                  = 0,
    };

    struct InventoryRoundControl_2Fields const inventory_config_2 = {
        .max_queries_since_valid_epc = 0};

    // stop after one round
    struct StopConditions const stop_cond = {.max_duration_us      = 500000,
                                             .max_number_of_rounds = 1,
                                             .max_number_of_tags   = 0};

    // kick off the inventory round
    struct Ex10Result ex10_result =
        get_ex10_reader()->continuous_inventory(antenna,
                                                rf_mode,
                                                transmit_power_cdbm,
                                                &inventory_config,
                                                &inventory_config_2,
                                                select_config,
                                                &stop_cond,
                                                dual_target,
                                                false);
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        ex10_ex_eprintf("\n\nContinuous Inventory failure\n");
        print_ex10_result(ex10_result);
        return -1;
    }

    return ex10_result.error;
}

/**
 * Return pointer to next enabled Gen2 Access Command
 * NOTE: Assumes at least one valid access command at index 0
 */
static struct Gen2CommandSpec const* next_cmd(void)
{
    static size_t                 cmd_index = 0u;
    struct Gen2CommandSpec const* next      = &access_cmds[0u];

    for (size_t iter = cmd_index; iter < 10u; iter++)
    {
        if (auto_access_enables[iter])
        {
            next      = &access_cmds[iter];
            cmd_index = iter + 1u;
            break;
        }
    }
    return next;
}

/**
 * Run inventory round, execute auto access sequence
 * then parse through the event fifo data and make sure
 * we get what we expect.
 */
static int run_access_read_write_seq(void)
{
    uint16_t write_value = (uint16_t)get_ex10_random()->get_random();

    if (setup_gen2_write_read_sequence(write_value) != 0)
    {
        ex10_ex_eprintf("Setting up write read command sequence failed\n");
        return -1;
    }

    // Recording the start time
    uint32_t const start_time_ms = get_ex10_time_helpers()->time_now();

    get_ex10_helpers()->discard_packets(false, true, false);

    if (run_inventory_round(SelectAll, NULL))
    {
        ex10_ex_eprintf("Run Inventory Round returned failed\n");
        return -1;
    }
    // this should all be over in less than a second
    uint32_t const                failsafe_timeout_ms = 1000u;
    struct EventFifoPacket const* packet              = NULL;

    int              total_singulations      = 0;
    int              total_gen2_transactions = 0;
    uint16_t         reply_words[10u]        = {0};
    struct Gen2Reply reply = {.error_code = NoError, .data = reply_words};

    bool inventory_done = false;
    while (inventory_done == false)
    {
        if ((failsafe_timeout_ms) && (get_ex10_time_helpers()->time_elapsed(
                                          start_time_ms) > failsafe_timeout_ms))
        {
            ex10_ex_eprintf(
                "Timed out before inventory summary packets received\n");
            return -1;
        }
        struct Ex10Reader const* ex10_reader = get_ex10_reader();
        packet                               = ex10_reader->packet_peek();
        if (packet != NULL)
        {
            if (packet->packet_type == TagRead)
            {
                total_singulations++;
                // don't care about the tag read data just how
                // many were singulated
            }
            else if (packet->packet_type == Gen2Transaction)
            {
                total_gen2_transactions++;
                struct Gen2CommandSpec const* comm_spec = next_cmd();
                // if the command is a read we grab the data so
                // it can be verified below
                if (comm_spec->command == Gen2Read)
                {
                    get_ex10_gen2_commands()->decode_reply(
                        comm_spec->command, packet, &reply);
                    if (get_ex10_gen2_commands()->check_error(reply))
                    {
                        ex10_ex_eprintf("Decoding Gen2 command reply failed\n");
                        return -1;
                    }
                }
            }
            else if (packet->packet_type == ContinuousInventorySummary)
            {
                inventory_done = true;
            }
            ex10_reader->packet_remove();
        }
    }

    /* Last gen2 reply has result of read command */
    if (reply_words[0] == write_value)
    {
        ex10_ex_printf(
            "Response 0x%04x from Read command matched what was written\n",
            reply_words[0]);
    }
    else
    {
        ex10_ex_eprintf(
            "Expected: 0x%04x, read: 0x%04x\n", write_value, reply_words[0]);
        get_ex10_gen2_commands()->check_error(reply);

        return -1;
    }
    /* make sure we did two transactions */
    if (total_gen2_transactions != 2)
    {
        ex10_ex_eprintf("Number of Gen2Transaction expected: 2, received: %d\n",
                        total_gen2_transactions);
        return -1;
    }
    /* make sure we only sigulated one tag */
    if (total_singulations != 1)
    {
        ex10_ex_eprintf("Number of singulations expected: 1, received: %d\n",
                        total_singulations);
        return -1;
    }

    return 0;
}

int main(void)
{
    ex10_ex_printf("Starting write+read sequence example\n");

    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    int result = run_access_read_write_seq();
    if (result == -1)
    {
        ex10_ex_eprintf("Read/write access issue.\n");
        get_ex10_reader()->stop_transmitting();
    }

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending write+read sequence example\n");
    return result;
}
