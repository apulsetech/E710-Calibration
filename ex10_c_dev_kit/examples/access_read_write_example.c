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
 * @file access_read_write_example.c
 * @details The access read write example uses the Ex10Reader to singulate a
 *  tag, and then writes a random number to the tag user memory.
 */

#include <stdbool.h>
#include <stdint.h>

#include "board/ex10_osal.h"
#include "board/ex10_random.h"
#include "board/time_helpers.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/gen2_tx_command_manager.h"
#include "ex10_api/rf_mode_definitions.h"


/* Settings used when running this example */
static uint32_t const inventory_duration_ms = 500;  // Duration in milliseconds
static uint8_t const  antenna               = 1;
static uint16_t const rf_mode               = mode_148;
static uint16_t const transmit_power_cdbm   = 3000;
static uint8_t const  initial_q             = 2;
static bool const     dual_target           = true;
static uint8_t const  session               = 0;
static bool const     tag_focus_enable      = false;
static bool const     fast_id_enable        = false;


/* Global state */
static struct InfoFromPackets packet_info = {0u, 0u, 0u, 0u, {0u}};
static struct Gen2CommandSpec access_cmds[MaxTxCommandCount]    = {0u};
static bool                   halted_enables[MaxTxCommandCount] = {0u};

/**
 * Before starting inventory, setup gen2 sequence to write a random 16-bit
 * value to user memory bank offset 0, and then read back the word at that
 * location
 *
 * @return int  0 On success.
 * @return int -1 On error. Error can happen when the command was not
 * successfully encoded and appended to the command buffer
 */
static struct Ex10Result setup_gen2_write_read_sequence(uint16_t data_word)
{
    /* Setup read and write commands ahead of time */

    struct Ex10Gen2TxCommandManager const* g2tcm =
        get_ex10_gen2_tx_command_manager();
    g2tcm->clear_local_sequence();

    struct WriteCommandArgs write_args = {
        .memory_bank  = User,
        .word_pointer = 0u,
        .data         = data_word,
    };

    struct Gen2CommandSpec write_cmd = {
        .command = Gen2Write,
        .args    = &write_args,
    };

    size_t            cmd_index = 0;
    struct Ex10Result ex10_result =
        g2tcm->encode_and_append_command(&write_cmd, 0, &cmd_index);
    if (ex10_result.error || cmd_index != 0u)
    {
        ex10_ex_eprintf("Encoding and appending the write command failed\n");
        print_ex10_result(ex10_result);
        return ex10_result;
    }

    halted_enables[cmd_index] = true;
    access_cmds[cmd_index]    = write_cmd;

    struct ReadCommandArgs read_args = {
        .memory_bank  = User,
        .word_pointer = 0u,
        .word_count   = 1u,
    };

    struct Gen2CommandSpec read_cmd = {
        .command = Gen2Read,
        .args    = &read_args,
    };

    ex10_result = g2tcm->encode_and_append_command(&read_cmd, 1u, &cmd_index);
    if (ex10_result.error || cmd_index != 1u)
    {
        ex10_ex_eprintf("Encoding and appending the read command failed\n");
        print_ex10_result(ex10_result);
        return ex10_result;
    }

    halted_enables[cmd_index] = true;
    access_cmds[cmd_index]    = read_cmd;

    ex10_result = g2tcm->write_sequence();
    if (ex10_result.error)
    {
        ex10_ex_eprintf("Gen2 write sequence failed.\n");
        print_ex10_result(ex10_result);
        return ex10_result;
    }

    ex10_result = g2tcm->write_halted_enables(
        halted_enables, MaxTxCommandCount, &cmd_index);

    return ex10_result;
}

