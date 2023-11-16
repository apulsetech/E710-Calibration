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

/**
 * @file measure_chip_sensors.c
 *
 * @details Ex10 chip sensor measurement example
 *
 * This example measures all of the Aux ADC inputs and the SJC solution
 * determined by the SJC Op inside the reader chip firmware, sweeping TX
 * power and frequency to characterize the behavior of a hardware with an
 * attached antenna or load.
 *
 * This allows characterization of LO and RX power measurements to determine
 * intrinsic self-jammer performance (using a 50 Ohm termination at the antenna
 * port) and performance with a specific antenna.
 *
 * If SJC errors occur, modifications may be made to the cw_on op to ignore
 * those errors, allowing the example to gather data in the invalid
 * configurations.
 *
 * The results are printed in a CSV format so they can be easily analyzed in
 * software like Excel.
 *
 * Console printing can be piped to a file in Linux using the pipe or tee:
 *
 * `` $ ./build/e710_ref_design/examples/measure_chip_sensors.bin |
 *          tee ./measure_chip_sensors_log_1.csv``
 */


#include "board/board_spec.h"
#include "board/ex10_osal.h"
#include "board/time_helpers.h"
#include "calibration.h"
#include "ex10_api/aggregate_op_builder.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init_core.h"
#include "ex10_api/ex10_event_fifo_queue.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/ex10_test.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/rf_mode_definitions.h"

// --- Example settings that are user modifiable

// The region used. This will determine the RF channels list.
static const enum Ex10RegionId region_id = REGION_FCC;
// The antenna port for testing.
static const uint8_t antenna = 1;
// The amount of time to keep Tx Ramped up to power.
static const uint16_t cw_time_on_ms = 50;
// Set to true if errors are to be printed to stderr when encountered.
// Error messages are suppressed so they are not interspersed with CSV data.
static const bool print_errors = false;
// As soon as the AggregateOp is received, exit the EventFifo processing loop
// and move onto the next measurement, not waiting for cw_time_on_ms.
static const bool enable_go_fast = true;
// The RF mode to use. Note: since the carrier is not modulated,
// should not affect results.
static const enum RfModes rf_mode = mode_5;
// Enable Tx power droop compensation during measurements.
// Not recommended in this application. In this application the
// droop compensation will interfere with measurement data.
static const bool droop_comp_enable = false;
// Enable or disable SDD logs. SDD uses the DBG SPI pins on the Impinj Reader
// Chip as a SPI output device.
static const bool enable_sdd_logs = false;

// CW power level sweep settings: [lower_limit ... upper_limit]
static int16_t const  power_tx_cdBm_init     = 0;     // Tx power lower limit.
static int16_t const  power_tx_cdBm_maximum  = 3210;  // Tx power upper limit.
static uint16_t const power_tx_cdB_step_size = 100;   // Tx power increment.

enum AggregateOpDecode
{
    AggregateOpDecodeNever   = 0,
    AggregateOpDecodeOnError = 1,
    AggregateOpDecodeAlways  = 2,
};

// Determine whether AggregateOp errors debugging is printed out.
// In normal operation, do not print the AggregateOp debug trace,
// The last OpId and OpsStatus will be logged to the CSV data.
static enum AggregateOpDecode const debug_aggregate_op = AggregateOpDecodeNever;

// --- End of example settings

// When taking ADC measurements, the results will fill this array.
static uint16_t adc_result[AUX_ADC_RESULTS_REG_ENTRIES];

// The ADC measurement start channel must always be zero for the
// power detector indexing to work properly.
static uint16_t const ADC_CHANNEL_START    = 0;
static uint16_t const ADC_CHANNEL_COUNT    = 15;
static uint8_t const  LO_PDET_INDEX_OFFSET = 0u;

static struct Ex10Result handle_aggregate_op(
    struct AggregateOpSummary const* aggregate_op_summary)
{
    bool const error_occurred =
        (aggregate_op_summary->last_inner_op_error != ErrorNone);

    if ((error_occurred && (debug_aggregate_op >= AggregateOpDecodeOnError)) ||
        (debug_aggregate_op >= AggregateOpDecodeAlways))
    {
        ex10_ex_eprintf("Aggregate Op decode: -----------------\n");
        struct Ex10AggregateOpBuilder const* agg_op_builder =
            get_ex10_aggregate_op_builder();
        agg_op_builder->print_aggregate_op_errors(aggregate_op_summary);
    }

    if (error_occurred)
    {
        struct OpsStatusFields const ops_status = {
            .op_id     = aggregate_op_summary->last_inner_op_run,
            .busy      = false,
            .Reserved0 = 0,
            .error     = aggregate_op_summary->last_inner_op_error,
            .rfu       = 0,
        };

        return make_ex10_ops_error(ops_status);
    }

