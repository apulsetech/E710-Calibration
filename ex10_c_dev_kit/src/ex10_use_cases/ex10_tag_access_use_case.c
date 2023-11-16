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

#include "ex10_use_cases/ex10_tag_access_use_case.h"

/// @enum TagAccessInventoryState keep track of the inventory state.
enum TagAccessInventoryState
{
    /// Continuous inventory is not in progress.
    InventoryIdle,
    /// Inventory is in progress.
    InventoryOngoing,
    /// Halted on Tag
    InventoryHalted,
    /// Tag Lost for regulatory timer
    InventoryTagLostRegulatory,
    /// The host requested inventory stop
    InventoryStopRequested,
};

/**
 * @struct TagAccessState
 * State variables that are set in update_inventory_state().
 */
struct TagAccessState
{
    /// Initialized in to InventoryIdle in init() and managed by the
    /// event fifo interrupt handler
    enum TagAccessInventoryState volatile state;

    /// Initial Q
    uint8_t initial_q;

    /// The Q from the end of the previous inventory round.
    uint8_t previous_q;

    /// The min_q_count from the previous inventory round.
    uint8_t min_q_count;

    /// The queries_since_valid_epc_count from the previous round.
    uint8_t queries_since_valid_epc_count;

    /// The callback to notify the subscriber of a new packet.
    void (*tag_halted_callback)(struct EventFifoPacket const*,
                                enum HaltedCallbackResult*,
                                struct Ex10Result*);
};

/**
 * Ex10TagAccessUseCase private state variables.
 * These are initialized in the init() so that if
 * it is called multiple times, it will return to the
 * same starting condition.
 */
static struct InventoryParams inventory_params;
static struct TagAccessState  tag_access_state;

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

// Called by the interrupt handler thread when there is a fifo related
// interrupt.  This handler is checking the event fifo events
// and checking to see if there is a inventory summary and if it needs
// to start the next round.  It also manages the internal state of the
// inventory round. And then passes the packets on to the main context
// though the event fifo queue.
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
            // tag access state machine. Discontinue processing.
            ex10_eprintf(
                "Invalid packet encountered during the tag access use case "
                "packet parsing, packet end pos: %zu\n",
                bytes.length);
            break;
        }
        if (packet.packet_type == InventoryRoundSummary)
        {
            const uint8_t reason =
                packet.static_data->inventory_round_summary.reason;
            if (reason == InventorySummaryRegulatory)
            {
                // Save Q to use for next round's initial Q.
                struct InventoryRoundSummary const* round_summary =
                    &(packet.static_data->inventory_round_summary);

                struct InventoryRoundControlFields config =
                    inventory_params.inventory_config;
                struct InventoryRoundControl_2Fields config_2 =
                    inventory_params.inventory_config_2;
                config.initial_q              = round_summary->final_q;
                config_2.starting_min_q_count = round_summary->min_q_count;
                config_2.starting_max_queries_since_valid_epc_count =
                    round_summary->queries_since_valid_epc_count;
                struct Ex10Result ex10_result =
                    get_ex10_inventory()->start_inventory(
                        inventory_params.antenna,
                        inventory_params.rf_mode,
                        inventory_params.tx_power_cdbm,
                        &config,
                        &config_2,
                        inventory_params.send_selects);
                if (ex10_result.error)
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

                    tag_access_state.state = InventoryIdle;
                }
            }

            if (tag_access_state.state == InventoryHalted)
            {
                // if we were halted on a tag we must have lost the
                // tag should only happen when we ramp down for regulatory
                // but just in case we get a summary
                tag_access_state.state = InventoryTagLostRegulatory;
            }
        }
        else if (packet.packet_type == TagRead &&
                 packet.static_data->tag_read.halted_on_tag)
        {
            // got a TagRead with a successful halt
            tag_access_state.state = InventoryHalted;
        }
    }

    // The FifoBufferNode must be placed into the reader list after the
    // tag access inventory state is updated within the IRQ_N monitor thread
    // context.
    get_ex10_event_fifo_queue()->list_node_push_back(fifo_buffer_node);
}

static struct Ex10Result init(void)
{
    ex10_memzero(&tag_access_state, sizeof(tag_access_state));
    ex10_memzero(&inventory_params, sizeof(inventory_params));
    tag_access_state.state = InventoryIdle;

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
        .halted                  = true,
        .event_fifo_above_thresh = true,
        .event_fifo_full         = true,
        .inventory_round_done    = true,
        .halted_sequence_done    = true,
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

static void register_halted_callback(
    void (*tag_halted_callback)(struct EventFifoPacket const*,
                                enum HaltedCallbackResult*,
                                struct Ex10Result*))
{
    tag_access_state.tag_halted_callback = tag_halted_callback;
}

// This publish packets function is the function on the main thread that
// receives the packets from the event_fifo_queue and processes them.
// It looks for tag read packets to hand off to the callback and processes
// the other relevant packets to determine when the inventory round is complete
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

            if (packet->packet_type == Ex10ResultPacket)
            {
                ex10_result =
                    packet->static_data->ex10_result_packet.ex10_result;

                get_ex10_event_fifo_printer()->print_packets(packet);
            }

            // if this packet happens to be an round summary and the reason is
            // not a regulatory stop, we should set the flag that we are done
            // with the round.  But we will not process the packet here
            if (packet->packet_type == InventoryRoundSummary)
            {
                const uint8_t reason =
                    packet->static_data->inventory_round_summary.reason;
                if (reason != InventorySummaryRegulatory)
                {
                    inventory_done = true;
                }
            }

