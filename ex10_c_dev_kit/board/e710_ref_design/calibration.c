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

#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "board/ex10_rx_baseband_filter.h"
#include "board_spec_constants.h"
#include "calibration.h"
#include "calibration_v5.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_utils.h"
#include "rssi_compensation_lut.h"

// Assume the ADC LUTs have the same length, represented by pdet2_adc_lut
#define ADCS_LENGTH                                \
    (sizeof(cal_params->lower_band_pdet_adc_lut) / \
     sizeof(cal_params->lower_band_pdet_adc_lut.pdet2_adc_lut))

enum DrmSizes
{
    DRM_ANALOG_LENGTH     = 2,
    NON_DRM_ANALOG_LENGTH = 3,
};

static uint8_t cal_version                                  = 0;
static uint8_t customer_cal_version                         = 0;
static int16_t drm_analog_offset[DRM_ANALOG_LENGTH]         = {0, 0};
static int16_t non_drm_analog_offset[NON_DRM_ANALOG_LENGTH] = {0, 0, 0};

/**
 * This function calculates inter/extra-polated value (x_new, y_new) from
 * existing points (x, y).
 * @param x  independent axis
 * @param y  dependent axis
 * @param axis_length  the length of the x and y axis
 * @param x_new  new point along independent axis
 *
 * @return y_new  new point along dependent axis
 */
static int16_t inter_extra_polate(const int16_t* x,
                                  const int16_t* y,
                                  const uint16_t axis_length,
                                  int16_t        x_new)
{
    int32_t y_new = 0;

    if (x_new < x[0])
    {
        y_new = ((y[0] - y[1]) * (x_new - x[0])) / (x[0] - x[1]) + y[0];
    }
    else if (x_new > x[axis_length - 1])
    {
        y_new = ((y[axis_length - 1] - y[axis_length - 2]) *
                 (x_new - x[axis_length - 1])) /
                    (x[axis_length - 1] - x[axis_length - 2]) +
                y[axis_length - 1];
    }
    else
    {
        for (uint16_t i = 1u; i < axis_length; ++i)
        {
            if (x[i] == x_new)
            {
                y_new = y[i];
                break;
            }
            else if (x[i] > x_new)
            {
                y_new = ((x[i] - x_new) * y[i - 1]) / (x[i] - x[i - 1]);
                y_new += ((x_new - x[i - 1]) * y[i]) / (x[i] - x[i - 1]);
                break;
            }
        }
    }
    return (int16_t)y_new;
}

/**
 * Function fills out the analog gain offsets in rssi log2 units.
 * @param cal_params   Pointer to the calibration table
 * @param rx_settings  Settings corresponding to RxGainControl register
 * @param antenna      Antenna port used
 * @param rf_band      Which RF band we are using
 *
 * @return gain_offset  gain offset
 */
static int16_t get_gain_offset(struct Ex10CalibrationParamsV5 const* cal_params,
                               const struct RxGainControlFields* rx_settings,
                               uint8_t                           antenna,
                               enum RfFilter                     rf_band)
{
    int16_t gain_offset = 0;

    int16_t const ofs_rx_atten =
        cal_params->rssi_rx_att_lut.rx_att_gain_lut[rx_settings->rx_atten];
    int16_t const ofs_pga1_gain =
        cal_params->rssi_pga1_lut.pga1_lut[rx_settings->pga1_gain];
    int16_t const ofs_pga2_gain =
        cal_params->rssi_pga2_lut.pga2_lut[rx_settings->pga2_gain];
    int16_t const ofs_pga3_gain =
        cal_params->rssi_pga3_lut.pga3_lut[rx_settings->pga3_gain];
    int16_t const ofs_mixer_gain =
        cal_params->rssi_mixer_gain_lut.mixer_gain_lut[rx_settings->mixer_gain];

    int16_t const ofs_antenna =
        cal_params->rssi_antenna_lut
            .antenna_lut[cal_params->rssi_antennas.antenna[antenna]];
    int16_t const ofs_frequency =
        rf_band == LOWER_BAND
            ? cal_params->lower_band_rssi_freq_offset.freq_shift
            : cal_params->upper_band_rssi_freq_offset.freq_shift;

    gain_offset =
        (ofs_rx_atten + ofs_pga1_gain + ofs_pga2_gain + ofs_pga3_gain +
         ofs_mixer_gain + ofs_antenna + ofs_frequency);

    return gain_offset;
}

/**
 * Initializes the local analog offset arrays to properly compute the analog
 * frequency offset during runtime
 */
static void init_drm_analog_offsets(void)
{
    struct Ex10CalibrationParamsV5 const* cal_params =
        get_ex10_cal_v5()->get_params();

    static int16_t calibration_modes_blf_analog_offset[NUM_CALIBRATION_MODES] =
        {0, 0, 0, 0, 0, 0, 0, 0};

    struct RssiCompensationLut const* rssi_comp = get_ex10_rssi_compensation();

    uint16_t offset_len = ARRAY_SIZE(calibration_modes_blf_analog_offset);
    for (uint16_t i = 0; i < offset_len; ++i)
    {
        for (uint16_t j = 0;
             j < ARRAY_SIZE(cal_params->rssi_rf_mode_lut.rf_mode_lut);
             j++)
        {
            if (cal_params->rssi_rf_modes.rf_modes[j] ==
                rssi_comp->calibration_modes[i])
            {
                calibration_modes_blf_analog_offset[i] =
                    cal_params->rssi_rf_mode_lut.rf_mode_lut[j];
                break;
            }
        }
    }

    // drm 250 BLF - mode 7
    static_assert((uint32_t)DRM_250_IDX < (uint32_t)DRM_ANALOG_LENGTH,
                  "DRM index out of bounds");
    drm_analog_offset[DRM_250_IDX] =
        (calibration_modes_blf_analog_offset[DRM_M4_250_IDX] -
         rssi_comp->calibration_modes_blf_digital_offset[DRM_M4_250_IDX]);
    // drm 320 BLF - mode 5
    static_assert((uint32_t)DRM_320_IDX < (uint32_t)DRM_ANALOG_LENGTH,
                  "DRM index out of bounds");
    drm_analog_offset[DRM_320_IDX] =
        (calibration_modes_blf_analog_offset[DRM_M4_320_IDX] -
         rssi_comp->calibration_modes_blf_digital_offset[DRM_M4_320_IDX]);

    // non-drm 160 BLF - mode 13
    static_assert((uint32_t)NON_DRM_160_IDX < (uint32_t)NON_DRM_ANALOG_LENGTH,
                  "Non-DRM index out of bounds");
    non_drm_analog_offset[NON_DRM_160_IDX] =
        (calibration_modes_blf_analog_offset[NON_DRM_M8_160_IDX] -
         rssi_comp->calibration_modes_blf_digital_offset[NON_DRM_M8_160_IDX]);
    // non-drm 320 BLF - modes 3, 12
    static_assert((uint32_t)NON_DRM_320_IDX < (uint32_t)NON_DRM_ANALOG_LENGTH,
                  "Non-DRM index out of bounds");
    non_drm_analog_offset[NON_DRM_320_IDX] =
        (calibration_modes_blf_analog_offset[NON_DRM_M2_320_IDX_0] -
         rssi_comp->calibration_modes_blf_digital_offset[NON_DRM_M2_320_IDX_0]);
    non_drm_analog_offset[NON_DRM_320_IDX] +=
        (calibration_modes_blf_analog_offset[NON_DRM_M2_320_IDX_1] -
         rssi_comp->calibration_modes_blf_digital_offset[NON_DRM_M2_320_IDX_1]);
    non_drm_analog_offset[NON_DRM_320_IDX] /= 2;
    // non-drm 640 BLF - modes 1, 15
    static_assert((uint32_t)NON_DRM_640_IDX < (uint32_t)NON_DRM_ANALOG_LENGTH,
                  "Non-DRM index out of bounds");
    non_drm_analog_offset[NON_DRM_640_IDX] =
        (calibration_modes_blf_analog_offset[NON_DRM_M2_640_IDX] -
         rssi_comp->calibration_modes_blf_digital_offset[NON_DRM_M2_640_IDX]);
    non_drm_analog_offset[NON_DRM_640_IDX] +=
        (calibration_modes_blf_analog_offset[NON_DRM_M4_640_IDX] -
         rssi_comp->calibration_modes_blf_digital_offset[NON_DRM_M4_640_IDX]);
    non_drm_analog_offset[NON_DRM_640_IDX] /= 2;
}

