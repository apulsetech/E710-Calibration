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

#include "ex10_modules/ex10_listen_before_talk.h"
#include "board/board_spec.h"
#include "board/ex10_osal.h"
#include "calibration.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/version_info.h"
#include "ex10_modules/ex10_ramp_module_manager.h"

/**
 * @struct Ex10LbtParams
 * Ex10LbtParams private parameters.
 */
struct Ex10LbtParams
{
    int32_t  lbt_offset_khz;
    uint8_t  rssi_count_exp;
    uint8_t  passes_required;
    int32_t  lbt_pass_threshold_cdbm;
    uint32_t max_rssi_measurements;
    uint16_t measurement_delay_us;
    int16_t  last_rssi;
    uint32_t last_frequency_khz;
    uint32_t total_num_rssi_measurements;
};

/*
 * Ex10LbtParams are loaded with the defaults
 * that apply to REGION_JAPAN2
 *
 * These are the recommended settings to meet Japanese
 * Listen Before Talk regulatory requirements.
 *
 * Each call to the LBT OP consists of 5 rssi reads with
 * a 0.238 ms delay between each read for a total OP execution
 * time of 9.9 ms
 *
 * max_rssi_measurements = 5000 is for certification testing,
 * functionally equivalent to 10 second timeout
 *
 * The -74.50 dBm threshold provides 0.5 dB of margin to account
 * for RSSI variability
 *
 * We strongly recommend that customers do not modify
 * lbt_offset_khz
 *
 * These variables can be modified using the provided setter
 * functions
 */
static struct Ex10LbtParams lbt_params = {
    .lbt_offset_khz              = -200,
    .rssi_count_exp              = 11,
    .passes_required             = 5,
    .lbt_pass_threshold_cdbm     = -7450,
    .max_rssi_measurements       = 5000,
    .measurement_delay_us        = 238,
    .last_rssi                   = INT16_MIN,
    .last_frequency_khz          = 0,
    .total_num_rssi_measurements = 0,
};

// forward declaration
static void lbt_pre_ramp_callback(struct Ex10Result* ex10_result);

static struct Ex10Result init(void)
{
    return get_ex10_ramp_module_manager()->register_ramp_callbacks(
        lbt_pre_ramp_callback, NULL);
}

static struct Ex10Result deinit(void)
{
    get_ex10_ramp_module_manager()->unregister_ramp_callbacks();

    return make_ex10_success();
}

static void set_rssi_count(uint8_t rssi_count_exp)
{
    lbt_params.rssi_count_exp = rssi_count_exp;
}

static void set_passes_required(uint8_t passes_required)
{
    lbt_params.passes_required = passes_required;
}

static void set_lbt_pass_threshold_cdbm(int32_t lbt_pass_threshold_cdbm)
{
    lbt_params.lbt_pass_threshold_cdbm = lbt_pass_threshold_cdbm;
}

static void set_max_rssi_measurements(uint32_t max_rssi_measurements)
{
    lbt_params.max_rssi_measurements = max_rssi_measurements;
}

static void set_measurement_delay_us(uint16_t measurement_delay_us)
{
    lbt_params.measurement_delay_us = measurement_delay_us;
}

static int16_t get_last_rssi_measurement(void)
{
    return lbt_params.last_rssi;
}

static uint32_t get_last_frequency_khz(void)
{
    return lbt_params.last_frequency_khz;
}

static uint32_t get_total_num_rssi_measurements(void)
{
    return lbt_params.total_num_rssi_measurements;
}

static struct RxGainControlFields get_default_lbt_rx_analog_configs(void)
{
    return (struct RxGainControlFields){.rx_atten   = RxAttenAtten_12_dB,
                                        .pga1_gain  = Pga1GainGain_12_dB,
                                        .pga2_gain  = Pga2GainGain_6_dB,
                                        .pga3_gain  = Pga3GainGain_18_dB,
                                        .Reserved0  = 0u,
                                        .mixer_gain = MixerGainGain_20p7_dB,
                                        .pga1_rin_select = false,
                                        .Reserved1       = 0u,
                                        .mixer_bandwidth = true};
}

