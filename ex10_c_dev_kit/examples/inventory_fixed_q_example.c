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
 * @file inventory_fixed_q_example.c
 * @details The inventory example below uses 'fixed q' test mode and is not
 *  optimized for read rate.
 */

#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/rf_mode_definitions.h"

/* Settings used when running this example */
static const uint32_t inventory_duration_ms = 10 * 1000;
static const uint8_t  antenna               = 1;
static const uint16_t rf_mode               = mode_148;
static const uint16_t transmit_power_cdbm   = 3000;
static const uint8_t  initial_q             = 4;
static const uint8_t  select_all            = 0;
static const bool     dual_target           = true;
static const uint8_t  session               = 0;
static const bool     tag_focus_enable      = false;
static const bool     fast_id_enable        = false;

/* Global state */
static uint8_t target = 0;

static int inventory_example(void)
{
    /* Used for info in reading out the event FIFO */
    struct InfoFromPackets packet_info = {0u, 0u, 0u, 0u, {0u}};

    struct InventoryRoundControlFields inventory_config = {
        .initial_q            = initial_q,
        .max_q                = initial_q,
        .min_q                = initial_q,
        .num_min_q_cycles     = 1,
        .fixed_q_mode         = true,
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

    ex10_ex_printf("Total Singulations: %zd\n", packet_info.total_singulations);

    if (packet_info.total_singulations == 0)
    {
        ex10_ex_eprintf("No tags found in inventory\n");
        return 1;
    }

    return 0;
}

int main(void)
{
    ex10_ex_printf("Starting inventory with fixed Q example\n");

    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    int result = inventory_example();

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending inventory with fixed Q example\n");
    return result;
}