/**
 * Function fills out the analog offset in rssi log2 units.
 *
 * @param baseband_freq  the baseband frequency of the RX signal
 * @param drm            if the DRM external filter is being used
 *
 * @return analog_offset  analog baseband frequency offset
 */
static int16_t get_analog_baseband_freq_offset(int16_t baseband_freq,
                                               uint8_t drm)
{
    // Using the same array lengths as the analog offset arrays to ensure we can
    // compare them
    const int16_t drm_analog_freq_khz[DRM_ANALOG_LENGTH]         = {250, 320};
    const int16_t non_drm_analog_freq_khz[NON_DRM_ANALOG_LENGTH] = {
        160, 320, 640};

    const int16_t analog_offset =
        (drm == 0) ? inter_extra_polate(non_drm_analog_freq_khz,
                                        non_drm_analog_offset,
                                        NON_DRM_ANALOG_LENGTH,
                                        baseband_freq)
                   : inter_extra_polate(drm_analog_freq_khz,
                                        drm_analog_offset,
                                        DRM_ANALOG_LENGTH,
                                        baseband_freq);
    return analog_offset;
}

/**
 * Get mode offset for RSSI compensation. Print a message to notify
 * user/log when characterization correction data is used.
 * @param rf_mode        RF mode
 * @param baseband_freq  baseband frequency in kHz
 *
 * @return offset based on RF mode
 */
static int16_t get_mode_rssi_offset(enum RfModes rf_mode, int16_t baseband_freq)
{
    uint16_t rx_mode_idx          = 0;
    int16_t  mode_offset          = 0;
    int16_t  analog_offset        = 0;
    int16_t  digital_offset       = 0;
    uint8_t  mode_calibrated_flag = 0;

    if (baseband_freq < 0)
    {
        baseband_freq = baseband_freq * (-1);
    }

    struct RssiCompensationLut const* rssi_comp = get_ex10_rssi_compensation();

    for (uint16_t idx = 0; idx < NUM_MODES; ++idx)
    {
        if (rf_mode == (enum RfModes)rssi_comp->rf_modes[idx])
        {
            rx_mode_idx = rssi_comp->rf_mode_to_rx_mode[idx];

            mode_calibrated_flag = 1;

            const bool is_drm =
                get_ex10_rx_baseband_filter()->rf_mode_is_drm(rf_mode);
            analog_offset =
                get_analog_baseband_freq_offset(baseband_freq, is_drm);

            digital_offset = inter_extra_polate(
                rssi_comp->digital_frequency_points,
                rssi_comp->rx_mode_digital_correction[rx_mode_idx],
                NUM_RSSI_MEASUREMENTS,
                baseband_freq);

            mode_offset = (analog_offset + digital_offset);
            break;
        }
    }

    if (mode_calibrated_flag == 0)
    {
        ex10_eprintf(
            "RF Mode %u is not supported in the Calibration table on this "
            "device. RX compensation is using 0 correction for mode.\n",
            rf_mode);
    }

    return mode_offset;
}

/**
 * Get lbt baseband offset for RSSI compensation.
 *
 * @param cordic_freq_offset  cordic frequency shift
 *
 * @return offset based on RF mode
 */
static int16_t get_lbt_baseband_offset(uint16_t cordic_freq_offset)
{  // currently assuming non-DRM filter
    int16_t const analog_offset =
        get_analog_baseband_freq_offset((int16_t)cordic_freq_offset, 0);

    int16_t const baseband_offset =
        analog_offset + get_ex10_rssi_compensation()->lbt_digital_correction;
    return baseband_offset;
}

/**
 * Get temp offset for RSSI compensation. Print a message to notify
 * user/log when characterization correction data is used.
 * @param cal_params pointer to the calibration table
 * @param temp_adc   temp adc from the adc aux
 *
 * @return offset based on temperature
 */
static int16_t get_temp_offset(struct Ex10CalibrationParamsV5 const* cal_params,
                               uint16_t                              temp_adc)
{
    int32_t temp_offset = 0;

    if (temp_adc)
    {
        // -0.616 rssi_log2 per temp_adc
        temp_offset =
            ((-616 * (temp_adc -
                      cal_params->rssi_temp_intercept.rssi_temp_intercept)) +
             500) /
            1000;
    }

    return (int16_t)temp_offset;
}

/**
 * Convert from log2 to cdBm
 *
 * @param cal_params pointer to the calibration table
 * @param log2_val   value in log2
 *
 * @return value in cdBm
 */
static int16_t log2_to_cdbm(struct Ex10CalibrationParamsV5 const* cal_params,
                            uint16_t                              log2_val)
{
    int32_t const log2_val_32 = (int32_t)log2_val;
    int32_t const rssi_dflt_cdbm =
        cal_params->rssi_rx_default_pwr.input_powers * 100;
    int32_t const rssi_dflt_log2 =
        cal_params->rssi_rx_default_log2.power_shifts;

    // Conversion from log2 to dB is 20*log10(2)/128, or 0.047035
    int32_t const cdbm_val =
        (((log2_val_32 - rssi_dflt_log2) * 470 + rssi_dflt_cdbm * 100) + 50) /
        100;

    return (int16_t)cdbm_val;
}

/**
 * Convert from cdBm to log2.
 *
 * @param cal_params    pointer to the calibration table
 * @param cdbm_val      value in cdBm
 *
 * @return value in log2.
 */
static uint16_t cdbm_to_log2(struct Ex10CalibrationParamsV5 const* cal_params,
                             int16_t                               cdbm_val)
{
    int32_t       log2_val = 0;
    int32_t const rssi_dflt_cdbm =
        cal_params->rssi_rx_default_pwr.input_powers * 100;
    int32_t const rssi_dflt_log2 =
        cal_params->rssi_rx_default_log2.power_shifts;

    // Conversion from log2 to dB is 20*log10(2)/128, or 0.047035
    log2_val = ((cdbm_val - rssi_dflt_cdbm) * 100 + 235) / 470 + rssi_dflt_log2;

    return (uint16_t)log2_val;
}

/**
 * Compensates RSSI value for temperature, analog settings, and RF mode
 * based on calibration table.
 *
 * @param rssi_raw      Raw RSSI_LOG_2 value from firmware op
 * @param rf_mode       RF mode
 * @param rx_settings   Settings corresponding to RxGainControl register
 * @param antenna       Antenna port used
 * @param rf_band       Which RF band we are using
 * @param temp_adc      Temperature ADC code
 * @param baseband_freq The baseband frequency at which we expect the signal
 *
 * @return int16_t     Compensated RSSI value in cdBm
 */
