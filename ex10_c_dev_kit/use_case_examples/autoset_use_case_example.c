/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2023 Impinj, Inc. All rights reserved.                      *
 *                                                                           *
 *****************************************************************************/

/**
 * @file autoset_use_case_example.c
 * @details The autoset use case example illustrates how to
 *
 * - Initialize an array of inventory sequence structures to iterate through
 *   the Autoset RF modes.
 * - Begin and end the inventory iteration through the Autoset RF modes.
 *
 * The Autoset RF mode inventory sequence progresses from the fastest, least
 * sensitive RF mode to the slowest most sensitive RF mode. This sequence
 * is optimized to find the most number of tags in the field of view in the
 * least amount of time.
 */

#include <errno.h>

#include "board/board_spec.h"
#include "board/ex10_osal.h"
#include "calibration.h"
#include "ex10_api/aggregate_op_builder.h"
#include "ex10_api/board_init_core.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_autoset_modes.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_regulatory/ex10_default_region_names.h"

#include "ex10_modules/ex10_ramp_module_manager.h"

#include "ex10_use_cases/ex10_inventory_sequence_use_case.h"

#include "utils/ex10_inventory_command_line.h"
#include "utils/ex10_select_commands.h"
#include "utils/ex10_use_case_example_errors.h"

static bool const debug_aggregate_op = true;

// Inventory sequence counting and timestamp.
// These need to be initialized prior to calling
static size_t   inventory_round_number  = 0u;
static uint32_t inventory_start_time_us = 0u;

struct RfModeInventoryStats
{
    uint16_t mode;         ///< The RF mode used during the inventory.
    uint8_t  target;       ///< The session inventory flag: target_A, target_B.
    size_t   tag_count;    ///< The number of tags singulated during inventory.
    uint32_t duration_us;  ///< The inventory duration in microseconds.
};

static struct RfModeInventoryStats rf_mode_stats[AUTOSET_RF_MODE_COUNT * 2u];

static void print_tag_counter_header(void)
{
    ex10_ex_printf("rf_mode, target, tag_count, duration_s, read_rate\n");
}

static void print_rf_mode_stats_row(
    struct RfModeInventoryStats const* rf_mode_stat)
{
    uint32_t const read_rate = ex10_calculate_read_rate(
        rf_mode_stat->tag_count, rf_mode_stat->duration_us);
    uint32_t const duration_ms = rf_mode_stat->duration_us / 1000u;

    ex10_ex_printf("%7u,", rf_mode_stat->mode);
    ex10_ex_printf("      %c,", (rf_mode_stat->target == target_A) ? 'A' : 'B');
    ex10_ex_printf("%10zu, ", rf_mode_stat->tag_count);
    ex10_ex_printf("%6u.%03u, ", duration_ms / 1000u, duration_ms % 1000u);
    ex10_ex_printf("%9u", read_rate);
    ex10_ex_printf("\n");
}

static struct RfModeInventoryStats print_rf_mode_stats(
    enum AutosetModeId                 autoset_mode_id,
    struct RfModeInventoryStats const* autoset_stats,
    size_t                             tag_counter_length)
{
    struct RfModeInventoryStats autoset_mode_stats = {
        .mode        = (uint16_t)autoset_mode_id,
        .target      = autoset_stats[0u].target,
        .tag_count   = 0u,
        .duration_us = 0u,
    };

    for (size_t iter = 0u; iter < tag_counter_length; ++iter)
    {
        print_rf_mode_stats_row(&autoset_stats[iter]);
        autoset_mode_stats.tag_count += autoset_stats[iter].tag_count;
        autoset_mode_stats.duration_us += autoset_stats[iter].duration_us;
    }
    print_rf_mode_stats_row(&autoset_mode_stats);

    return autoset_mode_stats;
}

/**
 * Send the initial select command: ramp up Tx and send it.
 *
 * Only send this select command once; not every time Tx is ramped up.
 * The Ex10Ops.inventory() and Ex10Reader.inventory() SDK's parameter
 * send_selects will cause the selects programmed into the Tx command buffer
 * to be sent each time Tx is ramped up. Transitioning tags from A -> B
 * every time Tx ramps up would be self-defeating.
 *
 * @return struct Ex10Result
 *         Indicates whether the function was successful or not.
 */