static struct Ex10Result listen_before_talk_multi(
    uint8_t                           antenna,
    uint8_t                           rssi_count,
    struct LbtControlFields           lbt_settings,
    uint32_t*                         frequencies_khz,
    int32_t*                          lbt_offsets,
    int16_t*                          rssi_measurements,
    struct RxGainControlFields const* lbt_rx_gains)
{
    if ((frequencies_khz == NULL) || (lbt_offsets == NULL) ||
        (rssi_measurements == NULL))
    {
        return make_ex10_sdk_error(Ex10ListenBeforeTalk,
                                   Ex10SdkErrorNullPointer);
    }

    const struct Ex10ActiveRegion* region = get_ex10_active_region();

    // create an array of all synth params to use
    struct RfSynthesizerControlFields
        synth_param_array[RF_SYNTHESIZER_CONTROL_REG_ENTRIES];
    ex10_memzero(synth_param_array, sizeof(synth_param_array));

    for (size_t idx = 0; idx < lbt_settings.num_rssi_measurements; idx++)
    {
        struct SynthesizerParams synth_param;
        struct Ex10Result        ex10_result =
            region->get_synthesizer_params(frequencies_khz[idx], &synth_param);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        synth_param_array[idx].r_divider = synth_param.r_divider_index;
        synth_param_array[idx].n_divider = synth_param.n_divider;
        synth_param_array[idx].lf_type   = 1u;
    }

    // create an array of all lbt offsets to use
    struct LbtOffsetFields lbt_offset_settings[LBT_OFFSET_REG_ENTRIES];
    ex10_memzero(lbt_offset_settings, sizeof(lbt_offset_settings));
    for (size_t idx = 0; idx < lbt_settings.num_rssi_measurements; idx++)
    {
        lbt_offset_settings[idx].khz = lbt_offsets[idx];
    }

    // Tx power not used for LBT because it is a measurement when the
    // transmitter is off. The dummy measurement is created since it is a needed
    // parameter to get gpio settings. The actual power choice here does not
    // matter.
    int16_t const dummy_tx_power_cdbm = 0;
    // Enforce usage of the HPF
    enum BasebandFilterType const rx_baseband_filter = BasebandFilterHighpass;

    // Find the appropriate gpio settings
    struct GpioPinsSetClear gpio_pins;
    struct Ex10Result       ex10_result =
        get_ex10_board_spec()->get_gpio_output_pins_set_clear(
            &gpio_pins,
            antenna,
            dummy_tx_power_cdbm,
            rx_baseband_filter,
            region->get_rf_filter());
    if (ex10_result.error)
    {
        return ex10_result;
    }

    const struct Ex10Ops* ops = get_ex10_ops();
    // Set the gpio to enforce the HPF
    ex10_result = ops->set_clear_gpio_pins(&gpio_pins);
    if (ex10_result.error)
    {
        return ex10_result;
    }
    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    struct RxGainControlFields used_rx_gains = *lbt_rx_gains;
    if (lbt_settings.override == 0)
    {
        used_rx_gains = get_default_lbt_rx_analog_configs();
    }

    // Run listen before talk
    ex10_result = ops->run_listen_before_talk(&lbt_settings,
                                              &used_rx_gains,
                                              lbt_offset_settings,
                                              synth_param_array,
                                              rssi_count);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    const struct Ex10Protocol* proto = get_ex10_protocol();
    proto->read(&measured_rssi_log2_reg, rssi_measurements);
    const struct Ex10Calibration* calibration = get_ex10_calibration();
    for (uint8_t iter = 0; iter < lbt_settings.num_rssi_measurements; iter++)
    {
        enum ProductSku const sku_val    = proto->get_sku();
        int16_t const         sku_offset = (sku_val == SkuE310) ? -2239 : 0;

        uint16_t curr_temp_adc = 0;
        ex10_result = get_ex10_rf_power()->measure_and_read_adc_temperature(
            &curr_temp_adc);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        rssi_measurements[iter] = calibration->get_compensated_lbt_rssi(
                                      (uint16_t)rssi_measurements[iter],
                                      &used_rx_gains,
                                      antenna,
                                      region->get_rf_filter(),
                                      curr_temp_adc) +
                                  sku_offset;
    }
    return make_ex10_success();
}

