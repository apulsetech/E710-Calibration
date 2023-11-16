/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2021 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#ifdef LTTNG_ENABLE
#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER pi_ex10sdk

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "ex10_api/trace.h"

#if !defined(_TRACE_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define _TRACE_H

#include "ex10_api/application_register_definitions.h"
#include "ex10_api/ex10_ops.h"
#include <lttng/tracepoint.h>

// clang-format off

//
// board/*/gpio_driver.c
//
TRACEPOINT_EVENT(pi_ex10sdk, GPIO_ready_n_low, TP_ARGS(), TP_FIELDS())
TRACEPOINT_LOGLEVEL(pi_ex10sdk, GPIO_ready_n_low, TRACE_DEBUG)

TRACEPOINT_EVENT(pi_ex10sdk, GPIO_irq_n_low, TP_ARGS(), TP_FIELDS())
TRACEPOINT_LOGLEVEL(pi_ex10sdk, GPIO_irq_n_low, TRACE_INFO)

TRACEPOINT_EVENT(pi_ex10sdk, GPIO_mutex_lock_request,
    TP_ARGS(int, tid),
    TP_FIELDS(ctf_integer(int, tid, tid))
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, GPIO_mutex_lock_request, TRACE_DEBUG)

TRACEPOINT_EVENT(pi_ex10sdk, GPIO_mutex_lock_acquired,
    TP_ARGS(int, tid),
    TP_FIELDS(ctf_integer(int, tid, tid))
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, GPIO_mutex_lock_acquired, TRACE_DEBUG)

TRACEPOINT_EVENT(pi_ex10sdk, GPIO_mutex_unlock,
    TP_ARGS(int, tid),
    TP_FIELDS(ctf_integer(int, tid, tid))
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, GPIO_mutex_unlock, TRACE_DEBUG)


//
// command_transactor.c
//
TRACEPOINT_EVENT(pi_ex10sdk, CMD_send,
    TP_ARGS(uint8_t const*, send_payload,
            uint16_t, payload_length
    ),
    TP_FIELDS(
        ctf_sequence_hex(uint8_t, send_payload, send_payload, uint16_t, payload_length)
    )
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, CMD_send, TRACE_DEBUG)

TRACEPOINT_EVENT(pi_ex10sdk, CMD_recv,
    TP_ARGS(uint8_t const*, recv_data,
        uint16_t, recv_length
    ),
    TP_FIELDS(
        ctf_sequence_hex(uint8_t, recv_data, recv_data, uint16_t, recv_length)
    )
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, CMD_recv, TRACE_DEBUG)


//
// ex10_protocol.c
//
TRACEPOINT_EVENT(pi_ex10sdk, PROTOCOL_write,
    TP_ARGS(int, tid,
            uint16_t, address_a,
            uint16_t, length_a,
            uint8_t const*, buffer_a
    ),
    TP_FIELDS(
        ctf_integer(int, tid, tid)
        ctf_integer_hex(uint16_t, address, address_a)
        ctf_integer(uint16_t, length, length_a)
        ctf_sequence_hex(uint8_t, buffer, buffer_a, uint16_t, length_a)
    )
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, PROTOCOL_write, TRACE_DEBUG)

TRACEPOINT_EVENT(pi_ex10sdk, PROTOCOL_read,
    TP_ARGS(int, tid,
            uint16_t, address_a,
            uint16_t, length_a,
            uint8_t const*, buffer_a
    ),
    TP_FIELDS(
        ctf_integer(int, tid, tid)
        ctf_integer_hex(uint16_t, address, address_a)
        ctf_integer(uint16_t, length, length_a)
        ctf_sequence_hex(uint8_t, buffer, buffer_a, uint16_t, length_a)
    )
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, PROTOCOL_read, TRACE_DEBUG)

TRACEPOINT_EVENT(pi_ex10sdk, PROTOCOL_read_fifo,
    TP_ARGS(uint16_t, num_bytes,
            uint8_t const*, fifo_bytes),
    TP_FIELDS(
        ctf_integer(uint16_t, num_bytes, num_bytes)
        ctf_sequence_hex(uint8_t, fifo_bytes, fifo_bytes, uint16_t, num_bytes)
    )
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, PROTOCOL_read_fifo, TRACE_DEBUG)

