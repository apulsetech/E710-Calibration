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

#pragma once

#include <stdint.h>
#include <sys/types.h>

#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/rf_mode_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

/** This value is used as a return value to signal error in functions which
 * normally return ADC codes. The ADC is 12 bit, so this value will never
 * coincide with actual ADC readings. */
#define CAL_FUNC_NOT_SUPPORTED ((uint16_t)0xFFFF)

struct Ex10Calibration
{
    /**
     * Initialize the Calibration object.
     *
     * @return int16_t If > 0, the calibration version.
     *                 If < 0, uncalibrated or unknown calibration.
     */
    int16_t (*init)(struct Ex10Protocol const* ex10_ops);

    /** Release any resources used by the Calibration object. */
    void (*deinit)(void);

    /**
     * Converts power to ADC code based on calibration, temp, frequency.
     *
     * @param tx_power_cdbm      Target Tx power in cdBm.
     * @param temperature_adc    The measured temperature ADC count.
     * @param frequency_khz      The RF frequency in kHz.
     * @param rf_band            The regulatory band as contained in the
     *                           calibration data.
     * @param power_detector_adc A pointer to enum for which adc channel(s) to
     *                           use for power ramp
     *
     * @return uint16_t ADC code corresponding to power. If the function is not
     * supported with the current calibration version, it will return
     * CAL_FUNC_NOT_SUPPORTED.
     *
     * @note: enum AuxAdcControlChannelEnableBits power_detector_adc updated
     *        with which adc channel should be used to ramp.
     */
    uint16_t (*power_to_adc)(
        int16_t                              tx_power_cdbm,
        uint32_t                             frequency_khz,
        uint16_t                             temperature_adc,
        bool                                 temp_comp_enabled,
        enum RfFilter                        rf_band,
        enum AuxAdcControlChannelEnableBits* power_detector_adc);

    /**
     * Converts reverse power to ADC code based on calibration, temp, frequency.
     *
     * @param reverse_power_cdBm The RF power amplitude in cdBm.
     * mBm.
     * @param temperature_adc    The measured temperature ADC count.
     * @param frequency_khz      The RF frequency in kHz.
     * @param rf_band            The regulatory band as contained in the
     *                           calibration data.
     * @param reverse_power_detector_adc A pointer to enum for which adc
     * channel(s) to use to measure reverse power.
     *
     * @return uint16_t ADC code corresponding to power. If the function is not
     * supported with the current calibration version, it will return
     * CAL_FUNC_NOT_SUPPORTED.
     *
     * @note: enum AuxAdcControlChannelEnableBits power_detector_adc updated
     *        with which reverse power channel to use to measure reflection.
     */
    uint16_t (*reverse_power_to_adc)(
        int16_t                              reverse_power_cdBm,
        uint32_t                             frequency_khz,
        uint16_t                             temperature_adc,
        bool                                 temp_comp_enabled,
        enum RfFilter                        rf_band,
        enum AuxAdcControlChannelEnableBits* reverse_power_detector_adc);

    /**
     * Create ADC params based on current Ex10 params.
     * @param tx_power_cdbm   Target Tx power in cdBm.
     * @param frequency_khz   Current Frequency in kHz
     * @param temperature_adc The reading from the temp ADC
     * @param rf_band         Which RF band we are using
     * @return The corrected  ADC power based on all params
     */
    struct PowerConfigs (*get_power_control_params)(int16_t  tx_power_cdbm,
                                                    uint32_t frequency_khz,
                                                    uint16_t temperature_adc,
                                                    bool     temp_comp_enabled,
                                                    enum RfFilter rf_band);

    /**
     * Compensates RSSI value for temperature, analog settings, and RF mode
     * based on calibration table.
     * @param rssi_raw    Raw RSSI_LOG_2 value from firmware op
     * @param temp_adc    Temperature ADC code
     * @param rf_mode     RF mode
     * @param rx_settings Settings corresponding to RxGainControl register
     * @param antenna     Antenna port used
     * @param rf_band     Which RF band we are using
     *
     * @note Pass temperature_adc = 0 to use most recent stored temperature
     *
     * @return Compensated RSSI value in cdBm
     */
    int16_t (*get_compensated_rssi)(
        uint16_t                          rssi_raw,
        enum RfModes                      rf_mode,
        const struct RxGainControlFields* rx_settings,
        uint8_t                           antenna,
        enum RfFilter                     rf_band,
        uint16_t                          temp_adc);

    /**
     * Convert RSSI value in cdBm back to RSSI log2 counts using the
     * temperature, analog settings, and RF mode.
     * @param rssi_cdbm       Compensated RSSI value in cdBm
     * @param temp_adc        Temperature ADC code
     * @param rf_mode         RF mode
     * @param rx_settings     Settings corresponding to RxGainControl register
     * @param antenna         Antenna port used
     * @param rf_band         Which RF band we are using
     *
     * @note Pass temperature_adc = 0 to use most recent stored temperature
     *
     * @return RSSI value in LOG2 counts derived from the RSSI value in cdBm
     */
    uint16_t (*get_rssi_log2)(int16_t                           rssi_cdbm,
                              enum RfModes                      rf_mode,
                              const struct RxGainControlFields* rx_settings,
                              uint8_t                           antenna,
                              enum RfFilter                     rf_band,
                              uint16_t                          temp_adc);

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
    int16_t (*get_compensated_lbt_rssi)(
        uint16_t                          rssi_raw,
        const struct RxGainControlFields* rx_settings,
        uint8_t                           antenna,
        enum RfFilter                     rf_band,
        uint16_t                          temp_adc);

    /**
     * Returns the current board calibrations version
     *
     * @return The current board calibration version
     */
    uint8_t (*get_cal_version)(void);

    /**
     * Returns the current board custmer calibrations version
     *
     * @return The current board customer calibration version
     */
    uint8_t (*get_customer_cal_version)(void);
};

struct Ex10Calibration const* get_ex10_calibration(void);

#ifdef __cplusplus
}
#endif