    return make_ex10_success();
}

static void print_csv_header(void)
{
    ex10_ex_printf("Starting test characterization of PDETs and SJCs\n");
    ex10_ex_printf("Carrier frequency (kHz),TX Power Target (cdBm),");
    ex10_ex_printf("LO PDET0,LO PDET1,LO PDET2,LO PDET3,");
    ex10_ex_printf("RX PDET0,RX PDET1,RX PDET2,RX PDET3,");
    ex10_ex_printf("TestMux0,TestMux1,TestMux2,TestMux3,");
    ex10_ex_printf("Temp ADC,LO PDET SUM,RX PDET SUM,");
    ex10_ex_printf("SJC atten,");
    ex10_ex_printf("CDAC I value,CDAC I residue,");
    ex10_ex_printf("CDAC Q value,CDAC Q residue,");

    ex10_ex_printf("coarse atten,tx scalar,dc offset,");
    ex10_ex_printf("LO PDET index,LO PDET target counts,");
    ex10_ex_printf("LO PDET target error,");
    ex10_ex_printf("CW on op error,CW on op error code,");
    ex10_ex_printf("CW on op last op,");
    ex10_ex_printf("aggregate op error,aggregate op error code,");
    ex10_ex_printf("aggregate last op,");

    // CSV columns must end with a trailing comma or it won't parse correctly.
    ex10_ex_printf("\n");
}

static struct Ex10Result measure_and_print_adc_channels(void)
{
    // measure Aux ADC channels
    struct Ex10Result const ex10_result =
        get_ex10_rf_power()->measure_and_read_aux_adc(
            ADC_CHANNEL_START, ADC_CHANNEL_COUNT, adc_result);

    if (print_errors && ex10_result.error)
    {
        ex10_ex_eprintf("measure_and_read_aux_adc failed:\n");
        print_ex10_result(ex10_result);
    }

    // print out Aux ADC channel measurements
    for (uint16_t adc_result_index = 0; adc_result_index < ADC_CHANNEL_COUNT;
         adc_result_index++)
    {
        ex10_ex_printf("%u,", adc_result[adc_result_index]);
    }

    return ex10_result;
}

static void read_and_print_tx_power_regs(void)
{
    // note: cal params are private so we can't access them here.
    struct TxCoarseGainFields                  tx_atten;
    struct TxFineGainFields                    tx_fine_gain;
    struct PowerControlLoopAuxAdcControlFields power_detector_adc;
    struct PowerControlLoopAdcTargetFields     adc_target;
    struct DcOffsetFields                      dc_offset_fields;

    // note: read order matters (maybe some of these fields are longer than 16
    // bits?)
    get_ex10_protocol()->read(&tx_coarse_gain_reg, &tx_atten);
    get_ex10_protocol()->read(&tx_fine_gain_reg, &tx_fine_gain);
    get_ex10_protocol()->read(&power_control_loop_aux_adc_control_reg,
                              &power_detector_adc);
    get_ex10_protocol()->read(&power_control_loop_adc_target_reg, &adc_target);
    get_ex10_protocol()->read(&dc_offset_reg, &dc_offset_fields);

    // this register is a bit enable, do this to decode the index
    uint8_t power_detector_adc_index = UINT8_MAX;
    switch (power_detector_adc.channel_enable_bits)
    {
        case (1u << 0u):
            power_detector_adc_index = 0;
            break;
        case (1u << 1u):
            power_detector_adc_index = 1;
            break;
        case (1u << 2u):
            power_detector_adc_index = 2;
            break;
        case (1u << 3u):
            power_detector_adc_index = 3;
            break;
        default:
            break;
    }

    int16_t const PDET_adc_target_error =
        (int16_t)(adc_target.adc_target_value -
                  adc_result[LO_PDET_INDEX_OFFSET + power_detector_adc_index]);

    ex10_ex_printf("%d,", tx_atten.tx_atten);
    // Note: This is the resulting TX scalar, not the setting from the host
    ex10_ex_printf("%d,", tx_fine_gain.tx_scalar);
    ex10_ex_printf("%d,", dc_offset_fields.offset);
    ex10_ex_printf("%d,", power_detector_adc_index);
    ex10_ex_printf("%d,", adc_target.adc_target_value);
    ex10_ex_printf("%d,", PDET_adc_target_error);
}

