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

#include "ex10_api/ex10_reader.h"
#include "board/board_spec.h"
#include "board/ex10_gpio.h"
#include "board/ex10_osal.h"
#include "calibration.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/byte_span.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_event_fifo_queue.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/ex10_test.h"
#include "ex10_api/fifo_buffer_list.h"
#include "ex10_api/gen2_tx_command_manager.h"
#include "ex10_api/linked_list.h"
#include "ex10_api/trace.h"
#include "ex10_api/version_info.h"

#include "ex10_modules/ex10_antenna_disconnect.h"
#include "ex10_modules/ex10_listen_before_talk.h"
#include "ex10_modules/ex10_ramp_module_manager.h"

/**
 * @struct ReaderInventoryParams
 * These parameters are set via host calls through the Ex10Reader interface.
 * They are never (and must never be) set from the fifo_data_handler thread.
 */
struct ReaderInventoryParams
{
    uint8_t                              antenna;
    enum RfModes                         rf_mode;
    int16_t                              tx_power_cdbm;
    struct InventoryRoundControlFields   inventory_config;
    struct InventoryRoundControl_2Fields inventory_config_2;
    bool                                 send_selects;
    struct StopConditions                stop_conditions;
    bool                                 dual_target;
    bool                                 remain_on;
    uint32_t                             start_time_us;
};

/**
 * @struct Ex10ReaderPrivate
 * Ex10Reader private state variables.
 */
struct Ex10ReaderPrivate
{
    // There is a bug around retrieving the calibration layer multiple times.
    // keeping this until that bug is fixed.
    struct RxGainControlFields      stored_analog_rx_fields;
    struct ReaderInventoryParams    inventory_params;
    struct ContinuousInventoryState inventory_state;
};

static struct Ex10ReaderPrivate reader = {
    .stored_analog_rx_fields = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    .inventory_params =
        {
            .antenna       = 0u,
            .rf_mode       = (enum RfModes)0u,
            .tx_power_cdbm = 0u,
            .send_selects  = false,
            .stop_conditions =
                {
                    .max_number_of_rounds = 0u,
                    .max_number_of_tags   = 0u,
                    .max_duration_us      = 0u,
                },
            .dual_target   = false,
            .remain_on     = false,
            .start_time_us = 0u,
        },
    .inventory_state =
        {
            .state                         = InvIdle,
            .done_reason                   = InventorySummaryNone,
            .initial_inventory_config      = {0},
            .previous_q                    = 0u,
            .min_q_count                   = 0u,
            .queries_since_valid_epc_count = 0u,
            .stop_reason                   = SRNone,
            .round_count                   = 0u,
            .tag_count                     = 0u,
            .target                        = target_A,
        },
};

/* Forward declarations */
static void fifo_data_handler(struct FifoBufferNode* fifo_buffer);
static bool interrupt_handler(struct InterruptStatusFields irq_status);
static struct Ex10Result insert_fifo_event(
    const bool                    trigger_irq,
    struct EventFifoPacket const* event_packet);
static struct Ex10Result stop_transmitting(void);
static struct Ex10Result start_inventory(
    uint8_t                                     antenna,
    enum RfModes                                rf_mode,
    int16_t                                     tx_power_cdbm,
    struct InventoryRoundControlFields const*   inventory_config,
    struct InventoryRoundControl_2Fields const* inventory_config_2,
    bool                                        send_selects,
    bool                                        remain_on);

static void init(enum Ex10RegionId region_id)
{
    // the region is initialized in the core setup now
    (void)region_id;

    get_ex10_event_fifo_queue()->init();

    ex10_memzero(&reader.inventory_state, sizeof(reader.inventory_state));
    reader.inventory_state.state = InvIdle;
}

/**
 * @details
 * Error handling: it is best to work through as much of this initialization
 * as possible and return an error code at the end, thereby setting up the
 * reader state variables into as consistent a form possible.
 */
