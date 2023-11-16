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

#include <string.h>
#include <sys/types.h>

#include "board/board_spec.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/fifo_buffer_list.h"
#include "ex10_api/sjc_accessor.h"
#include "ex10_api/trace.h"

static struct Ex10Result radio_power_control(bool enable);
static struct Ex10Result wait_op_completion(void);
static struct Ex10Result wait_op_completion_with_timeout(uint32_t);

static void init(void) {}

static void release(void) {}

static struct Ex10Result wait_op_completion(void)
{
    return get_ex10_protocol()->wait_op_completion();
}

static struct Ex10Result wait_op_completion_with_timeout(uint32_t timeout_ms)
{
    return get_ex10_protocol()->wait_op_completion_with_timeout(timeout_ms);
}

static struct OpsStatusFields read_ops_status(void)
{
    struct OpsStatusFields ops_status;
    get_ex10_protocol()->read(&ops_status_reg, &ops_status);
    return ops_status;
}

static struct Ex10Result start_log_test(uint32_t period, uint16_t repeat)
{
    struct LogTestPeriodFields const     log_period      = {.period = period};
    struct LogTestWordRepeatFields const log_word_repeat = {.repeat = repeat};
    struct OpsControlFields const ops_control_data       = {.op_id = LogTestOp};

    struct RegisterInfo const* const regs[] = {
        &log_test_period_reg,
        &log_test_word_repeat_reg,
        &ops_control_reg,
    };