static struct Ex10Result send_initial_select(uint8_t      antenna,
                                             enum RfModes rf_mode,
                                             uint32_t     frequency_khz,
                                             int16_t      tx_power_cdbm)
{
    uint16_t          temperature_adc = UINT16_MAX;
    struct Ex10Result ex10_result =
        get_ex10_rf_power()->measure_and_read_adc_temperature(&temperature_adc);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    bool const temp_comp_enabled =
        get_ex10_board_spec()->temperature_compensation_enabled(
            temperature_adc);

    if (frequency_khz)
    {
        get_ex10_active_region()->set_single_frequency(frequency_khz);
    }
    struct CwConfig cw_config;
    get_ex10_rf_power()->build_cw_configs(antenna,
                                          rf_mode,
                                          tx_power_cdbm,
                                          temperature_adc,
                                          temp_comp_enabled,
                                          &cw_config);

    /// Tx power droop compensation with 25ms interval and .01 dB step.
    struct PowerDroopCompensationFields const droop_comp_defaults =
        get_ex10_rf_power()->get_droop_compensation_defaults();
    ex10_result = get_ex10_rf_power()->set_rf_mode(rf_mode);
    if (ex10_result.error)
    {
        return ex10_result;
    }
    ex10_result = get_ex10_rf_power()->cw_on(&cw_config.gpio,
                                             &cw_config.power,
                                             &cw_config.synth,
                                             &cw_config.timer,
                                             &droop_comp_defaults);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    struct Ex10Ops const* ex10_ops = get_ex10_ops();
    ex10_result                    = ex10_ops->send_select();
    if (ex10_result.error)
    {
        return ex10_result;
    }
    ex10_result = ex10_ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    return make_ex10_success();
}

static void packet_subscriber_callback(struct EventFifoPacket const* packet,
                                       struct Ex10Result* ex10_result)
{
    *ex10_result                   = make_ex10_success();
    enum Verbosity const verbosity = ex10_command_line_verbosity();

    if (verbosity == PRINT_EVERYTHING)
    {
        // Printing of TagRead packets has RSSI compensated handling below.
        if (packet->packet_type != TagRead)
        {
            get_ex10_event_fifo_printer()->print_packets(packet);
        }
    }

    switch (packet->packet_type)
    {
        case TagRead:
            rf_mode_stats[inventory_round_number].tag_count += 1u;
            if (verbosity >= PRINT_SCOPED_EVENTS)
            {
                struct Ex10InventorySequenceUseCase const* use_case =
                    get_ex10_inventory_sequence_use_case();

                struct InventoryRoundConfigBasic const* inventory_round =
                    use_case->get_inventory_round();

                get_ex10_event_fifo_printer()
                    ->print_event_tag_read_compensated_rssi(
                        packet,
                        inventory_round->rf_mode,
                        inventory_round->antenna,
                        get_ex10_active_region()->get_rf_filter(),
                        get_ex10_ramp_module_manager()
                            ->retrieve_adc_temperature());
            }
            break;

        case AggregateOpSummary:
        {
            // Note: The AggregateOpSummary packet is not published unless the
            // Ex10InventorySequenceUseCase.enable_packet_filter(false) is set.
            // This packet is used to debug aggregate ops if/when they fail.
            struct AggregateOpSummary const* aggregate_op_summary =
                &packet->static_data->aggregate_op_summary;

            if (aggregate_op_summary->last_inner_op_error != ErrorNone)
            {
                if (debug_aggregate_op)
                {
                    struct Ex10AggregateOpBuilder const* agg_op_builder =
                        get_ex10_aggregate_op_builder();
                    agg_op_builder->print_aggregate_op_errors(
                        aggregate_op_summary);
                }

                struct OpsStatusFields const ops_status = {
                    .op_id     = aggregate_op_summary->last_inner_op_run,
                    .busy      = false,
                    .Reserved0 = 0,
                    .error     = aggregate_op_summary->last_inner_op_error,
                    .rfu       = 0,
                };

                *ex10_result = make_ex10_ops_error(ops_status);
            }
        }
        break;

        case InventoryRoundSummary:
        {
            struct InventoryRoundSummary const* round_summary =
                &packet->static_data->inventory_round_summary;
            enum InventorySummaryReason const reason =
                (enum InventorySummaryReason)round_summary->reason;

            if (reason == InventorySummaryDone)
            {
                if (verbosity >= PRINT_SCOPED_EVENTS)
                {
                    ex10_ex_printf(
                        "[%10u us] ----- "
                        "Inventory round, RF mode: %3u, complete\n",
                        packet->us_counter,
                        rf_mode_stats[inventory_round_number].mode);
                }

                rf_mode_stats[inventory_round_number].duration_us =
                    packet->us_counter - inventory_start_time_us;
                inventory_start_time_us = packet->us_counter;
                inventory_round_number += 1u;
            }
            // Note: All other InventorySummaryReason values, which affect
            // state and error handling are performed within the
            // Ex10InventorySequenceUseCase object. No need to handle them here.
        }
        break;

        default:
            break;
    }
}

