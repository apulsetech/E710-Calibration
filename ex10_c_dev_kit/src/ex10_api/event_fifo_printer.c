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
#include <string.h>
#include <time.h>

#include "calibration.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_continuous_inventory_common.h"
#include "ex10_api/ex10_gen2_reply_string.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/print_data.h"

static void print_event_tx_ramp_up(struct EventFifoPacket const* packet)
{
    ex10_printf("[%10u us] Tx ramp up on channel %u kHz\n",
                packet->us_counter,
                packet->static_data->tx_ramp_up.carrier_frequency);
}

static void print_event_tx_ramp_down(struct EventFifoPacket const* packet)
{
    char const* reason = NULL;
    switch (packet->static_data->tx_ramp_down.reason)
    {
        case RampDownHost:
            reason = "user";
            break;
        case RampDownRegulatory:
            reason = "regulatory";
            break;
        default:
            reason = "unknown";
            break;
    }
    ex10_printf("[%10u us] Tx ramp down, reason (%u) %s)\n",
                packet->us_counter,
                packet->static_data->tx_ramp_down.reason,
                reason);
}

static void print_event_inventory_round_summary(
    struct EventFifoPacket const* packet)
{
    char const* reason = NULL;
    switch (packet->static_data->inventory_round_summary.reason)
    {
        case InventorySummaryDone:
            reason = "done";
            break;
        case InventorySummaryHost:
            reason = "host";
            break;
        case InventorySummaryRegulatory:
            reason = "regulatory";
            break;
        case InventorySummaryEventFifoFull:
            reason = "event_fifo_full";
            break;
        case InventorySummaryTxNotRampedUp:
            reason = "tx_not_ramped_up";
            break;
        case InventorySummaryInvalidParam:
            reason = "invalid_param";
            break;
        case InventorySummaryLmacOverload:
            reason = "lmac_overload";
            break;
        default:
            reason = "unknown";
            break;
    }

    ex10_printf(
        "[%10u us] Inventory round stopped - reason: (%u) %s, duration_us: %u, "
        "total_slots: %u, num_slots: %u, empty_slots: %u, "
        "single_slots: %u, collided_slots: %u\n",
        packet->us_counter,
        packet->static_data->inventory_round_summary.reason,
        reason,
        packet->static_data->inventory_round_summary.duration_us,
        packet->static_data->inventory_round_summary.total_slots,
        packet->static_data->inventory_round_summary.num_slots,
        packet->static_data->inventory_round_summary.empty_slots,
        packet->static_data->inventory_round_summary.single_slots,
        packet->static_data->inventory_round_summary.collided_slots);
}

static void print_event_q_changed(struct EventFifoPacket const* packet)
{
    struct QChanged const q_packet = packet->static_data->q_changed;
    ex10_printf(
        "[%10u us] Q changed - number of slots: %u, empty slots: %u, single "
        "slots: %u, collided slots: %u, q value: %u, query sent: %u\n",
        packet->us_counter,
        q_packet.num_slots,
        q_packet.empty_slots,
        q_packet.single_slots,
        q_packet.collided_slots,
        q_packet.q_value,
        q_packet.sent_query);
}

static void event_tag_read_common(struct EventFifoPacket const* packet)
{
    struct TagReadFields tag_read =
        get_ex10_event_parser()->get_tag_read_fields(
            packet->dynamic_data,
            packet->dynamic_data_length,
            packet->static_data->tag_read.type,
            packet->static_data->tag_read.tid_offset);

    ex10_printf("[%10u us] PC: 0x%04X",
                packet->us_counter,
                ex10_swap_bytes(*(tag_read.pc)));
    if (tag_read.xpc_w1)
    {
        uint16_t const xpc_w1 = ex10_bytes_to_uint16(tag_read.xpc_w1);
        ex10_printf(", XPC W1: 0x%04X", xpc_w1);
    }
    if (tag_read.xpc_w2)
    {
        uint16_t const xpc_w2 = ex10_bytes_to_uint16(tag_read.xpc_w2);
        ex10_printf(", XPC W2: 0x%04X", xpc_w2);
    }
    ex10_printf(", EPC: 0x ");
    ex10_print_data_line(tag_read.epc, tag_read.epc_length);
    if (tag_read.stored_crc)
    {
        uint16_t const stored_crc = ex10_bytes_to_uint16(tag_read.stored_crc);
        ex10_printf(", CRC: 0x%04X", stored_crc);
    }
    ex10_printf(", Halted Status: %d",
                packet->static_data->tag_read.halted_on_tag);

    if ((packet->static_data->tag_read.type == TagReadTypeEpcWithTid) ||
        (packet->static_data->tag_read.type == TagReadTypeEpcWithFastIdTid))
    {
        ex10_printf(", TID: ");
        for (size_t idx = 0; idx < tag_read.tid_length; idx++)
        {
            ex10_printf("%02X", tag_read.tid[idx]);
        }
    }
}