static struct Ex10Result init_ex10(void)
{
    struct Ex10Protocol const* protocol = get_ex10_protocol();
    protocol->unregister_fifo_data_callback();
    struct Ex10Result ex10_result =
        protocol->register_fifo_data_callback(fifo_data_handler);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = protocol->unregister_interrupt_callback();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Note: Ex10Ops.init_ex10() clears the interrupt mask.
    // Therefore, Ex10Reader.init_ex10() should be called after
    // Ex10Ops.init_ex10() in board initialization ex10_typical_board_setup().
    struct InterruptMaskFields const register_int_mask = {
        .op_done                 = true,
        .halted                  = true,
        .event_fifo_above_thresh = true,
        .event_fifo_full         = true,
        .inventory_round_done    = true,
        .halted_sequence_done    = true,
        .command_error           = true,
        .aggregate_op_done       = false,
    };
    ex10_result = protocol->register_interrupt_callback(register_int_mask,
                                                        interrupt_handler);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Set the GPIO initial levels and enables to the value specified in the
    // board layer.
    struct GpioPinsSetClear const gpio_pins_set_clear =
        get_ex10_board_spec()->get_default_gpio_setup();
    struct Ex10Ops const* ops = get_ex10_ops();
    ex10_result               = ops->set_clear_gpio_pins(&gpio_pins_set_clear);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }
    // Prepare the buffer for gen2 commands
    get_ex10_gen2_tx_command_manager()->init();

    // Register the reader post ramp up callback to occur after cw_on calls
    struct Ex10AntennaDisconnect const* antenna_disconnect =
        get_ex10_antenna_disconnect();
    antenna_disconnect->init();

    return ex10_result;
}

static void read_calibration(void)
{
    get_ex10_calibration()->init(get_ex10_protocol());
}

static struct Ex10Result deinit(void)
{
    // Don't rely on init() being called.
    // Ex10Reader might not have been initialized; i.e. bootloader startup.
    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();
    struct Ex10Result          ex10_result =
        ex10_protocol->unregister_interrupt_callback();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_protocol->unregister_fifo_data_callback();

    get_ex10_antenna_disconnect()->deinit();

    return make_ex10_success();
}


static void push_continuous_inventory_summary_packet(
    struct EventFifoPacket const* event_packet,
    struct Ex10Result             ex10_result)
{
    uint32_t const duration_us =
        event_packet->us_counter - reader.inventory_params.start_time_us;

    uint8_t const stop_reason = (uint8_t)reader.inventory_state.stop_reason;

    struct ContinuousInventorySummary summary = {
        .duration_us                = duration_us,
        .number_of_inventory_rounds = reader.inventory_state.round_count,
        .number_of_tags             = reader.inventory_state.tag_count,
        .reason                     = stop_reason,
        .last_op_id                 = 0u,
        .last_op_error              = ErrorNone,
        .packet_rfu_1               = 0u,
    };

