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
 * @file regions_usage_example.c
 * @details  This example shows how to manipulate the regions module and
 *  generate custom regions, or direct the SDK to run on specific channels
 */

#include <stdlib.h>

#include "board_spec_constants.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/rf_mode_definitions.h"


/* Settings used when running this example */
static const uint8_t           antenna             = 1u;
static uint16_t                rf_mode             = mode_103;
static const uint16_t          transmit_power_cdbm = 3000u;
static const uint8_t           initial_q           = 4u;
static const uint8_t           max_q               = 15u;
static const uint8_t           select_all          = 0u;
static const bool              dual_target         = true;
static const uint8_t           session             = 0u;
static const enum Ex10RegionId region_id           = REGION_FCC;
/* Global state */
static uint8_t target = 0;

static int watch_fifo(uint16_t* channel_list, uint8_t channel_list_len)
{
    // Use the iterator to ensure we run through our new frequency list
    uint8_t                  inv_iter       = 0;
    bool                     inventory_done = false;
    bool                     verbose_prints = false;
    struct Ex10Reader const* reader         = get_ex10_reader();

    while (false == inventory_done)
    {
        // Check the event fifo
        struct EventFifoPacket const* packet = reader->packet_peek();
        while (packet != NULL)
        {
            if (verbose_prints)
            {
                get_ex10_event_fifo_printer()->print_packets(packet);
            }
            // If continuous inventory is done, we can exit
            if (packet->packet_type == ContinuousInventorySummary)
            {
                if (packet->static_data->continuous_inventory_summary.reason !=
                    SRMaxDuration)
                {
                    ex10_ex_eprintf(
                        "The done reason did not match the expected reason "
                        "set in the continuous inventory parameters.\n");
                    return -1;
                }
                inventory_done = true;
            }
            else if (packet->packet_type == TxRampUp)
            {
                uint32_t channel_khz =
                    get_ex10_regulatory()->calculate_channel_khz(
                        region_id, channel_list[inv_iter]);
                if (channel_khz !=
                    packet->static_data->tx_ramp_up.carrier_frequency)
                {
                    ex10_ex_eprintf(
                        "The inventory did not ramp to the next specified "
                        "channel.\n");
                    return -1;
                }
                inv_iter += 1u;
                if (inv_iter >= channel_list_len)
                {
                    inv_iter = 0u;
                }
            }

            reader->packet_remove();
            packet = reader->packet_peek();
        }
    }
    return 0;
}

static int continuous_inventory_single_frequency(
    struct InventoryRoundControlFields* inventory_config)
{
    const struct StopConditions stop_conditions = {
        .max_number_of_tags   = 0u,
        .max_duration_us      = 2 * 1000u * 1000u,
        .max_number_of_rounds = 0u,
    };

    struct InventoryRoundControl_2Fields const inventory_config_2 = {
        .max_queries_since_valid_epc = 16u};

    // Create a channel list to search for in the fifo
    uint16_t channel_list[1]  = {1};
    uint8_t  channel_list_len = 1;

    // Grab the region and get the starting frequency for channel 0
    struct Ex10Region const* curr_region =
        get_ex10_regulatory()->get_region(region_id);
    uint32_t frequency_khz = curr_region->regulatory_channels.start_freq_khz;

    // Set the region to use the single frequency corresponding to the channel
    get_ex10_active_region()->set_single_frequency(frequency_khz);

    struct Ex10Result ex10_result =
        get_ex10_reader()->continuous_inventory(antenna,
                                                rf_mode,
                                                transmit_power_cdbm,
                                                inventory_config,
                                                &inventory_config_2,
                                                NULL,
                                                &stop_conditions,
                                                dual_target,
                                                true);
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    return watch_fifo(channel_list, channel_list_len);
}

static int inventory_update_channel_list(
    struct InventoryRoundControlFields* inventory_config)
{
    const struct StopConditions stop_conditions = {
        .max_number_of_tags   = 0u,
        .max_duration_us      = 2 * 1000u * 1000u,
        .max_number_of_rounds = 0u,
    };

    struct InventoryRoundControl_2Fields const inventory_config_2 = {
        .max_queries_since_valid_epc = 0u};

    uint16_t channel_list[4]  = {4, 5, 6, 7};
    uint8_t  channel_list_len = ARRAY_SIZE(channel_list);

    // Base our new region off of the default region
    struct Ex10Region const* active_region =
        get_ex10_regulatory()->get_region(region_id);
    struct Ex10Region new_region = *active_region;

    // Update the usable channels
    new_region.regulatory_channels.usable = channel_list;
    new_region.regulatory_channels.count  = channel_list_len;

    // Now set the region that gets returned from get_region and
    // set the current region.
    get_ex10_regulatory()->set_region(region_id, &new_region);
    get_ex10_active_region()->set_region(region_id, TCXO_FREQ_KHZ);

    // Now pass NULL and 0 for the channel list in continuous inventory, which
    // will use the defaults we have now set in the region.
    struct Ex10Result ex10_result =
        get_ex10_reader()->continuous_inventory(antenna,
                                                rf_mode,
                                                transmit_power_cdbm,
                                                inventory_config,
                                                &inventory_config_2,
                                                NULL,
                                                &stop_conditions,
                                                dual_target,
                                                true);
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    return watch_fifo(channel_list, channel_list_len);
}

int main(void)
{
    ex10_ex_printf("Starting regions usage example\n");

    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, region_id);

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
        .min_q                = 0,
        .num_min_q_cycles     = 2u,
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

    int result = continuous_inventory_single_frequency(&inventory_config);
    if (result == 0)
    {
        result = inventory_update_channel_list(&inventory_config);
    }

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending regions usage example\n");
    return result;
}