static struct Ex10Result autoset_inventory_single_target(
    struct InventoryOptions const* inventory_options,
    uint8_t                        target)
{
    // Clear the tag counters.
    ex10_memzero(rf_mode_stats, sizeof(rf_mode_stats));

    struct InventoryRoundConfigBasic inventory_configs[AUTOSET_RF_MODE_COUNT];

    struct AutosetRfModes const* autoset_rf_modes =
        get_ex10_autoset_modes()->get_autoset_rf_modes(
            inventory_options->mode.autoset_mode_id);

    if (autoset_rf_modes == NULL)
    {
        ex10_ex_eprintf("Invalid autoset mode id: %u\n",
                        inventory_options->mode.autoset_mode_id);
        return make_ex10_app_error(Ex10ApplicationErrorBadParamValue);
    }

    if (autoset_rf_modes->rf_modes_length != ARRAY_SIZE(inventory_configs))
    {
        // This should never happen. If it does, it is a programming error.
        return make_ex10_app_error(Ex10ApplicationErrorBadParamValue);
    }

    struct Ex10Result ex10_result =
        get_ex10_autoset_modes()->init_autoset_basic_inventory_sequence(
            inventory_configs,
            AUTOSET_RF_MODE_COUNT,
            autoset_rf_modes,
            inventory_options->antenna,
            inventory_options->tx_power_cdbm,
            target,
            inventory_options->session);

    if (ex10_result.error)
    {
        ex10_ex_eprintf(
            "Ex10AutosetModes.init_autoset_basic_inventory_sequence()"
            "failed:\n");
        print_ex10_result(ex10_result);
        return ex10_result;
    }

    for (size_t index = 0u; index < ARRAY_SIZE(inventory_configs); ++index)
    {
        inventory_configs[index].inventory_config.initial_q =
            inventory_options->initial_q;
    }

    for (size_t index = 0u; index < ARRAY_SIZE(inventory_configs); ++index)
    {
        rf_mode_stats[index].mode = (uint16_t)inventory_configs[index].rf_mode;
        rf_mode_stats[index].target =
            inventory_configs[index].inventory_config.target;
    }

    struct InventoryRoundSequence const inventory_sequence = {
        .type_id = INVENTORY_ROUND_CONFIG_BASIC,
        .configs = inventory_configs,
        .count   = ARRAY_SIZE(inventory_configs),
    };

    ex10_ex_printf("Starting autoset example single target %c:\n",
                   (target == target_A) ? 'A' : 'B');

    // Send the select command using the most sensitive RF mode in the Autoset
    // sequence. This will always be the last mode in the sequence.
    enum RfModes const select_rf_mode =
        inventory_configs[AUTOSET_RF_MODE_COUNT - 1].rf_mode;
    ex10_result = send_initial_select(inventory_options->antenna,
                                      select_rf_mode,
                                      inventory_options->frequency_khz,
                                      inventory_options->tx_power_cdbm);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    inventory_round_number  = 0u;
    inventory_start_time_us = get_ex10_ops()->get_device_time();

    ex10_result =
        get_ex10_inventory_sequence_use_case()->run_inventory_sequence(
            &inventory_sequence);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    print_tag_counter_header();
    struct RfModeInventoryStats const autoset_stats =
        print_rf_mode_stats(inventory_options->mode.autoset_mode_id,
                            &rf_mode_stats[0],
                            AUTOSET_RF_MODE_COUNT);

    if (autoset_stats.tag_count == 0)
    {
        ex10_result = make_ex10_app_error(Ex10ApplicationTagCount);
    }

    ex10_ex_printf("Ending autoset target %c\n",
                   (target == target_A) ? 'A' : 'B');
    return ex10_result;
}

