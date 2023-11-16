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

#include "board/board_spec.h"
#include "board/ex10_osal.h"

#include "ex10_api/application_registers.h"
#include "ex10_api/byte_span.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_event_fifo_queue.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/fifo_buffer_list.h"
#include "ex10_api/gen2_tx_command_manager.h"

#include "ex10_modules/ex10_ramp_module_manager.h"

#include "ex10_use_cases/ex10_continuous_inventory_use_case.h"

/// @enum InventoryState Keep track of the continuous inventory state.
enum InventoryState
{
    /// Continuous inventory is not in progress.
    InvIdle,
    /// Continuous inventory is in progress.
    InvOngoing,
    /// The host requested continuous inventory stop via stop_transmitting().
    InvStopRequested,
};

/**
 * @struct ContinuousInventoryState
 * State variables that are set in update_inventory_state().
 */
struct ContinuousInventoryState
{
    /// Initialized in to InvOngoing inventory_continuous()
    /// Set in update_inventory_state().
    enum InventoryState state;

    /// The reason for inventory round completion.
    enum InventorySummaryReason done_reason;

    /// Initial Q
    uint8_t initial_q;

    /// The Q from the end of the previous inventory round.
    uint8_t previous_q;

    /// The min_q_count from the previous inventory round.
    uint8_t min_q_count;

    /// The queries_since_valid_epc_count from the previous round.
    uint8_t queries_since_valid_epc_count;

    /// Set in check_stop_conditions;
    /// Used to push the ContinuousInventorySummary EventFifo packet
    enum StopReason stop_reason;

    /// Initialized to zero in inventory_continuous(), updated by packets
    /// in update_inventory_state().
    size_t round_count;

    /// Counts the number of tags singulated when performing a
    /// continuous inventory sequence.
    size_t tag_count;

    /// Holds the current target state of the reader when performing a
    /// continuous inventory sequence.
    uint8_t target;

    /// If true, publish all packets.
    /// If false, publish TagRead and InventoryRoundSummary packets.
    bool publish_all_packets;

    /// If true, command the LMAC to do auto access
    /// If false, do not
    bool enable_auto_access;

    /// If true the auto access commands will abort the sequence
    /// if an access command fails
    /// If false, all access commands enabled will be sent
    bool abort_on_fail;

    /// The callback to notify the subscriber of a new packet.
    void (*packet_subscriber_callback)(struct EventFifoPacket const*,
                                       struct Ex10Result*);
};

/**
 * Ex10ContinuousInventory private state variables.
 * These are initialized in the init() so that if
 * it is called multiple times, it will return to the
 * same starting condition.
 */
static struct InventoryParams          inventory_params;
static bool                            inventory_dual_target;
static struct ContinuousInventoryState inventory_state;
static struct StopConditions           stop_conditions;
static uint32_t                        start_time_us;

static bool check_stop_conditions(uint32_t timestamp_us)
{
    // If the reason is already set, we return so as to retain the original stop
    // reason
    if (inventory_state.stop_reason != SRNone)
    {
        return true;
    }

    if (stop_conditions.max_number_of_rounds > 0u)
    {
        if (inventory_state.round_count >= stop_conditions.max_number_of_rounds)
        {
            inventory_state.stop_reason = SRMaxNumberOfRounds;
            return true;
        }
    }
    if (stop_conditions.max_number_of_tags > 0u)
    {
        if (inventory_state.tag_count >= stop_conditions.max_number_of_tags)
        {
            inventory_state.stop_reason = SRMaxNumberOfTags;
            return true;
        }
    }
    if (stop_conditions.max_duration_us > 0u)
    {
        // Packet before start checks for packets which occurred before the
        // continuous inventory round was started.
        bool const     packet_before_start = (start_time_us > timestamp_us);
        uint32_t const elapsed_us =
            (packet_before_start)
                ? ((UINT32_MAX - start_time_us) + timestamp_us + 1)
                : (timestamp_us - start_time_us);
        if (elapsed_us >= stop_conditions.max_duration_us)
        {
            inventory_state.stop_reason = SRMaxDuration;
            return true;
        }
    }
    if (inventory_state.state == InvStopRequested)
    {
        inventory_state.stop_reason = SRHost;
        return true;
    }
    return false;
}

