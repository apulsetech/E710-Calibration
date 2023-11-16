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

/**
 * @file print_calibration_v5.c
 * @details  This utility can read in the calibration data stored in the
 *  Impinj reader chip and print out the details to stdout
 */

#include <stddef.h>

#include "calibration.h"
#include "calibration_v5.h"
#include "ex10_api/board_init.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_print.h"


static void print_calibration(struct Ex10CalibrationParamsV5 const* calibration)
{
    // clang-format off
// Impinj_calgen | gen_cal_v5_c_example {

    ex10_ex_printf("CalibrationVersion:\n");
    ex10_ex_printf("    cal_file_version: %d\n", calibration->calibration_version.cal_file_version);
    ex10_ex_printf("CustomerCalibrationVersion:\n");
    ex10_ex_printf("    customer_cal_file_version: %d\n", calibration->customer_calibration_version.customer_cal_file_version);
    ex10_ex_printf("VersionStrings:\n");
    ex10_ex_printf("    power_detect_cal_type: %d\n", calibration->version_strings.power_detect_cal_type);
    ex10_ex_printf("    forward_power_cal_type: %d\n", calibration->version_strings.forward_power_cal_type);
    ex10_ex_printf("    power_detector_temp_comp_type: %d\n", calibration->version_strings.power_detector_temp_comp_type);
    ex10_ex_printf("    forward_power_temp_comp_type: %d\n", calibration->version_strings.forward_power_temp_comp_type);
    ex10_ex_printf("    power_detector_freq_comp_type: %d\n", calibration->version_strings.power_detector_freq_comp_type);
    ex10_ex_printf("    forward_power_freq_comp_type: %d\n", calibration->version_strings.forward_power_freq_comp_type);
    ex10_ex_printf("UserBoardId:\n");
    ex10_ex_printf("    user_board_id: %d\n", calibration->user_board_id.user_board_id);
    ex10_ex_printf("TxScalarCal:\n");
    ex10_ex_printf("    tx_scalar_cal: %d\n", calibration->tx_scalar_cal.tx_scalar_cal);
    ex10_ex_printf("PerBandRfFilter:\n");
    ex10_ex_printf("    low_freq_limit: %f\n", calibration->rf_filter_upper_band.low_freq_limit);
    ex10_ex_printf("    high_freq_limit: %f\n", calibration->rf_filter_upper_band.high_freq_limit);
    ex10_ex_printf("ValidPdetAdcs:\n");
    ex10_ex_printf("    valid_min_adc: %d\n", calibration->valid_pdet_adcs.valid_min_adc);
    ex10_ex_printf("    valid_max_adc: %d\n", calibration->valid_pdet_adcs.valid_max_adc);
    ex10_ex_printf("ControlLoopParams:\n");
    ex10_ex_printf("    loop_gain_divisor: %d\n", calibration->control_loop_params.loop_gain_divisor);
    ex10_ex_printf("    error_threshold: %d\n", calibration->control_loop_params.error_threshold);
    ex10_ex_printf("    max_iterations: %d\n", calibration->control_loop_params.max_iterations);
    ex10_ex_printf("PerBandPdetAdcLut:\n");
    ex10_ex_printf("    pdet0_adc_lut:");
    for (size_t idx = 0u; idx < 31u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->upper_band_pdet_adc_lut.pdet0_adc_lut[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("    pdet1_adc_lut:");
    for (size_t idx = 0u; idx < 31u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->upper_band_pdet_adc_lut.pdet1_adc_lut[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("    pdet2_adc_lut:");
    for (size_t idx = 0u; idx < 31u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->upper_band_pdet_adc_lut.pdet2_adc_lut[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("PerBandFwdPowerCoarsePwrCal:\n");
    ex10_ex_printf("    coarse_attn_cal:");
    for (size_t idx = 0u; idx < 31u; ++idx)
    {
        ex10_ex_printf(" %f,", calibration->upper_band_fwd_power_coarse_pwr_cal.coarse_attn_cal[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("PerBandFwdPowerTempSlope:\n");
    ex10_ex_printf("    fwd_power_temp_slope: %f\n", calibration->upper_band_fwd_power_temp_slope.fwd_power_temp_slope);
    ex10_ex_printf("PerBandCalTemp:\n");
    ex10_ex_printf("    cal_temp_a_d_c: %d\n", calibration->upper_band_cal_temp.cal_temp_a_d_c);
    ex10_ex_printf("PerBandLoPdetTempSlope:\n");
    ex10_ex_printf("    lo_pdet_temp_slope:");
    for (size_t idx = 0u; idx < 3u; ++idx)
    {
        ex10_ex_printf(" %f,", calibration->upper_band_lo_pdet_temp_slope.lo_pdet_temp_slope[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("PerBandLoPdetFreqLut:\n");
    ex10_ex_printf("    lo_pdet_freq_adc_shifts0:");
    for (size_t idx = 0u; idx < 4u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->upper_band_lo_pdet_freq_lut.lo_pdet_freq_adc_shifts0[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("    lo_pdet_freq_adc_shifts1:");
    for (size_t idx = 0u; idx < 4u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->upper_band_lo_pdet_freq_lut.lo_pdet_freq_adc_shifts1[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("    lo_pdet_freq_adc_shifts2:");
    for (size_t idx = 0u; idx < 4u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->upper_band_lo_pdet_freq_lut.lo_pdet_freq_adc_shifts2[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("PerBandLoPdetFreqs:\n");
    ex10_ex_printf("    lo_pdet_freqs:");
    for (size_t idx = 0u; idx < 4u; ++idx)
    {
        ex10_ex_printf(" %f,", calibration->upper_band_lo_pdet_freqs.lo_pdet_freqs[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("PerBandFwdPwrFreqLut:\n");
    ex10_ex_printf("    fwd_pwr_shifts:");
    for (size_t idx = 0u; idx < 4u; ++idx)
    {
        ex10_ex_printf(" %f,", calibration->upper_band_fwd_pwr_freq_lut.fwd_pwr_shifts[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("DcOffsetCal:\n");
    ex10_ex_printf("    dc_offset:");
    for (size_t idx = 0u; idx < 31u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->dc_offset_cal.dc_offset[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("RssiRfModes:\n");
    ex10_ex_printf("    rf_modes:");
    for (size_t idx = 0u; idx < 32u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->rssi_rf_modes.rf_modes[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("RssiRfModeLut:\n");
    ex10_ex_printf("    rf_mode_lut:");
    for (size_t idx = 0u; idx < 32u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->rssi_rf_mode_lut.rf_mode_lut[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("RssiPga1Lut:\n");
    ex10_ex_printf("    pga1_lut:");
    for (size_t idx = 0u; idx < 4u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->rssi_pga1_lut.pga1_lut[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("RssiPga2Lut:\n");
    ex10_ex_printf("    pga2_lut:");
    for (size_t idx = 0u; idx < 4u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->rssi_pga2_lut.pga2_lut[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("RssiPga3Lut:\n");
    ex10_ex_printf("    pga3_lut:");
    for (size_t idx = 0u; idx < 4u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->rssi_pga3_lut.pga3_lut[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("RssiMixerGainLut:\n");
    ex10_ex_printf("    mixer_gain_lut:");
    for (size_t idx = 0u; idx < 4u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->rssi_mixer_gain_lut.mixer_gain_lut[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("RssiRxAttLut:\n");
    ex10_ex_printf("    rx_att_gain_lut:");
    for (size_t idx = 0u; idx < 4u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->rssi_rx_att_lut.rx_att_gain_lut[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("RssiAntennas:\n");
    ex10_ex_printf("    antenna:");
    for (size_t idx = 0u; idx < 8u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->rssi_antennas.antenna[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("RssiAntennaLut:\n");
    ex10_ex_printf("    antenna_lut:");
    for (size_t idx = 0u; idx < 8u; ++idx)
    {
        ex10_ex_printf(" %d,", calibration->rssi_antenna_lut.antenna_lut[idx]);
    }
    ex10_ex_printf("\n");
    ex10_ex_printf("PerBandRssiFreqOffset:\n");
    ex10_ex_printf("    freq_shift: %d\n", calibration->upper_band_rssi_freq_offset.freq_shift);
    ex10_ex_printf("RssiRxDefaultPwr:\n");
    ex10_ex_printf("    input_powers: %d\n", calibration->rssi_rx_default_pwr.input_powers);
    ex10_ex_printf("RssiRxDefaultLog2:\n");
    ex10_ex_printf("    power_shifts: %d\n", calibration->rssi_rx_default_log2.power_shifts);
    ex10_ex_printf("RssiTempSlope:\n");
    ex10_ex_printf("    rssi_temp_slope: %f\n", calibration->rssi_temp_slope.rssi_temp_slope);
    ex10_ex_printf("RssiTempIntercept:\n");
    ex10_ex_printf("    rssi_temp_intercept: %d\n", calibration->rssi_temp_intercept.rssi_temp_intercept);
// Impinj_calgen }
    // clang-format on
}

int main(void)
{
    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    uint8_t cal_version = get_ex10_calibration()->get_cal_version();

    if (cal_version == 5)
    {
        get_ex10_cal_v5()->init(get_ex10_protocol());

        struct Ex10CalibrationParamsV5 const* calibration =
            get_ex10_cal_v5()->get_params();
        print_calibration(calibration);
    }
    else
    {
        ex10_ex_eprintf(
            "The current calibration version of %d does not match the "
            "example\n",
            cal_version);
    }

    ex10_typical_board_teardown();

    return 0;
}