    if (ex10_result.error)
    {
        if (ex10_result.module == Ex10ModuleDevice)
        {
            switch (ex10_result.result_code.device)
            {
                case Ex10DeviceErrorCommandsNoResponse:
                case Ex10DeviceErrorCommandsWithResponse:
                    summary.reason = SRDeviceCommandError;
                    break;

                case Ex10DeviceErrorOps:
                    summary.last_op_id =
                        ex10_result.device_status.ops_status.op_id;
                    summary.last_op_error =
                        ex10_result.device_status.ops_status.error;
                    summary.reason = SROpError;
                    break;

                case Ex10DeviceErrorOpsTimeout:
                    summary.last_op_id =
                        ex10_result.device_status.ops_status.op_id;
                    summary.last_op_error =
                        ex10_result.device_status.ops_status.error;
                    summary.reason = SRSdkTimeoutError;
                    break;

                default:
                    // Unexpected error path, should not reach here
                    break;
            }
        }
        else
        {
            switch (ex10_result.result_code.sdk)
            {
                case Ex10SdkErrorAggBufferOverflow:
                    summary.reason = SRDeviceAggregateBufferOverflow;
                    break;

                case Ex10AboveThreshold:
                    summary.reason = SRDeviceRampCallbackError;
                    break;

                default:
                    // Unexpected error path, should not reach here
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
    insert_fifo_event(trigger_irq, &summary_packet);
}

static bool check_stop_conditions(uint32_t timestamp_us)
{
    // if the reason is already set, we return so as to retain the original stop
    // reason
    if (reader.inventory_state.stop_reason != SRNone)
    {
        return true;
    }

    if (reader.inventory_params.stop_conditions.max_number_of_rounds > 0u)
    {
        if (reader.inventory_state.round_count >=
            reader.inventory_params.stop_conditions.max_number_of_rounds)
        {
            reader.inventory_state.stop_reason = SRMaxNumberOfRounds;
            return true;
        }
    }
    if (reader.inventory_params.stop_conditions.max_number_of_tags > 0u)
    {
        if (reader.inventory_state.tag_count >=
            reader.inventory_params.stop_conditions.max_number_of_tags)
        {
            reader.inventory_state.stop_reason = SRMaxNumberOfTags;
            return true;
        }
    }
    if (reader.inventory_params.stop_conditions.max_duration_us > 0u)
    {
        // packet before start checks for packets which occurred before the
        // continuous inventory round was started.
        bool const packet_before_start =
            (reader.inventory_params.start_time_us > timestamp_us);
        uint32_t const elapsed_us =
            (packet_before_start)
                ? ((UINT32_MAX - reader.inventory_params.start_time_us) +
                   timestamp_us + 1)
                : (timestamp_us - reader.inventory_params.start_time_us);
        if (elapsed_us >=
            reader.inventory_params.stop_conditions.max_duration_us)
        {
            reader.inventory_state.stop_reason = SRMaxDuration;
            return true;
        }
    }
    if (reader.inventory_state.state == InvStopRequested)
    {
        reader.inventory_state.stop_reason = SRHost;
        return true;
    }
    return false;
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
    InventorySummaryEventFifoFull // Don't care
    InventorySummaryTxNotRampedUp // Don't care
    InventorySummaryInvalidParam  // Don't care
    InventorySummaryLmacOverload  // Don't care
    */

    bool reset_q = false;
    if (reader.inventory_params.dual_target)
    {
        // Flip target if round is done, not for regulatory or error.
        if (reader.inventory_state.done_reason == InventorySummaryDone)
        {
            reader.inventory_state.target ^= 1u;
            reset_q = true;
        }

        // If CW is not on and our session is zero (no persistence after power),
        // we need to switch the target to A.
        if ((get_ex10_rf_power()->get_cw_is_on() == false) &&
            (reader.inventory_params.inventory_config.session == 0))
        {
            reset_q                       = true;
            reader.inventory_state.target = target_A;
        }
    }
    else
    {
        if (reader.inventory_state.done_reason == InventorySummaryDone)
        {
            reset_q = true;
        }
    }

    struct InventoryRoundControlFields inventory_config =
        reader.inventory_params.inventory_config;
    inventory_config.target = reader.inventory_state.target;

    struct InventoryRoundControl_2Fields inventory_config_2 =
        reader.inventory_params.inventory_config_2;

    // Preserve Q  and internal LMAC counters across rounds or reset for
    // new target.
    if (reset_q)
    {
        // Reset Q for target flip (done above) or for normal end of round.
        inventory_config.initial_q =
            reader.inventory_state.initial_inventory_config.initial_q;
        inventory_config_2.starting_min_q_count                       = 0;
        inventory_config_2.starting_max_queries_since_valid_epc_count = 0;
    }
    else if (reader.inventory_state.done_reason == InventorySummaryRegulatory)
    {
        // Preserve Q across rounds
        inventory_config.initial_q = reader.inventory_state.previous_q;
        inventory_config_2.starting_min_q_count =
            reader.inventory_state.min_q_count;
        inventory_config_2.starting_max_queries_since_valid_epc_count =
            reader.inventory_state.queries_since_valid_epc_count;
    }

    return start_inventory(reader.inventory_params.antenna,
                           reader.inventory_params.rf_mode,
                           reader.inventory_params.tx_power_cdbm,
                           &inventory_config,
                           &inventory_config_2,
                           reader.inventory_params.send_selects,
                           reader.inventory_params.remain_on);
}

// Called by the interrupt handler thread when there is a non-fifo related
// interrupt.
static bool interrupt_handler(struct InterruptStatusFields irq_status)
{
    (void)irq_status;
    // returns true to tell the interrupt handler to grab the fifo data
    return true;
}

static void handle_continuous_inventory_error(
    struct Ex10Result             ex10_result,
    struct EventFifoPacket const* packet)
{
    struct FifoBufferNode* result_buffer_node =
        make_ex10_result_fifo_packet(ex10_result, packet->us_counter);

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
        get_ex10_event_fifo_queue()->list_node_push_back(result_buffer_node);
    }

    reader.inventory_state.state = InvIdle;

    // The error from ex10 result needs to become the new stop reason
    reader.inventory_state.stop_reason =
        get_ex10_inventory()->ex10_result_to_continuous_inventory_error(
            ex10_result);
    push_continuous_inventory_summary_packet(packet, ex10_result);
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
        if (reader.inventory_state.state != InvIdle)
        {
            if (packet.packet_type == TagRead)
            {
                reader.inventory_state.tag_count += 1;
            }
            else if (packet.packet_type == InventoryRoundSummary)
            {
                struct Ex10Result ex10_result = make_ex10_success();
                const uint8_t     reason =
                    packet.static_data->inventory_round_summary.reason;

                reader.inventory_state.min_q_count =
                    packet.static_data->inventory_round_summary.min_q_count;
                reader.inventory_state.queries_since_valid_epc_count =
                    packet.static_data->inventory_round_summary
                        .queries_since_valid_epc_count;
                reader.inventory_state.done_reason = reason;

                switch (reason)
                {
                    case InventorySummaryDone:
                    case InventorySummaryHost:
                        // only count the round as done if the LMAC said it was
                        // done or the host told it to stop.  Any other reason
                        // for stopping is not a complete round, but possibly a
                        // reason to continue the inventory round.
                        reader.inventory_state.round_count += 1;
                        break;
                    case InventorySummaryRegulatory:
                        reader.inventory_state.previous_q =
                            packet.static_data->inventory_round_summary.final_q;
                        break;
                    case InventorySummaryUnsupported:
                    case InventorySummaryTxNotRampedUp:
                        // No special action. Continue continuous inventory.
                        break;
                    case InventorySummaryEventFifoFull:
                        ex10_result = make_ex10_sdk_error(Ex10ModuleReader,
                                                          Ex10SdkEventFifoFull);
                        break;
                    case InventorySummaryInvalidParam:
                        ex10_result = make_ex10_sdk_error(
                            Ex10ModuleReader, Ex10InventoryInvalidParam);
                        break;
                    case InventorySummaryLmacOverload:
                        ex10_result = make_ex10_sdk_error(Ex10ModuleReader,
                                                          Ex10SdkLmacOverload);
                        break;
                    case InventorySummaryNone:
                    default:
                        ex10_result = make_ex10_sdk_error(
                            Ex10ModuleReader,
                            Ex10InventorySummaryReasonInvalid);
                        break;
                }

                // If the error is set, continuous inventory will be stopped and
                // a summary sent.
                if (ex10_result.error)
                {
                    handle_continuous_inventory_error(ex10_result, &packet);
                }
                else if (check_stop_conditions(packet.us_counter))
                {
                    // Otherwise check if continuous inventory stopped frmo one
                    // of the expected stop conditions
                    reader.inventory_state.state = InvIdle;
                    push_continuous_inventory_summary_packet(&packet,
                                                             ex10_result);
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
    }

    // The FifoBufferNode must be placed into the reader list after the
    // continuous inventory state is updated within the IRQ_N monitor thread
    // context.
    get_ex10_event_fifo_queue()->list_node_push_back(fifo_buffer_node);
}

static struct EventFifoPacket const* packet_peek(void)
{
    return get_ex10_event_fifo_queue()->packet_peek();
}

static void packet_remove(void)
{
    get_ex10_event_fifo_queue()->packet_remove();
}

static bool packets_available(void)
{
    return (get_ex10_event_fifo_queue()->packet_peek() != NULL);
}

static struct Ex10Result continue_from_halted(bool nak)
{
    return get_ex10_ops()->continue_from_halted(nak);
}

static struct Ex10RegulatoryTimers const regulatory_timers_disabled = {
    .nominal_ms          = 0u,
    .extended_ms         = 0u,
    .regulatory_ms       = 0u,
    .off_same_channel_ms = 0u};

static struct Ex10Result build_cw_configs(uint8_t          antenna,
                                          enum RfModes     rf_mode,
                                          int16_t          tx_power_cdbm,
                                          uint32_t         frequency_khz,
                                          bool             remain_on,
                                          struct CwConfig* cw_config)
{
    struct Ex10RampModuleManager const* ramp_module_manager =
        get_ex10_ramp_module_manager();
    uint16_t temperature_adc = ramp_module_manager->retrieve_adc_temperature();

    // If CW is already on, there is no need to measure temp for power settings.
    if (false == get_ex10_rf_power()->get_cw_is_on())
    {
        struct Ex10Result ex10_result =
            get_ex10_rf_power()->measure_and_read_adc_temperature(
                &temperature_adc);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        ramp_module_manager->store_adc_temperature(temperature_adc);
    }

    // If the temperature reading was invalid or cw is already on,
    // disable temperature compensation.
    bool const temp_comp_enabled =
        get_ex10_board_spec()->temperature_compensation_enabled(
            temperature_adc);

    if (frequency_khz)
    {
        get_ex10_active_region()->set_single_frequency(frequency_khz);
    }
    get_ex10_rf_power()->build_cw_configs(antenna,
                                          rf_mode,
                                          tx_power_cdbm,
                                          temperature_adc,
                                          temp_comp_enabled,
                                          cw_config);
    if (remain_on)
    {
        cw_config->timer = regulatory_timers_disabled;
    }
    return make_ex10_success();
}


static struct Ex10Result continuous_inventory(
    uint8_t                                     antenna,
    enum RfModes                                rf_mode,
    int16_t                                     tx_power_cdbm,
    struct InventoryRoundControlFields const*   inventory_config,
    struct InventoryRoundControl_2Fields const* inventory_config_2,
    bool                                        send_selects,
    struct StopConditions const*                stop_conditions,
    bool                                        dual_target,
    bool                                        remain_on)
{
    // Ensure the proper configs were passed in.
    if ((stop_conditions == NULL) || (inventory_config == NULL) ||
        (inventory_config_2 == NULL))
    {
        return make_ex10_sdk_error(Ex10ModuleReader, Ex10SdkErrorNullPointer);
    }

    // Marking that we are in continuous inventory mode and reset all
    // config parameters.
    reader.inventory_state.state       = InvOngoing;
    reader.inventory_state.stop_reason = SRNone;
    reader.inventory_state.round_count = 0u;

    // Save initial inventory_state values to reset Q on target flip.
    // Note: InventorySummaryReason enum value zero is not enumerated, and
    reader.inventory_state.initial_inventory_config      = *inventory_config;
    reader.inventory_state.previous_q                    = 0u;
    reader.inventory_state.min_q_count                   = 0u;
    reader.inventory_state.queries_since_valid_epc_count = 0u;
    reader.inventory_state.done_reason                   = InventorySummaryNone;
    reader.inventory_state.tag_count                     = 0u;
    reader.inventory_state.target = inventory_config->target;

    // Store passed in params
    reader.inventory_params.antenna            = antenna;
    reader.inventory_params.rf_mode            = rf_mode;
    reader.inventory_params.tx_power_cdbm      = tx_power_cdbm;
    reader.inventory_params.inventory_config   = *inventory_config;
    reader.inventory_params.inventory_config_2 = *inventory_config_2;
    reader.inventory_params.send_selects       = send_selects;
    reader.inventory_params.stop_conditions    = *stop_conditions;
    reader.inventory_params.dual_target        = dual_target;
    reader.inventory_params.remain_on          = remain_on;
    reader.inventory_params.start_time_us = get_ex10_ops()->get_device_time();

    // Begin inventory
    struct Ex10Result const ex10_result = start_inventory(antenna,
                                                          rf_mode,
                                                          tx_power_cdbm,
                                                          inventory_config,
                                                          inventory_config_2,
                                                          send_selects,
                                                          remain_on);
    if (ex10_result.error)
    {
        reader.inventory_state.state = InvIdle;
    }
    return ex10_result;
}

static struct Ex10Result reader_ramp_for_inventory(
    uint8_t                              antenna,
    enum RfModes                         rf_mode,
    int16_t                              tx_power_cdbm,
    bool                                 remain_on,
    struct PowerDroopCompensationFields* droop_comp_fields)
{
    struct Ex10RfPower const* rf_power = get_ex10_rf_power();
    struct Ex10Ops const*     ops      = get_ex10_ops();

    // Update the channel time tracking before kicking off the
    // next inventory round. This will be used to update the
    // regulatory timers if the inventory call needs to ramp up
    // again.
    struct Ex10Result ex10_result =
        get_ex10_active_region()->update_channel_time_tracking();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    uint32_t        frequency_khz = 0;
    struct CwConfig cw_config;
    ex10_result = build_cw_configs(
        antenna, rf_mode, tx_power_cdbm, frequency_khz, remain_on, &cw_config);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Now that we know we are going to attempt to ramp up
    // we should update the pre-post ramp variables.
    struct Ex10RampModuleManager const* ramp_module_manager =
        get_ex10_ramp_module_manager();
    ramp_module_manager->store_pre_ramp_variables(antenna);
    ramp_module_manager->store_post_ramp_variables(
        tx_power_cdbm, get_ex10_active_region()->get_next_channel_khz());

    ex10_result = rf_power->cw_on(&cw_config.gpio,
                                  &cw_config.power,
                                  &cw_config.synth,
                                  &cw_config.timer,
                                  droop_comp_fields);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Ensure all ops done before sending a select
    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Read back the analog rx settings since we ran sjc in cw_on.
    // Store the results in the local variable stored_analog_rx_fields.
    return get_ex10_protocol()->read(&rx_gain_control_reg,
                                     &reader.stored_analog_rx_fields);
}

static struct Ex10Result start_inventory(
    uint8_t                                     antenna,
    enum RfModes                                rf_mode,
    int16_t                                     tx_power_cdbm,
    struct InventoryRoundControlFields const*   inventory_config,
    struct InventoryRoundControl_2Fields const* inventory_config_2,
    bool                                        send_selects,
    bool                                        remain_on)
{
    struct Ex10Protocol const* protocol = get_ex10_protocol();

    // Check to make sure that an op isn't running (say if inventory is
    // called twice by accident).
    if (protocol->is_op_currently_running())
    {
        return make_ex10_success();
    }

    // continue_continuous_inventory() uses reader.inventory_state.target to
    // set the future values of inventory_config->target. Store it here.
    reader.inventory_state.target = inventory_config->target;

    // Cache the antenna and mode members of reader.inventory_params
    // for calculating RSSI compensation.
    // Generally, the reader.inventory_params is specific to continuous
    // inventory operation, which call this function to start the next
    // inventory once the inventory completes. When setting inventory_params
    // members, be sure that the values are consistent with the continuous
    // inventory operation.
    reader.inventory_params.antenna = antenna;
    reader.inventory_params.rf_mode = rf_mode;

    struct Ex10RfPower const*           rf_power = get_ex10_rf_power();
    struct PowerDroopCompensationFields droop_comp_fields =
        rf_power->get_droop_compensation_defaults();

    struct Ex10Result ex10_result = rf_power->set_rf_mode(rf_mode);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    if (get_ex10_rf_power()->get_cw_is_on() == false)
    {
        ex10_result = reader_ramp_for_inventory(
            antenna, rf_mode, tx_power_cdbm, remain_on, &droop_comp_fields);
        if (ex10_result.error)
        {
            return ex10_result;
        }
    }
    ex10_result = get_ex10_inventory()->run_inventory(
        inventory_config, inventory_config_2, send_selects);

    // There is a race condition where the sdk checks for cw, the device
    // reports it is ramped up, then it ramps down before select is run.
    // If this occurs, ramp up and and rerun. Any errors after this get
    // returned.
    if (ex10_result.error &&
        ex10_result.device_status.ops_status.op_id == SendSelectOp &&
        ex10_result.device_status.ops_status.error == ErrorInvalidTxState)
    {
        ex10_result = reader_ramp_for_inventory(
            antenna, rf_mode, tx_power_cdbm, remain_on, &droop_comp_fields);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        ex10_result = get_ex10_inventory()->run_inventory(
            inventory_config, inventory_config_2, send_selects);
    }
    return ex10_result;
}

static struct Ex10Result inventory(
    uint8_t                                     antenna,
    enum RfModes                                rf_mode,
    int16_t                                     tx_power_cdbm,
    struct InventoryRoundControlFields const*   inventory_config,
    struct InventoryRoundControl_2Fields const* inventory_config_2,
    bool                                        send_selects,
    bool                                        remain_on)
{
    return start_inventory(antenna,
                           rf_mode,
                           tx_power_cdbm,
                           inventory_config,
                           inventory_config_2,
                           send_selects,
                           remain_on);
}

static struct Ex10Result cw_test(uint8_t      antenna,
                                 enum RfModes rf_mode,
                                 int16_t      tx_power_cdbm,
                                 uint32_t     frequency_khz,
                                 bool         remain_on)
{
    struct Ex10RampModuleManager const* ramp_module_manager =
        get_ex10_ramp_module_manager();
    uint16_t temperature_adc = ramp_module_manager->retrieve_adc_temperature();

    // back into the Ex10Test module, and just have reader be a wrapper
    bool const temp_comp_enabled =
        get_ex10_board_spec()->temperature_compensation_enabled(
            temperature_adc);

    struct Ex10Result ex10_result = get_ex10_rf_power()->set_rf_mode(rf_mode);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    if (false == get_ex10_rf_power()->get_cw_is_on())
    {
        ex10_result = get_ex10_rf_power()->measure_and_read_adc_temperature(
            &temperature_adc);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        ramp_module_manager->store_adc_temperature(temperature_adc);

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
        if (remain_on)
        {
            cw_config.timer = regulatory_timers_disabled;
        }

        ramp_module_manager->store_pre_ramp_variables(antenna);
        ramp_module_manager->store_post_ramp_variables(tx_power_cdbm,
                                                       frequency_khz);

        struct PowerDroopCompensationFields droop_comp_defaults =
            get_ex10_rf_power()->get_droop_compensation_defaults();
        ex10_result = get_ex10_rf_power()->cw_on(&cw_config.gpio,
                                                 &cw_config.power,
                                                 &cw_config.synth,
                                                 &cw_config.timer,
                                                 &droop_comp_defaults);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        // Read back the analog rx settings since we ran sjc in cw_on.
        // Store the results in the local variable stored_analog_rx_fields.
        ex10_result = get_ex10_protocol()->read(
            &rx_gain_control_reg, &reader.stored_analog_rx_fields);
    }
    return ex10_result;
}

static struct Ex10Result prbs_test(uint8_t      antenna,
                                   enum RfModes rf_mode,
                                   int16_t      tx_power_cdbm,
                                   uint32_t     frequency_khz,
                                   bool         remain_on)
{
    struct Ex10RampModuleManager const* ramp_module_manager =
        get_ex10_ramp_module_manager();
    uint16_t temperature_adc = ramp_module_manager->retrieve_adc_temperature();

    // If CW is already on, there is no need to measure temp for power settings.
    if (false == get_ex10_rf_power()->get_cw_is_on())
    {
        struct Ex10Result ex10_result =
            get_ex10_rf_power()->measure_and_read_adc_temperature(
                &temperature_adc);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        ramp_module_manager->store_adc_temperature(temperature_adc);
    }

    bool const temp_comp_enabled =
        get_ex10_board_spec()->temperature_compensation_enabled(
            temperature_adc);

    (remain_on) ? get_ex10_active_region()->disable_regulatory_timers()
                : get_ex10_active_region()->reenable_regulatory_timers();

    return get_ex10_test()->prbs_test(antenna,
                                      rf_mode,
                                      tx_power_cdbm,
                                      frequency_khz,
                                      temperature_adc,
                                      temp_comp_enabled);
}

static struct Ex10Result ber_test(uint8_t      antenna,
                                  enum RfModes rf_mode,
                                  int16_t      tx_power_cdbm,
                                  uint32_t     frequency_khz,
                                  uint16_t     num_bits,
                                  uint16_t     num_packets,
                                  bool         delimiter_only)
{
    struct Ex10RampModuleManager const* ramp_module_manager =
        get_ex10_ramp_module_manager();
    uint16_t temperature_adc = ramp_module_manager->retrieve_adc_temperature();

    // If CW is already on, there is no need to measure temp for power settings.
    if (false == get_ex10_rf_power()->get_cw_is_on())
    {
        struct Ex10Result ex10_result =
            get_ex10_rf_power()->measure_and_read_adc_temperature(
                &temperature_adc);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        ramp_module_manager->store_adc_temperature(temperature_adc);
    }

    // If the temperature reading was invalid or cw is already on,
    // disable temperature compensation.
    bool const temp_comp_enabled =
        get_ex10_board_spec()->temperature_compensation_enabled(
            temperature_adc);

    return get_ex10_test()->ber_test(antenna,
                                     rf_mode,
                                     tx_power_cdbm,
                                     frequency_khz,
                                     num_bits,
                                     num_packets,
                                     delimiter_only,
                                     temperature_adc,
                                     temp_comp_enabled);
}

static struct Ex10Result etsi_burst_test(
    struct InventoryRoundControlFields const*   inventory_config,
    struct InventoryRoundControl_2Fields const* inventory_config_2,
    uint8_t                                     antenna,
    enum RfModes                                rf_mode,
    int16_t                                     tx_power_cdbm,
    uint16_t                                    on_time_ms,
    uint16_t                                    off_time_ms,
    uint32_t                                    frequency_khz)
{
    struct Ex10RampModuleManager const* ramp_module_manager =
        get_ex10_ramp_module_manager();
    uint16_t temperature_adc = ramp_module_manager->retrieve_adc_temperature();

    // If CW is already on, there is no need to measure temp for power settings.
    if (false == get_ex10_rf_power()->get_cw_is_on())
    {
        struct Ex10Result ex10_result =
            get_ex10_rf_power()->measure_and_read_adc_temperature(
                &temperature_adc);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        ramp_module_manager->store_adc_temperature(temperature_adc);
    }

    bool const temp_comp_enabled =
        get_ex10_board_spec()->temperature_compensation_enabled(
            temperature_adc);

    return get_ex10_test()->etsi_burst_test(inventory_config,
                                            inventory_config_2,
                                            antenna,
                                            rf_mode,
                                            tx_power_cdbm,
                                            on_time_ms,
                                            off_time_ms,
                                            frequency_khz,
                                            temperature_adc,
                                            temp_comp_enabled);
}

static struct Ex10Result insert_fifo_event(
    const bool                    trigger_irq,
    struct EventFifoPacket const* event_packet)
{
    return get_ex10_protocol()->insert_fifo_event(trigger_irq, event_packet);
}

static struct Ex10Result enable_sdd_logs(const struct LogEnablesFields enables,
                                         const uint8_t speed_mhz)
{
    return get_ex10_ops()->enable_sdd_logs(enables, speed_mhz);
}

static struct Ex10Result stop_transmitting(void)
{
    if (reader.inventory_state.state != InvIdle)
    {
        reader.inventory_state.state = InvStopRequested;
    }

    return get_ex10_rf_power()->stop_op_and_ramp_down();
}

static int16_t get_current_compensated_rssi(uint16_t rssi_raw)
{
    return get_ex10_calibration()->get_compensated_rssi(
        rssi_raw,
        reader.inventory_params.rf_mode,
        &reader.stored_analog_rx_fields,
        reader.inventory_params.antenna,
        get_ex10_active_region()->get_rf_filter(),
        get_ex10_ramp_module_manager()->retrieve_adc_temperature());
}

static uint16_t get_current_rssi_log2(int16_t rssi_cdbm)
{
    return get_ex10_calibration()->get_rssi_log2(
        rssi_cdbm,
        reader.inventory_params.rf_mode,
        &reader.stored_analog_rx_fields,
        reader.inventory_params.antenna,
        get_ex10_active_region()->get_rf_filter(),
        get_ex10_ramp_module_manager()->retrieve_adc_temperature());
}

static struct Ex10Result listen_before_talk_multi(
    uint8_t                 antenna,
    uint8_t                 rssi_count,
    struct LbtControlFields lbt_settings,
    uint32_t*               frequencies_khz,
    int32_t*                lbt_offsets,
    int16_t*                rssi_measurements)
{
    const struct Ex10ListenBeforeTalk* lbt = get_ex10_listen_before_talk();

    // this logic is not needed because the LBT module will ignore
    // the lbt_rx_gains itself if the override is not set.
    struct RxGainControlFields const lbt_rx_gains =
        (lbt_settings.override) ? reader.stored_analog_rx_fields
                                : lbt->get_default_lbt_rx_analog_configs();
    return lbt->listen_before_talk_multi(antenna,
                                         rssi_count,
                                         lbt_settings,
                                         frequencies_khz,
                                         lbt_offsets,
                                         rssi_measurements,
                                         &lbt_rx_gains);
}

static int16_t get_listen_before_talk_rssi(uint8_t  antenna,
                                           uint32_t frequency_khz,
                                           int32_t  lbt_offset,
                                           uint8_t  rssi_count,
                                           bool     override_used)
{
    const struct Ex10ListenBeforeTalk* lbt = get_ex10_listen_before_talk();

    struct RxGainControlFields const lbt_rx_gains =
        (override_used) ? reader.stored_analog_rx_fields
                        : lbt->get_default_lbt_rx_analog_configs();
    int16_t           lbt_rssi = 0;
    struct Ex10Result ex10_result =
        lbt->get_listen_before_talk_rssi(antenna,
                                         frequency_khz,
                                         lbt_offset,
                                         rssi_count,
                                         override_used,
                                         &lbt_rx_gains,
                                         &lbt_rssi);
    if (ex10_result.error)
    {
        return EX10_RSSI_INVALID;
    }
    return lbt_rssi;
}

static struct RxGainControlFields const* get_current_analog_rx_fields(void)
{
    return &reader.stored_analog_rx_fields;
}

static struct ContinuousInventoryState volatile const*
    get_continuous_inventory_state(void)
{
    return &(reader.inventory_state);
}

struct Ex10Reader const* get_ex10_reader(void)
{
    static struct Ex10Reader reader_instance = {
        .init                           = init,
        .init_ex10                      = init_ex10,
        .read_calibration               = read_calibration,
        .deinit                         = deinit,
        .continuous_inventory           = continuous_inventory,
        .inventory                      = inventory,
        .interrupt_handler              = interrupt_handler,
        .fifo_data_handler              = fifo_data_handler,
        .packet_peek                    = packet_peek,
        .packet_remove                  = packet_remove,
        .packets_available              = packets_available,
        .continue_from_halted           = continue_from_halted,
        .cw_test                        = cw_test,
        .prbs_test                      = prbs_test,
        .ber_test                       = ber_test,
        .etsi_burst_test                = etsi_burst_test,
        .insert_fifo_event              = insert_fifo_event,
        .enable_sdd_logs                = enable_sdd_logs,
        .stop_transmitting              = stop_transmitting,
        .build_cw_configs               = build_cw_configs,
        .get_current_compensated_rssi   = get_current_compensated_rssi,
        .get_current_rssi_log2          = get_current_rssi_log2,
        .listen_before_talk_multi       = listen_before_talk_multi,
        .get_listen_before_talk_rssi    = get_listen_before_talk_rssi,
        .get_current_analog_rx_fields   = get_current_analog_rx_fields,
        .get_continuous_inventory_state = get_continuous_inventory_state,
    };

    return &reader_instance;
}
