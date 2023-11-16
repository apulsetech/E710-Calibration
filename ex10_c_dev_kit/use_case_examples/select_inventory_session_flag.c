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
 * @file select_inventory_session_flag.c
 * @details Illustrates how to send all tags in a session from inventory
 *          flag state A to state B, or A to B, for a specific session.
 */

#include "board/board_spec.h"
#include "board/ex10_osal.h"
#include "board/time_helpers.h"
#include "calibration.h"
#include "ex10_api/board_init_core.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_event_fifo_queue.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_regulatory/ex10_default_region_names.h"

#include "utils/ex10_inventory_command_line.h"
#include "utils/ex10_select_commands.h"
#include "utils/ex10_use_case_example_errors.h"

static bool volatile select_sent = false;

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

static bool interrupt_handler(struct InterruptStatusFields irq_status)
{
    (void)irq_status;
    return true;
}

// Called by the interrupt handler thread when there is a fifo related
// interrupt.
static void fifo_data_handler(struct FifoBufferNode* fifo_buffer_node)
{
    enum Verbosity const          verbosity    = ex10_command_line_verbosity();
    struct Ex10EventParser const* event_parser = get_ex10_event_parser();
    struct ConstByteSpan          bytes        = fifo_buffer_node->fifo_data;
    while (bytes.length > 0u)
    {
        struct EventFifoPacket const packet =
            event_parser->parse_event_packet(&bytes);

        if (verbosity != SILENCE)
        {
            get_ex10_event_fifo_printer()->print_packets(&packet);
        }

        if (packet.packet_type == Gen2Transaction)
        {
            select_sent = true;
        }
    }

    // Release the fifo_buffer_node back to the free list for reuse.
    get_ex10_fifo_buffer_list()->free_list_put(fifo_buffer_node);
}

int main(int argc, char const* const argv[])
{
    select_sent = false;

    // If autoset_mode_id == 0 then the Autoset mode is determined using
    // region and SKU, unless otherwise specified on the command line.
    struct InventoryOptions inventory_options = {
        .region_name   = "FCC",
        .read_rate     = 0u,
        .antenna       = 1u,
        .frequency_khz = 0u,
        .remain_on     = false,
        .tx_power_cdbm = 3000,
        .mode          = {.rf_mode_id = mode_185},
        .target_spec   = 'A',
        .initial_q     = 0,
        .session       = SessionS2,
    };

    struct Ex10Result const ex10_result_command_line =
        ex10_inventory_parse_command_line(&inventory_options, argv, argc);

    enum Verbosity const verbosity = ex10_command_line_verbosity();
    if (verbosity != SILENCE)
    {
        ex10_print_inventory_command_line_settings(&inventory_options);
    }
    if (ex10_result_command_line.error || ex10_command_line_help_requested())
    {
        return ex10_result_command_line.error ? EINVAL : 0;
    }

    bool const target_check = (inventory_options.target_spec == 'A' ||
                               inventory_options.target_spec == 'B');
    if (target_check == false)
    {
        ex10_ex_eprintf("Invalid target specified: %c; must be 'A' or 'B'\n",
                        inventory_options.target_spec);
        return EINVAL;
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

    // The return value from main().
    int result = 0;

    ex10_result = ex10_set_default_gpio_setup();
    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_set_default_gpio_setup() failed:\n");
        print_ex10_result(ex10_result);
        result = -1;
    }

    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();
    get_ex10_calibration()->init(ex10_protocol);

    get_ex10_event_fifo_queue()->init();
    get_ex10_gen2_tx_command_manager()->init();

    ex10_protocol->register_fifo_data_callback(fifo_data_handler);

    struct InterruptMaskFields const interrupt_mask = {
        .op_done                 = true,
        .halted                  = false,
        .event_fifo_above_thresh = true,
        .event_fifo_full         = true,
        .inventory_round_done    = false,
        .halted_sequence_done    = false,
        .command_error           = false,
        .aggregate_op_done       = false,
    };
    ex10_result = ex10_protocol->register_interrupt_callback(interrupt_mask,
                                                             interrupt_handler);
    if (ex10_result.error)
    {
        ex10_ex_eprintf("register_interrupt_callback() failed:\n");
        print_ex10_result(ex10_result);
        result = -1;
    }

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
        ex10_ex_printf("Sending select command: target: %c, session: %u\n",
                       (target == target_A) ? 'A' : 'B',
                       inventory_options.session);

        ex10_result = send_initial_select(inventory_options.antenna,
                                          inventory_options.mode.rf_mode_id,
                                          inventory_options.frequency_khz,
                                          inventory_options.tx_power_cdbm);

        if (ex10_result.error)
        {
            result = -1;
            print_ex10_app_result(ex10_result);
        }
    }

    uint32_t const timeout_ms    = 1000u;
    uint32_t const start_time_ms = get_ex10_time_helpers()->time_now();
    while (select_sent == false &&
           get_ex10_time_helpers()->time_elapsed(start_time_ms) < timeout_ms)
    {
    }

    if (select_sent == false)
    {
        ex10_ex_eprintf("No  Gen2Transaction packet found\n");
        result = -1;
    }

    ex10_result = get_ex10_rf_power()->cw_off();
    if (ex10_result.error)
    {
        result = (result == 0) ? -1 : result;
    }

    ex10_core_board_teardown();

    ex10_ex_printf("Select example done: %d\n", result);
    return result;
}
