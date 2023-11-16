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
 * @file select_command_use_case_example.c
 * @details This example shows how to set a select command on the continuous
 *  inventory use case. After setting a desired select command, the command
 *  will be sent at the beginning of every inventory round during continuous
 *  inventory. In this example, the tag CRC is used as the select mask, but
 *  any bit in the memory bank can be used. Here is a brief description of
 *  what each task does:
 *  - Task 1. Obtain a valid tag CRC by running a dual target continuous
 *          inventory without a select command. This CRC value will be used as
 *          the select mask for Task 2.
 *  - Task 2. Run a series of dual target continuous inventory using different
 *          select command arguments.
 *          * Run 1 - Action000 with a select type of SelectedAsserted
 *          * Run 2 - Action000 with a select type of SelectNotAsserted
 *          * Run 3 - Action100 with a select type of SelectedAsserted
 *          * Run 4 - Action100 with a select type of SelectNotAsserted
 *
 *  All continuous inventory rounds in this example are setup to run in a dual
 *  target with a `max_number_of_rounds = 2` so that the tag's inventory flag
 *  can return to A state after every run.
 *
 *  For better performance, this example is currently not configured to print
 *  all tag information. This can be changed by using the 'verbose' inventory
 *  configuration parameter.
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
#include "ex10_api/gen2_commands.h"
#include "ex10_api/gen2_tx_command_manager.h"
#include "ex10_regulatory/ex10_default_region_names.h"
#include "utils/ex10_select_commands.h"

#include "ex10_use_cases/ex10_continuous_inventory_use_case.h"

#include "utils/ex10_inventory_command_line.h"
#include "utils/ex10_use_case_example_errors.h"

static const struct StopConditions stop_conditions = {
    .max_number_of_tags   = 0u,
    .max_duration_us      = 0u,
    .max_number_of_rounds = 2u,
};

static struct ContinuousInventorySummary continuous_inventory_summary = {
    .duration_us                = 0,
    .number_of_inventory_rounds = 0,
    .number_of_tags             = 0,
    .reason                     = SRNone,
    .last_op_id                 = 0,
    .last_op_error              = 0,
    .packet_rfu_1               = 0};

// This global variable stores the most recently participated tag's crc value
// after a continuous inventory round. In Task 1, this variable is being used to
// store the tag crc value, which is later used as the select mask in Task 2.
static uint16_t last_observed_tag_crc = 0u;

struct CrcSelectCommandStatus
{
    uint16_t        crc_of_interest;
    enum SelectType select_type;
    bool            matching_tag_SL;
    bool            not_matching_tag_SL;
};

// This select command status struct is used in the tag callback function to
// check whether the tag is allowed to participate in the inventory round.
// It should be updated before every continuous inventory round.
static struct CrcSelectCommandStatus select_status = {
    .crc_of_interest     = 0u,
    .select_type         = SelectAll,
    .matching_tag_SL     = false,
    .not_matching_tag_SL = false,
};

/**
 * This function is inserted as the callback in the ex10 continuous inventory
 * use case. If a uninvited tag participates, it returns an ex10 app error.
 *
 * @param packet           Either tag read packet or continuous inventory
 * summary packet
 * @param [out] result_ptr Ex10Result of the function
 */
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
        if (packet->packet_type == ContinuousInventorySummary ||
            packet->packet_type == TagRead)
        {
            get_ex10_event_fifo_printer()->print_packets(packet);
        }
    }
    // Only expects three types of packet:
    // TagRead, Gen2Transaction, and ContinuousInventorySummary packets
    if (packet->packet_type == TagRead)
    {
        struct TagReadFields tag_read =
            get_ex10_event_parser()->get_tag_read_fields(
                packet->dynamic_data,
                packet->dynamic_data_length,
                packet->static_data->tag_read.type,
                packet->static_data->tag_read.tid_offset);
        if (tag_read.stored_crc)
        {
            // store the most recent read tag crc value
            last_observed_tag_crc = ex10_bytes_to_uint16(tag_read.stored_crc);

            // sanity check: print message when wrong tag was read
            const bool is_matching_tag =
                ex10_bytes_to_uint16(tag_read.stored_crc) ==
                select_status.crc_of_interest;
            const bool tag_SL = is_matching_tag
                                    ? select_status.matching_tag_SL
                                    : select_status.not_matching_tag_SL;
            const bool tag_can_participate =
                (select_status.select_type == SelectAll) ||
                (select_status.select_type == SelectAll2) ||
                (select_status.select_type == SelectNotAsserted &&
                 tag_SL == false) ||
                (select_status.select_type == SelectAsserted && tag_SL == true);

            if (tag_can_participate == false)
            {
                ex10_ex_eprintf(
                    "Stopping continuous inventory round due to an "
                    "unselected Tag response\n");
                *result_ptr = make_ex10_app_error(Ex10ApplicationUnwelcomeTag);
                return;
            }
        }
    }
    else if (packet->packet_type == ContinuousInventorySummary)
    {
        continuous_inventory_summary =
            packet->static_data->continuous_inventory_summary;
    }
    else if (packet->packet_type == Gen2Transaction)
    {
        // Ignore the Gen2Transaction packets that are generated by the select
    }
    else
    {
        if (verbose < PRINT_EVERYTHING)
        {
            ex10_ex_eprintf(
                "Stopping continuous inventory round due to an "
                "unexpected packet, type: %u\n",
                packet->packet_type);
            *result_ptr = make_ex10_app_error(Ex10ApplicationInvalidPacketType);
            return;
        }
    }
}

