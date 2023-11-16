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

#include "board/board_spec.h"
#include "board/ex10_osal.h"

#include "ex10_api/application_registers.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_event_fifo_queue.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/fifo_buffer_list.h"
#include "ex10_api/gen2_tx_command_manager.h"

#include "ex10_modules/ex10_ramp_module_manager.h"

#include "ex10_use_cases/ex10_inventory_sequence_use_case.h"

/**
 * @struct InventorySequenceState
 * Progress through a requested sequence of inventory rounds.
 */
struct InventorySequenceState
{
    /// Points to the client supplied inventory sequences struct.
    /// - Set when Ex10InventorySequenceUseCase.run_inventory_sequence() is
    ///   called by the client.
    /// - Read by IRQ_N monitor thread context within fifo_data_handler()
    ///   call chain and the publish_packets() function.
    struct InventoryRoundSequence const* inventory_sequence;

    /// Iteration count through the inventory_sequence.configs array.
    /// - Initialized to zero when
    ///   Ex10InventorySequenceUseCase.run_inventory_sequence() is called.
    /// - Iterated up to, but not including, InventoryRoundSequence.count
    ///   during iteration through the sequence of inventory rounds
    ///   within the fifo_data_handler() execution context;
    ///   i.e. The IRQ_N monitor thread.
    size_t inventory_round_iter;

    /// The inventory round number associated with the packets being published.
    size_t inventory_round_packet_publisher;

    /// If true, publish all packets.
    /// If false, publish TagRead and InventoryRoundSummary packets.
    bool publish_all_packets;

    /// The callback to notify the subscriber of a new packet.
    /// Typically, this callback is set prior to calling
    /// Ex10InventorySequenceUseCase.run_inventory_sequence().
    void (*packet_subscriber_callback)(struct EventFifoPacket const*,
                                       struct Ex10Result*);
};

static struct InventorySequenceState inventory_state;

/**
 * Do the ugly work of bounds and type checking and casting to convert the
 * inventory_state.inventory_sequence void pointer into a validated
 * InventoryRoundConfigBasic pointer.
 *
 * @param iteration The inventory round iteration value.
 *
 * @return struct InventoryRoundConfigBasic const* A validated
 *         InventoryRoundConfigBasic pointer; NULL if invalid or if the
 *         iteration through the sequence count is complete.
 */
static struct InventoryRoundConfigBasic const* get_basic_inventory_round_config(
    size_t iteration)
{
    if (inventory_state.inventory_sequence->type_id !=
        INVENTORY_ROUND_CONFIG_BASIC)
    {
        return NULL;
    }

    if (iteration >= inventory_state.inventory_sequence->count)
    {
        return NULL;
    }

    struct InventoryRoundConfigBasic const* inventory_basic_configs =
        (struct InventoryRoundConfigBasic const*)
            inventory_state.inventory_sequence->configs;

    return &inventory_basic_configs[iteration];
}

static struct Ex10Result continue_inventory_sequence(
    struct InventoryRoundSummary const* round_summary)
{
    struct InventoryRoundConfigBasic const* inventory_round =
        get_basic_inventory_round_config(inventory_state.inventory_round_iter);

    enum InventorySummaryReason const summary_reason =
        (enum InventorySummaryReason)round_summary->reason;

    if (summary_reason == InventorySummaryRegulatory ||
        summary_reason == InventorySummaryTxNotRampedUp)
    {
        struct InventoryRoundControlFields inventory_config =
            inventory_round->inventory_config;

        struct InventoryRoundControl_2Fields inventory_config_2 =
            inventory_round->inventory_config_2;

        // Preserve Q across regulatory Inventory Ops.
        inventory_config.initial_q              = round_summary->final_q;
        inventory_config_2.starting_min_q_count = round_summary->min_q_count;
        inventory_config_2.starting_max_queries_since_valid_epc_count =
            round_summary->queries_since_valid_epc_count;

        return get_ex10_inventory()->start_inventory(
            inventory_round->antenna,
            inventory_round->rf_mode,
            inventory_round->tx_power_cdbm,
            &inventory_config,
            &inventory_config_2,
            inventory_round->send_selects);
    }
    else if (summary_reason == InventorySummaryDone ||
             summary_reason == InventorySummaryHost)
    {
        inventory_state.inventory_round_iter += 1u;
        struct InventoryRoundConfigBasic const* inventory_round_next =
            get_basic_inventory_round_config(
                inventory_state.inventory_round_iter);

        if (inventory_round_next)
        {
            struct InventoryRoundControlFields inventory_config =
                inventory_round_next->inventory_config;

            struct InventoryRoundControl_2Fields inventory_config_2 =
                inventory_round_next->inventory_config_2;

            // Reset Q state variables since the inventory is complete.
            inventory_config_2.starting_min_q_count                       = 0u;
            inventory_config_2.starting_max_queries_since_valid_epc_count = 0u;

            // If the next inventory round Tx power differs from the completed
            // inventory round, Tx will be ramped down and back up to the new
            // level immediately before the next round starts.
            if (inventory_round->tx_power_cdbm !=
                inventory_round_next->tx_power_cdbm)
            {
                struct Ex10Result ex10_result =
                    get_ex10_rf_power()->stop_op_and_ramp_down();
                if (ex10_result.error == true)
                {
                    return ex10_result;
                }
            }

            return get_ex10_inventory()->start_inventory(
                inventory_round->antenna,
                inventory_round_next->rf_mode,
                inventory_round_next->tx_power_cdbm,
                &inventory_config,
                &inventory_config_2,
                inventory_round_next->send_selects);
        }
        else
        {
            // Inventory sequencing complete; do nothing.
            // The host application needs parse this packet and decide what
            // action to take now that all inventory sequences have completed.
            return make_ex10_success();
        }
    }
    else if (summary_reason == InventorySummaryEventFifoFull)
    {
        return make_ex10_sdk_error(Ex10ModuleUseCase, Ex10SdkEventFifoFull);
    }
    else if (summary_reason == InventorySummaryLmacOverload)
    {
        return make_ex10_sdk_error(Ex10ModuleUseCase, Ex10SdkLmacOverload);
    }
    else if (summary_reason == InventorySummaryInvalidParam)
    {
        return make_ex10_sdk_error(Ex10ModuleUseCase,
                                   Ex10InventoryInvalidParam);
    }
    else
    {
        // The summary_reason is unknown. Treat it as an error.
    }

