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

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ex10_api/ex10_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
// Impinj_calgen | gen_calibration_v5_h {

#pragma pack(push)
struct CalibrationVersionV5 {
    uint8_t cal_file_version;
};
static_assert(sizeof(struct CalibrationVersionV5) == 1,
              "Size of packet header not packed properly");
struct CustomerCalibrationVersionV5 {
    uint8_t customer_cal_file_version;
};
static_assert(sizeof(struct CustomerCalibrationVersionV5) == 1,
              "Size of packet header not packed properly");
struct VersionStringsV5 {
    uint8_t power_detect_cal_type;
    uint8_t forward_power_cal_type;
    uint8_t power_detector_temp_comp_type;
    uint8_t forward_power_temp_comp_type;
    uint8_t power_detector_freq_comp_type;
    uint8_t forward_power_freq_comp_type;
};
static_assert(sizeof(struct VersionStringsV5) == 6,
              "Size of packet header not packed properly");
struct UserBoardIdV5 {
    uint16_t user_board_id;
};
static_assert(sizeof(struct UserBoardIdV5) == 2,
              "Size of packet header not packed properly");
struct TxScalarCalV5 {
    int16_t tx_scalar_cal;
};
static_assert(sizeof(struct TxScalarCalV5) == 2,
              "Size of packet header not packed properly");
struct PerBandRfFilterV5 {
    float low_freq_limit;
    float high_freq_limit;
};
static_assert(sizeof(struct PerBandRfFilterV5) == 8,
              "Size of packet header not packed properly");
struct ValidPdetAdcsV5 {
    uint16_t valid_min_adc;
    uint16_t valid_max_adc;
};
static_assert(sizeof(struct ValidPdetAdcsV5) == 4,
              "Size of packet header not packed properly");
struct ControlLoopParamsV5 {
    uint16_t loop_gain_divisor;
    uint8_t error_threshold;
    uint8_t max_iterations;
};
static_assert(sizeof(struct ControlLoopParamsV5) == 4,
              "Size of packet header not packed properly");
struct PerBandPdetAdcLutV5 {
    uint16_t pdet0_adc_lut[31u];
    uint16_t pdet1_adc_lut[31u];
    uint16_t pdet2_adc_lut[31u];
};
static_assert(sizeof(struct PerBandPdetAdcLutV5) == 186,
              "Size of packet header not packed properly");
struct PerBandFwdPowerCoarsePwrCalV5 {
    float coarse_attn_cal[31u];
};
static_assert(sizeof(struct PerBandFwdPowerCoarsePwrCalV5) == 124,
              "Size of packet header not packed properly");
struct PerBandFwdPowerTempSlopeV5 {
    float fwd_power_temp_slope;
};
static_assert(sizeof(struct PerBandFwdPowerTempSlopeV5) == 4,
              "Size of packet header not packed properly");
struct PerBandCalTempV5 {
    uint16_t cal_temp_a_d_c;
};
static_assert(sizeof(struct PerBandCalTempV5) == 2,
              "Size of packet header not packed properly");
struct PerBandLoPdetTempSlopeV5 {
    float lo_pdet_temp_slope[3u];
};
static_assert(sizeof(struct PerBandLoPdetTempSlopeV5) == 12,
              "Size of packet header not packed properly");
struct PerBandLoPdetFreqLutV5 {
    int16_t lo_pdet_freq_adc_shifts0[4u];
    int16_t lo_pdet_freq_adc_shifts1[4u];
    int16_t lo_pdet_freq_adc_shifts2[4u];
};
static_assert(sizeof(struct PerBandLoPdetFreqLutV5) == 24,
              "Size of packet header not packed properly");
struct PerBandLoPdetFreqsV5 {
    float lo_pdet_freqs[4u];
};
static_assert(sizeof(struct PerBandLoPdetFreqsV5) == 16,
              "Size of packet header not packed properly");
struct PerBandFwdPwrFreqLutV5 {
    float fwd_pwr_shifts[4u];
};
static_assert(sizeof(struct PerBandFwdPwrFreqLutV5) == 16,
              "Size of packet header not packed properly");
struct DcOffsetCalV5 {
    int32_t dc_offset[31u];
};
static_assert(sizeof(struct DcOffsetCalV5) == 124,
              "Size of packet header not packed properly");
struct RssiRfModesV5 {
    uint16_t rf_modes[32u];
};
static_assert(sizeof(struct RssiRfModesV5) == 64,
              "Size of packet header not packed properly");
struct RssiRfModeLutV5 {
    int16_t rf_mode_lut[32u];
};
static_assert(sizeof(struct RssiRfModeLutV5) == 64,
              "Size of packet header not packed properly");
struct RssiPga1LutV5 {
    int16_t pga1_lut[4u];
};
static_assert(sizeof(struct RssiPga1LutV5) == 8,
              "Size of packet header not packed properly");
struct RssiPga2LutV5 {
    int16_t pga2_lut[4u];
};
static_assert(sizeof(struct RssiPga2LutV5) == 8,
              "Size of packet header not packed properly");
struct RssiPga3LutV5 {
    int16_t pga3_lut[4u];
};
static_assert(sizeof(struct RssiPga3LutV5) == 8,
              "Size of packet header not packed properly");
struct RssiMixerGainLutV5 {
    int16_t mixer_gain_lut[4u];
};
static_assert(sizeof(struct RssiMixerGainLutV5) == 8,
              "Size of packet header not packed properly");