    void const* buffers[] = {
        &log_period,
        &log_word_repeat,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result set_atest_mux(uint32_t atest_mux_0,
                                       uint32_t atest_mux_1,
                                       uint32_t atest_mux_2,
                                       uint32_t atest_mux_3)
{
    uint32_t const atest_mux[] = {
        atest_mux_0, atest_mux_1, atest_mux_2, atest_mux_3};
    struct OpsControlFields const ops_control_data = {.op_id = SetATestMuxOp};

    struct RegisterInfo const* const regs[] = {
        &a_test_mux_reg,
        &ops_control_reg,
    };
    void const* buffers[] = {
        atest_mux,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result route_atest_pga3(void)
{
    uint32_t const    atest_pga3 = (1u << 4u);
    struct Ex10Result ex10_result =
        set_atest_mux(atest_pga3, atest_pga3, atest_pga3, atest_pga3);
    if (ex10_result.error)
    {
        return ex10_result;
    }
    return get_ex10_protocol()->wait_op_completion();
}

static struct Ex10Result measure_aux_adc(
    enum AuxAdcResultsAdcResult adc_channel_start,
    uint8_t                     num_channels)
{
    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();

    // Limit the number of ADC conversion channels to the possible range.
    if (adc_channel_start >= aux_adc_results_reg.num_entries)
    {
        return make_ex10_sdk_error(Ex10ModuleOps, Ex10SdkErrorBadParamValue);
    }

    uint8_t const max_channels =
        aux_adc_results_reg.num_entries - (uint8_t)adc_channel_start;
    num_channels = (num_channels <= max_channels) ? num_channels : max_channels;

    uint16_t const channel_enable_bits =
        (uint16_t)(((1u << num_channels) - 1u) << adc_channel_start);
    struct AuxAdcControlFields const adc_control = {
        .channel_enable_bits = channel_enable_bits, .rfu = 0u};

    struct RegisterInfo const* const regs[] = {
        &aux_adc_control_reg,
        &ops_control_reg,
    };

    struct OpsControlFields const ops_control_data = {.op_id = MeasureAdcOp};

    void const* buffers[] = {
        &adc_control,
        &ops_control_data,
    };

    // Run the MeasureAdcOp
    return ex10_protocol->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result set_aux_dac(uint8_t         dac_channel_start,
                                     uint8_t         num_channels,
                                     uint16_t const* dac_values)
{
    // Limit the number of ADC conversion channels to the possible range.
    if (dac_channel_start >= aux_dac_settings_reg.num_entries)
    {
        return make_ex10_sdk_error(Ex10ModuleOps, Ex10SdkErrorBadParamValue);
    }

    uint8_t const max_channels =
        aux_dac_settings_reg.num_entries - dac_channel_start;
    num_channels = (num_channels <= max_channels) ? num_channels : max_channels;

    uint8_t const channel_enable_bits =
        (uint8_t)(((1u << num_channels) - 1u) << dac_channel_start);
    struct AuxDacControlFields const dac_control = {
        .channel_enable_bits = channel_enable_bits, .rfu = 0u};

    // Create new reg info based off the subset of entries to use
    uint16_t const offset = dac_channel_start * aux_dac_settings_reg.length;
    struct RegisterInfo const dac_settings_reg = {
        .address     = aux_dac_settings_reg.address + offset,
        .length      = aux_dac_settings_reg.length,
        .num_entries = num_channels,
        .access      = ReadOnly,
    };

    struct OpsControlFields const ops_control_data = {.op_id = SetDacOp};

    struct RegisterInfo const* const regs[] = {
        &aux_dac_control_reg,
        &dac_settings_reg,
        &ops_control_reg,
    };

    void const* buffers[] = {
        &dac_control,
        dac_values,
        &ops_control_data,
    };

    // Run the SetDacOp
    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result set_rf_mode(enum RfModes mode)
{
    struct RfModeFields const        rf_mode = {.id = (uint16_t)mode};
    struct RegisterInfo const* const regs[]  = {&rf_mode_reg, &ops_control_reg};
    struct OpsControlFields const    ops_control_data = {.op_id = SetRfModeOp};

    void const* buffers[] = {
        &rf_mode,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result tx_ramp_up(int32_t dc_offset)
{
    struct DcOffsetFields const   offset           = {.offset = dc_offset};
    struct OpsControlFields const ops_control_data = {.op_id = TxRampUpOp};

    struct RegisterInfo const* const regs[] = {
        &dc_offset_reg,
        &ops_control_reg,
    };

    void const* buffers[] = {
        &offset,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result tx_ramp_down(void)
{
    return get_ex10_protocol()->start_op(TxRampDownOp);
}

static struct Ex10Result set_tx_coarse_gain(uint8_t tx_atten)
{
    struct TxCoarseGainFields const coarse_gain      = {.tx_atten = tx_atten};
    struct OpsControlFields const   ops_control_data = {.op_id =
                                                          SetTxCoarseGainOp};

    struct RegisterInfo const* const regs[] = {
        &tx_coarse_gain_reg,
        &ops_control_reg,
    };

    void const* buffers[] = {
        &coarse_gain,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result set_tx_fine_gain(int16_t tx_scalar)
{
    struct TxFineGainFields const tx_fine_gain     = {.tx_scalar = tx_scalar};
    struct OpsControlFields const ops_control_data = {.op_id = SetTxFineGainOp};

    struct RegisterInfo const* const regs[] = {
        &tx_fine_gain_reg,
        &ops_control_reg,
    };

    void const* buffers[] = {
        &tx_fine_gain,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result radio_power_control(bool enable)
{
    struct AnalogEnableFields const analog_enable    = {.all = enable};
    struct OpsControlFields const   ops_control_data = {
        .op_id = RadioPowerControlOp,
    };

    struct RegisterInfo const* const regs[] = {
        &analog_enable_reg,
        &ops_control_reg,
    };

    void const* buffers[] = {
        &analog_enable,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result set_analog_rx_config(
    struct RxGainControlFields const* analog_rx_fields)
{
    if (analog_rx_fields == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleOps, Ex10SdkErrorNullPointer);
    }

    struct RegisterInfo const* const regs[] = {
        &rx_gain_control_reg,
        &ops_control_reg,
    };

    struct OpsControlFields const ops_control_data = {.op_id = SetRxGainOp};

    void const* buffers[] = {
        analog_rx_fields,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result measure_rssi(uint8_t rssi_count)
{
    struct MeasureRssiCountFields const rssi_count_fields = {.samples =
                                                                 rssi_count};

    struct RegisterInfo const* const regs[] = {
        &measure_rssi_count_reg,
        &ops_control_reg,
    };

    struct OpsControlFields const ops_control_data = {.op_id = MeasureRssiOp};

    void const* buffers[] = {
        &rssi_count_fields,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result run_listen_before_talk(
    struct LbtControlFields const*           lbt_settings,
    struct RxGainControlFields const*        used_rx_gains,
    struct LbtOffsetFields const*            lbt_offsets,
    struct RfSynthesizerControlFields const* rf_synth_control,
    uint8_t                                  rssi_count)
{
    // For the RSSI measurement, we need to set the RSSI count register
    struct MeasureRssiCountFields const rssi_count_fields = {
        .samples = rssi_count,
    };
    struct OpsControlFields const ops_control_data = {
        .op_id = ListenBeforeTalkOp,
    };

    struct RegisterInfo const* const regs[] = {
        &lbt_control_reg,
        &rx_gain_control_reg,
        &lbt_offset_reg,
        &rf_synthesizer_control_reg,
        &measure_rssi_count_reg,
        &ops_control_reg,
    };

    void const* buffers[] = {
        lbt_settings,
        used_rx_gains,
        lbt_offsets,
        rf_synth_control,
        &rssi_count_fields,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result start_timer_op(uint32_t delay_us)
{
    struct DelayUsFields          delay_fields     = {.delay = delay_us};
    struct OpsControlFields const ops_control_data = {.op_id = UsTimerStartOp};

    struct RegisterInfo const* const regs[] = {
        &delay_us_reg,
        &ops_control_reg,
    };

    void const* buffers[] = {
        &delay_fields,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result wait_timer_op(void)
{
    struct OpsControlFields const ops_control_data = {.op_id = UsTimerWaitOp};
    return get_ex10_protocol()->write(&ops_control_reg, &ops_control_data);
}

static struct Ex10Result lock_synthesizer(uint8_t  r_divider_index,
                                          uint16_t n_divider)
{
    struct RfSynthesizerControlFields const synth_control = {
        .n_divider = n_divider, .r_divider = r_divider_index, .lf_type = 1u};
    struct RfSynthesizerControlFields
        synth_control_array[RF_SYNTHESIZER_CONTROL_REG_ENTRIES];
    synth_control_array[0] = synth_control;
    synth_control_array[1] = synth_control;
    synth_control_array[2] = synth_control;
    synth_control_array[3] = synth_control;
    synth_control_array[4] = synth_control;

    struct OpsControlFields const ops_control_data = {.op_id =
                                                          LockSynthesizerOp};

    struct RegisterInfo const* const regs[] = {
        &rf_synthesizer_control_reg,
        &ops_control_reg,
    };

    void const* buffers[] = {
        synth_control_array,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result start_event_fifo_test(uint32_t period,
                                               uint8_t  num_words)
{
    struct EventFifoTestPeriodFields const test_period = {.period = period};
    struct EventFifoTestPayloadNumWordsFields const payload_words = {
        .num_words = num_words};
    struct OpsControlFields const ops_control_data = {.op_id = EventFifoTestOp};

    struct RegisterInfo const* const regs[] = {
        &event_fifo_test_period_reg,
        &event_fifo_test_payload_num_words_reg,
        &ops_control_reg,
    };

    void const* buffers[] = {
        &test_period,
        &payload_words,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result enable_sdd_logs(const struct LogEnablesFields enables,
                                         const uint8_t speed_mhz)
{
    struct RegisterInfo const* const regs[] = {
        &log_enables_reg,
        &log_speed_reg,
    };

    struct LogSpeedFields log_speed = {.speed_mhz = speed_mhz, .rfu = 0u};

    void const* buffers[] = {
        &enables,
        &log_speed,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result send_gen2_halted_sequence(void)
{
    struct HaltedControlFields halted_controls = {
        .go = true, .resume = false, .nak_tag = false};
    return get_ex10_protocol()->write(&halted_control_reg, &halted_controls);
}

static struct Ex10Result continue_from_halted(bool nak)
{
    struct HaltedControlFields halted_controls = {
        .go = false, .resume = true, .nak_tag = nak};
    return get_ex10_protocol()->write(&halted_control_reg, &halted_controls);
}

static struct Ex10Result run_sjc(void)
{
    return get_ex10_protocol()->start_op(RxRunSjcOp);
}

static struct Ex10Result stop_op(void)
{
    return get_ex10_protocol()->stop_op();
}

static uint32_t read_gpio_output_enables(void)
{
    uint32_t output_enable = 0u;
    get_ex10_protocol()->read(&gpio_output_enable_reg, &output_enable);
    return output_enable;
}

static uint32_t read_gpio_output_levels(void)
{
    uint32_t output_level = 0u;
    get_ex10_protocol()->read(&gpio_output_level_reg, &output_level);
    return output_level;
}

static uint32_t get_device_time(void)
{
    uint32_t dev_time = 0u;
    get_ex10_protocol()->read(&timestamp_reg, &dev_time);
    return dev_time;
}

static struct GpioControlFields get_gpio(void)
{
    struct GpioControlFields const gpio_control = {
        .output_enable = read_gpio_output_enables(),
        .output_level  = read_gpio_output_levels()};

    return gpio_control;
}

static struct Ex10Result set_gpio(uint32_t gpio_levels, uint32_t gpio_enables)
{
    struct RegisterInfo const* const regs[] = {
        &gpio_output_enable_reg,
        &gpio_output_level_reg,
        &ops_control_reg,
    };

    struct OpsControlFields const ops_control_data = {.op_id = SetGpioOp};

    void const* buffers[] = {
        &gpio_enables,
        &gpio_levels,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result set_clear_gpio_pins(
    struct GpioPinsSetClear const* gpio_pins_set_clear)
{
    struct RegisterInfo const* const regs[] = {
        &gpio_output_level_set_reg,
        &gpio_output_level_clear_reg,
        &gpio_output_enable_set_reg,
        &gpio_output_enable_clear_reg,
        &ops_control_reg,
    };

    struct OpsControlFields const ops_control_data = {
        .op_id = SetClearGpioPinsOp,
    };

    void const* buffers[] = {
        &gpio_pins_set_clear->output_level_set,
        &gpio_pins_set_clear->output_level_clear,
        &gpio_pins_set_clear->output_enable_set,
        &gpio_pins_set_clear->output_enable_clear,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result start_inventory_round(
    struct InventoryRoundControlFields const*   configs,
    struct InventoryRoundControl_2Fields const* configs_2)
{
    tracepoint(pi_ex10sdk, OPS_start_inventory_round, configs, configs_2);

    struct RegisterInfo const* const regs[] = {
        &inventory_round_control_reg,
        &inventory_round_control_2_reg,
        &ops_control_reg,
    };

    struct OpsControlFields const ops_control_data = {
        .op_id = StartInventoryRoundOp};

    void const* buffers[] = {
        configs,
        configs_2,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result start_prbs(void)
{
    return get_ex10_protocol()->start_op(RunPrbsDataOp);
}

static struct Ex10Result start_hpf_override_test_op(
    struct HpfOverrideSettingsFields const* hpf_settings)
{
    struct RegisterInfo const* const regs[] = {
        &hpf_override_settings_reg,
        &ops_control_reg,
    };

    struct OpsControlFields const ops_control_data = {.op_id =
                                                          HpfOverrideTestOp};

    void const* buffers[] = {
        hpf_settings,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result start_ber_test(uint16_t num_bits,
                                        uint16_t num_packets,
                                        bool     delimiter_only)
{
    // Determine whether to use a delimiter only instead of a full query
    struct BerModeFields ber_mode = {.del_only_mode = delimiter_only};

    struct BerControlFields ber_control = {.num_bits    = num_bits,
                                           .num_packets = num_packets};

    struct RegisterInfo const* const regs[] = {
        &ber_mode_reg,
        &ber_control_reg,
        &ops_control_reg,
    };

    struct OpsControlFields const ops_control_data = {.op_id = BerTestOp};

    void const* buffers[] = {
        &ber_mode,
        &ber_control,
        &ops_control_data,
    };

    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result send_select(void)
{
    tracepoint(pi_ex10sdk, OPS_send_select);

    // Send the select
    return get_ex10_protocol()->start_op(SendSelectOp);
}

static struct Ex10Result run_aggregate_op(void)
{
    // Start the op
    return get_ex10_protocol()->start_op(AggregateOp);
}

static struct Ex10Result run_power_control_loop(
    struct PowerConfigs* power_config)
{
    // Adc target 0 means we don't intend to run power control
    if (power_config->adc_target == 0)
    {
        return make_ex10_sdk_error(Ex10ModuleOps, Ex10SdkErrorBadParamValue);
    }

    // Registers used to configure the power control loop
    struct PowerControlLoopAuxAdcControlFields const adc_control = {
        .channel_enable_bits = (uint16_t)power_config->power_detector_adc};
    struct PowerControlLoopGainDivisorFields const gain_divisor = {
        .gain_divisor = power_config->loop_gain_divisor};
    struct PowerControlLoopMaxIterationsFields const max_iterations = {
        .max_iterations = power_config->max_iterations};
    struct PowerControlLoopAdcTargetFields const adc_target = {
        .adc_target_value = power_config->adc_target};
    struct PowerControlLoopAdcThresholdsFields const adc_thresholds = {
        .loop_stop_threshold = power_config->loop_stop_threshold,
        .op_error_threshold  = power_config->op_error_threshold};
    struct OpsControlFields const ops_control_data = {.op_id =
                                                          PowerControlLoopOp};

    struct RegisterInfo const* const regs[] = {
        &power_control_loop_aux_adc_control_reg,
        &power_control_loop_gain_divisor_reg,
        &power_control_loop_max_iterations_reg,
        &power_control_loop_adc_target_reg,
        &power_control_loop_adc_thresholds_reg,
        &ops_control_reg,
    };

    void const* buffers[] = {&adc_control,
                             &gain_divisor,
                             &max_iterations,
                             &adc_target,
                             &adc_thresholds,
                             &ops_control_data};

    // Write the controls for the power control loop and start the op
    return get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
}

static struct Ex10Result run_etsi_burst(void)
{
    return get_ex10_protocol()->start_op(EtsiBurstOp);
}

static struct Ex10Ops const ex10_ops = {
    .init                            = init,
    .release                         = release,
    .read_ops_status                 = read_ops_status,
    .start_log_test                  = start_log_test,
    .set_atest_mux                   = set_atest_mux,
    .route_atest_pga3                = route_atest_pga3,
    .measure_aux_adc                 = measure_aux_adc,
    .set_aux_dac                     = set_aux_dac,
    .set_rf_mode                     = set_rf_mode,
    .tx_ramp_up                      = tx_ramp_up,
    .tx_ramp_down                    = tx_ramp_down,
    .set_tx_coarse_gain              = set_tx_coarse_gain,
    .set_tx_fine_gain                = set_tx_fine_gain,
    .radio_power_control             = radio_power_control,
    .set_analog_rx_config            = set_analog_rx_config,
    .measure_rssi                    = measure_rssi,
    .run_listen_before_talk          = run_listen_before_talk,
    .start_timer_op                  = start_timer_op,
    .wait_timer_op                   = wait_timer_op,
    .lock_synthesizer                = lock_synthesizer,
    .start_event_fifo_test           = start_event_fifo_test,
    .enable_sdd_logs                 = enable_sdd_logs,
    .send_gen2_halted_sequence       = send_gen2_halted_sequence,
    .continue_from_halted            = continue_from_halted,
    .run_sjc                         = run_sjc,
    .wait_op_completion              = wait_op_completion,
    .wait_op_completion_with_timeout = wait_op_completion_with_timeout,
    .run_aggregate_op                = run_aggregate_op,
    .stop_op                         = stop_op,
    .get_gpio                        = get_gpio,
    .set_gpio                        = set_gpio,
    .set_clear_gpio_pins             = set_clear_gpio_pins,
    .start_inventory_round           = start_inventory_round,
    .start_prbs                      = start_prbs,
    .start_hpf_override_test_op      = start_hpf_override_test_op,
    .run_etsi_burst                  = run_etsi_burst,
    .start_ber_test                  = start_ber_test,
    .send_select                     = send_select,
    .get_device_time                 = get_device_time,
    .run_power_control_loop          = run_power_control_loop,
};

struct Ex10Ops const* get_ex10_ops(void)
{
    return &ex10_ops;
}