    // The summary_reason is unknown. Treat it as an error.
    return make_ex10_sdk_error(Ex10ModuleUseCase,
                               Ex10InventorySummaryReasonInvalid);
}

/**
 * In this use case, no interrupts are handled apart from processing EventFifo
 * packets.
 *
 * @param irq_status Unused
 * @return bool      Always return true to enable EventFifo packet call backs
 *                   on to the event_fifo_handler() function.
 */
static bool interrupt_handler(struct InterruptStatusFields irq_status)
{
    (void)irq_status;
    return true;
}

// Called by the interrupt handler thread when there is a fifo related
// interrupt.
static void fifo_data_handler(struct FifoBufferNode* fifo_buffer_node)
{
    struct Ex10EventParser const* event_parser = get_ex10_event_parser();
    struct ConstByteSpan          bytes        = fifo_buffer_node->fifo_data;
    while (bytes.length > 0u)
    {
        struct EventFifoPacket const packet =
            event_parser->parse_event_packet(&bytes);
        if (packet.packet_type == InventoryRoundSummary)
        {
            struct InventoryRoundSummary const* round_summary =
                &packet.static_data->inventory_round_summary;

            struct Ex10Result const ex10_result =
                continue_inventory_sequence(round_summary);

            if (ex10_result.error == true)
            {
                struct FifoBufferNode* result_buffer_node =
                    make_ex10_result_fifo_packet(ex10_result,
                                                 packet.us_counter);

                if (result_buffer_node)
                {
                    // The Ex10ResultPacket will be placed into the reader
                    // list with full details on the encountered error.
                    // Note that the microseconds counter from the
                    // InventorySummary packet will be provided in the
                    // Ex10Result packet.
                    // This is a hint to correlate the Ex10Result packet
                    // (created here) with the received InventorySummary
                    // packet that triggered the continue inventory
                    // operation and encountered this error.
                    get_ex10_event_fifo_queue()->list_node_push_back(
                        result_buffer_node);
                }
            }
        }
    }

    // The FifoBufferNode must be placed into the reader list after the
    // continuous inventory state is updated within the IRQ_N monitor thread
    // context.
    get_ex10_event_fifo_queue()->list_node_push_back(fifo_buffer_node);
}

static struct Ex10Result init(void)
{
    ex10_memzero(&inventory_state, sizeof(inventory_state));

    get_ex10_event_fifo_queue()->init();
    get_ex10_gen2_tx_command_manager()->init();

    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();
    struct Ex10Result          ex10_result =
        ex10_protocol->register_fifo_data_callback(fifo_data_handler);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    struct InterruptMaskFields const interrupt_mask = {
        .op_done                 = false,
        .halted                  = false,
        .event_fifo_above_thresh = true,
        .event_fifo_full         = true,
        .inventory_round_done    = true,
        .halted_sequence_done    = false,
        .command_error           = false,
        .aggregate_op_done       = false,
    };
    return ex10_protocol->register_interrupt_callback(interrupt_mask,
                                                      interrupt_handler);
}

static struct Ex10Result deinit(void)
{
    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();
    struct Ex10Result          ex10_result =
        ex10_protocol->unregister_interrupt_callback();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_protocol->unregister_fifo_data_callback();

    return make_ex10_success();
}

static void register_packet_subscriber_callback(
    void (*packet_subscriber_callback)(struct EventFifoPacket const*,
                                       struct Ex10Result*))
{
    inventory_state.packet_subscriber_callback = packet_subscriber_callback;
}

static void enable_packet_filter(bool enable_filter)
{
    inventory_state.publish_all_packets = (enable_filter == false);
}