struct RssiRxAttLutV5 {
    int16_t rx_att_gain_lut[4u];
};
static_assert(sizeof(struct RssiRxAttLutV5) == 8,
              "Size of packet header not packed properly");
struct RssiAntennasV5 {
    uint8_t antenna[8u];
};
static_assert(sizeof(struct RssiAntennasV5) == 8,
              "Size of packet header not packed properly");
struct RssiAntennaLutV5 {
    int16_t antenna_lut[8u];
};
static_assert(sizeof(struct RssiAntennaLutV5) == 16,
              "Size of packet header not packed properly");
struct PerBandRssiFreqOffsetV5 {
    int16_t freq_shift;
};
static_assert(sizeof(struct PerBandRssiFreqOffsetV5) == 2,
              "Size of packet header not packed properly");
struct RssiRxDefaultPwrV5 {
    int16_t input_powers;
};
static_assert(sizeof(struct RssiRxDefaultPwrV5) == 2,
              "Size of packet header not packed properly");
struct RssiRxDefaultLog2V5 {
    int16_t power_shifts;
};
static_assert(sizeof(struct RssiRxDefaultLog2V5) == 2,
              "Size of packet header not packed properly");
struct RssiTempSlopeV5 {
    float rssi_temp_slope;
};
static_assert(sizeof(struct RssiTempSlopeV5) == 4,
              "Size of packet header not packed properly");
struct RssiTempInterceptV5 {
    uint16_t rssi_temp_intercept;
};
static_assert(sizeof(struct RssiTempInterceptV5) == 2,
              "Size of packet header not packed properly");
#pragma pack(pop)

struct Ex10CalibrationParamsV5 {
    struct CalibrationVersionV5             calibration_version;
    struct CustomerCalibrationVersionV5     customer_calibration_version;
    struct VersionStringsV5                 version_strings;
    struct UserBoardIdV5                    user_board_id;
    struct TxScalarCalV5                    tx_scalar_cal;
    struct PerBandRfFilterV5                rf_filter_upper_band;
    struct PerBandRfFilterV5                rf_filter_lower_band;
    struct ValidPdetAdcsV5                  valid_pdet_adcs;
    struct ControlLoopParamsV5              control_loop_params;
    struct PerBandPdetAdcLutV5              upper_band_pdet_adc_lut;
    struct PerBandFwdPowerCoarsePwrCalV5    upper_band_fwd_power_coarse_pwr_cal;
    struct PerBandFwdPowerTempSlopeV5       upper_band_fwd_power_temp_slope;
    struct PerBandCalTempV5                 upper_band_cal_temp;
    struct PerBandLoPdetTempSlopeV5         upper_band_lo_pdet_temp_slope;
    struct PerBandLoPdetFreqLutV5           upper_band_lo_pdet_freq_lut;
    struct PerBandLoPdetFreqsV5             upper_band_lo_pdet_freqs;
    struct PerBandFwdPwrFreqLutV5           upper_band_fwd_pwr_freq_lut;
    struct PerBandPdetAdcLutV5              lower_band_pdet_adc_lut;
    struct PerBandFwdPowerCoarsePwrCalV5    lower_band_fwd_power_coarse_pwr_cal;
    struct PerBandFwdPowerTempSlopeV5       lower_band_fwd_power_temp_slope;
    struct PerBandCalTempV5                 lower_band_cal_temp;
    struct PerBandLoPdetTempSlopeV5         lower_band_lo_pdet_temp_slope;
    struct PerBandLoPdetFreqLutV5           lower_band_lo_pdet_freq_lut;
    struct PerBandLoPdetFreqsV5             lower_band_lo_pdet_freqs;
    struct PerBandFwdPwrFreqLutV5           lower_band_fwd_pwr_freq_lut;
    struct DcOffsetCalV5                    dc_offset_cal;
    struct RssiRfModesV5                    rssi_rf_modes;
    struct RssiRfModeLutV5                  rssi_rf_mode_lut;
    struct RssiPga1LutV5                    rssi_pga1_lut;
    struct RssiPga2LutV5                    rssi_pga2_lut;
    struct RssiPga3LutV5                    rssi_pga3_lut;
    struct RssiMixerGainLutV5               rssi_mixer_gain_lut;
    struct RssiRxAttLutV5                   rssi_rx_att_lut;
    struct RssiAntennasV5                   rssi_antennas;
    struct RssiAntennaLutV5                 rssi_antenna_lut;
    struct PerBandRssiFreqOffsetV5          upper_band_rssi_freq_offset;
    struct PerBandRssiFreqOffsetV5          lower_band_rssi_freq_offset;
    struct RssiRxDefaultPwrV5               rssi_rx_default_pwr;
    struct RssiRxDefaultLog2V5              rssi_rx_default_log2;
    struct RssiTempSlopeV5                  rssi_temp_slope;
    struct RssiTempInterceptV5              rssi_temp_intercept;
};
// Impinj_calgen }
// clang-format on

struct Ex10CalibrationV5
{
    /**
     * Read the calibration parameters from the Ex10 into the C library
     * struct CalibrationParameters store.
     *
     * @param ex10_protocol The protocol object used to communicate with the
     * Ex10.
     */
    void (*init)(struct Ex10Protocol const* ex10_protocol);

    /**
     * @return struct CalibrationParameters const* The pointer to the C library
     * struct CalibrationParameters store.
     */
    struct Ex10CalibrationParamsV5 const* (*get_params)(void);
};

struct Ex10CalibrationV5 const* get_ex10_cal_v5(void);

#ifdef __cplusplus
}
#endif
