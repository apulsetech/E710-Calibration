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

#include <stddef.h>
#include <stdint.h>

#include "calibration_v5.h"
#include "ex10_api/application_registers.h"

// clang-format off
// Impinj_calgen | gen_calibration_v5_c {

static struct Ex10CalibrationParamsV5 calibration_parameters;

static struct Ex10CalibrationParamsV5 const calibration_parameters_default =
{
    .calibration_version = {
        .cal_file_version = 255,
    },
    .customer_calibration_version = {
        .customer_cal_file_version = 0,
    },
    .version_strings = {
        .power_detect_cal_type = 5,
        .forward_power_cal_type = 1,
        .power_detector_temp_comp_type = 2,
        .forward_power_temp_comp_type = 1,
        .power_detector_freq_comp_type = 2,
        .forward_power_freq_comp_type = 1,
    },
    .user_board_id = {
        .user_board_id = 0,
    },
    .tx_scalar_cal = {
        .tx_scalar_cal = 1152,
    },
    .rf_filter_upper_band = {
        .low_freq_limit = 0.0,
        .high_freq_limit = 0.0,
    },
    .rf_filter_lower_band = {
        .low_freq_limit = 0.0,
        .high_freq_limit = 0.0,
    },
    .valid_pdet_adcs = {
        .valid_min_adc = 0,
        .valid_max_adc = 0,
    },
    .control_loop_params = {
        .loop_gain_divisor = 0,
        .error_threshold = 0,
        .max_iterations = 0,
    },
    .upper_band_pdet_adc_lut = {
        .pdet0_adc_lut = { 0 },
        .pdet1_adc_lut = { 0 },
        .pdet2_adc_lut = { 0 },
    },
    .upper_band_fwd_power_coarse_pwr_cal = {
        .coarse_attn_cal = { 0.0 },
    },
    .upper_band_fwd_power_temp_slope = {
        .fwd_power_temp_slope = 0.0,
    },
    .upper_band_cal_temp = {
        .cal_temp_a_d_c = 0,
    },
    .upper_band_lo_pdet_temp_slope = {
        .lo_pdet_temp_slope = { 0.0 },
    },
    .upper_band_lo_pdet_freq_lut = {
        .lo_pdet_freq_adc_shifts0 = { 0 },
        .lo_pdet_freq_adc_shifts1 = { 0 },
        .lo_pdet_freq_adc_shifts2 = { 0 },
    },
    .upper_band_lo_pdet_freqs = {
        .lo_pdet_freqs = { 0.0 },
    },
    .upper_band_fwd_pwr_freq_lut = {
        .fwd_pwr_shifts = { 0.0 },
    },
    .lower_band_pdet_adc_lut = {
        .pdet0_adc_lut = { 0 },
        .pdet1_adc_lut = { 0 },
        .pdet2_adc_lut = { 0 },
    },
    .lower_band_fwd_power_coarse_pwr_cal = {
        .coarse_attn_cal = { 0.0 },
    },
    .lower_band_fwd_power_temp_slope = {
        .fwd_power_temp_slope = 0.0,
    },
    .lower_band_cal_temp = {
        .cal_temp_a_d_c = 0,
    },
    .lower_band_lo_pdet_temp_slope = {
        .lo_pdet_temp_slope = { 0.0 },
    },
    .lower_band_lo_pdet_freq_lut = {
        .lo_pdet_freq_adc_shifts0 = { 0 },
        .lo_pdet_freq_adc_shifts1 = { 0 },
        .lo_pdet_freq_adc_shifts2 = { 0 },
    },
    .lower_band_lo_pdet_freqs = {
        .lo_pdet_freqs = { 0.0 },
    },
    .lower_band_fwd_pwr_freq_lut = {
        .fwd_pwr_shifts = { 0.0 },
    },
    .dc_offset_cal = {
        .dc_offset = { 0 },
    },
    .rssi_rf_modes = {
        .rf_modes = { 0 },
    },
    .rssi_rf_mode_lut = {
        .rf_mode_lut = { 0 },
    },
    .rssi_pga1_lut = {
        .pga1_lut = { 0 },
    },
    .rssi_pga2_lut = {
        .pga2_lut = { 0 },
    },
    .rssi_pga3_lut = {
        .pga3_lut = { 0 },
    },
    .rssi_mixer_gain_lut = {
        .mixer_gain_lut = { 0 },
    },
    .rssi_rx_att_lut = {
        .rx_att_gain_lut = { 0 },
    },
    .rssi_antennas = {
        .antenna = { 0 },
    },
    .rssi_antenna_lut = {
        .antenna_lut = { 0 },
    },
    .upper_band_rssi_freq_offset = {
        .freq_shift = 0,
    },
    .lower_band_rssi_freq_offset = {
        .freq_shift = 0,
    },
    .rssi_rx_default_pwr = {
        .input_powers = 0,
    },
    .rssi_rx_default_log2 = {
        .power_shifts = 0,
    },
    .rssi_temp_slope = {
        .rssi_temp_slope = 0.0,
    },
    .rssi_temp_intercept = {
        .rssi_temp_intercept = 0,
    },
};

