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
 * @file continuous_inventory_example.c
 * @details The continuous inventory example below is optimized for faster read
 * rates. This example calls continuous_inventory() helper function, which calls
 * continuous_inventory() function from the Ex10Reader layer. Meaning the SDK
 * is responsible for starting each inventory round, allowing faster read rates.
 * For better performance, this example is currently not configured to print
 * each inventoried EPC. This can be changed by using the 'verbose' inventory
 * configuration parameter.
 * The inventory example below is optimized for approximately 256 tags in FOV.
 * To adjust dynamic Q algorithm for other tag populations, the following
 * parameters should be updated:
 *
 * - initial_q
 * - max_q
 * - min_q
 * - min_q_cycles
 * - max_queries_since_valid_epc
 *
 * For additional details regarding these parameters please refer
 * to 'InventoryRoundControl' and 'InventoryRoundControl_2' registers
 * descriptions in the Ex10 Reader Chip SDK documentation.
 */

#include <stdlib.h>

#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/rf_mode_definitions.h"


/* Settings used when running this example */
static const uint8_t  antenna                     = 1u;
static const uint16_t rf_mode                     = mode_103;
static const uint16_t transmit_power_cdbm         = 3000u;
static const uint8_t  initial_q                   = 8u;
static const uint8_t  max_q                       = 15u;
static const uint8_t  min_q                       = 0u;
static const uint8_t  num_min_q_cycles            = 1u;
static const uint16_t max_queries_since_valid_epc = 16u;
static const uint8_t  select_all                  = 0u;
static const bool     dual_target                 = true;
static const uint8_t  session                     = 0u;
static const uint8_t  target                      = 0u;

static int continuous_inventory_example(uint32_t min_read_rate)
{
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
        .tag_focus_enable     = false,
        .fast_id_enable       = false,
    };

    struct InventoryRoundControl_2Fields const inventory_config_2 = {
        .max_queries_since_valid_epc = max_queries_since_valid_epc};

    struct InfoFromPackets packet_info = {0u, 0u, 0u, 0u, {0u}};

    struct ContinuousInventorySummary continuous_inventory_summary = {0};

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
        .verbose               = false,
    };

    const struct StopConditions stop_conditions = {
        .max_number_of_tags   = 0u,
        .max_duration_us      = 10u * 1000u * 1000u,
        .max_number_of_rounds = 0u,
    };

    struct ContInventoryHelperParams cihp = {
        .inventory_params = &inventory_params,
        .stop_conditions  = &stop_conditions,
        .summary_packet   = &continuous_inventory_summary};


    if (get_ex10_helpers()->continuous_inventory(&cihp))
    {
        return -1;
    }

    if (continuous_inventory_summary.reason != SRMaxDuration)
    {
        return -1;
    }

    uint32_t const read_rate =
        ex10_calculate_read_rate(continuous_inventory_summary.number_of_tags,
                                 continuous_inventory_summary.duration_us);

    // The number of microseconds per second.
    uint32_t const us_per_s = 1000u * 1000u;
    ex10_ex_printf(
        "Read rate = %u - tags: %u / seconds %u.%03u (Mode %u)\n",
        read_rate,
        continuous_inventory_summary.number_of_tags,
        continuous_inventory_summary.duration_us / us_per_s,
        (continuous_inventory_summary.duration_us % us_per_s) / 1000u,
        rf_mode);

    if (continuous_inventory_summary.number_of_tags == 0)
    {
        ex10_ex_eprintf("No tags found in inventory\n");
        return -1;
    }

    if (read_rate < min_read_rate)
    {
        ex10_ex_eprintf("Read rate of %u below minimal threshold of %u\n",
                        read_rate,
                        min_read_rate);
        return -1;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    ex10_ex_printf("Starting continuous inventory example\n");

    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    uint32_t min_read_rate = 0;

    if (argc == 2)
    {
        min_read_rate = (uint32_t)atoi(argv[1]);
    }

    int result = continuous_inventory_example(min_read_rate);

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending continuous inventory example\n");
    return result;
}