static struct Ex10Result run_cw_test(int16_t  power_tx_cdBm,
                                     uint32_t frequency_kHz)
{
    // Take a temperature measurement before running Ex10Test.cw_test().
    // This will enable calibration to work correctly.
    uint16_t                temperature_adc = 0u;
    struct Ex10Result const ex10_result_adc_temperature =
        get_ex10_rf_power()->measure_and_read_adc_temperature(&temperature_adc);
    if (print_errors && ex10_result_adc_temperature.error)
    {
        ex10_ex_eprintf("measure_and_read_adc_temperature() failed:\n");
        print_ex10_result(ex10_result_adc_temperature);
    }

    bool const temp_compensation_enabled =
        (ex10_result_adc_temperature.error == false) &&
        get_ex10_board_spec()->temperature_compensation_enabled(
            temperature_adc);

    if (temp_compensation_enabled == false)
    {
        ex10_ex_eprintf(
            "temp_compensation_enabled = false, temperature_adc = %u\n",
            temperature_adc);
    }

    struct PowerDroopCompensationFields droop_comp =
        get_ex10_rf_power()->get_droop_compensation_defaults();
    droop_comp.enable = droop_comp_enable;
    struct Ex10Result const ex10_result_cw_test =
        get_ex10_test()->cw_test(antenna,
                                 rf_mode,
                                 power_tx_cdBm,
                                 frequency_kHz,
                                 &droop_comp,
                                 temperature_adc,
                                 temp_compensation_enabled);
    if (print_errors && ex10_result_cw_test.error)
    {
        ex10_ex_eprintf("cw_test() failed:\n");
        print_ex10_result(ex10_result_cw_test);
    }

    // Note: The temperature measurement errors are ignored.
    // The test continues with uncompensated Tx output.
    // This return result is used as output data in CSV rows.
    return ex10_result_cw_test;
}

static struct Ex10Result run_cw_test_sequence(int16_t  power_tx_cdBm,
                                              uint32_t frequency_kHz)
{
    struct Ex10Result const ex10_result_cw_test =
        run_cw_test(power_tx_cdBm, frequency_kHz);
    struct Ex10Result ex10_result = ex10_result_cw_test;

    enum OpId      aggregate_last_op_id    = Idle;
    enum OpsStatus aggregate_op_error_code = ErrorNone;
    bool           aggregate_op_error      = false;

    uint32_t const start_time = get_ex10_time_helpers()->time_now();
    while (get_ex10_time_helpers()->time_elapsed(start_time) < cw_time_on_ms)
    {
        // If "fast mode", then exit the event processing loop,
        // once the AggregateOp EventFifo packet is received.
        if (enable_go_fast && (aggregate_last_op_id != Idle))
        {
            break;
        }

        struct EventFifoPacket const* packet =
            get_ex10_event_fifo_queue()->packet_peek();
        if (packet)
        {
            if (packet->packet_type == TxRampDown)
            {
                if (print_errors)
                {
                    // Note: this is a serious error and indicates the test is
                    // not running properly. It should not be ignored.
                    ex10_ex_eprintf("Unexpected TxRampDown encountered\n");
                    return make_ex10_sdk_error(Ex10ModuleApplication,
                                               Ex10InvalidEventFifoPacket);
                }
            }
            else if (packet->packet_type == AggregateOpSummary)
            {
                struct AggregateOpSummary const* summary =
                    &packet->static_data->aggregate_op_summary;
                aggregate_last_op_id    = summary->last_inner_op_run;
                aggregate_op_error_code = summary->last_inner_op_error;
                aggregate_op_error      = (aggregate_op_error_code != 0);

                handle_aggregate_op(&packet->static_data->aggregate_op_summary);
            }

            get_ex10_event_fifo_queue()->packet_remove();
        }
    }

    if (print_errors && (aggregate_last_op_id == Idle))
    {
        // Note: this is a serious error and indicates the test is
        // not running properly. It should not be ignored.
        ex10_ex_eprintf("Expected AggregateOpSummary not encountered\n");
        return make_ex10_sdk_error(Ex10ModuleApplication,
                                   Ex10InvalidEventFifoPacket);
    }

    // print out carrier frequency and TX power target
    ex10_ex_printf("%u,", frequency_kHz);
    ex10_ex_printf("%d,", power_tx_cdBm);

    struct Ex10Result const ex10_result_adc = measure_and_print_adc_channels();
    ex10_result = ex10_result.error ? ex10_result : ex10_result_adc;

    // get SJC solution (SJC was already performed as part of cw_on)
    struct Ex10SjcAccessor const* sjc_accessor = get_ex10_sjc();
    struct SjcResultPair const    sjc_results = sjc_accessor->get_sjc_results();
    struct RxGainControlFields const rx_gain_control =
        get_ex10_protocol()->get_analog_rx_config();

    // print out SJC solution
    ex10_ex_printf("%u,", rx_gain_control.rx_atten);
    ex10_ex_printf("%d,", sjc_results.i.cdac);
    ex10_ex_printf("%d,", sjc_results.i.residue);
    ex10_ex_printf("%d,", sjc_results.q.cdac);
    ex10_ex_printf("%d,", sjc_results.q.residue);

    read_and_print_tx_power_regs();

    ex10_ex_printf("%u,", ex10_result_cw_test.error);
    ex10_ex_printf("%u,", ex10_result_cw_test.device_status.ops_status.error);
    ex10_ex_printf("%u,", ex10_result_cw_test.device_status.ops_status.op_id);
    ex10_ex_printf("%u,", aggregate_op_error);
    ex10_ex_printf("%u,", aggregate_op_error_code);
    ex10_ex_printf("%u,", aggregate_last_op_id);

    ex10_ex_printf("\n");

    // ramp down Tx, flush the event fifo, and move to test the next sequence
    get_ex10_rf_power()->stop_op_and_ramp_down();
    bool const print_packets = false;
    bool const flush_packets = true;
    bool const debug_agg_op  = false;
    ex10_discard_packets(print_packets, flush_packets, debug_agg_op);

    return ex10_result;
}