static struct Ex10Result get_listen_before_talk_rssi(
    uint8_t                           antenna,
    uint32_t                          frequency_khz,
    int32_t                           lbt_offset,
    uint8_t                           rssi_count,
    bool                              override_used,
    struct RxGainControlFields const* lbt_rx_gains,
    int16_t*                          lbt_rssi_out)
{
    // Note: that the lbt_rssi_out is only expecting one result
    struct LbtControlFields lbt_settings = {
        .override              = override_used,
        .narrow_bandwidth_mode = false,
        .num_rssi_measurements = 1,
        .measurement_delay_us  = 0,
    };
    int16_t           rssi_results[MEASURED_RSSI_LOG2_REG_ENTRIES];
    struct Ex10Result ex10_result = listen_before_talk_multi(antenna,
                                                             rssi_count,
                                                             lbt_settings,
                                                             &frequency_khz,
                                                             &lbt_offset,
                                                             rssi_results,
                                                             lbt_rx_gains);
    *lbt_rssi_out                 = rssi_results[0];
    return ex10_result;
}

struct MultiLbtRssiInfo
{
    const int32_t pass_threshold;
    const uint8_t passes_required;
    uint8_t       num_measurements;
    uint8_t       under_limit_count;
    int16_t       highest_successive_rssi;
    int16_t       highest_rssi;
};

static void count_under_rssi_limit(int16_t*                 rssi_measurements,
                                   struct MultiLbtRssiInfo* lbt_rssi_info)
{
    for (size_t idx = 0; idx < lbt_rssi_info->num_measurements; idx++)
    {
        const int16_t curr_rssi_cdbm = rssi_measurements[idx];

        // stash away the current measurement in case anyone asks
        // later
        lbt_params.last_rssi = curr_rssi_cdbm;

        if (curr_rssi_cdbm < lbt_rssi_info->pass_threshold)
        {
            lbt_rssi_info->under_limit_count += 1;
            lbt_rssi_info->highest_successive_rssi =
                (curr_rssi_cdbm > lbt_rssi_info->highest_successive_rssi)
                    ? curr_rssi_cdbm
                    : lbt_rssi_info->highest_successive_rssi;
        }
        else
        {
            lbt_rssi_info->under_limit_count = 0;
            // Set back to the min since all previous measurements are now not
            // consecutive passes.
            lbt_rssi_info->highest_successive_rssi = INT16_MIN;
            lbt_rssi_info->highest_rssi =
                (curr_rssi_cdbm > lbt_rssi_info->highest_rssi)
                    ? curr_rssi_cdbm
                    : lbt_rssi_info->highest_rssi;
        }
    }
}

