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

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "board/ex10_osal.h"
#include "board/time_helpers.h"
#include "ex10_api/aggregate_op_builder.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_device_time.h"
#include "ex10_api/ex10_gen2_reply_string.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/gen2_tx_command_manager.h"
#include "ex10_api/print_data.h"

static bool check_gen2_error(struct Gen2Reply const* reply)
{
    return get_ex10_gen2_commands()->check_error(*reply);
}

static void print_aggregate_op_errors(
    const struct AggregateOpSummary agg_summary)
{
    get_ex10_aggregate_op_builder()->print_aggregate_op_errors(&agg_summary);
}

static bool inventory_halted(void)
{
    return get_ex10_inventory()->inventory_halted();
}

static uint16_t read_rssi_value_from_op(uint8_t rssi_count)
{
    struct Ex10Ops const* ops = get_ex10_ops();

    // We first start the rssi op
    struct Ex10Result ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return 0;
    }

    ex10_result = ops->measure_rssi(rssi_count);
    if (ex10_result.error)
    {
        return 0;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return 0;
    }

    uint16_t rssi_array[MEASURED_RSSI_LOG2_REG_ENTRIES];
    ex10_result =
        get_ex10_protocol()->read(&measured_rssi_log2_reg, rssi_array);
    if (ex10_result.error)
    {
        return 0;
    }

    // Only the first one is needed unless doing LBT
    return rssi_array[0];
}

static bool deep_copy_packet(struct EventFifoPacket*       dst,
                             struct EventFifoPacket const* src)
{
    return (ex10_deep_copy_packet(dst, src).error == false);
}

static void print_command_result_fields(struct CommandResultFields const* error)
{
    ex10_eprint_command_result_fields(error);
}

static bool copy_tag_read_data(struct TagReadData*         dst,
                               struct TagReadFields const* src)
{
    return (ex10_copy_tag_read_data(dst, src).error == false);
}

static void clear_info_from_packets(struct InfoFromPackets* return_info)
{
    return_info->gen2_transactions     = 0;
    return_info->total_singulations    = 0;
    return_info->total_tid_count       = 0;
    return_info->times_halted          = 0;
    return_info->access_tag.epc_length = 0;
    return_info->access_tag.tid_length = 0;
}

/* Print information based on the EventFifo contents. */
static void examine_packets(struct EventFifoPacket const* packet,
                            struct InfoFromPackets*       return_info)
{
    if (packet == NULL)
    {
        return;
    }

    if (packet->packet_type == TagRead)
    {
        struct TagReadFields const tag_read =
            get_ex10_event_parser()->get_tag_read_fields(
                packet->dynamic_data,
                packet->dynamic_data_length,
                packet->static_data->tag_read.type,
                packet->static_data->tag_read.tid_offset);

        copy_tag_read_data(&(return_info->access_tag), &(tag_read));
        return_info->total_singulations += 1;
        return_info->times_halted +=
            packet->static_data->tag_read.halted_on_tag;

        if ((packet->static_data->tag_read.type == TagReadTypeEpcWithTid) ||
            (packet->static_data->tag_read.type == TagReadTypeEpcWithFastIdTid))
        {
            return_info->total_tid_count += 1;
        }
    }
    else if (packet->packet_type == Gen2Transaction)
    {
        return_info->gen2_transactions += 1;
    }
}

static enum InventoryHelperReturns simple_inventory(
    struct InventoryHelperParams* ihp)
{
    struct Ex10Reader const* reader          = get_ex10_reader();
    bool                     round_done      = true;
    struct Ex10TimeHelpers*  host_time       = get_ex10_time_helpers();
    struct Ex10DeviceTime*   ex10_time       = get_ex10_device_time();
    uint32_t                 host_start_time = host_time->time_now();
    uint32_t                 ex10_start_time = ex10_time->time_now();

    // Clear the number of tags found so that if we halt, we can return
    clear_info_from_packets(ihp->packet_info);
    ex10_discard_packets(ihp->verbose, true, false);