struct CalibrationOffset
{
    uint16_t  const source;
    uintptr_t const destination;
    size_t    const length;
};

static struct CalibrationOffset const offset_table[] =
{
    {
        .source      =   0,
        .destination = offsetof(struct Ex10CalibrationParamsV5, calibration_version),
        .length      = sizeof(calibration_parameters.calibration_version)
    },
    {
        .source      =   1,
        .destination = offsetof(struct Ex10CalibrationParamsV5, customer_calibration_version),
        .length      = sizeof(calibration_parameters.customer_calibration_version)
    },
    {
        .source      =   4,
        .destination = offsetof(struct Ex10CalibrationParamsV5, version_strings),
        .length      = sizeof(calibration_parameters.version_strings)
    },
    {
        .source      =  10,
        .destination = offsetof(struct Ex10CalibrationParamsV5, user_board_id),
        .length      = sizeof(calibration_parameters.user_board_id)
    },
    {
        .source      =  12,
        .destination = offsetof(struct Ex10CalibrationParamsV5, tx_scalar_cal),
        .length      = sizeof(calibration_parameters.tx_scalar_cal)
    },
    {
        .source      =  14,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rf_filter_upper_band),
        .length      = sizeof(calibration_parameters.rf_filter_upper_band)
    },
    {
        .source      =  22,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rf_filter_lower_band),
        .length      = sizeof(calibration_parameters.rf_filter_lower_band)
    },
    {
        .source      =  30,
        .destination = offsetof(struct Ex10CalibrationParamsV5, valid_pdet_adcs),
        .length      = sizeof(calibration_parameters.valid_pdet_adcs)
    },
    {
        .source      =  34,
        .destination = offsetof(struct Ex10CalibrationParamsV5, control_loop_params),
        .length      = sizeof(calibration_parameters.control_loop_params)
    },
    {
        .source      =  40,
        .destination = offsetof(struct Ex10CalibrationParamsV5, upper_band_pdet_adc_lut),
        .length      = sizeof(calibration_parameters.upper_band_pdet_adc_lut)
    },
    {
        .source      = 226,
        .destination = offsetof(struct Ex10CalibrationParamsV5, upper_band_fwd_power_coarse_pwr_cal),
        .length      = sizeof(calibration_parameters.upper_band_fwd_power_coarse_pwr_cal)
    },
    {
        .source      = 350,
        .destination = offsetof(struct Ex10CalibrationParamsV5, upper_band_fwd_power_temp_slope),
        .length      = sizeof(calibration_parameters.upper_band_fwd_power_temp_slope)
    },
    {
        .source      = 354,
        .destination = offsetof(struct Ex10CalibrationParamsV5, upper_band_cal_temp),
        .length      = sizeof(calibration_parameters.upper_band_cal_temp)
    },
    {
        .source      = 356,
        .destination = offsetof(struct Ex10CalibrationParamsV5, upper_band_lo_pdet_temp_slope),
        .length      = sizeof(calibration_parameters.upper_band_lo_pdet_temp_slope)
    },
    {
        .source      = 368,
        .destination = offsetof(struct Ex10CalibrationParamsV5, upper_band_lo_pdet_freq_lut),
        .length      = sizeof(calibration_parameters.upper_band_lo_pdet_freq_lut)
    },
    {
        .source      = 392,
        .destination = offsetof(struct Ex10CalibrationParamsV5, upper_band_lo_pdet_freqs),
        .length      = sizeof(calibration_parameters.upper_band_lo_pdet_freqs)
    },
    {
        .source      = 424,
        .destination = offsetof(struct Ex10CalibrationParamsV5, upper_band_fwd_pwr_freq_lut),
        .length      = sizeof(calibration_parameters.upper_band_fwd_pwr_freq_lut)
    },
    {
        .source      = 456,
        .destination = offsetof(struct Ex10CalibrationParamsV5, lower_band_pdet_adc_lut),
        .length      = sizeof(calibration_parameters.lower_band_pdet_adc_lut)
    },
    {
        .source      = 642,
        .destination = offsetof(struct Ex10CalibrationParamsV5, lower_band_fwd_power_coarse_pwr_cal),
        .length      = sizeof(calibration_parameters.lower_band_fwd_power_coarse_pwr_cal)
    },
    {
        .source      = 766,
        .destination = offsetof(struct Ex10CalibrationParamsV5, lower_band_fwd_power_temp_slope),
        .length      = sizeof(calibration_parameters.lower_band_fwd_power_temp_slope)
    },
    {
        .source      = 770,
        .destination = offsetof(struct Ex10CalibrationParamsV5, lower_band_cal_temp),
        .length      = sizeof(calibration_parameters.lower_band_cal_temp)
    },
    {
        .source      = 772,
        .destination = offsetof(struct Ex10CalibrationParamsV5, lower_band_lo_pdet_temp_slope),
        .length      = sizeof(calibration_parameters.lower_band_lo_pdet_temp_slope)
    },
    {
        .source      = 784,
        .destination = offsetof(struct Ex10CalibrationParamsV5, lower_band_lo_pdet_freq_lut),
        .length      = sizeof(calibration_parameters.lower_band_lo_pdet_freq_lut)
    },
    {
        .source      = 808,
        .destination = offsetof(struct Ex10CalibrationParamsV5, lower_band_lo_pdet_freqs),
        .length      = sizeof(calibration_parameters.lower_band_lo_pdet_freqs)
    },
    {
        .source      = 840,
        .destination = offsetof(struct Ex10CalibrationParamsV5, lower_band_fwd_pwr_freq_lut),
        .length      = sizeof(calibration_parameters.lower_band_fwd_pwr_freq_lut)
    },
    {
        .source      = 872,
        .destination = offsetof(struct Ex10CalibrationParamsV5, dc_offset_cal),
        .length      = sizeof(calibration_parameters.dc_offset_cal)
    },
    {
        .source      = 996,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_rf_modes),
        .length      = sizeof(calibration_parameters.rssi_rf_modes)
    },
    {
        .source      = 1060,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_rf_mode_lut),
        .length      = sizeof(calibration_parameters.rssi_rf_mode_lut)
    },
    {
        .source      = 1124,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_pga1_lut),
        .length      = sizeof(calibration_parameters.rssi_pga1_lut)
    },
    {
        .source      = 1132,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_pga2_lut),
        .length      = sizeof(calibration_parameters.rssi_pga2_lut)
    },
    {
        .source      = 1140,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_pga3_lut),
        .length      = sizeof(calibration_parameters.rssi_pga3_lut)
    },
    {
        .source      = 1148,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_mixer_gain_lut),
        .length      = sizeof(calibration_parameters.rssi_mixer_gain_lut)
    },
    {
        .source      = 1156,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_rx_att_lut),
        .length      = sizeof(calibration_parameters.rssi_rx_att_lut)
    },
    {
        .source      = 1164,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_antennas),
        .length      = sizeof(calibration_parameters.rssi_antennas)
    },
    {
        .source      = 1172,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_antenna_lut),
        .length      = sizeof(calibration_parameters.rssi_antenna_lut)
    },
    {
        .source      = 1188,
        .destination = offsetof(struct Ex10CalibrationParamsV5, upper_band_rssi_freq_offset),
        .length      = sizeof(calibration_parameters.upper_band_rssi_freq_offset)
    },
    {
        .source      = 1190,
        .destination = offsetof(struct Ex10CalibrationParamsV5, lower_band_rssi_freq_offset),
        .length      = sizeof(calibration_parameters.lower_band_rssi_freq_offset)
    },
    {
        .source      = 1192,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_rx_default_pwr),
        .length      = sizeof(calibration_parameters.rssi_rx_default_pwr)
    },
    {
        .source      = 1194,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_rx_default_log2),
        .length      = sizeof(calibration_parameters.rssi_rx_default_log2)
    },
    {
        .source      = 1196,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_temp_slope),
        .length      = sizeof(calibration_parameters.rssi_temp_slope)
    },
    {
        .source      = 1200,
        .destination = offsetof(struct Ex10CalibrationParamsV5, rssi_temp_intercept),
        .length      = sizeof(calibration_parameters.rssi_temp_intercept)
    },
};
// Impinj_calgen }
// clang-format on

static void init(struct Ex10Protocol const* ex10_protocol)
{
    uint16_t const source_base      = calibration_info_reg.address;
    uint8_t* const destination_base = (uint8_t*)&calibration_parameters;
    size_t const offset_count = sizeof(offset_table) / sizeof(offset_table[0u]);

    for (struct CalibrationOffset const* offset = &offset_table[0u];
         offset < &offset_table[offset_count];
         ++offset)
    {
        uint16_t const source_address = source_base + offset->source;
        uint8_t* destination_pointer  = destination_base + offset->destination;
        ex10_protocol->read_partial(
            source_address, (uint16_t)offset->length, destination_pointer);
    }
}

static struct Ex10CalibrationParamsV5 const* get_params(void)
{
    size_t version =
        calibration_parameters.calibration_version.cal_file_version;
    if (version == 0x05)
    {
        return &calibration_parameters;
    }
    else
    {
        return &calibration_parameters_default;
    }
}

static struct Ex10CalibrationV5 const ex10_cal_v5 = {
    .init       = init,
    .get_params = get_params,
};

struct Ex10CalibrationV5 const* get_ex10_cal_v5(void)
{
    return &ex10_cal_v5;
}
