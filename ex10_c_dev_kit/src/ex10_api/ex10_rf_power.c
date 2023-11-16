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

#include "ex10_api/ex10_rf_power.h"

#include "board/board_spec.h"
#include "board/ex10_osal.h"
#include "board/ex10_rx_baseband_filter.h"
#include "calibration.h"
#include "ex10_api/aggregate_op_builder.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/trace.h"
#include "ex10_api/version_info.h"
#include "ex10_modules/ex10_ramp_module_manager.h"

/// Tx power droop compensation with 25ms interval and .01dB step.
static struct PowerDroopCompensationFields droop_comp_defaults = {
    .enable                   = true,
    .compensation_interval_ms = 25,
    .fine_gain_step_cd_b      = 10,
};

static struct Ex10Result set_analog_rx_config(
    struct RxGainControlFields const* analog_rx_fields)
{
    if (analog_rx_fields == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleRfPower, Ex10SdkErrorNullPointer);
    }

    // update the device settings
    return get_ex10_ops()->set_analog_rx_config(analog_rx_fields);
}

static struct Ex10Result init_ex10(void)
{
    struct Ex10Ops const* ops = get_ex10_ops();

    // Enable the Ex10 analog power supplies by running the RadioPowerControlOp.
    struct Ex10Result ex10_result = ops->radio_power_control(true);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // A cached copy of the RxGainControl settings.
    ex10_result = set_analog_rx_config(
        get_ex10_board_spec()->get_default_rx_analog_config());
    if (ex10_result.error)
    {
        return ex10_result;
    }

    return ops->wait_op_completion();
}

static bool get_cw_is_on(void)
{
    struct CwIsOnFields cw_is_on;
    get_ex10_protocol()->read(&cw_is_on_reg, &cw_is_on);
    return cw_is_on.is_on;
}

static struct Ex10Result cw_off(void)
{
    tracepoint(pi_ex10sdk, OPS_cw_off_manual);
    return get_ex10_ops()->tx_ramp_down();
}

static struct Ex10Result stop_op_and_ramp_down(void)
{
    struct Ex10Ops const* ops = get_ex10_ops();

    struct Ex10Result ex10_result = ops->stop_op();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }
    ex10_result = cw_off();
    if (ex10_result.error)
    {
        return ex10_result;
    }
    return ops->wait_op_completion();
}

static void set_regulatory_timers(
    struct Ex10RegulatoryTimers const* timer_config)
{
    struct NominalStopTimeFields const nominal_timer = {
        .dwell_time = timer_config->nominal_ms};
    struct ExtendedStopTimeFields const extended_timer = {
        .dwell_time = timer_config->extended_ms};
    struct RegulatoryStopTimeFields const regulatory_timer = {
        .dwell_time = timer_config->regulatory_ms};
    // ETSI burst ramps up and down on the same channel
    struct EtsiBurstOffTimeFields const off_timer = {
        .off_time = timer_config->off_same_channel_ms, .rfu = 0u};

    struct RegisterInfo const* const regs[] = {
        &nominal_stop_time_reg,
        &extended_stop_time_reg,
        &regulatory_stop_time_reg,
        &etsi_burst_off_time_reg,
    };
    void const* buffers[] = {
        &nominal_timer,
        &extended_timer,
        &regulatory_timer,
        &off_timer,
    };

    get_ex10_protocol()->write_multiple(
        regs, buffers, sizeof(regs) / sizeof(void const*));
}

static struct Ex10Result ramp_transmit_power(
    struct PowerConfigs*               power_config,
    struct Ex10RegulatoryTimers const* timer_config)
{
    struct Ex10Ops const* ops = get_ex10_ops();
    ops->set_tx_coarse_gain(power_config->tx_atten);

    struct Ex10Result ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->set_tx_fine_gain(power_config->tx_scalar);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    set_regulatory_timers(timer_config);
    ex10_result = ops->tx_ramp_up(power_config->dc_offset);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    return ops->run_power_control_loop(power_config);
}