static int16_t get_compensated_rssi_baseband_freq_offset(
    uint16_t                          rssi_raw,
    enum RfModes                      rf_mode,
    const struct RxGainControlFields* rx_settings,
    uint8_t                           antenna,
    enum RfFilter                     rf_band,
    uint16_t                          temp_adc,
    int16_t                           baseband_freq)
{
    if (cal_version != 0x05)
    {
        return (int16_t)rssi_raw;
    }

    struct Ex10CalibrationParamsV5 const* cal_params =
        get_ex10_cal_v5()->get_params();

    int16_t const gain_offset =
        get_gain_offset(cal_params, rx_settings, antenna, rf_band);
    int16_t const mode_offset = get_mode_rssi_offset(rf_mode, baseband_freq);
    int16_t const temp_offset = get_temp_offset(cal_params, temp_adc);

    int32_t const rssi_log2_compensated =
        rssi_raw - gain_offset - mode_offset - temp_offset;

    int16_t const rssi_cdbm =
        log2_to_cdbm(cal_params, (uint16_t)rssi_log2_compensated);

    return rssi_cdbm;
}

static uint16_t get_rf_mode_baseband_freq_khz(enum RfModes rf_mode)
{
    struct RssiCompensationLut const* rssi_comp = get_ex10_rssi_compensation();

    for (uint16_t idx = 0; idx < NUM_MODES; ++idx)
    {
        if (rf_mode == (enum RfModes)rssi_comp->rf_modes[idx])
        {
            return rssi_comp
                ->rx_modes_blf_khz[rssi_comp->rf_mode_to_rx_mode[idx]];
        }
    }

    // Error case: Requested RF mode does not exist.
    /// @todo Is this the right thing to do?
    return 0;
}

/**
 * Compensates RSSI value for temperature, analog settings, and RF mode
 * based on calibration table.
 *
 * @param rssi_raw     Raw RSSI_LOG_2 value from firmware op
 * @param rf_mode      RF mode
 * @param rx_settings  Settings corresponding to RxGainControl register
 * @param antenna      Antenna port used
 * @param rf_band      Which RF band we are using
 * @param temp_adc     Temperature ADC code
 *
 * @return int16_t     Compensated RSSI value in cdBm
 */
static int16_t get_compensated_rssi(
    uint16_t                          rssi_raw,
    enum RfModes                      rf_mode,
    const struct RxGainControlFields* rx_settings,
    uint8_t                           antenna,
    enum RfFilter                     rf_band,
    uint16_t                          temp_adc)
{
    int16_t const baseband_freq =
        (int16_t)get_rf_mode_baseband_freq_khz(rf_mode);
    int16_t const rssi_cdbm =
        get_compensated_rssi_baseband_freq_offset(rssi_raw,
                                                  rf_mode,
                                                  rx_settings,
                                                  antenna,
                                                  rf_band,
                                                  temp_adc,
                                                  baseband_freq);
    return rssi_cdbm;
}

/**
 * Convert RSSI value in cdBm back to RSSI log2 counts using the
 * temperature, analog settings, and RF mode.
 *
 * @param rssi_cdbm      Compensated RSSI value in cdBm
 * @param rf_mode        RF mode
 * @param rx_settings    Settings corresponding to RxGainControl register
 * @param antenna        Antenna port used
 * @param rf_band        Which RF band we are using
 * @param temp_adc       Temperature ADC code
 * @param baseband_freq  The baseband frequency at which we expect the signal
 *
 * @return RSSI value in LOG2 counts derived from the RSSI value in cdBm
 */
static uint16_t get_rssi_log2_baseband_freq_offset(
    int16_t                           rssi_cdbm,
    enum RfModes                      rf_mode,
    const struct RxGainControlFields* rx_settings,
    uint8_t                           antenna,
    enum RfFilter                     rf_band,
    uint16_t                          temp_adc,
    int16_t                           baseband_freq)
{
    if (cal_version != 0x05)
    {
        return (uint16_t)0u;
    }

    struct Ex10CalibrationParamsV5 const* cal_params =
        get_ex10_cal_v5()->get_params();

    int16_t const gain_offset =
        get_gain_offset(cal_params, rx_settings, antenna, rf_band);
    int16_t const mode_offset = get_mode_rssi_offset(rf_mode, baseband_freq);
    int16_t const temp_offset = get_temp_offset(cal_params, temp_adc);

    uint16_t const rssi_log2_compensated = cdbm_to_log2(cal_params, rssi_cdbm);

    int16_t const rssi_log2 = (int16_t)rssi_log2_compensated + gain_offset +
                              mode_offset + temp_offset;
    if (rssi_log2 <= 0)
    {
        return (uint16_t)0u;
    }

    return (uint16_t)rssi_log2;
}

/**
 * Convert RSSI value in cdBm back to RSSI log2 counts using the
 * temperature, analog settings, and RF mode.
 *
 * @param rssi_cdbm       Compensated RSSI value in cdBm
 * @param temp_adc        Temperature ADC code
 * @param rf_mode         RF mode
 * @param rx_settings     Settings corresponding to RxGainControl register
 * @param antenna         Antenna port used
 * @param rf_band         Which RF band we are using
 *
 * @return RSSI value in LOG2 counts derived from the RSSI value in cdBm
 */
static uint16_t get_rssi_log2(int16_t                           rssi_cdbm,
                              enum RfModes                      rf_mode,
                              const struct RxGainControlFields* rx_settings,
                              uint8_t                           antenna,
                              enum RfFilter                     rf_band,
                              uint16_t                          temp_adc)
{
    int16_t const baseband_freq =
        (int16_t)get_rf_mode_baseband_freq_khz(rf_mode);
    uint16_t const rssi_log2 =
        get_rssi_log2_baseband_freq_offset(rssi_cdbm,
                                           rf_mode,
                                           rx_settings,
                                           antenna,
                                           rf_band,
                                           temp_adc,
                                           baseband_freq);
    return rssi_log2;
}

/**
 * Compensates LBT RSSI value for temperature, analog settings, and RF
 * mode based on calibration table.
 *
 * @param rssi_raw     Raw RSSI_LOG_2 value from firmware op
 * @param temp_adc     Temperature ADC code
 * @param rx_settings  Settings corresponding to RxGainControl register
 * @param antenna      Antenna port used
 * @param rf_band      Which RF band we are using
 *
 * @return Compensated RSSI value in cdBm
 */
static int16_t get_compensated_lbt_rssi(
    uint16_t                          rssi_raw,
    const struct RxGainControlFields* rx_settings,
    uint8_t                           antenna,
    enum RfFilter                     rf_band,
    uint16_t                          temp_adc)
{
    if (cal_version != 0x05)
    {
        return (int16_t)rssi_raw;
    }

    struct Ex10CalibrationParamsV5 const* cal_params =
        get_ex10_cal_v5()->get_params();

    int16_t const gain_offset =
        get_gain_offset(cal_params, rx_settings, antenna, rf_band);
    int16_t const baseband_offset = get_lbt_baseband_offset(
        get_ex10_rssi_compensation()->lbt_cordic_freq_shift);
    int16_t const temp_offset = get_temp_offset(cal_params, temp_adc);

    int32_t const rssi_log2_compensated =
        rssi_raw - gain_offset - baseband_offset - temp_offset;

    int16_t const rssi_cdbm =
        log2_to_cdbm(cal_params, (uint16_t)rssi_log2_compensated);

    return rssi_cdbm;
}

