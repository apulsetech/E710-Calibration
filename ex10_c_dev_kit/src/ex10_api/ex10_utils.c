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

#include <stdbool.h>

#include "board/board_spec.h"
#include "board/ex10_osal.h"

#include "ex10_api/aggregate_op_builder.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init_core.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/ex10_event_fifo_queue.h"
#include "ex10_api/ex10_gen2_reply_string.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/print_data.h"

uint16_t ex10_swap_bytes(const uint16_t value)
{
    return (uint16_t)(value << 8u) | (uint16_t)(value >> 8u);
}

struct Ex10Result ex10_set_default_gpio_setup(void)
{
    // Set the GPIO initial levels and enables to the value specified in the
    // board layer.
    struct GpioPinsSetClear const gpio_pins_set_clear =
        get_ex10_board_spec()->get_default_gpio_setup();
    struct Ex10Ops const* ops = get_ex10_ops();
    struct Ex10Result     ex10_result =
        ops->set_clear_gpio_pins(&gpio_pins_set_clear);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    return ops->wait_op_completion();
}

ssize_t ex10_discard_packets(bool print_packets,
                             bool flush_packets,
                             bool debug_aggregate_op)
{
    size_t                           packet_count = 0u;
    struct Ex10Protocol const*       protocol     = get_ex10_protocol();
    struct Ex10EventFifoQueue const* queue        = get_ex10_event_fifo_queue();
    struct Ex10Result                ex10_result  = make_ex10_success();

    // Flush all packets from the device event fifo buffer
    if (flush_packets)
    {
        ex10_result = protocol->insert_fifo_event(true, NULL);
        if (ex10_result.error)
        {
            return -1;
        }

        ex10_result = protocol->wait_for_event_fifo_empty();
    }

    if (ex10_result.error)
    {
        return -1;
    }

    struct EventFifoPacket const* packet = queue->packet_peek();
    while (packet)
    {
        if (print_packets)
        {
            get_ex10_event_fifo_printer()->print_packets(packet);
        }
        if (packet->packet_type == InvalidPacket)
        {
            ex10_eprintf("Invalid packet occurred with no known cause\n");
            return -1;
        }
        else if (packet->packet_type == AggregateOpSummary &&
                 debug_aggregate_op)
        {
            struct Ex10AggregateOpBuilder const* agg_op_builder =
                get_ex10_aggregate_op_builder();
            agg_op_builder->print_aggregate_op_errors(
                &packet->static_data->aggregate_op_summary);
        }
        packet_count += 1u;
        queue->packet_remove();
        packet = queue->packet_peek();
    }

    return (ssize_t)packet_count;
}

struct Ex10Result ex10_deep_copy_packet(struct EventFifoPacket*       dst,
                                        struct EventFifoPacket const* src)
{
    if (dst == NULL || dst->static_data == NULL || dst->dynamic_data == NULL ||
        src == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleUtils, Ex10SdkErrorNullPointer);
    }

    dst->packet_type = src->packet_type;
    dst->us_counter  = src->us_counter;
    dst->is_valid    = src->is_valid;

    // cast away the const for the destination
    union PacketData* dst_static_data =
        (union PacketData*)(uintptr_t)dst->static_data;

    int copy_result = ex10_memcpy(dst_static_data,
                                  sizeof(union PacketData),
                                  src->static_data,
                                  src->static_data_length);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleUtils, Ex10MemcpyFailed);
    }
    dst->static_data_length = src->static_data_length;

    const size_t num_bytes = dst->dynamic_data_length < src->dynamic_data_length
                                 ? dst->dynamic_data_length
                                 : src->dynamic_data_length;

    // cast away the const for the destination
    uint8_t* dst_dynamic_data = (uint8_t*)(uintptr_t)dst->dynamic_data;
    copy_result               = ex10_memcpy(dst_dynamic_data,
                              dst->dynamic_data_length,
                              src->dynamic_data,
                              num_bytes);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleUtils, Ex10MemcpyFailed);
    }
    dst->dynamic_data_length = num_bytes;

    return (num_bytes == src->dynamic_data_length)
               ? make_ex10_success()
               : make_ex10_sdk_error(Ex10ModuleUtils,
                                     Ex10SdkErrorBadParamLength);
}