static struct Ex10Result measure_and_read_aux_adc(
    enum AuxAdcResultsAdcResult adc_channel_start,
    uint8_t                     num_channels,
    uint16_t*                   adc_results)
{
    struct Ex10Result ex10_result =
        get_ex10_ops()->measure_aux_adc(adc_channel_start, num_channels);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Wait for completion so we can read from the ADC
    ex10_result = get_ex10_ops()->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Read out the adc count from the device
    uint16_t const offset =
        (uint16_t)adc_channel_start * aux_adc_results_reg.length;
    struct RegisterInfo const adc_results_reg = {
        .address     = aux_adc_results_reg.address + offset,
        .length      = aux_adc_results_reg.length,
        .num_entries = num_channels,
        .access      = ReadOnly,
    };
    get_ex10_protocol()->read(&adc_results_reg, adc_results);
    return ex10_result;
}

static struct Ex10Result measure_and_read_adc_temperature(uint16_t* temp_adc)
{
    enum AuxAdcResultsAdcResult adc_channel_start = AdcResultTemperature;
    uint8_t                     num_channels      = 1u;
    return measure_and_read_aux_adc(adc_channel_start, num_channels, temp_adc);
}

static struct Ex10Result set_rf_mode(enum RfModes rf_mode)
{

    struct Ex10Ops const* ops = get_ex10_ops();

    // set the mode in the Ex10 device modem
    ops->set_rf_mode(rf_mode);
    struct Ex10Result ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // set the GPIO pin for the DRM mode.
    const enum BasebandFilterType rx_baseband_filter =
        get_ex10_rx_baseband_filter()->choose_rx_baseband_filter(rf_mode);
    struct Ex10GpioHelpers const* gpio_helpers  = get_ex10_gpio_helpers();
    struct GpioPinsSetClear gpio_pins_set_clear = {.output_enable_clear = 0,
                                                   .output_enable_set   = 0,
                                                   .output_level_clear  = 0,
                                                   .output_level_set    = 0};

    ex10_result = gpio_helpers->set_rx_baseband_filter(&gpio_pins_set_clear,
                                                       rx_baseband_filter);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->set_clear_gpio_pins(&gpio_pins_set_clear);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    tracepoint(pi_ex10sdk, OPS_set_rf_mode, rf_mode, &gpio_pins_set_clear);

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    return make_ex10_success();
}

static struct Ex10Result build_cw_configs(uint8_t          antenna,
                                          enum RfModes     rf_mode,
                                          int16_t          tx_power_cdbm,
                                          uint16_t         temperature_adc,
                                          bool             temp_comp_enabled,
                                          struct CwConfig* cw_config)
{
    struct SynthesizerParams synth_params;
    ex10_memzero(&synth_params, sizeof(synth_params));

    const struct Ex10ActiveRegion* region = get_ex10_active_region();

    uint32_t frequency_khz = region->get_next_channel_khz();
    region->get_synthesizer_params(frequency_khz, &synth_params);

    cw_config->synth.r_divider = synth_params.r_divider_index;
    cw_config->synth.n_divider = synth_params.n_divider;
    cw_config->synth.lf_type   = true;

    struct Ex10Result ex10_result =
        region->get_next_channel_regulatory_timers(&cw_config->timer);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // this is redundant as the set RF mode will have set this too
    // but at this point it doesn't really do any harm.  Maybe
    // we update the board spec API in the future ?
    const enum BasebandFilterType rx_baseband_filter =
        get_ex10_rx_baseband_filter()->choose_rx_baseband_filter(rf_mode);

    ex10_result = get_ex10_board_spec()->get_gpio_output_pins_set_clear(
        &cw_config->gpio,
        antenna,
        tx_power_cdbm,
        rx_baseband_filter,
        region->get_rf_filter());
    if (ex10_result.error)
    {
        return ex10_result;
    }

    cw_config->rf_mode = rf_mode;