static int run_inventory_rounds(enum SelectType const select_type)
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
        .halt_on_all_tags     = true,
        .tag_focus_enable     = tag_focus_enable,
        .fast_id_enable       = fast_id_enable,
    };

    struct InventoryRoundControl_2Fields const inventory_config_2 = {
        .max_queries_since_valid_epc = 0};

    struct InventoryHelperParams ihp = {
        .antenna               = antenna,
        .rf_mode               = rf_mode,
        .tx_power_cdbm         = transmit_power_cdbm,
        .inventory_config      = &inventory_config,
        .inventory_config_2    = &inventory_config_2,
        .send_selects          = false,
        .remain_on             = false,
        .dual_target           = dual_target,
        .inventory_duration_ms = inventory_duration_ms,
        .packet_info           = &packet_info,
        .verbose               = true};

    if (get_ex10_helpers()->simple_inventory(&ihp))
    {
        return -1;
    }

    if (!packet_info.total_singulations)
    {
        ex10_ex_printf("No tags found in inventory\n");
        return -1;
    }

    return 0;
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
        if (halted_enables[iter])
        {
            next      = &access_cmds[iter];
            cmd_index = iter + 1u;
            break;
        }
    }
    return next;
}

/**
 * Run inventory round, halt on first tag, execute gen2 sequence
 */
static int run_access_read_write_seq(void)
{
    struct Ex10Reader const*  reader  = get_ex10_reader();
    struct Ex10Helpers const* helpers = get_ex10_helpers();
    uint16_t const    write_value = (uint16_t)get_ex10_random()->get_random();
    struct Ex10Result ex10_result = setup_gen2_write_read_sequence(write_value);
    if (ex10_result.error)
    {
        ex10_ex_eprintf("Setting up write read command sequence\n");
        return -1;
    }

    if (run_inventory_rounds(SelectAll) != 0)
    {
        ex10_ex_eprintf("Running inventory round failed\n");
        reader->stop_transmitting();
        return -1;
    }

    /* Should be halted on a tag now */
    bool halted = helpers->inventory_halted();
    if (halted == false)
    {
        ex10_ex_eprintf("Failed to halt on a tag\n");
        return -1;
    }

    ex10_ex_printf("Sending write and read commands\n");

    /* Trigger stored Gen2 sequence */
    ex10_result = get_ex10_ops()->send_gen2_halted_sequence();
    if (ex10_result.error)
    {
        ex10_ex_eprintf("Failed to send Gen2 halted sequence\n");
        return -1;
    }

    /* Wait for Gen2Transaction packets to be returned */
    uint32_t const   timeout          = 1000;
    uint16_t         reply_words[10u] = {0};
    struct Gen2Reply reply = {.error_code = NoError, .data = reply_words};

    size_t gen2_packet_count_expected = 2u;

    uint32_t const start_time = get_ex10_time_helpers()->time_now();
    while (get_ex10_time_helpers()->time_elapsed(start_time) < timeout &&
           gen2_packet_count_expected)
    {
        struct EventFifoPacket const* packet = reader->packet_peek();
        while (packet != NULL)
        {
            get_ex10_event_fifo_printer()->print_packets(packet);
            if (packet->packet_type == Gen2Transaction)
            {
                gen2_packet_count_expected--;
                reply.error_code = NoError;
                ex10_memzero(reply_words, sizeof(reply_words));

                get_ex10_gen2_commands()->decode_reply(
                    next_cmd()->command, packet, &reply);
                if (get_ex10_gen2_commands()->check_error(reply))
                {
                    ex10_ex_eprintf("Decoding the gen2 reply failed\n");
                    return -1;
                }
            }
            reader->packet_remove();
            packet = reader->packet_peek();
        }
    }

    if (gen2_packet_count_expected != 0u)
    {
        ex10_ex_eprintf(
            "Did not receive expected number of Gen2Transaction packets\n");
        return -1;
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

    /* Demonstrate continuing to next tag, not used here. */
    ex10_result = reader->continue_from_halted(false);
    if (ex10_result.error)
    {
        return -1;
    }

    return 0;
}

int main(void)
{
    ex10_ex_printf("Starting write read sequence example\n");

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
        ex10_ex_printf("Gen2 access issue.\n");
        get_ex10_reader()->stop_transmitting();
    }

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending write read sequence example\n");
    return result;
}
