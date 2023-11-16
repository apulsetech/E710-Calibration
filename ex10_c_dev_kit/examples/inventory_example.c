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
 * @file inventory_example.c
 * The inventory example below demonstrates inventory operation managed by the
 * top level application.
 * This example calls simple_inventory() helper function, which repeatedly calls
 * inventory() function from the Ex10Reader layer. Meaning the application layer
 * is responsible for starting each inventory round, allowing closer control
 * over Ex10 inventory operation. The inventory example below is optimized for
 * approximately 256 tags in FOV. To adjust dynamic Q algorithm for other tag
 * populations, the following parameters should be updated:
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
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/rf_mode_definitions.h"

/* Settings used when running this example */
static const uint32_t inventory_duration_ms       = 10 * 1000;
static const uint8_t  antenna                     = 1;
static const uint16_t rf_mode                     = mode_148;
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
static const uint8_t  target                      = 0u;

static int inventory_example(uint32_t min_read_rate)
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
        .verbose               = true,
    };

    switch (get_ex10_helpers()->simple_inventory(&ihp))
    {
        case InvHelperSuccess:
            break;
        case InvHelperOpStatusError:
            ex10_ex_eprintf("Op error during inventory\n");
            return -1;
        case InvHelperStopConditions:
            ex10_ex_eprintf("Stop conditions hit in simple inventory\n");
            return -1;
        case InvHelperTimeout:
            ex10_ex_eprintf("Timeout reached\n");
            return -1;
        default:
            return -1;
    }

    uint32_t const read_rate = ex10_calculate_read_rate(
        packet_info.total_singulations, inventory_duration_ms * 1000u);

    ex10_ex_printf("Read rate = %u - tags: %zu / seconds %u.%03u (Mode %u)\n",
                   read_rate,
                   packet_info.total_singulations,
                   inventory_duration_ms / 1000u,
                   inventory_duration_ms % 1000u,
                   rf_mode);

    if (packet_info.total_singulations == 0)
    {
        ex10_ex_eprintf("No tags found in inventory\n");
        return 1;
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
    ex10_ex_printf("Starting inventory example\n");

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
    int result = inventory_example(min_read_rate);

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending inventory example\n");
    return result;
}