/**
 * Runs continuous inventory
 *
 * @return struct Ex10Result Ex10 result of the function
 */
static struct Ex10Result run_continuous_inventory(
    enum SelectType                select_type,
    bool                           send_selects,
    struct InventoryOptions const* inventory_options)
{
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
        .select          = (uint8_t)select_type,
        .send_selects    = send_selects,
        .stop_conditions = &stop_conditions,
        .dual_target     = dual_target};

    struct Ex10ContinuousInventoryUseCase const* ciuc =
        get_ex10_continuous_inventory_use_case();

    struct Ex10Result ex10_result = ciuc->continuous_inventory(&params);

    return ex10_result;
}

/**
 * Set a select action command with a select mask of tag_crc and the given
 * action
 *
 * @param tag_crc Tag crc of interest
 * @param action Action parameter
 * @return int 0 if the CRC select action command is successfully set. Return
 * -1, otherwise.
 */
static int set_crc_select_action_command(const uint16_t          tag_crc,
                                         const enum SelectAction action)
{
    const enum SelectMemoryBank memory_bank = SelectEPC;
    const uint32_t              bit_pointer = 0u;
    const bool                  truncate    = false;
    // get select mask with the given tag_crc value
    uint8_t        select_mask_buffer[2u] = {0u};
    struct BitSpan select_mask            = {select_mask_buffer, 16u};
    select_mask.data[0u]                  = (uint8_t)(tag_crc);
    select_mask.data[1u]                  = (uint8_t)(tag_crc >> 8u);

    const ssize_t select_command_idx =
        get_ex10_select_commands()->set_select_command(SelectedFlag,
                                                       action,
                                                       memory_bank,
                                                       bit_pointer,
                                                       select_mask,
                                                       truncate);

    if (select_command_idx < 0)
    {
        return -1;
    }
    const ssize_t enable_select_result =
        get_ex10_select_commands()->enable_select_command(
            (size_t)select_command_idx);

    if (enable_select_result < 0)
    {
        return -1;
    }

    return 0;
}

/**
 * Updates the select status base on the given select action type.
 *
 * @param crc_of_interest Tag crc which will be the select mask
 * @param action Action parameter
 */
static void update_select_status(uint16_t          crc_of_interest,
                                 enum SelectType   select_type,
                                 enum SelectAction action)
{
    select_status.crc_of_interest = crc_of_interest;
    select_status.select_type     = select_type;

    if (action == Action000)
    {
        select_status.matching_tag_SL     = 1u;
        select_status.not_matching_tag_SL = 0u;
    }
    else if (action == Action100)
    {
        select_status.matching_tag_SL     = 0u;
        select_status.not_matching_tag_SL = 1u;
    }
}

static struct Ex10Result select_command_example(
    struct InventoryOptions const* inventory_options)
{
    struct Ex10ContinuousInventoryUseCase const* ciuc =
        get_ex10_continuous_inventory_use_case();

    ciuc->init();
    // Clear out any left over packets
    ex10_discard_packets(false, true, false);
    ciuc->register_packet_subscriber_callback(packet_subscriber_callback);
    ciuc->enable_packet_filter(ex10_command_line_verbosity() <
                               PRINT_EVERYTHING);