static struct InventoryRoundSequence const* get_inventory_sequence(void)
{
    return inventory_state.inventory_sequence;
}

static struct InventoryRoundConfigBasic const* get_inventory_round(void)
{
    return get_basic_inventory_round_config(
        inventory_state.inventory_round_packet_publisher);
}

static struct Ex10Result publish_packets(void)
{
    bool              inventory_done = false;
    struct Ex10Result ex10_result    = make_ex10_success();

    struct Ex10EventFifoQueue const* event_fifo_queue =
        get_ex10_event_fifo_queue();
    struct EventFifoPacket const* packet = NULL;

    while (inventory_done == false && ex10_result.error == false)
    {
        uint32_t const packet_wait_timeout_us = 200u * 1000u;
        event_fifo_queue->packet_wait_with_timeout(packet_wait_timeout_us);
        packet = event_fifo_queue->packet_peek();
        if (packet != NULL)
        {
            if (packet->packet_type == Ex10ResultPacket)
            {
                ex10_result =
                    packet->static_data->ex10_result_packet.ex10_result;

                get_ex10_event_fifo_printer()->print_packets(packet);
            }

            if (inventory_state.packet_subscriber_callback != NULL)
            {
                if (inventory_state.publish_all_packets ||
                    packet->packet_type == TagRead ||
                    packet->packet_type == InventoryRoundSummary)
                {
                    inventory_state.packet_subscriber_callback(packet,
                                                               &ex10_result);
                    // The inventory may be stopped by the client application,
                    // without creating an error condition.
                    if ((ex10_result.customer == true) ||
                        (ex10_result.result_code.raw != 0u))
                    {
                        inventory_done = true;
                    }
                }
            }

            // Process InventoryRoundSummary packets after the callback since
            // inventory done status will be handled here.
            if (packet->packet_type == InventoryRoundSummary)
            {
                struct InventoryRoundSummary const* round_summary =
                    &packet->static_data->inventory_round_summary;
                enum InventorySummaryReason const reason =
                    (enum InventorySummaryReason)round_summary->reason;

                if (reason == InventorySummaryDone ||
                    reason == InventorySummaryHost)
                {
                    inventory_state.inventory_round_packet_publisher += 1u;
                    if (inventory_state.inventory_round_packet_publisher >=
                        inventory_state.inventory_sequence->count)
                    {
                        inventory_done = true;
                    }
                }
            }
        }
        event_fifo_queue->packet_remove();
    }

    return ex10_result;
}

static struct Ex10Result run_inventory_sequence(
    struct InventoryRoundSequence const* inventory_sequence)
{
    if (inventory_sequence == NULL || inventory_sequence->configs == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleUseCase, Ex10SdkErrorNullPointer);
    }

    if (inventory_sequence->count == 0u)
    {
        return make_ex10_sdk_error(Ex10ModuleUseCase,
                                   Ex10SdkErrorBadParamValue);
    }

    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();

    inventory_state.inventory_sequence               = inventory_sequence;
    inventory_state.inventory_round_iter             = 0u;
    inventory_state.inventory_round_packet_publisher = 0u;

    struct InventoryRoundConfigBasic const* inventory_round =
        get_basic_inventory_round_config(inventory_state.inventory_round_iter);

    if (inventory_round == NULL)
    {
        // If the initial round comes back as a NULL pointer, then
        // a parameter was set incorrectly;
        // i.e. InventoryRoundSequence.type != INVENTORY_ROUND_CONFIG_BASIC.
        return make_ex10_sdk_error(Ex10ModuleUseCase,
                                   Ex10SdkErrorBadParamValue);
    }

    if (ex10_protocol->is_op_currently_running() == true)
    {
        return make_ex10_sdk_error(Ex10ModuleUseCase, Ex10SdkErrorOpRunning);
    }

    struct Ex10Result ex10_result = get_ex10_inventory()->start_inventory(
        inventory_round->antenna,
        inventory_round->rf_mode,
        inventory_round->tx_power_cdbm,
        &inventory_round->inventory_config,
        &inventory_round->inventory_config_2,
        inventory_round->send_selects);

    if (ex10_result.error == false)
    {
        /*
         * Note: even if the client did not register a packet subscriber,
         * continue with the inventory sequence.
         * Inventory requested with no packet subscriber is a valid use case.
         * The publish packets needs to be called in order to maintain state;
         * even when there is no subscriber.
         */
        ex10_result = publish_packets();
    }

    return ex10_result;
}

static struct Ex10InventorySequenceUseCase ex10_inventory_sequence_use_case = {
    .init                                = init,
    .deinit                              = deinit,
    .register_packet_subscriber_callback = register_packet_subscriber_callback,
    .enable_packet_filter                = enable_packet_filter,
    .get_inventory_sequence              = get_inventory_sequence,
    .get_inventory_round                 = get_inventory_round,
    .run_inventory_sequence              = run_inventory_sequence,
};

struct Ex10InventorySequenceUseCase const* get_ex10_inventory_sequence_use_case(
    void)
{
    return &ex10_inventory_sequence_use_case;
}