/**
 * Compensates for frequency shift in coarse gain power output using linear
 * interpolation (Calibration v5)
 *
 * @param cal_params    The Calibration V5 data.
 * @param frequency_khz Transmit frequency in kHz
 * @param rf_band       "UpperBand" or "LowerBand" as prefix to cal params
 * @note This compensation algorithm requires same number of array elements in
 *       both frequency list and forward power frequency lookup tables
 * @note The calibration array of frequencies is monotonically increasing
 *
 * @return: Power (dB) offset to compensate frequency
 */
static float compensate_coarse_freq_v5(
    struct Ex10CalibrationParamsV5 const* cal_params,
    uint32_t                              frequency_khz,
    enum RfFilter                         rf_band)
{
    uint16_t const freq_count =
        (rf_band == LOWER_BAND)
            ? ARRAY_SIZE(cal_params->lower_band_lo_pdet_freqs.lo_pdet_freqs)
            : ARRAY_SIZE(cal_params->upper_band_lo_pdet_freqs.lo_pdet_freqs);

    float const* const freqs =
        (rf_band == LOWER_BAND)
            ? cal_params->lower_band_lo_pdet_freqs.lo_pdet_freqs
            : cal_params->upper_band_lo_pdet_freqs.lo_pdet_freqs;

    float const* const pwr_shifts =
        (rf_band == LOWER_BAND)
            ? cal_params->lower_band_fwd_pwr_freq_lut.fwd_pwr_shifts
            : cal_params->upper_band_fwd_pwr_freq_lut.fwd_pwr_shifts;

    float const freq = (float)frequency_khz / (float)1000.0;

    // Special cases (out of band) - return nearest value
    if (freq < freqs[0u])
    {
        return pwr_shifts[0u];
    }

    if (freq > freqs[freq_count - 1u])
    {
        return pwr_shifts[freq_count - 1u];
    }

    // Search LUT for nearest element(s). To avoid indexing out of bounds if
    // freq == lowest entry in table, start search at 2nd entry.
    float power_ofs_freq = 0;
    for (uint16_t idx = 1u; idx < freq_count; ++idx)
    {
        if (freq <= freqs[idx])
        {
            // Value lies between idx and (idx-1) in table.  Interpolate.
            float fraction =
                (freq - freqs[idx - 1u]) / (freqs[idx] - freqs[idx - 1u]);
            power_ofs_freq =
                pwr_shifts[idx - 1u] +
                fraction * (pwr_shifts[idx] - pwr_shifts[idx - 1u]);
            break;
        }
    }

    return power_ofs_freq;
}

/**
 * Picks the Tx attenuator setting based on a given
 * power, frequency, and temperature.
 *
 * @param tx_power_cdbm         Target Tx power in cdBm
 * @param frequency_khz         The RF frequency in kHz.
 * @param temperature_adc       The measured temperature ADC count.
 * @param rf_band               The regulatory band as contained in the
 *                              calibration data.
 * @param temp_comp_enabled     If enabled, the provided temperature ADC
 *                              will be used to compensate the target power.
 *                              If disabled, temperature compensation will
 *                              not be applied to the target power.
 *
 * @return uint8_t Tx attenuator index corresponding to requested power.
 */
static uint8_t choose_coarse_atten_v5(int16_t       tx_power_cdbm,
                                      uint32_t      frequency_khz,
                                      uint16_t      temperature_adc,
                                      bool          temp_comp_enabled,
                                      enum RfFilter rf_band)
{
    struct Ex10CalibrationParamsV5 const* cal_params =
        get_ex10_cal_v5()->get_params();

    struct PerBandFwdPowerTempSlopeV5 const t_comp_slope =
        (rf_band == LOWER_BAND) ? cal_params->lower_band_fwd_power_temp_slope
                                : cal_params->upper_band_fwd_power_temp_slope;
    struct PerBandCalTempV5 const t_comp_offset =
        (rf_band == LOWER_BAND) ? cal_params->lower_band_cal_temp
                                : cal_params->upper_band_cal_temp;

    float const    t_slope = t_comp_slope.fwd_power_temp_slope;
    uint16_t const t_0     = t_comp_offset.cal_temp_a_d_c;

    float tx_power_dbm = (float)tx_power_cdbm / (float)100.0;

    // If compensation is enabled, apply temperature and frequency compensation
    float power_ofs_temp = 0;
    float power_ofs_freq = 0;
    if (temp_comp_enabled)
    {
        power_ofs_temp = t_slope * (temperature_adc - t_0);
    }
    power_ofs_freq =
        compensate_coarse_freq_v5(cal_params, frequency_khz, rf_band);
    tx_power_dbm -= (power_ofs_temp + power_ofs_freq);

    float const* const atten_target_power =
        (rf_band == LOWER_BAND)
            ? cal_params->lower_band_fwd_power_coarse_pwr_cal.coarse_attn_cal
            : cal_params->upper_band_fwd_power_coarse_pwr_cal.coarse_attn_cal;

    uint16_t const atten_target_power_length =
        (rf_band == LOWER_BAND)
            ? ARRAY_SIZE(cal_params->lower_band_fwd_power_coarse_pwr_cal
                             .coarse_attn_cal)
            : ARRAY_SIZE(cal_params->upper_band_fwd_power_coarse_pwr_cal
                             .coarse_attn_cal);

    // Lower attenuation entries may have been skipped during calibration
    // because the power at these points was too high. These were set to 0xff
    // values and should not be selected by open loop stage.
    // We will start the selection from the first valid entry.
    uint16_t start_idx = 0;
    for (start_idx = 0u; start_idx < atten_target_power_length; ++start_idx)
    {
        if (atten_target_power[start_idx] < (float)255.0)
        {
            break;
        }
    }

    // If we haven't found a single valid entry, default to lowest attenuation
    // setting.
    if (start_idx == atten_target_power_length)
    {
        return (uint8_t)(atten_target_power_length - 1u);
    }

    // If the requested power out is higher than the power at the top of the
    // calibration table (lowest valid attenuation), do not search higher
    // attenuation values.
    if (tx_power_dbm > atten_target_power[start_idx])
    {
        return (uint8_t)start_idx;
    }

    // Search coarse power calibration table for most appropriate value.
    // Start search at max calibrated power, return lowest calibrated power
    // if no good match is found.
    uint16_t atten_idx = atten_target_power_length - 1u;
    for (uint16_t idx = start_idx; idx < atten_target_power_length; ++idx)
    {
        if ((fabsf(atten_target_power[idx] - tx_power_dbm) <=
             fabsf(atten_target_power[idx + 1u] - tx_power_dbm)) ||
            (idx == atten_target_power_length - 1u))
        {
            atten_idx = idx;
            break;
        }
    }

    return (uint8_t)atten_idx;
}

/**
 * Helper function for validate that a given ADC value falls within the valid
 * range given in the calibration parameters.
 *
 * @param valid_min_adc     Min valid adc value
 * @param valid_max_adc     Max valid adc value
 * @param adc1              The pdet adc value to validate
 * @param adc2              The pdet adc value to validate
 *
 * @return bool ADC values in valid range
 */