    cw_config->power = get_ex10_calibration()->get_power_control_params(
        tx_power_cdbm,
        frequency_khz,
        temperature_adc,
        temp_comp_enabled,
        region->get_rf_filter());

    return make_ex10_success();
}

static struct Ex10Result cw_on(
    struct GpioPinsSetClear const*             gpio_controls,
    struct PowerConfigs*                       power_config,
    struct RfSynthesizerControlFields const*   synth_control,
    struct Ex10RegulatoryTimers const*         timer_config,
    struct PowerDroopCompensationFields const* droop_comp)
{
    // Prevent redundant CW on
    if (get_cw_is_on())
    {
        // CW is already on, early return
        return make_ex10_success();
    }

    uint8_t agg_data[AGGREGATE_OP_BUFFER_REG_LENGTH];
    ex10_memzero(agg_data, sizeof(agg_data));
    struct ByteSpan agg_buffer = {.data = agg_data, .length = 0};
    struct Ex10AggregateOpBuilder const* agg_builder =
        get_ex10_aggregate_op_builder();

    tracepoint(pi_ex10sdk,
               OPS_cw_on,
               gpio_controls,
               power_config,
               synth_control,
               timer_config);

    if (!agg_builder->append_set_clear_gpio_pins(gpio_controls, &agg_buffer))
    {
        return make_ex10_sdk_error(Ex10ModuleRfPower,
                                   Ex10SdkErrorAggBufferOverflow);
    }

    if (!agg_builder->append_lock_synthesizer(
            synth_control->r_divider, synth_control->n_divider, &agg_buffer))
    {
        return make_ex10_sdk_error(Ex10ModuleRfPower,
                                   Ex10SdkErrorAggBufferOverflow);
    }

    // If the coarse gain has changes, store the new val and run the op
    if (!agg_builder->append_set_tx_coarse_gain(power_config->tx_atten,
                                                &agg_buffer))
    {
        return make_ex10_sdk_error(Ex10ModuleRfPower,
                                   Ex10SdkErrorAggBufferOverflow);
    }

    if (!agg_builder->append_set_tx_fine_gain(power_config->tx_scalar,
                                              &agg_buffer))
    {
        return make_ex10_sdk_error(Ex10ModuleRfPower,
                                   Ex10SdkErrorAggBufferOverflow);
    }

    if (!agg_builder->append_set_regulatory_timers(timer_config, &agg_buffer))
    {
        return make_ex10_sdk_error(Ex10ModuleRfPower,
                                   Ex10SdkErrorAggBufferOverflow);
    }

    if (!agg_builder->append_droop_compensation(droop_comp, &agg_buffer))
    {
        return make_ex10_sdk_error(Ex10ModuleRfPower,
                                   Ex10SdkErrorAggBufferOverflow);
    }

    // If there is an off time to observe, we will insert the timer here.
    // Note that this off time is not the region default, this is specifically
    // after any additional has been checked in the Ex10Regulatory layer to
    // ensure this time is needed.
    if (timer_config->off_same_channel_ms)
    {
        if (!agg_builder->append_start_timer_op(
                timer_config->off_same_channel_ms * 1000, &agg_buffer) ||
            !agg_builder->append_wait_timer_op(&agg_buffer))
        {
            return make_ex10_sdk_error(Ex10ModuleRfPower,
                                       Ex10SdkErrorAggBufferOverflow);
        }
    }

    if (!agg_builder->append_tx_ramp_up(power_config->dc_offset, &agg_buffer) ||
        !agg_builder->append_power_control(power_config, &agg_buffer) ||
        !agg_builder->append_run_sjc(&agg_buffer))
    {
        return make_ex10_sdk_error(Ex10ModuleRfPower,
                                   Ex10SdkErrorAggBufferOverflow);
    }

    struct Ex10RampModuleManager const* ramp_module_manager =
        get_ex10_ramp_module_manager();