static struct Ex10Result push_continuous_inventory_summary_packet(
    struct EventFifoPacket const* event_packet,
    struct Ex10Result             ex10_result)
{
    uint32_t const duration_us = event_packet->us_counter - start_time_us;

    struct ContinuousInventorySummary summary = {
        .duration_us                = duration_us,
        .number_of_inventory_rounds = inventory_state.round_count,
        .number_of_tags             = inventory_state.tag_count,
        .reason                     = (uint8_t)inventory_state.stop_reason,
        .last_op_id                 = 0u,
        .last_op_error              = ErrorNone,
        .packet_rfu_1               = 0u,
    };

    if (ex10_result.error)
    {
        // Use the error mapping if there is an error. If there isn't, then stop
        // reason may be timer, rounds, or tags.
        enum StopReason result_reason =
            get_ex10_inventory()->ex10_result_to_continuous_inventory_error(
                ex10_result);
        summary.reason = (uint8_t)result_reason;

        if (ex10_result.module == Ex10ModuleDevice)
        {
            switch (ex10_result.result_code.device)
            {
                case Ex10DeviceErrorOps:
                    summary.last_op_id =
                        ex10_result.device_status.ops_status.op_id;
                    summary.last_op_error =
                        ex10_result.device_status.ops_status.error;
                    break;

                case Ex10DeviceErrorOpsTimeout:
                    summary.last_op_id =
                        ex10_result.device_status.ops_status.op_id;
                    summary.last_op_error =
                        ex10_result.device_status.ops_status.error;
                    break;

                default:
                    break;
            }
        }
    }

    struct EventFifoPacket const summary_packet = {
        .packet_type         = ContinuousInventorySummary,
        .us_counter          = event_packet->us_counter,
        .static_data         = (union PacketData const*)&summary,
        .static_data_length  = sizeof(struct ContinuousInventorySummary),
        .dynamic_data        = NULL,
        .dynamic_data_length = 0u,
        .is_valid            = true,
    };

    bool const trigger_irq = true;
    return get_ex10_protocol()->insert_fifo_event(trigger_irq, &summary_packet);
}

static void push_ex10_result_packet(struct Ex10Result ex10_result,
                                    uint32_t          us_counter)
{
    struct FifoBufferNode* result_buffer_node =
        make_ex10_result_fifo_packet(ex10_result, us_counter);

    if (result_buffer_node)
    {
        // The Ex10ResultPacket will be placed into the reader
        // list with full details on the encountered error.
        // Note that the specified microseconds counter will be
        // provided in the Ex10Result packet.
        // This counter is a hint to correlate the Ex10Result packet
        // created here with the previously received Event FIFO packet
        // that triggered an operation that later encountered this error.
        get_ex10_event_fifo_queue()->list_node_push_back(result_buffer_node);
    }
}

/**
 * Called in response to receiving the InventoryRoundSummary packet within the
 * fifo_data_handler(); i.e. IRQ_N monitor thread context.
 * When the stop conditions are not met, then "continue inventory".
 *
 * @return struct Ex10Result The return value from the call to the
 *         StartInventoryRoundOp (0xB0).
 */
static struct Ex10Result continue_continuous_inventory(void)
{
    /* Behavior for stop reasons:
    InventorySummaryDone          // Flip target (dual target), reset Q
    InventorySummaryHost          // Don't care
    InventorySummaryRegulatory    // Preserve Q
    InventorySummaryEventFifoFull // Stop Continuous inventory
    InventorySummaryTxNotRampedUp // Don't care
    InventorySummaryInvalidParam  // Stop Continuous inventory
    InventorySummaryLmacOverload  // Stop Continuous inventory
    */

    bool reset_q = false;
    if (inventory_dual_target)
    {
        // Flip target if round is done, not for regulatory or error.
        if (inventory_state.done_reason == InventorySummaryDone)
        {
            inventory_state.target ^= 1u;
            reset_q = true;
        }

        // If CW is not on and our session is zero (no persistence after power),
        // we need to switch the target to A.
        if ((inventory_params.inventory_config.session == 0) &&
            (get_ex10_rf_power()->get_cw_is_on() == false))
        {
            inventory_state.target = target_A;
            reset_q                = true;
        }
    }
    else if (inventory_state.done_reason == InventorySummaryDone)
    {
        reset_q = true;
    }

    inventory_params.inventory_config.target = inventory_state.target;

