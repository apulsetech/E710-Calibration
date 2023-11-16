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
 * @file continuous_inventory_use_case_lbt_antenna_disconnect_example.c
 * @details  The ontinuous inventory example below is optimized for read rates.
 *  This example sets up the continuous inventory use case and calls
 *  the continuous_inventory() function from the Ex1ContinuousInventoryUseCase.
 *  This means the SDK is responsible for starting each inventory round,
 *  allowing faster read rates. For better performance, this example is
 *  currently not configured to print each inventoried EPC.
 *  This can be changed by using the 'verbose' inventory configuration
 * parameter. The inventory example below is optimized for approximately 256
 * tags in FOV. To adjust dynamic Q algorithm for other tag populations, the
 * following parameters should be updated:
 *
 *  - initial_q
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "calibration.h"
#include "ex10_api/board_init_core.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_regulatory/ex10_default_region_names.h"

#include "ex10_use_cases/ex10_continuous_inventory_use_case.h"

#include "ex10_modules/ex10_antenna_disconnect_and_listen_before_talk.h"
#include "ex10_modules/ex10_ramp_module_manager.h"

#include "utils/ex10_inventory_command_line.h"
#include "utils/ex10_use_case_example_errors.h"

// The number of microseconds per second.
#define us_per_s 1000000u

// the regulatory timers in japan are 4 seconds
#define JAPAN_REGION_TIME_S 4u
#define NUMBER_OF_RAMPS 5u

static const struct StopConditions stop_conditions = {
    .max_number_of_tags   = 0u,
    .max_duration_us      = JAPAN_REGION_TIME_S * NUMBER_OF_RAMPS * us_per_s,
    .max_number_of_rounds = 0u,
};

static struct ContinuousInventorySummary continuous_inventory_summary = {
    .duration_us                = 0,
    .number_of_inventory_rounds = 0,
    .number_of_tags             = 0,
    .reason                     = SRNone,
    .last_op_id                 = 0,
    .last_op_error              = 0,
    .packet_rfu_1               = 0};

static void packet_subscriber_callback(struct EventFifoPacket const* packet,
                                       struct Ex10Result*            result_ptr)
{
    *result_ptr                  = make_ex10_success();
    enum Verbosity const verbose = ex10_command_line_verbosity();

    if (verbose >= PRINT_EVERYTHING)
    {
        get_ex10_event_fifo_printer()->print_packets(packet);
    }
    else if (verbose == PRINT_SCOPED_EVENTS)
    {
        if (packet->packet_type == TagRead ||
            packet->packet_type == ContinuousInventorySummary)
        {
            get_ex10_event_fifo_printer()->print_packets(packet);
        }
    }

    if (packet->packet_type == ContinuousInventorySummary)
    {
        continuous_inventory_summary =
            packet->static_data->continuous_inventory_summary;
    }
}

static struct Ex10Result continuous_inventory_use_case_example(
    struct InventoryOptions const* inventory_options)
{
    ex10_ex_printf("Starting continuous inventory use case with lbt example\n");

    struct Ex10ContinuousInventoryUseCase const* ciuc =
        get_ex10_continuous_inventory_use_case();

    ciuc->init();
    // Clear out any left over packets
    ex10_discard_packets(false, true, false);

    // Install lbt module
    get_ex10_antenna_disconnect_and_listen_before_talk()->init();

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

    struct Ex10Result ex10_result = ciuc->continuous_inventory(&params);
    if (ex10_result.error)
    {
        // Something bad happened so we exit with an error
        // (we assume that the user was notified by whatever set the error)
        return ex10_result;
    }

    if (continuous_inventory_summary.reason != SRMaxDuration)
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
        ex10_ex_printf("No tags found in inventory\n");
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
        .region_name   = "JAPAN2",
        .read_rate     = 0u,
        .antenna       = 1u,
        .frequency_khz = 0u,
        .remain_on     = false,
        .tx_power_cdbm = 3000,
        .mode          = {.rf_mode_id = mode_148},
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
        if (ex10_result.error)
        {
            print_ex10_app_result(ex10_result);
            if (ex10_result.module == Ex10AntennaDisconnect)
            {
                struct Ex10AntennaDisconnectListenBeforeTalk const* ad_lbt =
                    get_ex10_antenna_disconnect_and_listen_before_talk();
                ex10_eprintf(
                    "Reverse Power Threshold Exceeded, Measured: %d Threshold: "
                    "%d\n",
                    ad_lbt->get_last_reverse_power_adc(),
                    ad_lbt->get_last_reverse_power_adc_threshold());
            }
            else if (ex10_result.module == Ex10ListenBeforeTalk)
            {
                struct Ex10AntennaDisconnectListenBeforeTalk const* ad_lbt =
                    get_ex10_antenna_disconnect_and_listen_before_talk();
                ex10_eprintf(
                    "LBT prevented ramping on channel %ld with RSSI of %d",
                    ad_lbt->get_last_frequency_khz(),
                    ad_lbt->get_last_rssi_measurement());
            }
        }
    }
    else
    {
        print_ex10_result(ex10_result);
    }

    ex10_core_board_teardown();
    return ex10_result.error ? -1 : 0;
}