    while (host_time->time_elapsed(host_start_time) <
           ihp->inventory_duration_ms)
    {
        if (ihp->packet_info->times_halted > 0)
        {
            break;
        }
        if (round_done)
        {
            round_done = false;
            struct Ex10Result const ex10_result =
                reader->inventory(ihp->antenna,
                                  ihp->rf_mode,
                                  ihp->tx_power_cdbm,
                                  ihp->inventory_config,
                                  ihp->inventory_config_2,
                                  ihp->send_selects,
                                  ihp->remain_on);

            if (ex10_result.error)
            {
                // If we hit a regulatory boundary, keep going
                if (ex10_result.result_code.device == Ex10DeviceErrorOps &&
                    ex10_result.device_status.ops_status.error ==
                        ErrorInvalidTxState)
                {
                    round_done = true;
                }
                else
                {
                    print_ex10_result(ex10_result);
                    ex10_discard_packets(true, true, true);
                    return InvHelperOpStatusError;
                }
            }
            if (ihp->dual_target)
            {
                ihp->inventory_config->target = !ihp->inventory_config->target;
            }
        }

        struct EventFifoPacket const* packet = reader->packet_peek();

        while (packet)
        {
            examine_packets(packet, ihp->packet_info);
            if (host_time->time_elapsed(host_start_time) >
                ihp->inventory_duration_ms)
            {
                break;
            }
            if (ihp->verbose)
            {
                get_ex10_event_fifo_printer()->print_packets(packet);
            }
            if (packet->packet_type == InvalidPacket)
            {
                ex10_eprintf("Invalid packet occurred with no known cause\n");
                ex10_discard_packets(true, true, true);
                return InvHelperOpStatusError;
            }
            else if (packet->packet_type == InventoryRoundSummary)
            {
                round_done = true;
            }
            else if (packet->packet_type == TxRampDown)
            {
                // Note that session 0 is used and thus on a transmit power
                // down the tag state is reverted to A. If one chose to use
                // a session with persistence between power cycles, this
                // could go away.
                ihp->inventory_config->target = 0;
                round_done                    = true;
            }

            reader->packet_remove();
            packet = reader->packet_peek();
        }
    }

    // If we are told to halt on tags we return to the user after halting, and
    // thus don't clean up
    if (false == ihp->inventory_config->halt_on_all_tags)
    {
        // Regulatory timers will automatically ramp us down, but we are being
        // explicit here.
        reader->stop_transmitting();

        while (false == round_done)
        {
            struct EventFifoPacket const* packet = reader->packet_peek();
            while (packet != NULL)
            {
                examine_packets(packet, ihp->packet_info);
                if (ihp->verbose)
                {
                    get_ex10_event_fifo_printer()->print_packets(packet);
                }
                if (packet->packet_type == InvalidPacket)
                {
                    ex10_eprintf(
                        "Invalid packet occurred with no known cause\n");
                    ex10_discard_packets(true, true, true);
                    return InvHelperOpStatusError;
                }
                else if (packet->packet_type == InventoryRoundSummary)
                {
                    round_done = true;
                }
                reader->packet_remove();
                packet = reader->packet_peek();
            }
        }
    }

    if (ihp->verbose)
    {
        ex10_printf("Ex10 Device Time elapsed: %d ms\n",
                    ex10_time->time_elapsed(ex10_start_time));
        ex10_printf("Host Time elapsed: %d ms\n",
                    host_time->time_elapsed(host_start_time));
    }

    return InvHelperSuccess;
}

static enum InventoryHelperReturns continuous_inventory(
    struct ContInventoryHelperParams* cihp)
{
    struct Ex10Reader const*      reader         = get_ex10_reader();
    bool                          inventory_done = false;
    struct InventoryHelperParams* ihp            = cihp->inventory_params;

    if ((cihp->stop_conditions->max_number_of_rounds == 0) &&
        (cihp->stop_conditions->max_number_of_tags == 0) &&
        (cihp->stop_conditions->max_duration_us == 0))
    {
        return InvHelperStopConditions;
    }

    // Clear the number of tags found so that if we halt, we can return
    clear_info_from_packets(ihp->packet_info);
    ex10_discard_packets(false, true, false);

    // Recording the start time
    struct Ex10TimeHelpers* host_time       = get_ex10_time_helpers();
    struct Ex10DeviceTime*  ex10_time       = get_ex10_device_time();
    uint32_t                host_start_time = host_time->time_now();
    uint32_t                ex10_start_time = ex10_time->time_now();

    // Starting continuous inventory
    struct Ex10Result const ex10_result =
        reader->continuous_inventory(ihp->antenna,
                                     ihp->rf_mode,
                                     ihp->tx_power_cdbm,
                                     ihp->inventory_config,
                                     ihp->inventory_config_2,
                                     ihp->send_selects,
                                     cihp->stop_conditions,
                                     ihp->dual_target,
                                     false);
    if (ex10_result.error)
    {
        print_ex10_result(ex10_result);
        ex10_discard_packets(true, true, true);
        return InvHelperOpStatusError;
    }

    // Adding a failsafe timer to ensure that this function completes
    uint32_t const failsafe_timeout_ms =
        500u * (cihp->stop_conditions->max_duration_us / 1000u);