static struct Ex10Result autoset_inventory_dual_target(
    struct InventoryOptions const* inventory_options)
{
    // Clear the tag counters.
    ex10_memzero(rf_mode_stats, sizeof(rf_mode_stats));

    // Allocate the enough inventory sequence configurations for dual target
    // A -> B -> A for each RF mode.
    static struct InventoryRoundConfigBasic
        inventory_configs[AUTOSET_RF_MODE_COUNT * 2u];

    struct AutosetRfModes const* autoset_rf_modes =
        get_ex10_autoset_modes()->get_autoset_rf_modes(
            inventory_options->mode.autoset_mode_id);

    if (autoset_rf_modes == NULL)
    {
        ex10_ex_eprintf("Invalid autoset mode id: %u\n",
                        inventory_options->mode.autoset_mode_id);
        return make_ex10_app_error(Ex10ApplicationErrorBadParamValue);
    }

    if (autoset_rf_modes->rf_modes_length * 2u != ARRAY_SIZE(inventory_configs))
    {
        return make_ex10_app_error(Ex10ApplicationErrorBadParamValue);
    }

    // Initialize Autoset for A targets:
    struct Ex10Result ex10_result =
        get_ex10_autoset_modes()->init_autoset_basic_inventory_sequence(
            &inventory_configs[0u],
            AUTOSET_RF_MODE_COUNT,
            autoset_rf_modes,
            inventory_options->antenna,
            inventory_options->tx_power_cdbm,
            target_A,
            inventory_options->session);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Initialize Autoset for B targets:
    ex10_result =
        get_ex10_autoset_modes()->init_autoset_basic_inventory_sequence(
            &inventory_configs[autoset_rf_modes->rf_modes_length],
            AUTOSET_RF_MODE_COUNT,
            autoset_rf_modes,
            inventory_options->antenna,
            inventory_options->tx_power_cdbm,
            target_B,
            inventory_options->session);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    for (size_t index = 0u; index < ARRAY_SIZE(inventory_configs); ++index)
    {
        inventory_configs[index].inventory_config.initial_q =
            inventory_options->initial_q;
    }

    for (size_t index = 0u; index < ARRAY_SIZE(inventory_configs); ++index)
    {
        rf_mode_stats[index].mode = (uint16_t)inventory_configs[index].rf_mode;
        rf_mode_stats[index].target =
            inventory_configs[index].inventory_config.target;
    }

    struct InventoryRoundSequence const inventory_sequence = {
        .type_id = INVENTORY_ROUND_CONFIG_BASIC,
        .configs = inventory_configs,
        .count   = autoset_rf_modes->rf_modes_length * 2u,
    };

    ex10_ex_printf("Starting autoset example dual target:\n");

    // Send the select command using the most sensitive RF mode in the Autoset
    // sequence. This will always be the last mode in the sequence.
    enum RfModes const select_rf_mode =
        inventory_configs[AUTOSET_RF_MODE_COUNT - 1].rf_mode;
    ex10_result = send_initial_select(inventory_options->antenna,
                                      select_rf_mode,
                                      inventory_options->frequency_khz,
                                      inventory_options->tx_power_cdbm);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    inventory_round_number  = 0u;
    inventory_start_time_us = get_ex10_ops()->get_device_time();

    ex10_result =
        get_ex10_inventory_sequence_use_case()->run_inventory_sequence(
            &inventory_sequence);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    print_tag_counter_header();
    struct RfModeInventoryStats const autoset_stats_A =
        print_rf_mode_stats(inventory_options->mode.autoset_mode_id,
                            &rf_mode_stats[0],
                            AUTOSET_RF_MODE_COUNT);
    struct RfModeInventoryStats const autoset_stats_B =
        print_rf_mode_stats(inventory_options->mode.autoset_mode_id,
                            &rf_mode_stats[AUTOSET_RF_MODE_COUNT],
                            AUTOSET_RF_MODE_COUNT);

    if (autoset_stats_A.tag_count != autoset_stats_B.tag_count)
    {
        // If the tag population is large and collided, then allow for
        // differences in inventory singulated tag counts.
        bool const enforce_error = (autoset_stats_A.tag_count <= 10);
        ex10_ex_eprintf("%s: tag_count: A: %zu != B: %zu\n",
                        enforce_error ? "error" : "warning",
                        autoset_stats_A.tag_count,
                        autoset_stats_B.tag_count);
        if (enforce_error)
        {
            ex10_result = make_ex10_app_error(Ex10ApplicationTagCount);
        }
    }