static void print_event_tag_read_compensated_rssi(
    struct EventFifoPacket const* packet,
    enum RfModes                  rf_mode,
    uint8_t                       antenna,
    enum RfFilter                 rf_filter,
    uint16_t                      adc_temperature)
{
    event_tag_read_common(packet);

    int16_t const compensated_rssi_cdbm =
        get_ex10_calibration()->get_compensated_rssi(
            packet->static_data->tag_read.rssi,
            (uint16_t)rf_mode,
            (const struct RxGainControlFields*)&packet->static_data->tag_read
                .rx_gain_settings,
            antenna,
            rf_filter,
            adc_temperature);

    ex10_printf(", RSSI (cdbm): %d", compensated_rssi_cdbm);
    ex10_printf("\n");
}

static void print_event_tag_read_raw_rssi(struct EventFifoPacket const* packet)
{
    event_tag_read_common(packet);
    ex10_printf(", Raw RSSI: %d", packet->static_data->tag_read.rssi);
    ex10_printf("\n");
}

static void print_event_gen2_transaction(struct EventFifoPacket const* packet)
{
    ex10_printf(
        "[%10u us] Gen2Transaction - transaction id: %u, status: (%u) %s, "
        "num_bits: %u, data: ",
        packet->us_counter,
        packet->static_data->gen2_transaction.transaction_id,
        packet->static_data->gen2_transaction.status,
        get_ex10_gen2_transaction_status_string(
            packet->static_data->gen2_transaction.status),
        packet->static_data->gen2_transaction.num_bits);

    size_t const reply_byte_length =
        (packet->static_data->gen2_transaction.num_bits + 7u) / 8u;

    ex10_print_data_line(packet->dynamic_data, reply_byte_length);
    ex10_printf("\n");
}

static void print_event_continuous_inventory_summary(
    struct EventFifoPacket const* packet)
{
    char const* reason = NULL;
    switch (packet->static_data->continuous_inventory_summary.reason)
    {
        case SRNone:
            reason = "none";
            break;
        case SRHost:
            reason = "host";
            break;
        case SRMaxNumberOfRounds:
            reason = "max rounds hit";
            break;
        case SRMaxNumberOfTags:
            reason = "max tags hit";
            break;
        case SRMaxDuration:
            reason = "max duration_us hit";
            break;
        case SROpError:
            reason = "op error";
            break;
        case SRSdkTimeoutError:
            reason = "timeout";
            break;
        case SRDeviceCommandError:
            reason = "device command error";
            break;
        default:
            reason = "unknown";
            break;
    }

    ex10_printf(
        "[%10u us] Continuous inventory stopped - reason: %u '%s', "
        "duration_us: %u, "
        "number_of_inventory_rounds: %u, number_of_tags: %u\n",
        packet->us_counter,
        packet->static_data->continuous_inventory_summary.reason,
        reason,
        packet->static_data->continuous_inventory_summary.duration_us,
        packet->static_data->continuous_inventory_summary
            .number_of_inventory_rounds,
        packet->static_data->continuous_inventory_summary.number_of_tags);
}

static void print_event_hello_world(struct EventFifoPacket const* packet)
{
    ex10_printf("[%10u us] Hello from E%3x, Reset reason: 0x%02x, cond: %u\n",
                packet->us_counter,
                packet->static_data->hello_world.sku,
                packet->static_data->hello_world.reset_reason,
                packet->static_data->hello_world.crash_info_conditional);
}

static void print_event_custom(struct EventFifoPacket const* packet)
{
    ex10_printf("[%10u us] Custom packet - length: %u, data: ",
                packet->us_counter,
                packet->static_data->custom.payload_len);
    if (packet->dynamic_data_length > 16u)
    {
        ex10_printf("\n");
        ex10_print_data(
            packet->dynamic_data, packet->dynamic_data_length, DataPrefixIndex);
    }
    else
    {
        ex10_print_data_line(packet->dynamic_data, packet->dynamic_data_length);
        ex10_printf("\n");
    }
}

