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
#include <stdlib.h>

#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/rf_mode_definitions.h"


/* Settings used when running this example */
static const uint8_t  antenna                     = 1u;
static const uint16_t rf_mode                     = mode_103;
static const uint16_t transmit_power_cdbm         = 3000u;
static const uint8_t  initial_q                   = 2u;
static const uint8_t  max_q                       = 15u;
static const uint8_t  min_q                       = 0u;
static const uint8_t  num_min_q_cycles            = 1u;
static const uint16_t max_queries_since_valid_epc = 16u;
static const uint8_t  select_all                  = 0u;
static const bool     dual_target                 = true;
static const uint8_t  session                     = 0u;
static const uint8_t  target                      = 0u;


static struct InfoFromPackets            packet_info = {0u, 0u, 0u, 0u, {0u}};
static struct ContinuousInventorySummary continuous_inventory_summary = {0};
static struct StopConditions             stop_conditions              = {0};

// The max time duration allowed for continuous inventory in all cases.
// When no tags are found in the field of view, then the test will stop
// after this time duration. Otherwise it may run forever.
static uint32_t const max_duration_us = 4u * 1000u * 1000u;

static int continuous_inventory_stop_on_inventory_round_count(
    struct ContInventoryHelperParams cihp)
{
    stop_conditions.max_number_of_tags   = 0u;
    stop_conditions.max_duration_us      = max_duration_us;
    stop_conditions.max_number_of_rounds = 7u;

    enum InventoryHelperReturns const start_status =
        get_ex10_helpers()->continuous_inventory(&cihp);
    if (start_status != InvHelperSuccess)
    {
        ex10_ex_eprintf("continuous_inventory() failed: %u\n", start_status);
        return -1;
    }

    ex10_ex_printf("Total Singulations: %zu\n", packet_info.total_singulations);
    ex10_ex_printf("Stop Reason: %u\n", continuous_inventory_summary.reason);
    ex10_ex_printf("Time of inventory: %u.%03u s\n",
                   cihp.summary_packet->duration_us / (1000u * 1000u),
                   (cihp.summary_packet->duration_us / 1000u) % 1000u);

    if (packet_info.total_singulations == 0)
    {
        ex10_ex_eprintf("No tags found\n");
        return -1;
    }

    if (continuous_inventory_summary.reason != SRMaxNumberOfRounds)
    {
        ex10_ex_eprintf(
            "Continuous inventory stop reason expected: %u, read: %u\n",
            SRMaxNumberOfRounds,
            continuous_inventory_summary.reason);
        return -1;
    }

    return 0;
}

static int continuous_inventory_stop_on_max_tags_count(
    struct ContInventoryHelperParams cihp)
{
    // Note: The max duration time is
    stop_conditions.max_number_of_tags   = 40u;
    stop_conditions.max_duration_us      = max_duration_us;
    stop_conditions.max_number_of_rounds = 0u;

    enum InventoryHelperReturns const start_status =
        get_ex10_helpers()->continuous_inventory(&cihp);
    if (start_status != InvHelperSuccess)
    {
        ex10_ex_eprintf("continuous_inventory() failed: %u\n", start_status);
        return -1;
    }

    ex10_ex_printf("Total Singulations: %zu\n", packet_info.total_singulations);
    ex10_ex_printf("Stop Reason: %u\n", continuous_inventory_summary.reason);
    ex10_ex_printf("Time of inventory: %u.%03u s\n",
                   cihp.summary_packet->duration_us / (1000u * 1000u),
                   (cihp.summary_packet->duration_us / 1000u) % 1000u);

    if (packet_info.total_singulations == 0)
    {
        ex10_ex_eprintf("No tags found\n");
        return -1;
    }

    if (continuous_inventory_summary.reason != SRMaxNumberOfTags)
    {
        ex10_ex_eprintf(
            "Continuous inventory stop reason expected: %u, read: %u\n",
            SRMaxNumberOfTags,
            continuous_inventory_summary.reason);
        return -1;
    }

    return 0;
}

static int continuous_inventory_stop_on_duration(
    struct ContInventoryHelperParams cihp)
{
    stop_conditions.max_number_of_tags   = 0u;
    stop_conditions.max_duration_us      = max_duration_us;
    stop_conditions.max_number_of_rounds = 0u;

    enum InventoryHelperReturns const start_status =
        get_ex10_helpers()->continuous_inventory(&cihp);
    if (start_status != InvHelperSuccess)
    {
        ex10_ex_eprintf("continuous_inventory() failed: %u\n", start_status);
        return -1;
    }

    ex10_ex_printf("Total Singulations: %zu\n", packet_info.total_singulations);
    ex10_ex_printf("Stop Reason: %u\n", continuous_inventory_summary.reason);
    ex10_ex_printf("Time of inventory: %u.%03u s\n",
                   cihp.summary_packet->duration_us / (1000u * 1000u),
                   (cihp.summary_packet->duration_us / 1000u) % 1000u);

    if (packet_info.total_singulations == 0)
    {
        ex10_ex_eprintf("No tags found\n");
        return -1;
    }

    if (continuous_inventory_summary.reason != SRMaxDuration)
    {
        ex10_ex_eprintf(
            "Continuous inventory stop reason expected: %u, read: %u\n",
            SRMaxDuration,
            continuous_inventory_summary.reason);
        return -1;
    }

    return 0;
}

int main(void)
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

    struct ContInventoryHelperParams cihp = {
        .inventory_params = &inventory_params,
        .stop_conditions  = &stop_conditions,
        .summary_packet   = &continuous_inventory_summary};

    ex10_ex_printf("-----\n");
    ex10_ex_printf("Starting continuous inventory, stop on round count\n");
    int const result_round_count =
        continuous_inventory_stop_on_inventory_round_count(cihp);
    ex10_ex_printf("Stopped on round count\n");

    ex10_ex_printf("-----\n");
    ex10_ex_printf("Starting continuous inventory, stop on tag count\n");
    int const result_tag_count =
        continuous_inventory_stop_on_max_tags_count(cihp);
    ex10_ex_printf("Stopped on tag count 1\n");

    ex10_ex_printf("-----\n");
    ex10_ex_printf("Starting continuous inventory, stop on duration\n");
    int const result_duration_count =
        continuous_inventory_stop_on_duration(cihp);
    ex10_ex_printf("Stopped on duration\n");

    ex10_ex_printf("-----\n");
    ex10_ex_printf("Starting continuous inventory, stop on tag count 2\n");
    int const result_max_tags_count =
        continuous_inventory_stop_on_max_tags_count(cihp);
    ex10_ex_printf("Stopped on tag count 2\n");

    int result = result_round_count + result_tag_count + result_duration_count +
                 result_max_tags_count;

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending continuous inventory example\n");
    return result;
}