    struct InventoryRoundControlFields inventory_config =
        inventory_params.inventory_config;

    struct InventoryRoundControl_2Fields inventory_config_2 =
        inventory_params.inventory_config_2;

    // Preserve Q and internal LMAC counters across rounds or
    // reset for new target.
    if (reset_q)
    {
        // Reset Q for target flip (done above) or for normal end of round.
        inventory_config.initial_q = inventory_state.initial_q;

        inventory_config_2.starting_min_q_count                       = 0;
        inventory_config_2.starting_max_queries_since_valid_epc_count = 0;
    }
    else
    {
        if (inventory_state.done_reason == InventorySummaryRegulatory)
        {
            // Preserve Q across regulatory Inventory Ops.
            inventory_config.initial_q = inventory_state.previous_q;

            inventory_config_2.starting_min_q_count =
                inventory_state.min_q_count;
            inventory_config_2.starting_max_queries_since_valid_epc_count =
                inventory_state.queries_since_valid_epc_count;
        }
        // Else inventory stopped because the Q algorithm was done
        // so we use the inventory config values as they were
        // provided.
    }

    return get_ex10_inventory()->start_inventory(inventory_params.antenna,
                                                 inventory_params.rf_mode,
                                                 inventory_params.tx_power_cdbm,
                                                 &inventory_config,
                                                 &inventory_config_2,
                                                 inventory_params.send_selects);
}