TRACEPOINT_EVENT(pi_ex10sdk, PROTOCOL_start_op,
    TP_ARGS(uint8_t, op_id),
    TP_FIELDS(ctf_integer_hex(uint8_t, op_id, op_id))
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, PROTOCOL_start_op, TRACE_DEBUG)

TRACEPOINT_EVENT(pi_ex10sdk, PROTOCOL_op_done,
    TP_ARGS(uint8_t, op_id,
            uint8_t, error
    ),
    TP_FIELDS(
        ctf_integer_hex(uint8_t, op_id, op_id)
        ctf_integer(uint8_t, error, error)
    )
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, PROTOCOL_op_done, TRACE_DEBUG)

TRACEPOINT_EVENT(pi_ex10sdk, PROTOCOL_interrupt,
    TP_ARGS(struct InterruptStatusFields, irq_status),
    TP_FIELDS(
        ctf_integer(bool, op_done, irq_status.op_done)
        ctf_integer(bool, halted, irq_status.halted)
        ctf_integer(bool, event_fifo_above_thresh, irq_status.event_fifo_above_thresh)
        ctf_integer(bool, event_fifo_full, irq_status.event_fifo_full)
        ctf_integer(bool, inventory_round_done, irq_status.inventory_round_done)
    )
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, PROTOCOL_interrupt, TRACE_INFO)


//
// ex10_ops.c
//
TRACEPOINT_EVENT (pi_ex10sdk, OPS_set_rf_mode,
    TP_ARGS(
        uint16_t, rf_mode,
        struct GpioPinsSetClear const*, gpio_pins_set_clear
    ),
    TP_FIELDS(
        ctf_integer_hex(uint16_t, rf_mode, rf_mode)
        ctf_integer_hex(uint32_t, gpio_output_enable_set, gpio_pins_set_clear->output_enable_set)
        ctf_integer_hex(uint32_t, gpio_output_enable_clear, gpio_pins_set_clear->output_enable_clear)
        ctf_integer_hex(uint32_t, gpio_output_level_set, gpio_pins_set_clear->output_level_set)
        ctf_integer_hex(uint32_t, gpio_output_level_clear, gpio_pins_set_clear->output_level_clear)
    )
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, OPS_set_rf_mode, TRACE_INFO)

TRACEPOINT_EVENT(pi_ex10sdk, OPS_cw_on,
    TP_ARGS(
        struct GpioPinsSetClear const*, gpio_pins_set_clear,
        struct PowerConfigs const*,               power_config,
        struct RfSynthesizerControlFields const*, synth_control,
        struct Ex10RegulatoryTimers const*,           timer_config
    ),
    TP_FIELDS(
        ctf_integer_hex(uint32_t, gpio_output_enable_set, gpio_pins_set_clear->output_enable_set)
        ctf_integer_hex(uint32_t, gpio_output_enable_clear, gpio_pins_set_clear->output_enable_clear)
        ctf_integer_hex(uint32_t, gpio_output_level_set, gpio_pins_set_clear->output_level_set)
        ctf_integer_hex(uint32_t, gpio_output_level_clear, gpio_pins_set_clear->output_level_clear)
        ctf_integer(uint8_t, tx_atten, power_config->tx_atten)
        ctf_integer(uint16_t, tx_scalar, power_config->tx_scalar)
        ctf_integer(uint32_t, dc_offset, power_config->dc_offset)
        ctf_integer(uint32_t, adc_target, power_config->adc_target)
        ctf_integer(uint16_t, loop_stop_threshold, power_config->loop_stop_threshold)
        ctf_integer(uint16_t, op_error_threshold, power_config->op_error_threshold)
        ctf_integer(size_t, loop_gain_divisor, power_config->loop_gain_divisor)
        ctf_integer(size_t, loop_max_iterations, power_config->max_iterations)
        ctf_integer(uint16_t, synth_n_divider, synth_control->n_divider)
        ctf_integer(uint8_t, synth_r_divider_index, synth_control->n_divider)
        ctf_integer(bool, synth_lf_type, synth_control->lf_type)
        ctf_integer(uint32_t, timer_nominal, timer_config->nominal_ms)
        ctf_integer(uint32_t, timer_extended, timer_config->extended_ms)
        ctf_integer(uint32_t, timer_regulatory, timer_config->regulatory_ms)
        ctf_integer(uint32_t, timer_off_same_channel, timer_config->off_same_channel_ms)
    )
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, OPS_cw_on, TRACE_INFO)