    struct EventFifoPacket const* packet = NULL;

    while (false == inventory_done)
    {
        // If we are halting on tags, there is no reason to continue waiting for
        // inventory summary packet
        if (ihp->packet_info->total_singulations &&
            ihp->inventory_config->halt_on_all_tags)
        {
            break;
        }

        if ((failsafe_timeout_ms) &&
            (host_time->time_elapsed(host_start_time) > failsafe_timeout_ms))
        {
            return InvHelperTimeout;
        }

        packet = reader->packet_peek();
        if (packet != NULL)
        {
            examine_packets(packet, ihp->packet_info);
            if (ihp->verbose)
            {
                get_ex10_event_fifo_printer()->print_packets(packet);
            }
            if (packet->packet_type == InvalidPacket)
            {
                ex10_eprintf("Invalid packet occurred with no known cause\n");
                ex10_discard_packets(true, true, true);
                return InvHelperOpStatusError;
            }
            else if (packet->packet_type == ContinuousInventorySummary)
            {
                inventory_done = true;

                if (cihp->summary_packet)
                {
                    *cihp->summary_packet =
                        packet->static_data->continuous_inventory_summary;
                }
            }
            reader->packet_remove();
        }
    }

    if (ihp->verbose)
    {
        ex10_printf("Ex10 Device Time elapsed: %d ms\n",
                    ex10_time->time_elapsed(ex10_start_time));
        ex10_printf("Host Time elapsed: %d ms\n",
                    host_time->time_elapsed(host_start_time));
    }

    return InvHelperSuccess;
}

static const char* get_remain_reason_string(enum RemainReason remain_reason)
{
    switch (remain_reason)
    {
        case RemainReasonNoReason:
            return "NoReason";
        case RemainReasonReadyNAsserted:
            return "ReadyNAsserted";
        case RemainReasonApplicationImageInvalid:
            return "ApplicationImageInvalid";
        case RemainReasonResetCommand:
            return "ResetCommand";
        case RemainReasonCrash:
            return "Crash";
        case RemainReasonWatchdog:
            return "Watchdog";
        case RemainReasonLockup:
            return "Lockup";
        default:
            return "UNKNOWN";
    }
}

static struct Ex10Result send_single_halted_command(
    struct Gen2CommandSpec* cmd_spec)
{
    struct Ex10Gen2TxCommandManager const* g2tcm =
        get_ex10_gen2_tx_command_manager();
    // Clear the local buffer
    g2tcm->clear_local_sequence();
    // add a single command and enable
    size_t            cmd_index = 0;
    struct Ex10Result ex10_result =
        g2tcm->encode_and_append_command(cmd_spec, 0, &cmd_index);
    if (false == ex10_result.error)
    {
        bool halted_enables[MaxTxCommandCount];
        ex10_memzero(halted_enables, sizeof(halted_enables));

        halted_enables[cmd_index] = true;

        ex10_result = g2tcm->write_sequence();
        if (ex10_result.error)
        {
            ex10_ex_eprintf("Gen2 write sequence failed.\n");
            return ex10_result;
        }

        g2tcm->write_halted_enables(
            halted_enables, MaxTxCommandCount, &cmd_index);
        // Send the command
        get_ex10_ops()->send_gen2_halted_sequence();
    }
    return ex10_result;
}

static void fill_u32(uint32_t* dest, uint32_t value, size_t count)
{
    for (size_t iter = 0; iter < count; ++iter)
    {
        dest[iter] = value;
    }
}

static const struct Ex10Helpers ex10_helpers = {
    .print_command_result_fields = print_command_result_fields,
    .check_gen2_error            = check_gen2_error,
    .discard_packets             = ex10_discard_packets,
    .print_aggregate_op_errors   = print_aggregate_op_errors,
    .inventory_halted            = inventory_halted,
    .deep_copy_packet            = deep_copy_packet,
    .clear_info_from_packets     = clear_info_from_packets,
    .examine_packets             = examine_packets,
    .simple_inventory            = simple_inventory,
    .continuous_inventory        = continuous_inventory,
    .copy_tag_read_data          = copy_tag_read_data,
    .get_remain_reason_string    = get_remain_reason_string,
    .swap_bytes                  = ex10_swap_bytes,
    .read_rssi_value_from_op     = read_rssi_value_from_op,
    .send_single_halted_command  = send_single_halted_command,
    .fill_u32                    = fill_u32,
};

const struct Ex10Helpers* get_ex10_helpers(void)
{
    return &ex10_helpers;
}
