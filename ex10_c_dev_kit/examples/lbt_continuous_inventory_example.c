/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2022 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

/**
 * @file lbt_continuous_inventory_example.c
 * @details The inventory example below demonstrates continuous inventory using
 *  Listen Before Talk (LBT). This example calls the
 *  continuous_inventory_lbt() lbt helper function, which assigns an
 *  LBT-specific pre-ramp callback to check and then kicks off normal
 *  continuous inventory. The pre-ramp check will look for passing LBT RSSI
 *  values. If the values are over acceptable values, the continuous
 *  inventory will halt.
 *
 *   - lbt_offset_khz: The LBT offset to use when measuring RSSI.
 *   - rssi_count_exp: The RSSI integration count to use in the HW RSSI block.
 *   - passes_required: The number of good RSSI values needed in a row for
 *     overall LBT success.
 *
 *  This example requires a single pass for example's sake of not knowing the
 *  user operating environment. Adjust the number of passes according to user
 *  need.
 *
 *   - lbt_pass_threshold_cdbm: The threshold to fall below where the call will
 *     be added to the consecutive run count.
 *   - max_lbt_tries: the max number of times to check RSSI while attempting to
 *     getconsecutive good RSSI values.
 */

#include <stdlib.h>
#include <string.h>

#include "board/time_helpers.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_lbt_helpers.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/rf_mode_definitions.h"

/* Settings used when running this example */
static const uint8_t  antenna                     = 1;
static const uint16_t rf_mode                     = mode_222;
static const uint16_t transmit_power_cdbm         = 3000u;
static const uint8_t  initial_q                   = 8u;
static const uint8_t  max_q                       = 15u;
static const uint8_t  min_q                       = 0u;
static const uint8_t  num_min_q_cycles            = 1u;
static const uint16_t max_queries_since_valid_epc = 16u;
static const uint8_t  select_all                  = 0u;
static const bool     dual_target                 = true;
static const uint8_t  session                     = 0u;
static const bool     tag_focus_enable            = false;
static const bool     fast_id_enable              = false;

/* Global state */
static uint8_t target = 0u;

// The number of milliseconds per second.
static const uint32_t us_per_s = 1000u * 1000u;

static int continuous_inventory_with_lbt(uint32_t frequency_khz)
{
    /* Used for info in reading out the event FIFO */
    struct InfoFromPackets packet_info = {0u, 0u, 0u, 0u, {0u}};

    struct InventoryRoundControlFields inventory_config = {
        .initial_q            = initial_q,
        .max_q                = max_q,
        .min_q                = min_q,
        .num_min_q_cycles     = num_min_q_cycles,
        .fixed_q_mode         = false,
        .q_increase_use_query = false,
        .q_decrease_use_query = false,
        .session              = session,
        .select               = select_all,
        .target               = target,
        .halt_on_all_tags     = false,
        .tag_focus_enable     = tag_focus_enable,
        .fast_id_enable       = fast_id_enable,
    };

    struct InventoryRoundControl_2Fields inventory_config_2 = {
        .max_queries_since_valid_epc = max_queries_since_valid_epc};

    struct InventoryHelperParams inventory_params = {
        .antenna               = antenna,
        .rf_mode               = rf_mode,
        .tx_power_cdbm         = transmit_power_cdbm,
        .inventory_config      = &inventory_config,
        .inventory_config_2    = &inventory_config_2,
        .send_selects          = false,
        .remain_on             = false,
        .dual_target           = dual_target,
        .inventory_duration_ms = 0,  // irrelevant for continuous inventory
        .packet_info           = &packet_info,
        .verbose               = true,
    };

    struct ContinuousInventorySummary continuous_inventory_summary = {0};
    const struct StopConditions       stop_conditions              = {
        .max_number_of_tags   = 0u,
        .max_duration_us      = 10u * us_per_s,
        .max_number_of_rounds = 0u,
    };

    struct ContInventoryHelperParams cihp = {
        .inventory_params = &inventory_params,
        .stop_conditions  = &stop_conditions,
        .summary_packet   = &continuous_inventory_summary};

    if (frequency_khz != 0)
    {
        get_ex10_active_region()->set_single_frequency(frequency_khz);
    }

    // Start running continuous inventory
    uint32_t const start_time_ms = get_ex10_time_helpers()->time_now();
    enum InventoryHelperReturns ret_val =
        get_ex10_lbt_helpers()->continuous_inventory_lbt(&cihp);
    if (ret_val != InvHelperSuccess)
    {
        ex10_ex_eprintf("Starting continuous inventory failed: %d\n", ret_val);
        return -1;
    }

    struct Ex10Reader const* reader = get_ex10_reader();
    // Now wait for continuous inventory to end
    bool inventory_done = false;
    while (false == inventory_done)
    {
        // Check the event fifo
        struct EventFifoPacket const* packet = reader->packet_peek();
        while (packet != NULL)
        {
            get_ex10_helpers()->examine_packets(packet, &packet_info);
            get_ex10_event_fifo_printer()->print_packets(packet);
            // If continuous inventory is done, we can exit
            if (packet->packet_type == ContinuousInventorySummary)
            {
                // Update module variable continuous_inventory_summary:
                continuous_inventory_summary =
                    packet->static_data->continuous_inventory_summary;

                if (continuous_inventory_summary.reason != SRMaxDuration)
                {
                    ex10_ex_eprintf(
                        "The done reason was not max duration for continuous "
                        "inventory. Reason was: %d\n",
                        continuous_inventory_summary.reason);
                    return -2;
                }
                inventory_done = true;
            }

            reader->packet_remove();
            packet = reader->packet_peek();
        }
        uint64_t const failsafe_timeout_ms =
            stop_conditions.max_duration_us * 2;
        if (get_ex10_time_helpers()->time_elapsed(start_time_ms) >
            failsafe_timeout_ms)
        {
            ex10_ex_eprintf(
                "Timeout while waiting for continuous inventory completion");
            return -3;
        }
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
        rf_mode);
    (void)read_rate;

    if (packet_info.total_singulations == 0)
    {
        ex10_ex_eprintf("No tags found in inventory\n");
        return -4;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    ex10_printf("Starting continuous inventory with LBT\n");

    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_JAPAN2);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    uint32_t frequency_khz = 0;

    // Frequency is passed for testing purposes. If you pass a non-0
    // frequency here, the example will constantly ramp up to that frequency
    // over and over instead of hopping. The default of 0 tells the example
    // to hop normally.
    if (argc == 2)
    {
        frequency_khz = (uint32_t)atoi(argv[1]);
    }

    int result = continuous_inventory_with_lbt(frequency_khz);

    ex10_typical_board_teardown();
    ex10_printf("Ending continuous inventory with LBT\n");
    return result;
}