static bool valid_pdet_adcs(uint16_t valid_min_adc,
                            uint16_t valid_max_adc,
                            uint16_t adc1,
                            uint16_t adc2)
{
    if ((adc1 >= valid_min_adc && adc1 <= valid_max_adc) &&
        (adc2 >= valid_min_adc && adc2 <= valid_max_adc))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * Converts power detector block index to associated enum.
 * @param pdet_index The power detector block index, [0,2]
 * @param use_forward_power Selects the power detector:
 *      true:  forward (Tx) power detector
 *      false: reverse (Rx) power detector
 *
 * @return enum AuxAdcControlChannelEnableBits corresponding to index
 */
static enum AuxAdcControlChannelEnableBits pdet_index_to_enum(
    uint16_t pdet_index,
    bool     use_forward_power)
{
    enum AuxAdcControlChannelEnableBits lo_channel[] = {
        ChannelEnableBitsPowerLo0,
        ChannelEnableBitsPowerLo1,
        ChannelEnableBitsPowerLo2,
    };
    enum AuxAdcControlChannelEnableBits rx_channel[] = {
        ChannelEnableBitsPowerRx0,
        ChannelEnableBitsPowerRx1,
        ChannelEnableBitsPowerRx2,
    };

    enum AuxAdcControlChannelEnableBits enable_bits;
    if (pdet_index < ARRAY_SIZE(lo_channel))
    {
        enable_bits = (use_forward_power) ? lo_channel[pdet_index]
                                          : rx_channel[pdet_index];
    }
    else
    {
        enable_bits = ChannelEnableBitsNone;
    }
    return enable_bits;
}

/**
 * Compensates for temperature shift in power detector curves using
 * linear interpolation (Calibration v5)
 *
 * @param cal_params The calibration object.
 * @param temp_adc      Measured tempsense ADC.
 * @param pdet_idx      Index of the PDET stage to compensate temp.
 * @param rf_band "UpperBand" or "LowerBand" to determine cal params.
 *
 * @return float Power offset from target power to compensate temp
 */
static float compensate_pdet_temp_v5(
    struct Ex10CalibrationParamsV5 const* cal_params,
    uint16_t                              temp_adc,
    uint16_t                              pdet_idx,
    enum RfFilter                         rf_band)
{
    float power_offset_temp = 0;

    // Slopes of piecewise linear temperature compensation
    struct PerBandLoPdetTempSlopeV5 const temp_slopes =
        (rf_band == LOWER_BAND) ? cal_params->lower_band_lo_pdet_temp_slope
                                : cal_params->upper_band_lo_pdet_temp_slope;
    const float m_t = temp_slopes.lo_pdet_temp_slope[pdet_idx];

    struct PerBandCalTempV5 const offset =
        (rf_band == LOWER_BAND) ? cal_params->lower_band_cal_temp
                                : cal_params->upper_band_cal_temp;
    const uint16_t t_0 = offset.cal_temp_a_d_c;

    power_offset_temp = m_t * (temp_adc - t_0);

    return power_offset_temp;
}

/**
 * Compensates for frequency shift in power detector curves using linear
 * interpolation (Calibration v5)
 *
 * @param cal_params    The Calibration V5 data
 * @param frequency_khz Transmit frequency in kHz
 * @param pdet_idx      Index of the PDET stage to compensate freq
 * @param rf_band       "UpperBand" or "LowerBand" as prefix to cal params
 * @note This compensation algorithm requires same number of array elements in
 *       both frequency list and power compensation shift measurements
 * @note The calibration array of frequencies is monotonically increasing
 *
 * @return: Power offset (cdB) from target power to compensate frequency
 */
static float compensate_pdet_freq_v5(
    struct Ex10CalibrationParamsV5 const* cal_params,
    uint32_t                              frequency_khz,
    uint16_t                              pdet_idx,
    enum RfFilter                         rf_band)
{
    uint16_t const freq_count =
        (rf_band == LOWER_BAND)
            ? ARRAY_SIZE(cal_params->lower_band_lo_pdet_freqs.lo_pdet_freqs)
            : ARRAY_SIZE(cal_params->upper_band_lo_pdet_freqs.lo_pdet_freqs);

    float const* const freqs =
        (rf_band == LOWER_BAND)
            ? cal_params->lower_band_lo_pdet_freqs.lo_pdet_freqs
            : cal_params->upper_band_lo_pdet_freqs.lo_pdet_freqs;

    int16_t const* pwr_shifts = NULL;
    if (rf_band == LOWER_BAND)
    {
        if (pdet_idx == 2u)
        {
            pwr_shifts = cal_params->lower_band_lo_pdet_freq_lut
                             .lo_pdet_freq_adc_shifts2;
        }
        else if (pdet_idx == 1u)
        {
            pwr_shifts = cal_params->lower_band_lo_pdet_freq_lut
                             .lo_pdet_freq_adc_shifts1;
        }
        else
        {
            pwr_shifts = cal_params->lower_band_lo_pdet_freq_lut
                             .lo_pdet_freq_adc_shifts0;
        }
    }
    else
    {
        if (pdet_idx == 2u)
        {
            pwr_shifts = cal_params->upper_band_lo_pdet_freq_lut
                             .lo_pdet_freq_adc_shifts2;
        }
        else if (pdet_idx == 1u)
        {
            pwr_shifts = cal_params->upper_band_lo_pdet_freq_lut
                             .lo_pdet_freq_adc_shifts1;
        }
        else
        {
            pwr_shifts = cal_params->upper_band_lo_pdet_freq_lut
                             .lo_pdet_freq_adc_shifts0;
        }
    }

    float const freq = (float)frequency_khz / (float)1000.0;

    // Special cases (out of band) - return nearest value
    if (freq < freqs[0u])
    {
        return (float)pwr_shifts[0u];
    }

    if (freq > freqs[freq_count - 1u])
    {
        return (float)pwr_shifts[freq_count - 1u];
    }

    // Search LUT for nearest element(s)
    float pwr_shift = 0;
    for (uint16_t idx = 1u; idx < freq_count; ++idx)
    {
        if (freq <= freqs[idx])
        {
            // Value lies between idx and (idx-1) in table.  Interpolate.
            float fraction =
                (freq - freqs[idx - 1u]) / (freqs[idx] - freqs[idx - 1u]);
            pwr_shift = (float)pwr_shifts[idx - 1u] +
                        fraction * (pwr_shifts[idx] - pwr_shifts[idx - 1u]);
            break;
        }
    }
    return pwr_shift;
}

/**
 * Finds the first index with non cleared memory e.g. a value that is not a
 * uint8_t max of 255. The value in the buffer will be a float of 255.0, but
 * the decimal can be dropped.
 *
 * @param buffer The buffer to check through
 * @param length The length of the buffer thus this checks < length
 * non-inclusive.
 *
 * @return uint16_t The index of the first non-max value. If all indexes are
 * cleared, the length will be returned. Since this is a non-inclusive length,
 * this index is not a possible correct value.
 */
static uint16_t get_first_non_cleared_index(const float* buffer,
                                            uint16_t     length)
{
    for (uint16_t idx = 0; idx < length; ++idx)
    {
        if (buffer[idx] < 255.0)
        {
            return idx;
        }
    }
    return length;
}

/**
 * Converts power to ADC code based on calibration, temp, frequency.
 *
 * @param tx_power_cdbm      Target Tx power in cdBm
 * @param temperature_adc    The measured temperature ADC count.
 * @param temp_comp_enabled  If enabled, the provided temperature ADC
 *                           will be used to compensate the target adc target.
 *                           If disabled, temperature compensation will
 *                           not be applied to the target adc target.
 * @param frequency_khz      The RF frequency in kHz.
 * @param rf_band            The regulatory band as contained in the
 *                           calibration data.
 * @param power_detector_adc A pointer to enum for which adc channel(s) to use
 *                           for power ramp
 *
 * @return uint16_t ADC code corresponding to power.
 *         enum AuxAdcControlChannelEnableBits power_detector_adc updated with
 *         which adc channel should be used to ramp.
 */
static uint16_t power_to_adc(
    int16_t                              tx_power_cdbm,
    uint32_t                             frequency_khz,
    uint16_t                             temperature_adc,
    bool                                 temp_comp_enabled,
    enum RfFilter                        rf_band,
    enum AuxAdcControlChannelEnableBits* power_detector_adc)
{
    if (cal_version != 0x05)
    {
        return CAL_FUNC_NOT_SUPPORTED;
    }

    assert(power_detector_adc);

    struct Ex10CalibrationParamsV5 const* cal_params =
        get_ex10_cal_v5()->get_params();

    *power_detector_adc = ChannelEnableBitsNone;

    uint16_t const powers_length =
        (rf_band == LOWER_BAND)
            ? ARRAY_SIZE(cal_params->lower_band_fwd_power_coarse_pwr_cal
                             .coarse_attn_cal)
            : ARRAY_SIZE(cal_params->upper_band_fwd_power_coarse_pwr_cal
                             .coarse_attn_cal);

    float const* const powers =
        (rf_band == LOWER_BAND)
            ? cal_params->lower_band_fwd_power_coarse_pwr_cal.coarse_attn_cal
            : cal_params->upper_band_fwd_power_coarse_pwr_cal.coarse_attn_cal;

    uint16_t const* adcs[ADCS_LENGTH];

    uint16_t const adc_lut_length =
        (rf_band == LOWER_BAND)
            ? ARRAY_SIZE(cal_params->lower_band_pdet_adc_lut.pdet2_adc_lut)
            : ARRAY_SIZE(cal_params->upper_band_pdet_adc_lut.pdet2_adc_lut);

    adcs[2u] = (rf_band == LOWER_BAND)
                   ? cal_params->lower_band_pdet_adc_lut.pdet2_adc_lut
                   : cal_params->upper_band_pdet_adc_lut.pdet2_adc_lut;
    adcs[1u] = (rf_band == LOWER_BAND)
                   ? cal_params->lower_band_pdet_adc_lut.pdet1_adc_lut
                   : cal_params->upper_band_pdet_adc_lut.pdet1_adc_lut;
    adcs[0u] = (rf_band == LOWER_BAND)
                   ? cal_params->lower_band_pdet_adc_lut.pdet0_adc_lut
                   : cal_params->upper_band_pdet_adc_lut.pdet0_adc_lut;

    uint16_t       p_adc              = 0u;
    const uint16_t power_array_length = powers_length - 1u;
    // If all indices are cleared, the index returned will be the max length,
    // which will raise the lower acceptable bound equal to the max. The
    // requested power will fall below this and default to PDET 0;
    const uint16_t first_non_max_pwr_idx =
        get_first_non_cleared_index(powers, power_array_length);

    uint16_t p_adc_list[ADCS_LENGTH];

    uint16_t const valid_min_adc = cal_params->valid_pdet_adcs.valid_min_adc;
    uint16_t const valid_max_adc = cal_params->valid_pdet_adcs.valid_max_adc;

    // Searching for correct power detector block to use for power ramp
    for (uint16_t p_ind = ADCS_LENGTH - 1u; (int16_t)p_ind >= 0; --p_ind)
    {
        float pwr_ofs_temp = 0u;
        if (temp_comp_enabled)
        {
            pwr_ofs_temp = compensate_pdet_temp_v5(
                cal_params, temperature_adc, p_ind, rf_band);
        }
        float const pwr_ofs_freq =
            compensate_pdet_freq_v5(cal_params, frequency_khz, p_ind, rf_band) /
            (float)100.0;

        float const pwr_comp =
            (float)tx_power_cdbm / (float)100.0 - pwr_ofs_temp + pwr_ofs_freq;

        // Search LUT for nearest element(s). Assume monotonic LUT; adjacent
        // elements
        uint16_t i_1 = first_non_max_pwr_idx;
        uint16_t i_2 = i_1 + 1u;
        for (uint16_t idx = first_non_max_pwr_idx + 1u;
             idx < adc_lut_length - 1u;
             ++idx)
        {
            if ((pwr_comp >= powers[idx]) && (pwr_comp <= powers[idx - 1u]) &&
                valid_pdet_adcs(valid_min_adc,
                                valid_max_adc,
                                adcs[p_ind][i_1],
                                adcs[p_ind][i_2]))
            {
                i_1                 = idx - 1u;
                i_2                 = idx;
                *power_detector_adc = pdet_index_to_enum(p_ind, true);
                break;
            }
            if (fabs(pwr_comp - powers[idx]) < fabs(pwr_comp - powers[i_2]))
            {
                i_1 = i_2;
                i_2 = idx;
            }
        }

        uint16_t const adc1 = adcs[p_ind][i_1];
        uint16_t const adc2 = adcs[p_ind][i_2];
        float const    fraction =
            (pwr_comp - powers[i_2]) / (powers[i_1] - powers[i_2]);
        int32_t const adc = (int32_t)adc2 + (int32_t)((adc1 - adc2) * fraction);

        if (*power_detector_adc == pdet_index_to_enum(p_ind, true))
        {
            p_adc = (uint16_t)adc;
            break;  // breaks from p_ind for loop
        }
        p_adc_list[p_ind] = (uint16_t)adc;
    }

    // Since power is not in range of any cal curve, search each of the power
    // detector tables. Use the block that is closest to the valid range.
    if (*power_detector_adc == ChannelEnableBitsNone)
    {
        // Set the min to the first value
        uint16_t pdet_block_used = 0;
        uint32_t smallest_val =
            abs_int32(valid_max_adc + valid_min_adc - 2 * p_adc_list[0]);
        // Loop through and check if any values are outside of the min to max
        // range. If there is anything outside the max range, cap it, otherwise
        // look for the pdet block to use closest to the commanded power.
        bool pwr_target_too_low  = true;
        bool pwr_target_too_high = true;
        for (uint16_t pdet_block_search = ADCS_LENGTH; pdet_block_search > 0;
             --pdet_block_search)
        {
            uint16_t const pdet_block_index = pdet_block_search - 1;
            // Distance to pdet which we want to be as small as possible
            uint32_t const val = abs_int32(valid_max_adc + valid_min_adc -
                                           2 * p_adc_list[pdet_block_index]);
            pwr_target_too_low &=
                (p_adc_list[pdet_block_index] <= valid_min_adc);
            pwr_target_too_high &=
                (p_adc_list[pdet_block_index] >= valid_max_adc);

            if (val < smallest_val)
            {
                pdet_block_used = pdet_block_index;
                smallest_val    = val;
            }
        }

        // If a cap was hit, we choose the capped pdet_block_used and p_adc.
        // If the cap was not hit, the pdet_block_used was set to the pdet
        // closest to the command power in the loop. We then just need to set
        // the p_adc based off the p_adc_list.
        if (pwr_target_too_low)
        {
            // Choosing capped value due to being too low
            pdet_block_used = 0;
            p_adc           = valid_min_adc;
        }
        else if (pwr_target_too_high)
        {
            // Choosing capped value due to being too high
            pdet_block_used = ADCS_LENGTH - 1;
            p_adc           = valid_max_adc;
        }
        else
        {
            // Using pdet closest to commanded value
            p_adc = p_adc_list[pdet_block_used];
        }
        // Set the reference to power detector adc which is used by the caller
        *power_detector_adc = pdet_index_to_enum(pdet_block_used, true);
    }
    return p_adc;
}

struct ClosestPower
{
    float    power;
    int16_t  block_index;
    uint16_t power_index;
};

/**
 * Converts reverse power to ADC code based on calibration, temp, frequency.
 *
 * @param reverse_power_cdBm The RF power amplitude in cdBm.
 * @param temperature_adc    The measured temperature ADC count.
 * @param temp_comp_enabled  If enabled, the provided temperature ADC
 *                           will be used to compensate the target adc target.
 *                           If disabled, temperature compensation will
 *                           not be applied to the target adc target.
 * @param frequency_khz      The RF frequency in kHz.
 * @param rf_band            The regulatory band as contained in the
 *                           calibration data.
 * @param reverse_power_detector_adc A pointer to enum for which adc channel(s)
 * to use to measure reverse power.
 *
 * @return uint16_t ADC code corresponding to power.
 *         enum AuxAdcControlChannelEnableBits power_detector_adc updated with
 *         which reverse power channel to use to measure reflection.
 */
static uint16_t reverse_power_to_adc(
    int16_t                              reverse_power_cdBm,
    uint32_t                             frequency_khz,
    uint16_t                             temperature_adc,
    bool                                 temp_comp_enabled,
    enum RfFilter                        rf_band,
    enum AuxAdcControlChannelEnableBits* reverse_power_detector_adc)
{
    if (cal_version != 0x05)
    {
        return CAL_FUNC_NOT_SUPPORTED;
    }

    assert(reverse_power_detector_adc);

    struct Ex10CalibrationParamsV5 const* cal_params =
        get_ex10_cal_v5()->get_params();

    *reverse_power_detector_adc = ChannelEnableBitsNone;

    uint16_t const powers_length =
        (rf_band == LOWER_BAND)
            ? ARRAY_SIZE(cal_params->lower_band_fwd_power_coarse_pwr_cal
                             .coarse_attn_cal)
            : ARRAY_SIZE(cal_params->upper_band_fwd_power_coarse_pwr_cal
                             .coarse_attn_cal);

    float const* const powers =
        (rf_band == LOWER_BAND)
            ? cal_params->lower_band_fwd_power_coarse_pwr_cal.coarse_attn_cal
            : cal_params->upper_band_fwd_power_coarse_pwr_cal.coarse_attn_cal;

    uint16_t const* adcs[ADCS_LENGTH];

    uint16_t const adc_lut_length =
        (rf_band == LOWER_BAND)
            ? ARRAY_SIZE(cal_params->lower_band_pdet_adc_lut.pdet2_adc_lut)
            : ARRAY_SIZE(cal_params->upper_band_pdet_adc_lut.pdet2_adc_lut);

    adcs[2u] = (rf_band == LOWER_BAND)
                   ? cal_params->lower_band_pdet_adc_lut.pdet2_adc_lut
                   : cal_params->upper_band_pdet_adc_lut.pdet2_adc_lut;
    adcs[1u] = (rf_band == LOWER_BAND)
                   ? cal_params->lower_band_pdet_adc_lut.pdet1_adc_lut
                   : cal_params->upper_band_pdet_adc_lut.pdet1_adc_lut;
    adcs[0u] = (rf_band == LOWER_BAND)
                   ? cal_params->lower_band_pdet_adc_lut.pdet0_adc_lut
                   : cal_params->upper_band_pdet_adc_lut.pdet0_adc_lut;

    uint16_t       p_adc              = 0u;
    const uint16_t power_array_length = powers_length - 1u;
    // If all indices are cleared, the index returned will be the max length,
    // which will raise the lower acceptable bound equal to the max. The
    // requested power will fall below this and default to PDET 0;
    const uint16_t first_non_max_pwr_idx =
        get_first_non_cleared_index(powers, power_array_length);

    uint16_t const valid_min_adc = cal_params->valid_pdet_adcs.valid_min_adc;
    uint16_t const valid_max_adc = cal_params->valid_pdet_adcs.valid_max_adc;

    struct ClosestPower closest_power = {
        .power = USHRT_MAX, .block_index = 0, .power_index = 0};

    // Searching for correct power detector block to use for power ramp
    for (int16_t p_ind = ADCS_LENGTH - 1u; p_ind >= 0; --p_ind)
    {
        float pwr_ofs_temp = 0u;
        if (temp_comp_enabled)
        {
            pwr_ofs_temp = compensate_pdet_temp_v5(
                cal_params, temperature_adc, (uint16_t)p_ind, rf_band);
        }
        float const pwr_ofs_freq =
            compensate_pdet_freq_v5(
                cal_params, frequency_khz, (uint16_t)p_ind, rf_band) /
            (float)100.0;

        float const pwr_comp =
            reverse_power_cdBm / (float)100.0 - pwr_ofs_temp + pwr_ofs_freq;

        // Search LUT for nearest element(s). Assume monotonic LUT; adjacent
        // elements
        uint16_t i_1 = 0u;
        uint16_t i_2 = 0u;
        for (uint16_t idx = first_non_max_pwr_idx; idx < adc_lut_length - 1u;
             ++idx)
        {
            i_1 = idx;
            i_2 = i_1 + 1u;
            if ((pwr_comp >= powers[i_2]) && (pwr_comp <= powers[i_1]))
            {
                break;
            }
        }
        // Find the index closest to the necessary power in case there is no
        // perfect match or the pdet values are invalid.
        for (uint16_t idx = first_non_max_pwr_idx; idx < adc_lut_length - 1u;
             ++idx)
        {
            float power_diff = pwr_comp - powers[idx];
            if (power_diff < closest_power.power)
            {
                closest_power.block_index = p_ind;
                closest_power.power       = power_diff;
                closest_power.power_index = idx;
            }
        }

        uint16_t adc1  = adcs[p_ind][i_1];
        uint16_t adc2  = adcs[p_ind][i_2];
        float fraction = (pwr_comp - powers[i_2]) / (powers[i_1] - powers[i_2]);
        p_adc          = (uint16_t)(adc2 + (int16_t)((adc1 - adc2) * fraction));

        if (valid_pdet_adcs(valid_min_adc,
                            valid_max_adc,
                            adcs[p_ind][i_1],
                            adcs[p_ind][i_2]))
        {
            *reverse_power_detector_adc =
                pdet_index_to_enum((uint16_t)p_ind, false);
            break;  // breaks from p_ind for loop
        }
    }

    // Since power is not in range of any cal curve, search each of the power
    // detector tables. Use the block that is closest to the valid range.
    if (*reverse_power_detector_adc == ChannelEnableBitsNone)
    {
        uint16_t pdet_block_used = 0;
        float    under_cal_float = fabs(powers[adc_lut_length - 1]) * 100;
        float    over_cal_float  = fabs(powers[first_non_max_pwr_idx]) * 100;

        under_cal_float =
            under_cal_float < SHRT_MIN ? SHRT_MIN : under_cal_float;
        under_cal_float =
            under_cal_float > SHRT_MAX ? SHRT_MAX : under_cal_float;

        over_cal_float = over_cal_float < SHRT_MIN ? SHRT_MIN : over_cal_float;
        over_cal_float = over_cal_float > SHRT_MAX ? SHRT_MAX : over_cal_float;

        // Converting to int16_t to match reverse_power_cdBm
        const int16_t under_cal_range_cdBm = (int16_t)under_cal_float;
        const int16_t over_cal_range_cdBm  = (int16_t)over_cal_float;

        if (reverse_power_cdBm < under_cal_range_cdBm)
        {
            // Below calibrated power we use the lowest PDET
            pdet_block_used = 0;
            p_adc           = adcs[pdet_block_used][0];
            for (size_t coarse_index = 0; coarse_index <= adc_lut_length;
                 coarse_index++)
            {
                const uint16_t curr_val = adcs[pdet_block_used][coarse_index];
                const bool     curr_val_less = curr_val <= p_adc;
                if ((curr_val >= valid_min_adc) && curr_val_less)
                {
                    p_adc = adcs[pdet_block_used][coarse_index];
                }
            }
        }
        else if (reverse_power_cdBm > over_cal_range_cdBm)
        {
            // Above calibrated power we use the highest PDET
            pdet_block_used = ADCS_LENGTH - 1;
            p_adc           = adcs[pdet_block_used][0];
            for (size_t coarse_index = 0; coarse_index <= adc_lut_length;
                 coarse_index++)
            {
                const uint16_t curr_val = adcs[pdet_block_used][coarse_index];
                const bool     curr_val_greater = curr_val >= p_adc;
                if ((curr_val <= valid_max_adc) && curr_val_greater)
                {
                    p_adc = adcs[pdet_block_used][coarse_index];
                }
            }
        }
        else
        {
            // Use the closest value saved earlier
            p_adc = adcs[closest_power.block_index][closest_power.power_index];
        }

        *reverse_power_detector_adc =
            pdet_index_to_enum(pdet_block_used, false);
    }
    return p_adc;
}

static uint16_t get_adc_error_threshold(void const* cal_params)
{
    if (cal_version == 0x05)
    {
        assert(cal_params);
        return ((struct Ex10CalibrationParamsV5 const*)cal_params)
            ->control_loop_params.error_threshold;
    }
    else
    {
        return 5;
    }
}

static uint16_t get_loop_gain(void const* cal_params)
{
    if (cal_version == 0x05)
    {
        assert(cal_params);
        return ((struct Ex10CalibrationParamsV5 const*)cal_params)
            ->control_loop_params.loop_gain_divisor;
    }
    else
    {
        return 400;
    }
}

static uint32_t get_max_iterations(void const* cal_params)
{
    if (cal_version == 0x05)
    {
        assert(cal_params);
        return ((struct Ex10CalibrationParamsV5 const*)cal_params)
            ->control_loop_params.max_iterations;
    }
    else
    {
        return 10;
    }
}

static int16_t get_adjusted_tx_scalar(int16_t tx_scalar)
{
    // Note: This allows the user to drop the tx_scalar value and avoid
    // overshoot in the Tx waveform. This percentage can be changed according to
    // the user setup to meet overshoot specifications.
    uint8_t drop_percentage = 0;
    int32_t temp_scalar     = tx_scalar * (100 - drop_percentage);
    temp_scalar /= 100;
    return (int16_t)temp_scalar;
}

static struct PowerConfigs get_power_control_params(int16_t  tx_power_cdbm,
                                                    uint32_t frequency_khz,
                                                    uint16_t temperature_adc,
                                                    bool     temp_comp_enabled,
                                                    enum RfFilter rf_band)
{
    if (cal_version == 0xFF)
    {
        // If the calibration data is erased, the version will be read as FFs
        // and we'll use default calibration
        uint8_t const tx_atten =
            (uint8_t)(31u - (uint32_t)(tx_power_cdbm / 100));
        struct PowerConfigs const power_configs = {
            .tx_atten            = tx_atten,
            .tx_scalar           = 2047,
            .dc_offset           = 0,
            .adc_target          = 0,
            .loop_stop_threshold = get_adc_error_threshold(NULL),
            .op_error_threshold  = 6u * get_adc_error_threshold(NULL),
            .loop_gain_divisor   = get_loop_gain(NULL),
            .max_iterations      = get_max_iterations(NULL)};

        return power_configs;
    }

    struct Ex10CalibrationParamsV5 const* cal_params =
        get_ex10_cal_v5()->get_params();

    uint8_t const tx_atten = choose_coarse_atten_v5(tx_power_cdbm,
                                                    frequency_khz,
                                                    temperature_adc,
                                                    temp_comp_enabled,
                                                    rf_band);
    int16_t const tx_scalar =
        get_adjusted_tx_scalar(cal_params->tx_scalar_cal.tx_scalar_cal);

    enum AuxAdcControlChannelEnableBits power_detector_adc =
        ChannelEnableBitsNone;
    uint16_t const adc_target = power_to_adc(tx_power_cdbm,
                                             frequency_khz,
                                             temperature_adc,
                                             temp_comp_enabled,
                                             rf_band,
                                             &power_detector_adc);

    struct PowerConfigs const power_configs = {
        .tx_atten            = tx_atten,
        .tx_scalar           = tx_scalar,
        .dc_offset           = cal_params->dc_offset_cal.dc_offset[tx_atten],
        .adc_target          = adc_target,
        .loop_stop_threshold = get_adc_error_threshold(cal_params),
        .op_error_threshold  = 6u * get_adc_error_threshold(cal_params),
        .loop_gain_divisor   = get_loop_gain(cal_params),
        .max_iterations      = get_max_iterations(cal_params),
        .power_detector_adc  = power_detector_adc,
    };

    return power_configs;
}

static uint8_t get_cal_version(void)
{
    return cal_version;
}

static uint8_t get_customer_cal_version(void)
{
    return customer_cal_version;
}

static int16_t cal_init(struct Ex10Protocol const* ex10_protocol)
{
    // Read the cal version and customer cal version so we can parse the cal
    // info correctly
    uint16_t const source_address = calibration_info_reg.address;

    // Read 1 byte from offset 0 for the Impinj cal version
    struct Ex10Result ex10_result =
        ex10_protocol->read_partial(source_address, 1u, &cal_version);
    if (ex10_result.error)
    {
        return -1;
    }

    // Read 1 byte from offset 1 for the customer assigned cal version
    ex10_result = ex10_protocol->read_partial(
        source_address + offsetof(struct Ex10CalibrationParamsV5,
                                  customer_calibration_version),
        1u,
        &customer_cal_version);
    if (ex10_result.error)
    {
        return -1;
    }

    if (cal_version != 0xFF && cal_version != 0x05)
    {
        ex10_eprintf("Calibration version %u is not supported\n", cal_version);
        ex10_eprintf(
            "Please upgrade your calibration or erase the current one "
            "and run without calibration.\n");
    }

    if (cal_version == 0xFF)
    {
        ex10_eprintf("CALIBRATION NOT FOUND, DEFAULT SETTINGS WILL BE USED\n");
    }

    if (cal_version == 0x05 && customer_cal_version != 0x00)
    {
        ex10_eprintf("Customer calibration version %u will be used\n",
                     customer_cal_version);
    }

    // Read configs in from device
    if (cal_version == 0x05)
    {
        get_ex10_cal_v5()->init(ex10_protocol);
    }
    // Run initialization of arrays used in this layer
    // Note:
    // If the version is not supported, default configurations will be used.
    init_drm_analog_offsets();
    return (cal_version != 0x05) ? (int16_t)-1 : cal_version;
}

static const struct Ex10Calibration ex10_calibration = {
    .init                     = cal_init,
    .deinit                   = NULL,
    .power_to_adc             = power_to_adc,
    .reverse_power_to_adc     = reverse_power_to_adc,
    .get_power_control_params = get_power_control_params,
    .get_compensated_rssi     = get_compensated_rssi,
    .get_rssi_log2            = get_rssi_log2,
    .get_compensated_lbt_rssi = get_compensated_lbt_rssi,
    .get_cal_version          = get_cal_version,
    .get_customer_cal_version = get_customer_cal_version};

struct Ex10Calibration const* get_ex10_calibration(void)
{
    return &ex10_calibration;
}
