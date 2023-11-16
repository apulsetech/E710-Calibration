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
 * @file etsi_burst_example.c
 * @details The ETSI burst example shows how to use the ETSI burst test feature
 * with the Ex10Reader.
 */

#include "board/time_helpers.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/rf_mode_definitions.h"


/* Settings used when running this test */
static const uint32_t etsi_burst_time_on =
    15 * 1000;  // Duration in milliseconds
static const uint8_t  antenna             = 1;
static const uint16_t rf_mode             = mode_241;
static const uint16_t transmit_power_cdbm = 3000;
static const uint8_t  initial_q           = 4;
static const bool     verbose             = false;

static int etsi_burst_example(void)
{
    bool                     ramp_down_seen         = false;
    bool                     ramp_up_seen           = false;
    bool                     inventory_summary_seen = false;
    uint32_t                 start_time = get_ex10_time_helpers()->time_now();
    struct Ex10Reader const* reader     = get_ex10_reader();

    struct InventoryRoundControlFields inventory_config = {
        .initial_q            = initial_q,
        .max_q                = initial_q,
        .min_q                = initial_q,
        .num_min_q_cycles     = 1,
        .fixed_q_mode         = true,
        .q_increase_use_query = false,
        .q_decrease_use_query = false,
        .session              = 0,
        .select               = 0,
        .target               = 0,
        .halt_on_all_tags     = false,
        .tag_focus_enable     = false,
        .fast_id_enable       = false,
    };

    struct InventoryRoundControl_2Fields inventory_config_2 = {
        .max_queries_since_valid_epc = 0};

    // Choose any frequency to start on
    struct Ex10Result ex10_result = get_ex10_ops()->wait_op_completion();

    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    // Spec defined time to remain on and transmitting etsi burst for each round
    const uint16_t etsi_burst_on_time_ms = 40;
    // Spec defined time to remain off between transmitting rounds
    const uint16_t etsi_burst_off_time_ms = 5;
    ex10_result =
        reader->etsi_burst_test(&inventory_config,
                                &inventory_config_2,
                                antenna,
                                rf_mode,
                                transmit_power_cdbm,
                                etsi_burst_on_time_ms,
                                etsi_burst_off_time_ms,
                                0);  // Grab a frequency from the region

    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    // Throw away startup reports to ensure they are from etsi burst
    bool first_ramp_down_received = false;
    while (first_ramp_down_received == false)
    {
        struct EventFifoPacket const* packet = reader->packet_peek();
        if (packet)
        {
            if (packet->packet_type == TxRampDown)
            {
                first_ramp_down_received = true;
            }
            reader->packet_remove();
        }
    }

    // Begin loop to ensure etsi burst is running
    while (get_ex10_time_helpers()->time_elapsed(start_time) <
           etsi_burst_time_on)
    {
        struct EventFifoPacket const* packet = reader->packet_peek();
        if (packet)
        {
            if (verbose)
            {
                ex10_ex_printf("packet type: %d\n", packet->packet_type);
            }

            // Check for necessary events that are part of etsi burst
            if (packet->packet_type == TxRampDown)
            {
                if (packet->static_data->tx_ramp_down.reason !=
                    RampDownRegulatory)
                {
                    ex10_ex_eprintf(
                        "Tx ramp down reason expected: %u, read: %u\n",
                        RampDownRegulatory,
                        packet->static_data->tx_ramp_down.reason);
                    return -1;
                }
                ramp_down_seen = true;
            }
            else if (packet->packet_type == TxRampUp)
            {
                ramp_up_seen = true;
            }
            else if (packet->packet_type == InventoryRoundSummary)
            {
                inventory_summary_seen = true;
            }
            reader->packet_remove();
        }
    }

    reader->stop_transmitting();

    if (ramp_down_seen == false)
    {
        ex10_ex_eprintf("Ramp down not seen\n");
        return -1;
    }
    if (ramp_up_seen == false)
    {
        ex10_ex_eprintf("Ramp up not seen\n");
        return -1;
    }
    if (inventory_summary_seen == false)
    {
        ex10_ex_eprintf("Inventory summary not seen\n");
        return -1;
    }

    ex10_ex_printf("Ending ETSI Burst test\n");

    return 0;
}

int main(void)
{
    ex10_ex_printf("Starting ETSI Burst test\n");

    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_ETSI_LOWER);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    int result = etsi_burst_example();

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending ETSI Burst test\n");
    return result;
}