/**
 * In this use case no interrupts are handled apart from processing EventFifo
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

static void handle_continuous_inventory_error(
    struct Ex10Result             ex10_result,
    struct EventFifoPacket const* packet)
{
    push_ex10_result_packet(ex10_result, packet->us_counter);

    inventory_state.state = InvIdle;
    ex10_result = push_continuous_inventory_summary_packet(packet, ex10_result);
    if (ex10_result.error)
    {
        // This is a secondary error that happened while
        // handling an error that occurred during an attempt to
        // run continuous inventory. As this is not the primary
        // error encountered, it will be not be passed up, just
        // printing a message for debug.
        ex10_eprintf("Unable to push continuous summary packet:\n");
        print_ex10_result(ex10_result);
    }
}

// Called by the interrupt handler thread when there is a fifo related
// interrupt.
static void fifo_data_handler(struct FifoBufferNode* fifo_buffer_node)
{
    struct ConstByteSpan bytes = fifo_buffer_node->fifo_data;
    while (bytes.length > 0u)
    {
        struct Ex10EventParser const* event_parser = get_ex10_event_parser();
        struct EventFifoPacket const  packet =
            event_parser->parse_event_packet(&bytes);
        if (event_parser->get_packet_type_valid(packet.packet_type) == false)
        {
            // Invalid packets cannot be processed and will only confuse the
            // continuous inventory state machine. Discontinue processing.
            ex10_eprintf(
                "Invalid packet encountered during continuous inventory "
                "packet parsing, packet end pos: %zu\n",
                bytes.length);
            break;
        }
        if (packet.packet_type == TagRead)
        {
            inventory_state.tag_count += 1;
        }
        else if (packet.packet_type == InventoryRoundSummary)
        {
            struct Ex10Result ex10_result = make_ex10_success();
            const uint8_t     reason =
                packet.static_data->inventory_round_summary.reason;

            inventory_state.min_q_count =
                packet.static_data->inventory_round_summary.min_q_count;
            inventory_state.queries_since_valid_epc_count =
                packet.static_data->inventory_round_summary
                    .queries_since_valid_epc_count;
            inventory_state.done_reason = reason;

            switch (reason)
            {
                case InventorySummaryDone:
                case InventorySummaryHost:
                    // Only count the round as done if the LMAC said it was done
                    // or the host told it to stop.  Any other reason for
                    // stopping is not a complete round, but possibly a reason
                    // to continue the inventory round.
                    inventory_state.round_count += 1;
                    break;
                case InventorySummaryRegulatory:
                    // Save Q to use for next round's initial Q.
                    inventory_state.previous_q =
                        packet.static_data->inventory_round_summary.final_q;
                    break;
                case InventorySummaryUnsupported:
                case InventorySummaryTxNotRampedUp:
                    // No special action. Continue continuous inventory.
                    break;
                case InventorySummaryEventFifoFull:
                    ex10_result = make_ex10_sdk_error(Ex10ModuleUseCase,
                                                      Ex10SdkEventFifoFull);
                    break;
                case InventorySummaryInvalidParam:
                    ex10_result = make_ex10_sdk_error(
                        Ex10ModuleUseCase, Ex10InventoryInvalidParam);
                    break;
                case InventorySummaryLmacOverload:
                    ex10_result = make_ex10_sdk_error(Ex10ModuleUseCase,
                                                      Ex10SdkLmacOverload);
                    break;
                case InventorySummaryNone:
                default:
                    ex10_result = make_ex10_sdk_error(
                        Ex10ModuleUseCase, Ex10InventorySummaryReasonInvalid);
                    break;
            }

            // If the error is set, continuous inventory will be stopped and a
            // summary sent.
            if (ex10_result.error)
            {
                handle_continuous_inventory_error(ex10_result, &packet);
            }
            else if (check_stop_conditions(packet.us_counter))
            {
                // Otherwise check if continuous inventory stopped frmo one of
                // the expected stop conditions
                inventory_state.state = InvIdle;
                ex10_result = push_continuous_inventory_summary_packet(
                    &packet, make_ex10_success());
                if (ex10_result.error)
                {
                    push_ex10_result_packet(ex10_result, packet.us_counter);
                }
            }
            else
            {
                // otherwise continue on with continuous inventory
                ex10_result = continue_continuous_inventory();
                if (ex10_result.error)
                {
                    handle_continuous_inventory_error(ex10_result, &packet);
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
    ex10_memzero(&inventory_params, sizeof(inventory_params));
    ex10_memzero(&inventory_state, sizeof(inventory_state));
    ex10_memzero(&stop_conditions, sizeof(stop_conditions));
    inventory_state.state = InvIdle;
    start_time_us         = 0u;

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
    void (*callback)(struct EventFifoPacket const*, struct Ex10Result*))
{
    inventory_state.packet_subscriber_callback = callback;
}

static void enable_packet_filter(bool enable_filter)
{
    inventory_state.publish_all_packets = (enable_filter == false);
}

static void enable_auto_access(bool enable)
{
    inventory_state.enable_auto_access = enable;
}

static void enable_abort_on_fail(bool enable)
{
    inventory_state.abort_on_fail = enable;
}

static enum StopReason get_continuous_inventory_stop_reason(void)
{
    return inventory_state.stop_reason;
}

static struct Ex10Result publish_packets(void)
{
    bool inventory_done = false;

    struct Ex10EventFifoQueue const* event_fifo_queue =
        get_ex10_event_fifo_queue();
    struct EventFifoPacket const* packet = NULL;

    struct Ex10Result ex10_result = make_ex10_success();

    while (inventory_done == false && ex10_result.error == false)
    {
        uint32_t const packet_wait_timeout_us = 200u * 1000u;
        event_fifo_queue->packet_wait_with_timeout(packet_wait_timeout_us);
        packet = event_fifo_queue->packet_peek();

        if (packet != NULL)
        {
            if (packet->packet_type == InvalidPacket)
            {
                ex10_eprintf("Invalid packet occurred with no known cause\n");
                ex10_result = make_ex10_sdk_error(Ex10ModuleUseCase,
                                                  Ex10InvalidEventFifoPacket);
            }
            else if (packet->packet_type == Ex10ResultPacket)
            {
                ex10_result =
                    packet->static_data->ex10_result_packet.ex10_result;

                get_ex10_event_fifo_printer()->print_packets(packet);
            }
            else if (packet->packet_type == ContinuousInventorySummary)
            {
                inventory_done = true;
            }

            if (inventory_state.packet_subscriber_callback != NULL)
            {
                if (inventory_state.publish_all_packets ||
                    packet->packet_type == TagRead ||
                    packet->packet_type == ContinuousInventorySummary ||
                    packet->packet_type == Gen2Transaction)
                {
                    inventory_state.packet_subscriber_callback(packet,
                                                               &ex10_result);
                    // The inventory may be stopped by the client application,
                    // without creating an error condition.
                    if ((ex10_result.customer == true) ||
                        (ex10_result.result_code.raw != 0u))
                    {
                        inventory_state.state = InvStopRequested;
                    }
                }
            }

            event_fifo_queue->packet_remove();
        }
    }

    return ex10_result;
}

static struct Ex10Result continuous_inventory(
    struct Ex10ContinuousInventoryUseCaseParameters* params)
{
    if (params == NULL || params->stop_conditions == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleUseCase, Ex10SdkErrorNullPointer);
    }

    inventory_params.inventory_config.initial_q            = params->initial_q;
    inventory_params.inventory_config.max_q                = 15;
    inventory_params.inventory_config.min_q                = 0;
    inventory_params.inventory_config.num_min_q_cycles     = 1;
    inventory_params.inventory_config.fixed_q_mode         = false;
    inventory_params.inventory_config.q_increase_use_query = false;
    inventory_params.inventory_config.q_decrease_use_query = false;
    inventory_params.inventory_config.session              = params->session;
    inventory_params.inventory_config.select               = params->select;
    inventory_params.inventory_config.target               = params->target;
    inventory_params.inventory_config.halt_on_all_tags     = false;
    inventory_params.inventory_config.tag_focus_enable     = false;
    inventory_params.inventory_config.fast_id_enable       = false;
    inventory_params.inventory_config.abort_on_fail =
        inventory_state.abort_on_fail;
    inventory_params.inventory_config.always_ack = false;
    inventory_params.inventory_config.auto_access =
        inventory_state.enable_auto_access;
    inventory_params.inventory_config.halt_on_fail = false;
    inventory_params.inventory_config.rfu          = 0;

    inventory_params.inventory_config_2.max_queries_since_valid_epc = 16;
    inventory_params.inventory_config_2
        .starting_max_queries_since_valid_epc_count          = 0;
    inventory_params.inventory_config_2.starting_min_q_count = 0;
    inventory_params.inventory_config_2.Reserved0            = 0;

    // Marking that we are in continuous inventory mode and reset all
    // config parameters.
    inventory_state.state       = InvOngoing;
    inventory_state.stop_reason = SRNone;
    inventory_state.round_count = 0u;

    // Save initial inventory_state values to reset Q on target flip.
    inventory_state.initial_q                     = params->initial_q;
    inventory_state.previous_q                    = 0u;
    inventory_state.min_q_count                   = 0u;
    inventory_state.queries_since_valid_epc_count = 0u;
    inventory_state.done_reason                   = InventorySummaryNone;
    inventory_state.tag_count                     = 0u;
    inventory_state.target                        = params->target;

    // Store passed in params
    inventory_params.antenna       = params->antenna;
    inventory_params.rf_mode       = params->rf_mode;
    inventory_params.tx_power_cdbm = params->tx_power_cdbm;
    inventory_params.send_selects  = params->send_selects;
    inventory_dual_target          = params->dual_target;

    stop_conditions = *params->stop_conditions;
    start_time_us   = get_ex10_ops()->get_device_time();

    if (inventory_params.inventory_config.tag_focus_enable)
    {
        if (params->dual_target == true)
        {
            ex10_printf(
                "Note: When enabling the TagFocus feature, Impinj recommends "
                "running ex10_continuous_inventory_use_case in a "
                "single target configuration.\n");
        }
    }

    // Begin inventory
    struct Ex10Result const ex10_result = get_ex10_inventory()->start_inventory(
        inventory_params.antenna,
        inventory_params.rf_mode,
        inventory_params.tx_power_cdbm,
        &inventory_params.inventory_config,
        &inventory_params.inventory_config_2,
        inventory_params.send_selects);
    if (ex10_result.error)
    {
        inventory_state.state = InvIdle;
        return ex10_result;
    }

    return publish_packets();
}

// clang-format off
static struct Ex10ContinuousInventoryUseCase ex10_continuous_inventory_use_case = {
    .init                                 = init,
    .deinit                               = deinit,
    .register_packet_subscriber_callback  = register_packet_subscriber_callback,
    .enable_packet_filter                 = enable_packet_filter,
    .enable_auto_access                    = enable_auto_access,
    .enable_abort_on_fail                 = enable_abort_on_fail,
    .continuous_inventory                 = continuous_inventory,
    .get_continuous_inventory_stop_reason = get_continuous_inventory_stop_reason,
};
// clang-format on

struct Ex10ContinuousInventoryUseCase const*
    get_ex10_continuous_inventory_use_case(void)
{
    return &ex10_continuous_inventory_use_case;
}