TRACEPOINT_EVENT(pi_ex10sdk, OPS_cw_off_manual, TP_ARGS(), TP_FIELDS())
TRACEPOINT_LOGLEVEL(pi_ex10sdk, OPS_cw_off_manual, TRACE_INFO)

TRACEPOINT_EVENT(pi_ex10sdk, OPS_cw_off, TP_ARGS(), TP_FIELDS())
TRACEPOINT_LOGLEVEL(pi_ex10sdk, OPS_cw_off, TRACE_INFO)

TRACEPOINT_EVENT(pi_ex10sdk, OPS_start_inventory_round,
    TP_ARGS(struct InventoryRoundControlFields const*, inv_ctrl_1,
            struct InventoryRoundControl_2Fields const*, inv_ctrl_2),
    TP_FIELDS(
        ctf_integer(uint8_t, initial_q, inv_ctrl_1->initial_q)
        ctf_integer(uint8_t, max_q, inv_ctrl_1->max_q)
        ctf_integer(uint8_t, min_q, inv_ctrl_1->min_q)
        ctf_integer(uint8_t, num_min_q_cycles, inv_ctrl_1->num_min_q_cycles)
        ctf_integer(bool, fixed_q_mode, inv_ctrl_1->fixed_q_mode)
        ctf_integer(bool, q_increase_use_query, inv_ctrl_1->q_increase_use_query)
        ctf_integer(bool, q_decrease_use_query, inv_ctrl_1->q_decrease_use_query)
        ctf_integer(uint8_t, select, inv_ctrl_1->select)
        ctf_integer(bool, target, inv_ctrl_1->target)
        ctf_integer(bool, halt_on_all_tags, inv_ctrl_1->halt_on_all_tags)
        ctf_integer(bool, fast_id_enable, inv_ctrl_1->fast_id_enable)
        ctf_integer(bool, tag_focus_enable, inv_ctrl_1->tag_focus_enable)
        ctf_integer(uint16_t, max_queries_since_valid_epc, inv_ctrl_2->max_queries_since_valid_epc)
    )
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, OPS_start_inventory_round, TRACE_INFO)

TRACEPOINT_EVENT(pi_ex10sdk, OPS_send_select, TP_ARGS(), TP_FIELDS())
TRACEPOINT_LOGLEVEL(pi_ex10sdk, OPS_send_select, TRACE_INFO)

// Nothing is logged for the OPS_inventory tracepoint because all this
// information is already captured by OPS_cw_on, OPS_start_inventory_round, and
// OPS_send_select tracepoints.
TRACEPOINT_EVENT(pi_ex10sdk, OPS_inventory, TP_ARGS(), TP_FIELDS())
TRACEPOINT_LOGLEVEL(pi_ex10sdk, OPS_inventory, TRACE_INFO)

TRACEPOINT_EVENT(pi_ex10sdk, OPS_interrupt_handler, TP_ARGS(), TP_FIELDS())
TRACEPOINT_LOGLEVEL(pi_ex10sdk, OPS_interrupt_handler, TRACE_DEBUG)


//
// Process
//
TRACEPOINT_EVENT(pi_ex10sdk, EXEC_start,
    TP_ARGS(char const*, test_name),
    TP_FIELDS(ctf_string(test_name, test_name))
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, EXEC_start, TRACE_NOTICE)

TRACEPOINT_EVENT(pi_ex10sdk, EXEC_end,
    TP_ARGS(char const*, test_name),
    TP_FIELDS(ctf_string(test_name, test_name))
)
TRACEPOINT_LOGLEVEL(pi_ex10sdk, EXEC_end, TRACE_NOTICE)

// clang-format on

#endif

#include <lttng/tracepoint-event.h>

#else  // LTTNG_ENABLE

#define tracepoint(...) ((void)0)

#endif  // LTTNG_ENABLE