    if (inventory_options->frequency_khz != 0)
    {
        get_ex10_active_region()->set_single_frequency(
            inventory_options->frequency_khz);
    }

    if (inventory_options->remain_on)
    {
        get_ex10_active_region()->disable_regulatory_timers();
    }

    // Run normal continuous inventory (send_select=false, sel=SelectAll)
    // This is to obtain a valid tag crc value that will be used as the select
    // mask
    int run_idx = 0u;
    ex10_ex_printf(
        "Run%d: Running continuous inventory to get the last tag crc value\n",
        run_idx++);

    struct Ex10Result ex10_result =
        run_continuous_inventory(SelectAll, false, inventory_options);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Store the last observed tag crc
    const uint16_t crc_of_interest = last_observed_tag_crc;
    ex10_ex_printf("The chosen tag CRC is 0x%04X.\n", crc_of_interest);

    // Set and enable a select command with action paramter of 'Action000'
    if (set_crc_select_action_command(crc_of_interest, Action000))
    {
        return make_ex10_app_error(Ex10ApplicationBadSelect);
    }
    ex10_ex_printf(
        "\nThe select command is set to Action000 with a select mask of tag "
        "CRC 0x%04X.\n"
        "Action000 sets matching tag's SL=1 and not-matching tag's SL=0\n",
        crc_of_interest);

    // Run continuous inventory (with send_select=true, sel=SelectAsserted)
    // Tags with SL=1 will respond
    ex10_ex_printf(
        "Run%d: Running continuous inventory with sel=SL. Tags with matching "
        "CRC will respond.\n",
        run_idx++);
    update_select_status(crc_of_interest, SelectAsserted, Action000);
    ex10_result =
        run_continuous_inventory(SelectAsserted, true, inventory_options);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Run continuous inventory round (with send_select=true,
    // sel=SelectNotAsserted) Tags with SL=0 will respond
    ex10_ex_printf(
        "Run%d: Running continuous inventory with sel=~SL. Tags with "
        "not-matching CRC will respond.\n",
        run_idx++);
    update_select_status(crc_of_interest, SelectNotAsserted, Action000);
    ex10_result =
        run_continuous_inventory(SelectNotAsserted, true, inventory_options);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Set and enable a different select command with action parameter of
    // 'Action100'
    if (set_crc_select_action_command(crc_of_interest, Action100))
    {
        return make_ex10_app_error(Ex10ApplicationBadSelect);
    }
    ex10_ex_printf(
        "\nThe select command is set to Action100 with a select mask of tag "
        "CRC 0x%04X.\n"
        "Action100 sets matching tag's SL=0 and not-matching tag's SL=1\n",
        crc_of_interest);

    // Run continuous inventory round (with send_select=true,
    // sel=SelectAsserted) Tags with SL=1 will respond
    ex10_ex_printf(
        "Run%d: Running continuous inventory with sel=SL. Tags with "
        "not-matching CRC will respond.\n",
        run_idx++);
    update_select_status(crc_of_interest, SelectAsserted, Action100);
    ex10_result =
        run_continuous_inventory(SelectAsserted, true, inventory_options);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Run continuous inventory round (with send_select=true,
    // sel=SelectNotAsserted) Tags with SL=0 will respond
    ex10_ex_printf(
        "Run%d: Running continuous inventory with sel=~SL. Tags with matching "
        "CRC will respond.\n",
        run_idx++);
    update_select_status(crc_of_interest, SelectNotAsserted, Action100);
    ex10_result =
        run_continuous_inventory(SelectNotAsserted, true, inventory_options);
    return ex10_result;
}

int main(int argc, char const* const argv[])
{
    struct InventoryOptions inventory_options = {
        .region_name   = "FCC",
        .read_rate     = 0u,
        .antenna       = 1u,
        .frequency_khz = 0u,
        .remain_on     = false,
        .tx_power_cdbm = 3000,
        .mode          = {.rf_mode_id = mode_103},
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
        ex10_result = select_command_example(&inventory_options);
        if (ex10_result.error == true)
        {
            print_ex10_app_result(ex10_result);
        }
    }
    else
    {
        print_ex10_result(ex10_result);
    }

    ex10_core_board_teardown();
    return ex10_result.error ? -1 : 0;
}