            if (packet->packet_type == TagRead)
            {
                // Its a tag read, we need to call the callback
                if (tag_access_state.tag_halted_callback != NULL)
                {
                    // grab the halted on tag state because the callback will
                    // discard the packet so that it can read the next packets
                    const bool halted_on_tag =
                        packet->static_data->tag_read.halted_on_tag;
                    enum HaltedCallbackResult cb_result;
                    tag_access_state.tag_halted_callback(
                        packet, &cb_result, &ex10_result);
                    if (ex10_result.error)
                    {
                        return ex10_result;
                    }
                    const bool nak_tag = (cb_result == NakTagAndContinue);

                    if (halted_on_tag &&
                        tag_access_state.state != InventoryTagLostRegulatory)
                    {
                        // looks like we are still halted on the tag so we
                        // continue the inventory round
                        struct Ex10Result const ex10_continue =
                            get_ex10_ops()->continue_from_halted(nak_tag);
                        ex10_result =
                            ex10_result.error ? ex10_result : ex10_continue;
                    }
                }
                else
                {
                    // looks like there is no callback registered so we just
                    // continue to the next tag
                    event_fifo_queue->packet_remove();
                    ex10_result = get_ex10_ops()->continue_from_halted(false);
                }
            }
            else
            {
                // discard packet (if it was a tag read it should have been
                // removed by the callback to be able to read the gen2access
                // packets)
                event_fifo_queue->packet_remove();
            }
        }
    }

    return ex10_result;
}

static struct Ex10Result run_inventory(
    struct Ex10TagAccessUseCaseParameters* params)
{
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
    inventory_params.inventory_config.halt_on_all_tags     = true;
    inventory_params.inventory_config.tag_focus_enable     = false;
    inventory_params.inventory_config.fast_id_enable       = false;
    inventory_params.inventory_config.abort_on_fail        = false;
    inventory_params.inventory_config.always_ack           = false;
    inventory_params.inventory_config.auto_access          = false;
    inventory_params.inventory_config.halt_on_fail         = false;
    inventory_params.inventory_config.rfu                  = 0;

    inventory_params.inventory_config_2.max_queries_since_valid_epc = 16;
    inventory_params.inventory_config_2
        .starting_max_queries_since_valid_epc_count          = 0;
    inventory_params.inventory_config_2.starting_min_q_count = 0;
    inventory_params.inventory_config_2.Reserved0            = 0;

    // Marking that we are in tag access inventory mode and reset all
    // config parameters.
    tag_access_state.state = InventoryOngoing;

    // Save initial tag_access_state values to reset Q on target flip.
    // Note: InventorySummaryReason enum value zero is not enumerated, and
    // is used to initialize done_reason = 0; i.e. "not done".
    tag_access_state.initial_q                     = params->initial_q;
    tag_access_state.previous_q                    = 0u;
    tag_access_state.min_q_count                   = 0u;
    tag_access_state.queries_since_valid_epc_count = 0u;

    // Store passed in params
    inventory_params.antenna       = params->antenna;
    inventory_params.rf_mode       = params->rf_mode;
    inventory_params.tx_power_cdbm = params->tx_power_cdbm;
    inventory_params.send_selects  = params->send_selects;

    struct Ex10Result ex10_result =
        get_ex10_rf_power()->set_rf_mode(inventory_params.rf_mode);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Begin inventory
    ex10_result = get_ex10_inventory()->start_inventory(
        inventory_params.antenna,
        inventory_params.rf_mode,
        inventory_params.tx_power_cdbm,
        &inventory_params.inventory_config,
        &inventory_params.inventory_config_2,
        inventory_params.send_selects);
    if (ex10_result.error)
    {
        tag_access_state.state = InventoryIdle;
        return ex10_result;
    }

    return publish_packets();
}

static enum TagAccessResult execute_access_commands(void)
{
    enum TagAccessResult result = TagAccessSuccess;
    if (tag_access_state.state == InventoryHalted)
    {
        /* Trigger stored Gen2 sequence */
        struct Ex10Result ex10_result =
            get_ex10_ops()->send_gen2_halted_sequence();
        if (ex10_result.error)
        {
            result = TagAccessHaltSequenceWriteError;
        }
    }
    else
    {
        result = TagAccessTagLost;
    }
    // returns the current tag state to let the user
    // know if the access command was sent, the tag
    // was lost, or the tag is in a bad state.
    // (note that there is a little bit of a race here if the
    // regulatory timer goes off while the sequence is being sent
    // but hasn't been noticed by the interrupt handler yet)
    return result;
}

static struct EventFifoPacket const* get_fifo_packet(void)
{
    struct EventFifoPacket const* packet = NULL;
    while (packet == NULL)
    {
        packet = get_ex10_event_fifo_queue()->packet_peek();
    }
    return packet;
}

// this is a trivial helper, but it is more to make the API
// of the use case look right.
static void remove_fifo_packet(void)
{
    get_ex10_event_fifo_queue()->packet_remove();
}

static bool remove_halted_packet(void)
{
    struct EventFifoPacket const* packet = get_fifo_packet();
    bool const is_halted_packet          = (packet->packet_type == Halted);
    if (is_halted_packet == false)
    {
        ex10_ex_eprintf("packet_type %u != Halted\n", packet->packet_type);
    }

    remove_fifo_packet();
    return is_halted_packet;
}

static struct Ex10TagAccessUseCase ex10_tag_access_use_case = {
    .init                     = init,
    .deinit                   = deinit,
    .register_halted_callback = register_halted_callback,
    .run_inventory            = run_inventory,
    .execute_access_commands  = execute_access_commands,
    .get_fifo_packet          = get_fifo_packet,
    .remove_fifo_packet       = remove_fifo_packet,
    .remove_halted_packet     = remove_halted_packet,
};

struct Ex10TagAccessUseCase const* get_ex10_tag_access_use_case(void)
{
    return &ex10_tag_access_use_case;
}