static int16_t multi_listen_before_talk_rssi(uint8_t antenna)
{
    lbt_params.total_num_rssi_measurements = 0;
    lbt_params.last_frequency_khz =
        get_ex10_active_region()->get_next_channel_khz();

    // We want to use the same frequency and offset for each measurement
    uint32_t freq_array[RF_SYNTHESIZER_CONTROL_REG_ENTRIES];
    ex10_fill_u32(freq_array,
                  lbt_params.last_frequency_khz,
                  rf_synthesizer_control_reg.num_entries);

    int32_t offset_array[LBT_OFFSET_REG_ENTRIES];
    ex10_fill_u32((uint32_t*)offset_array,
                  (uint32_t)lbt_params.lbt_offset_khz,
                  lbt_offset_reg.num_entries);

    // Create an array for the output values
    int16_t rssi_measurements[MEASURED_RSSI_LOG2_REG_ENTRIES];
    ex10_memzero(rssi_measurements, sizeof(rssi_measurements));

    // The highest rssi value will be the highest value seen overall.
    // The highest successive rssi will be the max rssi seen in a consecutive
    // sequence. Aka if 3 RSSIs are under limit, then one is above limit, this
    // successive value is reset to the minimum.
    struct MultiLbtRssiInfo lbt_rssi_info = {
        .pass_threshold          = lbt_params.lbt_pass_threshold_cdbm,
        .passes_required         = lbt_params.passes_required,
        .num_measurements        = 0,
        .under_limit_count       = 0,
        .highest_successive_rssi = INT16_MIN,
        .highest_rssi            = INT16_MIN,
    };

    while (lbt_params.total_num_rssi_measurements <
           lbt_params.max_rssi_measurements)
    {
        if (lbt_rssi_info.passes_required >
            rf_synthesizer_control_reg.num_entries)
        {
            lbt_rssi_info.num_measurements =
                rf_synthesizer_control_reg.num_entries;
        }
        else
        {
            lbt_rssi_info.num_measurements = lbt_rssi_info.passes_required;
        }
        struct LbtControlFields lbt_settings = {
            .override              = false,
            .narrow_bandwidth_mode = false,
            .num_rssi_measurements = lbt_rssi_info.num_measurements,
            .measurement_delay_us  = lbt_params.measurement_delay_us,
        };

        struct RxGainControlFields dummy_rx_fields;

        // Run the op and return the number of rssi measurements specified
        struct Ex10Result const ex10_result =
            listen_before_talk_multi(antenna,
                                     lbt_params.rssi_count_exp,
                                     lbt_settings,
                                     freq_array,
                                     offset_array,
                                     rssi_measurements,
                                     &dummy_rx_fields);
        if (ex10_result.error)
        {
            return lbt_rssi_info.highest_rssi;
        }

        lbt_params.total_num_rssi_measurements +=
            lbt_rssi_info.num_measurements;

        // Run through each measurement, checks if under the allowed limit, and
        // count towards the number of consecutive passes needed.
        count_under_rssi_limit(rssi_measurements, &lbt_rssi_info);
        // under limit count updated in the count function
        if (lbt_rssi_info.under_limit_count >= lbt_rssi_info.passes_required)
        {
            return lbt_rssi_info.highest_successive_rssi;
        }
    }
    return lbt_rssi_info.highest_rssi;
}

static void lbt_pre_ramp_callback(struct Ex10Result* ex10_result)
{
    // Check lbt on the next channel
    int16_t lbt_rssi_cdbm = multi_listen_before_talk_rssi(
        get_ex10_ramp_module_manager()->retrieve_pre_ramp_antenna());

    *ex10_result = make_ex10_success();
    // If LBT exceeded the noise expectations, return false
    if (lbt_rssi_cdbm >= lbt_params.lbt_pass_threshold_cdbm)
    {
        *ex10_result =
            make_ex10_sdk_error(Ex10ListenBeforeTalk, Ex10AboveThreshold);
    }
    return;
}

static const struct Ex10ListenBeforeTalk ex10_listen_before_talk = {
    .init                              = init,
    .deinit                            = deinit,
    .set_rssi_count                    = set_rssi_count,
    .set_passes_required               = set_passes_required,
    .set_lbt_pass_threshold_cdbm       = set_lbt_pass_threshold_cdbm,
    .set_max_rssi_measurements         = set_max_rssi_measurements,
    .set_measurement_delay_us          = set_measurement_delay_us,
    .get_last_rssi_measurement         = get_last_rssi_measurement,
    .get_last_frequency_khz            = get_last_frequency_khz,
    .get_total_num_rssi_measurements   = get_total_num_rssi_measurements,
    .listen_before_talk_multi          = listen_before_talk_multi,
    .get_listen_before_talk_rssi       = get_listen_before_talk_rssi,
    .multi_listen_before_talk_rssi     = multi_listen_before_talk_rssi,
    .get_default_lbt_rx_analog_configs = get_default_lbt_rx_analog_configs,
    .lbt_pre_ramp_callback             = lbt_pre_ramp_callback,
};

const struct Ex10ListenBeforeTalk* get_ex10_listen_before_talk(void)
{
    return &ex10_listen_before_talk;
}