    struct Ex10Result ex10_result =
        ramp_module_manager->call_pre_ramp_callback();
    if (ex10_result.error)
    {
        // Note, if there are any issues in the callback, appropriate error
        // handling should happen there. The callback has the  context to
        // perform actions befitting the error. Meanwhile we will mark an
        // error occurred here for the user.
        return ex10_result;
    }

    // Add the exit instruction and set the buffer
    agg_builder->append_exit_instruction(&agg_buffer);
    agg_builder->set_buffer(&agg_buffer);

    // Run the aggregate op and wait for completion
    struct Ex10Ops const* ops = get_ex10_ops();
    ex10_result               = ops->run_aggregate_op();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Updating to the next channel for the next CwOn
    get_ex10_active_region()->update_active_channel();

    ex10_result = ramp_module_manager->call_post_ramp_callback();
    if (ex10_result.error)
    {
        // Note, if there are any issues in the callback, appropriate error
        // handling should happen there. The callback has the  context to
        // perform actions befitting the error. Meanwhile we will mark an
        // error occurred here for the user.
        return ex10_result;
    }

    return make_ex10_success();
}

static struct PowerDroopCompensationFields get_droop_compensation_defaults(void)
{
    return droop_comp_defaults;
}

static struct Ex10Result enable_droop_compensation(
    struct PowerDroopCompensationFields const* compensation)
{
    // Limit .interval and .step since keeping a relatively short interval
    // and small step size keeps Tx power changes within regulatory bounds.
    const uint8_t min_interval = 10u;
    const uint8_t max_interval = 40u;

    if (compensation->compensation_interval_ms < min_interval ||
        compensation->compensation_interval_ms > max_interval)
    {
        return make_ex10_sdk_error(Ex10ModuleRfPower,
                                   Ex10SdkErrorBadParamValue);
    }

    const uint8_t min_step = 5u;
    const uint8_t max_step = 15u;

    if (compensation->fine_gain_step_cd_b < min_step ||
        compensation->fine_gain_step_cd_b > max_step)
    {
        return make_ex10_sdk_error(Ex10ModuleRfPower,
                                   Ex10SdkErrorBadParamValue);
    }

    struct RegisterInfo const* const regs[] = {
        &power_droop_compensation_reg,
    };
    void const* buffers[] = {
        compensation,
    };

    return get_ex10_protocol()->write_multiple(
        regs, buffers, sizeof(regs) / sizeof(void const*));
}

static void disable_droop_compensation(void)
{
    struct PowerDroopCompensationFields compensation;
    get_ex10_protocol()->read(&power_droop_compensation_reg, &compensation);
    compensation.enable = false;

    struct RegisterInfo const* const regs[] = {
        &power_droop_compensation_reg,
    };
    void const* buffers[] = {
        &compensation,
    };

    get_ex10_protocol()->write_multiple(
        regs, buffers, sizeof(regs) / sizeof(void const*));
}

static const struct Ex10RfPower ex10_rf_power = {
    .init_ex10                        = init_ex10,
    .get_cw_is_on                     = get_cw_is_on,
    .stop_op_and_ramp_down            = stop_op_and_ramp_down,
    .cw_off                           = cw_off,
    .measure_and_read_aux_adc         = measure_and_read_aux_adc,
    .measure_and_read_adc_temperature = measure_and_read_adc_temperature,
    .set_rf_mode                      = set_rf_mode,
    .build_cw_configs                 = build_cw_configs,
    .cw_on                            = cw_on,
    .ramp_transmit_power              = ramp_transmit_power,
    .get_droop_compensation_defaults  = get_droop_compensation_defaults,
    .set_regulatory_timers            = set_regulatory_timers,
    .set_analog_rx_config             = set_analog_rx_config,
    .enable_droop_compensation        = enable_droop_compensation,
    .disable_droop_compensation       = disable_droop_compensation,
};

const struct Ex10RfPower* get_ex10_rf_power(void)
{
    return &ex10_rf_power;
}