static void print_event_power_control_loop_summary(
    struct EventFifoPacket const* packet)
{
    ex10_printf(
        "[%10u us] PowerControl - iterations_taken: %u, "
        "final_error: %d, final_tx_fine_gain: %d\n",
        packet->us_counter,
        packet->static_data->power_control_loop_summary.iterations_taken,
        packet->static_data->power_control_loop_summary.final_error,
        packet->static_data->power_control_loop_summary.final_tx_fine_gain);
}

static void print_aggregate_op_summary(struct EventFifoPacket const* packet)
{
    ex10_printf(
        "[%10u us] Aggregate op - op_run_count: %u, write_count: %u, "
        "insert_fifo_count: %u, final_buffer_byte_index: %u, "
        "total_jump_count: %u, last_inner_op: run: 0x%02x, error: 0x%02x, "
        "identifier: 0x%04x\n",
        packet->us_counter,
        packet->static_data->aggregate_op_summary.op_run_count,
        packet->static_data->aggregate_op_summary.write_count,
        packet->static_data->aggregate_op_summary.insert_fifo_count,
        packet->static_data->aggregate_op_summary.final_buffer_byte_index,
        packet->static_data->aggregate_op_summary.total_jump_count,
        packet->static_data->aggregate_op_summary.last_inner_op_run,
        packet->static_data->aggregate_op_summary.last_inner_op_error,
        packet->static_data->aggregate_op_summary.identifier);
}

static void print_halted(struct EventFifoPacket const* packet)
{
    ex10_printf("[%10u us] Lmac Halted with handle %u\n",
                packet->us_counter,
                packet->static_data->halted.halted_handle);
}

static void print_fifo_overflow_packet(struct EventFifoPacket const* packet)
{
    ex10_printf(
        "[%10u us] The packet causing the overflow was of type: %u\n",
        packet->us_counter,
        packet->static_data->fifo_overflow_packet.overflowing_packet_type);
}

static void print_ex10_result_packet(struct EventFifoPacket const* packet)
{
    ex10_printf(
        "[%10u us] Ex10Result packet received with the following data:\n",
        packet->us_counter);
    print_ex10_result(packet->static_data->ex10_result_packet.ex10_result);
}

static int32_t residue_magnitude(int32_t residue_i, int32_t residue_q)
{
    double const  root_2 = sqrt(2.0);
    int32_t const mag_2  = (residue_i * residue_i) + (residue_q * residue_q);
    return lround(sqrt(mag_2) / root_2);
}

static void print_event_sjc_measurement(struct EventFifoPacket const* packet)
{
    int32_t const residue_mag =
        residue_magnitude(packet->static_data->sjc_measurement.residue_i,
                          packet->static_data->sjc_measurement.residue_q);

    ex10_printf(
        "[%10u us] SJC c: (%5d, %5d), a: %u, f: %u, r: (%9d, %9d), %10d\n",
        packet->us_counter,
        packet->static_data->sjc_measurement.cdac_i,
        packet->static_data->sjc_measurement.cdac_q,
        packet->static_data->sjc_measurement.rx_atten,
        packet->static_data->sjc_measurement.flags,
        packet->static_data->sjc_measurement.residue_i,
        packet->static_data->sjc_measurement.residue_q,
        residue_mag);
}

static void print_event_write_profile_data(struct EventFifoPacket const* packet)
{
    ex10_printf("[%10u us] file_no: %d '",
                packet->us_counter,
                packet->static_data->write_profile_data.file_no);

    uint8_t const* const endp =
        packet->dynamic_data +
        packet->static_data->write_profile_data.payload_length;
    for (uint8_t const* iter = packet->dynamic_data; iter < endp; ++iter)
    {
        if (isprint(*iter))
        {
            ex10_printf("%c", *iter);
        }
        else
        {
            ex10_printf("\\x%02x", *iter);
        }
    }
    ex10_printf("'\n");
}

static void print_event_debug(struct EventFifoPacket const* packet)
{
    // Debug data has no one interpretation
    ex10_printf("[%10u us] Debug packet: length: %u, data: ",
                packet->us_counter,
                packet->static_data->debug.payload_len);

    if (packet->static_data->debug.payload_len > 32u)
    {
        ex10_printf("\n");
        ex10_print_data(
            packet->dynamic_data, packet->dynamic_data_length, DataPrefixIndex);
    }
    else
    {
        ex10_print_data_line(packet->dynamic_data, packet->dynamic_data_length);
        ex10_printf("\n");
    }
}

