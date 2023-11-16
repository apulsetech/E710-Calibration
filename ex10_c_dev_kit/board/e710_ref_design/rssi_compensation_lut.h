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

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * RSSI Compensation Look Up Table
 * These variables are look up tables used for RSSI compensation.
 *
 * These values are derived empirically but do not vary significantly from
 * part to part as they are digital offsets. These values, combined with
 * calibration data are used in unison to compensate for RSSI.
 */

/**
 * @enum RfModeDrmIndex
 * We have two measurement points on the external DRM filter. These
 * measurements were originally made to map the rssi compensation for
 * two modes: mode 5 (M 4, BLF 320) and mode 7 (M 4, BLF 250)
 */
enum RfModeDrmIndex
{
    DRM_250_IDX    = 0,
    DRM_320_IDX    = 1,
    DRM_M4_320_IDX = 2,
    DRM_M4_250_IDX = 3,
};

/**
 * @enum RfModeNonDrmIndex
 * We have six measurement points on the external DRM filter. These
 * measurements were originally made to map the rssi compensation for
 * six modes: 1 (M 2, BLF 640), 3 (M 2, BLF 320), 11 (M 1, BLF 640),
 * 12 (M 2, BLF 320), 13 (M 8, BLF 160), 15 (M 4, BLF 640)
 */
enum RfModeNonDrmIndex
{
    NON_DRM_160_IDX = 0,
    NON_DRM_320_IDX = 1,
    NON_DRM_640_IDX = 2,

    NON_DRM_M8_160_IDX   = 6,
    NON_DRM_M2_320_IDX_0 = 1,
    NON_DRM_M2_320_IDX_1 = 5,

    NON_DRM_M1_640_IDX = 4,
    NON_DRM_M2_640_IDX = 0,
    NON_DRM_M4_640_IDX = 7,
};

/**
 * @enum RxModeIndex
 */
enum RxModeIndex
{
    RX_MODE_5_IDX  = 0,
    RX_MODE_6_IDX  = 1,
    RX_MODE_11_IDX = 2,
    RX_MODE_12_IDX = 3,
    RX_MODE_16_IDX = 4,
    RX_MODE_17_IDX = 5,
    RX_MODE_18_IDX = 6,
    RX_MODE_21_IDX = 7,
    RX_MODE_23_IDX = 8,
    RX_MODE_36_IDX = 9,
    RX_MODE_51_IDX = 10,
    RX_MODE_44_IDX = 11,
    RX_MODE_48_IDX = 12,
};

/**
 * The number of Rx modes supported by the Impinj Reader Chip.
 * Each RF mode consists of a Tx and Rx mode.
 */
#define NUM_RX_MODES ((size_t)13)

/**
 * The number of frequency points of the rssi digital frequency response.
 * These points were derived empirically. At some point we want to
 * calculate these points through simulation.
 */
#define NUM_RSSI_MEASUREMENTS ((size_t)64)

/** The total number of RF modes supported by the Impinj Reader Chip. */
#define NUM_MODES ((size_t)52)

/**
 * The number of RF modes used for calibration.
 */
#define NUM_CALIBRATION_MODES ((size_t)8)

/**
 * @struct RssiCompensationLut
 * Calibration modes: [1,   3,   5,   7,   11,  12,  13, 15]
 *
 * The RSSI Compensation Look Up Table is used to interpolate/extrapolate
 * all RF modes based on the Rx baseband filter(i.e. Dense Reader Mode BPF or
 * non-DRM HPF) and the Backscatter Link Frequency (BLF).
 */
struct RssiCompensationLut
{
    /// The supported rf modes
    uint16_t const rf_modes[NUM_MODES];

    /// the RX modes that each rf mode corresponds to
    uint16_t const rf_mode_to_rx_mode[NUM_MODES];

    /// BLF in kHz for each Rx mode.
    uint16_t const rx_modes_blf_khz[NUM_RX_MODES];

    // The 8 calibration modes
    // E910, E710 - 1, 3, 5, 7, 11, 12, 13, 15
    // E510 - 1, 3, 5, 7, 12, 13, 15
    // E310 - 3, 5, 7, 12, 13
    int16_t const calibration_modes[NUM_CALIBRATION_MODES];

    /// The rssi digital correction at the BLF baseband frequency
    /// of each Calibration mode.
    /// This data was obtained empirically
    int16_t const calibration_modes_blf_digital_offset[NUM_CALIBRATION_MODES];

    /// The frequency points of the rssi digital frequency response.
    int16_t const digital_frequency_points[NUM_RSSI_MEASUREMENTS];

    /// Every Rx mode's rssi digital frequency response.
    int16_t const rx_mode_digital_correction[NUM_RX_MODES]
                                            [NUM_RSSI_MEASUREMENTS];

    /// LBT digital correction.
    /// This value was derived empirically.
    int16_t const lbt_digital_correction;

    /// LBT cordic frequency shift
    uint16_t const lbt_cordic_freq_shift;
};

struct RssiCompensationLut const* get_ex10_rssi_compensation(void);

#ifdef __cplusplus
}
#endif