    if (autoset_stats_A.tag_count == 0 || autoset_stats_B.tag_count == 0)
    {
        ex10_result = make_ex10_app_error(Ex10ApplicationTagCount);
    }
    ex10_ex_printf("Ending autoset dual target, error: %u\n",
                   ex10_result.error);
    return ex10_result;
}

int main(int argc, char const* const argv[])
{
    // If autoset_mode_id == 0 then the Autoset mode is determined using
    // region and SKU, unless otherwise specified on the command line.
    struct InventoryOptions inventory_options = {
        .region_name   = "FCC",
        .read_rate     = 0u,
        .antenna       = 1u,
        .frequency_khz = 0u,
        .remain_on     = false,
        .tx_power_cdbm = 3000,
        .mode          = {.autoset_mode_id = 0u},  // region, sku
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

    ex10_result                              = ex10_set_default_gpio_setup();
    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();
    get_ex10_calibration()->init(ex10_protocol);

    enum ProductSku const sku = ex10_protocol->get_sku();

    // If no autoset mode was specified on the command line,
    // use the default Autoset mode based on the region and SKU.
    if (inventory_options.mode.autoset_mode_id == AutosetMode_Invalid)
    {
        inventory_options.mode.autoset_mode_id =
            get_ex10_autoset_modes()->get_autoset_mode_id(region_id, sku);
    }

    ex10_ex_printf("SKU         : 0x%04x\n", sku);

    get_ex10_inventory_sequence_use_case()->init();
    get_ex10_inventory_sequence_use_case()->register_packet_subscriber_callback(
        packet_subscriber_callback);
    get_ex10_inventory_sequence_use_case()->enable_packet_filter(
        ex10_command_line_verbosity() < PRINT_EVERYTHING);

    // The return value from main().
    int result = ex10_result.error ? -1 : 0;

    enum SelectTarget const select_session =
        (enum SelectTarget)inventory_options.session;
    ssize_t const select_command_index_A =
        get_ex10_select_commands()->set_select_session_command(target_A,
                                                               select_session);
    ssize_t const select_command_index_B =
        get_ex10_select_commands()->set_select_session_command(target_B,
                                                               select_session);
    if ((select_command_index_A < 0) || (select_command_index_B < 0))
    {
        result = -1;
    }

    if (inventory_options.frequency_khz != 0)
    {
        get_ex10_active_region()->set_single_frequency(
            inventory_options.frequency_khz);
    }

    if (inventory_options.remain_on)
    {
        get_ex10_active_region()->disable_regulatory_timers();
    }

    if (inventory_options.target_spec == 'A' ||
        inventory_options.target_spec == 'B')
    {
        uint8_t const target =
            (inventory_options.target_spec == 'A') ? target_A : target_B;

        ssize_t const enable_select_result =
            get_ex10_select_commands()->enable_select_command(
                (target == target_A) ? (size_t)select_command_index_A
                                     : (size_t)select_command_index_B);
        if (enable_select_result < 0)
        {
            result = -1;
        }

        if (result == 0)
        {
            struct Ex10Result const ex10_result_inventory =
                autoset_inventory_single_target(&inventory_options, target);
            if (ex10_result_inventory.error)
            {
                result = -1;
                print_ex10_app_result(ex10_result_inventory);
            }
        }
    }
    else if (inventory_options.target_spec == 'D')
    {
        // In this example dual target is always A -> B.
        ssize_t const enable_select_result =
            get_ex10_select_commands()->enable_select_command(
                (size_t)select_command_index_A);
        if (enable_select_result < 0)
        {
            result = -1;
        }

        if (result == 0)
        {
            struct Ex10Result const ex10_result_inventory =
                autoset_inventory_dual_target(&inventory_options);
            if (ex10_result_inventory.error)
            {
                result = -1;
                print_ex10_app_result(ex10_result_inventory);
            }
        }
    }
    else
    {
        // ex10_check_inventory_command_line_settings() has checked that the
        // targets specified are correct. This else should never happen.
    }

    ex10_core_board_teardown();
    return result;
}