struct Ex10Result ex10_copy_tag_read_data(struct TagReadData*         dst,
                                          struct TagReadFields const* src)
{
    if (dst == NULL || src == NULL || src->epc == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleUtils, Ex10SdkErrorNullPointer);
    }

    if ((src->tid == NULL && src->tid_length > 0u))
    {
        return make_ex10_sdk_error(Ex10ModuleUtils, Ex10SdkErrorNullPointer);
    }

    dst->pc = ex10_bytes_to_uint16(src->pc);

    if (src->xpc_w1)
    {
        dst->xpc_w1          = ex10_bytes_to_uint16(src->xpc_w1);
        dst->xpc_w1_is_valid = true;
    }
    else
    {
        dst->xpc_w1          = 0;
        dst->xpc_w1_is_valid = false;
    }

    if (src->xpc_w2)
    {
        dst->xpc_w2          = ex10_bytes_to_uint16(src->xpc_w2);
        dst->xpc_w2_is_valid = true;
    }
    else
    {
        dst->xpc_w2          = 0;
        dst->xpc_w2_is_valid = false;
    }

    ex10_memzero(dst->epc, sizeof(dst->epc));
    size_t const epc_length = (sizeof(dst->epc) < src->epc_length)
                                  ? sizeof(dst->epc)
                                  : src->epc_length;
    int copy_result =
        ex10_memcpy(dst->epc, EPC_BUFFER_BYTE_LENGTH, src->epc, epc_length);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleUtils, Ex10MemcpyFailed);
    }

    dst->epc_length = epc_length;

    if (src->stored_crc)
    {
        dst->stored_crc          = ex10_bytes_to_uint16(src->stored_crc);
        dst->stored_crc_is_valid = true;
    }
    else
    {
        dst->stored_crc          = 0u;
        dst->stored_crc_is_valid = false;
    }

    ex10_memzero(dst->tid, sizeof(dst->tid));
    dst->tid_length = 0u;
    if (src->tid)
    {
        size_t const tid_length = (sizeof(dst->tid) < src->tid_length)
                                      ? sizeof(dst->tid)
                                      : src->tid_length;
        copy_result =
            ex10_memcpy(dst->tid, TID_LENGTH_BYTES, src->tid, tid_length);
        if (copy_result != 0)
        {
            return make_ex10_sdk_error(Ex10ModuleUtils, Ex10MemcpyFailed);
        }

        dst->tid_length = tid_length;
    }

    struct Ex10Result ex10_result = make_ex10_success();

    if (dst->epc_length != src->epc_length)
    {
        ex10_result =
            make_ex10_sdk_error(Ex10ModuleUtils, Ex10SdkErrorBadParamLength);
        ex10_eprintf("EPC copy failed, length: %zu != %zu\n ",
                     dst->epc_length,
                     src->epc_length);
    }

    if (dst->tid_length != src->tid_length)
    {
        ex10_result =
            make_ex10_sdk_error(Ex10ModuleUtils, Ex10SdkErrorBadParamLength);
        ex10_eprintf("TID copy failed, length: %zu != %zu\n ",
                     dst->tid_length,
                     src->tid_length);
    }

    return ex10_result;
}

void ex10_eprint_command_result_fields(struct CommandResultFields const* error)
{
    ex10_eputs("failed_result_code        : 0x%02x\n",
               error->failed_result_code);
    ex10_eputs("failed_command_code       : 0x%02x\n",
               error->failed_command_code);
    ex10_eputs("commands_since_first_error: %u\n",
               error->commands_since_first_error);
}


uint32_t ex10_bytes_to_uint32(void const* void_ptr)
{
    uint8_t const* bytes = (uint8_t const*)void_ptr;
    uint32_t       value = 0u;
    value |= bytes[3u];
    value <<= 8u;
    value |= bytes[2u];
    value <<= 8u;
    value |= bytes[1u];
    value <<= 8u;
    value |= bytes[0u];
    return value;
}

uint16_t ex10_bytes_to_uint16(void const* void_ptr)
{
    uint8_t const* bytes = (uint8_t const*)void_ptr;
    uint16_t       value = 0u;
    value |= bytes[1u];
    value <<= 8u;
    value |= bytes[0u];
    return value;
}

uint16_t abs_int16(int16_t signed_value)
{
    return (signed_value > 0) ? (uint16_t)signed_value
                              : (uint16_t)(-signed_value);
}

uint32_t abs_int32(int32_t signed_value)
{
    return (signed_value > 0) ? (uint32_t)signed_value
                              : (uint32_t)(-signed_value);
}

void ex10_fill_u32(uint32_t* dest, uint32_t value, size_t count)
{
    for (size_t iter = 0; iter < count; ++iter)
    {
        dest[iter] = value;
    }
}

uint32_t ex10_calculate_read_rate(uint32_t tag_count, uint32_t duration_us)
{
    float read_rate_us = (float)tag_count / (float)duration_us;
    return (uint32_t)(read_rate_us * 1.0E6);
}