static struct Ex10Result measure_chip_sensors(void)
{
    struct Ex10Region const* ex10_region =
        get_ex10_regulatory()->get_region(region_id);

    struct Ex10Result ex10_return = make_ex10_success();
    print_csv_header();

    // Sweep through all channels within a region; even the unusable channels.
    for (channel_index_t channel_index = 0u;
         channel_index < ex10_region->regulatory_channels.count;
         channel_index++)
    {
        for (int16_t power_tx_cdBm = power_tx_cdBm_init;
             power_tx_cdBm <= power_tx_cdBm_maximum;
             power_tx_cdBm += power_tx_cdB_step_size)
        {
            uint32_t const frequency_kHz =
                ex10_region->regulatory_channels.start_freq_khz +
                (channel_index * ex10_region->regulatory_channels.spacing_khz);

            struct Ex10Result const ex10_result =
                run_cw_test_sequence(power_tx_cdBm, frequency_kHz);
            ex10_return = ex10_return.error ? ex10_return : ex10_result;
        }
    }

    return ex10_return;
}

/**
 * In this use case, no interrupts are handled apart from processing EventFifo
 * packets.
 *
 * @param irq_status Unused
 * @return bool      true enables EventFifo packet call backs
 *                   on the event_fifo_handler() function.
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
    get_ex10_event_fifo_queue()->list_node_push_back(fifo_buffer_node);
}

static struct Ex10Result init(void)
{
    struct Ex10Result ex10_result = ex10_set_default_gpio_setup();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    get_ex10_event_fifo_queue()->init();

    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();
    get_ex10_calibration()->init(ex10_protocol);

    ex10_result = ex10_protocol->register_fifo_data_callback(fifo_data_handler);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    struct InterruptMaskFields const interrupt_mask = {
        .op_done                 = true,
        .halted                  = false,
        .event_fifo_above_thresh = true,
        .event_fifo_full         = true,
        .inventory_round_done    = false,
        .halted_sequence_done    = false,
        .command_error           = true,
        .aggregate_op_done       = true,
    };
    return ex10_protocol->register_interrupt_callback(interrupt_mask,
                                                      interrupt_handler);
}

static struct LogEnablesFields get_sdd_log_enable_fields(void)
{
    struct LogEnablesFields log_enables;
    ex10_memzero(&log_enables, sizeof(log_enables));

    log_enables.op_logs                     = true;
    log_enables.ramping_logs                = true;
    log_enables.config_logs                 = true;
    log_enables.lmac_logs                   = false;
    log_enables.sjc_solution_logs           = true;
    log_enables.rf_synth_logs               = true;
    log_enables.power_control_solution_logs = true;
    log_enables.aux_logs                    = true;
    log_enables.regulatory_logs             = false;
    log_enables.insert_fifo_event_logs      = true;
    log_enables.host_irq_logs               = false;
    log_enables.aggregate_op_logs           = true;
    log_enables.power_control_trace_logs    = true;

    return log_enables;
}

int main(void)
{
    struct Ex10Result ex10_result =
        ex10_core_board_setup(region_id, DEFAULT_SPI_CLOCK_HZ);
    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_core_board_setup() failed:\n");
        print_ex10_result(ex10_result);
    }

    if (enable_sdd_logs)
    {
        get_ex10_ops()->enable_sdd_logs(get_sdd_log_enable_fields(), 12u);
    }

    if (ex10_result.error == false)
    {
        ex10_result = init();
    }

    if (ex10_result.error == false)
    {
        ex10_result = measure_chip_sensors();
    }

    ex10_core_board_teardown();
    return ex10_result.error ? -1 : 0;
}