static void print_event_mystery(struct EventFifoPacket const* packet)
{
    // Unknown packets will have their static data set to zero
    // and the dynamic data will span all data following the header.
    ex10_printf("Mystery packet %u - length: %zu, data:\n",
                packet->packet_type,
                packet->dynamic_data_length);
    ex10_print_data(
        packet->dynamic_data, packet->dynamic_data_length, DataPrefixIndex);
}

static void print_invalid_packet(struct EventFifoPacket const* packet)
{
    (void)packet;
    ex10_printf("Invalid packet occurred with no known cause");
}

static void print_packets(struct EventFifoPacket const* packet)
{
    if (packet == NULL)
    {
        ex10_eprintf("Packet pointer is null\n");
        return;
    }
    switch (packet->packet_type)
    {
        case TxRampUp:
            print_event_tx_ramp_up(packet);
            break;
        case TxRampDown:
            print_event_tx_ramp_down(packet);
            break;
        case InventoryRoundSummary:
            print_event_inventory_round_summary(packet);
            break;
        case QChanged:
            print_event_q_changed(packet);
            break;
        case TagRead:
            print_event_tag_read_raw_rssi(packet);
            break;
        case Gen2Transaction:
            print_event_gen2_transaction(packet);
            break;
        case ContinuousInventorySummary:
            print_event_continuous_inventory_summary(packet);
            break;
        case HelloWorld:
            print_event_hello_world(packet);
            break;
        case Custom:
            print_event_custom(packet);
            break;
        case PowerControlLoopSummary:
            print_event_power_control_loop_summary(packet);
            break;
        case WriteProfileData:
            print_event_write_profile_data(packet);
            break;
        case SjcMeasurement:
            print_event_sjc_measurement(packet);
            break;
        case AggregateOpSummary:
            print_aggregate_op_summary(packet);
            break;
        case Halted:
            print_halted(packet);
            break;
        case FifoOverflowPacket:
            print_fifo_overflow_packet(packet);
            break;
        case Ex10ResultPacket:
            print_ex10_result_packet(packet);
            break;
        case Debug:
            print_event_debug(packet);
            break;
        case InvalidPacket:
            print_invalid_packet(packet);
            break;
        default:
            print_event_mystery(packet);
            break;
    }
}

static void print_tag_read_data(struct TagReadData const* tag_read_data)
{
    if (tag_read_data == NULL)
    {
        ex10_eprintf("tag_read_data pointer is null\n");
        return;
    }

    ex10_printf("PC: 0x%02x ", tag_read_data->pc);
    if (tag_read_data->xpc_w1_is_valid)
    {
        ex10_printf(" XPC W1: 0x%04x", tag_read_data->xpc_w1);
    }
    if (tag_read_data->xpc_w2_is_valid)
    {
        ex10_printf(" XPC W2: 0x%04x", tag_read_data->xpc_w2);
    }
    ex10_printf(" EPC: 0x");
    for (size_t index = 0u; index < tag_read_data->epc_length; ++index)
    {
        ex10_printf("%02x", tag_read_data->epc[index]);
    }

    if (tag_read_data->tid_length > 0u)
    {
        ex10_printf(" TID: 0x");
        for (size_t index = 0u; index < tag_read_data->tid_length; ++index)
        {
            ex10_printf("%02x'", tag_read_data->tid[index]);
        }
    }

    ex10_printf(", CRC: 0x");
    if (tag_read_data->stored_crc_is_valid)
    {
        ex10_printf("%04x", tag_read_data->stored_crc);
    }
    else
    {
        ex10_printf("none");
    }

    ex10_printf("\n");
}

static void print_tag_read_data_stdout(struct TagReadData const* tag_read_data)
{
    print_tag_read_data(tag_read_data);
}

static const struct Ex10EventFifoPrinter ex10_event_fifo_printer = {
    .print_packets = print_packets,
    .print_event_tag_read_compensated_rssi =
        print_event_tag_read_compensated_rssi,
    .print_tag_read_data = print_tag_read_data_stdout,
};

const struct Ex10EventFifoPrinter* get_ex10_event_fifo_printer(void)
{
    return &ex10_event_fifo_printer;
}
