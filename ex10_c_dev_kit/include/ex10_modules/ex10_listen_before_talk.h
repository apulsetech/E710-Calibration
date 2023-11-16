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

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ex10_api/ex10_result.h"

#ifdef __cplusplus
extern "C" {
#endif

// Value returned from get_listen_before_talk_rssi in the case of failure to
// retrieve the RSSI.
static int16_t const EX10_RSSI_INVALID = INT16_MIN;

struct Ex10ListenBeforeTalk
{
    /**
     * Initialize the LBT Module and register the ramp callbacks
     */
    struct Ex10Result (*init)(void);

    /**
     *  De-initialize the LBT Module and unregister the ramp callbacks
     */
    struct Ex10Result (*deinit)(void);

    void (*set_rssi_count)(uint8_t rssi_count_exp);
    void (*set_passes_required)(uint8_t passes_required);
    void (*set_lbt_pass_threshold_cdbm)(int32_t lbt_pass_threshold_cdbm);
    void (*set_max_rssi_measurements)(uint32_t set_max_rssi_measurements);
    void (*set_measurement_delay_us)(uint16_t measurement_delay_us);

    int16_t (*get_last_rssi_measurement)(void);
    uint32_t (*get_last_frequency_khz)(void);
    uint32_t (*get_total_num_rssi_measurements)(void);

    /**
     * Runs the LBT op and retrieves a single RSSI measurement through the
     * measured_rssi_log2_reg register. This value is then thrown through the
     * calibration layer compensation before being returned to the user.
     * @param antenna       The antenna to listen on.
     * @param frequency_khz If a frequency is specified here, this forces the
     *                      stack to always ramp up to the same frequency. If 0,
     *                      the region specific jump table will be used.
     * @param lbt_offset    The offset frequency for measuring the RSSI.
     * @param rssi_count    The integration count for measuring RSSI.
     *
     * @param override_used Determines whether or not to use the Rx gain
     * override setting in the LBT control register. This is normally false,
     * which means the default LBT-specific settings are used in RSSI
     * measurement and compensation. These default settings override the
     * RxGainControlFields in the RxGainControl register. If the override is
     * set, the user-specified settings in the RxGainControl register will be
     * used during measurement and compensation. Note that these user settings
     * must be set before this function call.
     *
     * @param lbt_rx_gains The rx gains to use during the lbt measurements.
     * @param lbt_rssi_out The compensated RSSI value in cdBm.
     * @return             The status showing any errors which occurred.
     */
    struct Ex10Result (*get_listen_before_talk_rssi)(
        uint8_t                           antenna,
        uint32_t                          frequency_khz,
        int32_t                           lbt_offset,
        uint8_t                           rssi_count,
        bool                              override_used,
        struct RxGainControlFields const* lbt_rx_gains,
        int16_t*                          lbt_rssi_out);

    /**
     * Runs the LBT op with a variable number of measurements. The user
     * configures the number of measurements up to 5 along with the delay
     * between each measurement. This delay will be the same between all
     * measurements. The user may also set the LBT offset and base frequency to
     * a different value for each measurement.
     * @param antenna       The antenna to listen on.
     * @param frequency_khz If a frequency is specified here, this forces the
     *                      stack to always ramp up to the same frequency. If 0,
     *                      the frequency used will iterate through the region
     * specific jump table.
     * @param rssi_count    The integration count for measuring RSSI.
     * @param lbt_settings  The lbt controls for the entire op. This tells the
     *                      op whether to use a delay between measurements, how
     *                      many measurements to make, whether or not to
     *                      override the RX gain settings, and if a narrower
     *                      bandwidth should be used.
     * @note: The override field determines whether or not to use the Rx gain
     * override setting in the LBT control register. This is normally false,
     * which means the default LBT-specific settings are used in RSSI
     * measurement and compensation. These default settings override the
     * RxGainControlFields in the RxGainControl register. If the override is
     * set, the user-specified settings in the RxGainControl register will be
     * used during measurement and compensation. Note that these user settings
     * must be set before this function call.
     *
     * @param frequencies_khz   The frequencies array for all LBT
     *                          measurements. Note that these can all differ.
     * @param lbt_offset        The offset frequency for measuring the RSSI
     *                          in each measurement. Note that these can all
     *                          differ.
     * @param rssi_measurements The rssi results are placed in this array.
     *                          Ensure that the array contains enough space
     *                          for the number of measurements that were
     *                          specified in lbt_settings.
     * @param lbt_rx_gains      The rx gains to use during the lbt
     * measurements.
     * @return                  The status showing any errors which occurred.
     */
    struct Ex10Result (*listen_before_talk_multi)(
        uint8_t                           antenna,
        uint8_t                           rssi_count,
        struct LbtControlFields           lbt_settings,
        uint32_t*                         frequencies_khz,
        int32_t*                          lbt_offsets,
        int16_t*                          rssi_measurements,
        struct RxGainControlFields const* lbt_rx_gains);

    /**
     * Runs the LBT op and analyzes the RSSI results with default lbt settings.
     * If the user wishes to alter the lbt_offset, rssi_count, passes_required,
     * lbt_pass_threshold_cdbm, or max_rssi_measurements, they must use the
     * corresponding setter functions.
     *
     * @param antenna         The antenna on which to make the measurements
     *
     * @return int16_t The max RSSI measured from the LBT op. If the
     *                 value returned is over the threshold specified
     *                 by 'lbt_pass_threshold_cdbm', then the op failed
     *                 to return a passing RSSI 'rssi_count' number of
     *                 times in a row before 'max_rssi_measurements'.
     */
    int16_t (*multi_listen_before_talk_rssi)(uint8_t antenna);

    struct RxGainControlFields (*get_default_lbt_rx_analog_configs)(void);

    /**
     * This function pointer is the pre ramp callback use by the LBT
     * ramping module.  It is intended to be used with a super ramping
     * module that includes the LBT behavior with another module
     * (the init() functionality should be used if using only this module)
     */
    void (*lbt_pre_ramp_callback)(struct Ex10Result* ex10_result);
};

const struct Ex10ListenBeforeTalk* get_ex10_listen_before_talk(void);

#ifdef __cplusplus
}
#endif
