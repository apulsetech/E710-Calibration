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
//
// Definitions and descriptions of the Ex10 RF modes
//

#pragma once

#include <stdbool.h>
#include <stdint.h>

// clang-format off
// IPJ_autogen | gen_c_host_api_rf_modes {

// Further details on all of the RF modes can be found in
// section 6.6 of the documentation.

// A list of all available rf modes for an Ex10 device
enum RfModes {

    // Available on devices: E510, E710, E910
    // lf_khz: 640, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 7.5
    mode_1 = 1,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_3 = 3,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 4
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_5 = 5,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 250, m: 4
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_7 = 7,

    // Available on devices: E710, E910
    // lf_khz: 640, m: 1
    // modulation: PR_ASK, pie: 2.0, tari_us: 7.5
    mode_11 = 11,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 15.0
    mode_12 = 12,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 160, m: 8
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_13 = 13,

    // Available on devices: E510, E710, E910
    // lf_khz: 640, m: 4
    // modulation: PR_ASK, pie: 2.0, tari_us: 7.5
    mode_15 = 15,

    // Available on devices: E710, E910
    // lf_khz: 640, m: 1
    // modulation: PR_ASK, pie: 2.0, tari_us: 7.5
    mode_102 = 102,

    // Available on devices: E710, E910
    // lf_khz: 640, m: 1
    // modulation: DSB, pie: 1.5, tari_us: 6.25
    mode_103 = 103,

    // Available on devices: E510, E710, E910
    // lf_khz: 640, m: 2
    // modulation: DSB, pie: 1.5, tari_us: 6.25
    mode_120 = 120,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_123 = 123,

    // Available on devices: E510, E710, E910
    // lf_khz: 640, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 7.5
    mode_124 = 124,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 15.0
    mode_125 = 125,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 4
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_141 = 141,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 250, m: 4
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_146 = 146,

    // Available on devices: E510, E710, E910
    // lf_khz: 640, m: 4
    // modulation: PR_ASK, pie: 2.0, tari_us: 7.5
    mode_147 = 147,

    // Available on devices: E510, E710, E910
    // lf_khz: 640, m: 4
    // modulation: PR_ASK, pie: 1.5, tari_us: 7.5
    mode_148 = 148,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 160, m: 8
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_185 = 185,

    // Available on devices: E710, E910
    // lf_khz: 426, m: 1
    // modulation: PR_ASK, pie: 2.0, tari_us: 15.0
    mode_202 = 202,

    // Available on devices: E710, E910
    // lf_khz: 426, m: 1
    // modulation: PR_ASK, pie: 1.5, tari_us: 12.5
    mode_203 = 203,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_222 = 222,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 15.0
    mode_223 = 223,

    // Available on devices: E510, E710, E910
    // lf_khz: 426, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 15.0
    mode_225 = 225,

    // Available on devices: E510, E710, E910
    // lf_khz: 426, m: 2
    // modulation: PR_ASK, pie: 1.5, tari_us: 12.5
    mode_226 = 226,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 4
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_241 = 241,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 250, m: 4
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_244 = 244,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 160, m: 8
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_285 = 285,

    // Available on devices: E710, E910
    // lf_khz: 640, m: 1
    // modulation: PR_ASK, pie: 2.0, tari_us: 7.5
    mode_302 = 302,

    // Available on devices: E510, E710, E910
    // lf_khz: 640, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 7.5
    mode_323 = 323,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_324 = 324,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 2
    // modulation: PR_ASK, pie: 2.0, tari_us: 15.0
    mode_325 = 325,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 320, m: 4
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_342 = 342,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 250, m: 4
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_343 = 343,

    // Available on devices: E510, E710, E910
    // lf_khz: 640, m: 4
    // modulation: PR_ASK, pie: 2.0, tari_us: 7.5
    mode_344 = 344,

    // Available on devices: E510, E710, E910
    // lf_khz: 640, m: 4
    // modulation: PR_ASK, pie: 1.5, tari_us: 7.5
    mode_345 = 345,

    // Available on devices: E310, E510, E710, E910
    // lf_khz: 160, m: 8
    // modulation: PR_ASK, pie: 2.0, tari_us: 20.0
    mode_382 = 382,
};
// IPJ_autogen }
// clang-format on

enum TxMod
{
    DSB,
    PR_ASK
};

struct RfModeInfo
{
    uint16_t rf_mode;
    int      blf_khz;
    int      miller;
    int      lpf_bw_khz;
    int      tx_mod;
    float    pie;
    float    tari_us;
};

int   get_blf_khz(uint16_t rf_mode);
int   get_miller(uint16_t rf_mode);
int   get_lpf_bw_khz(uint16_t rf_mode);
int   get_tx_mod(uint16_t);
float get_pie(uint16_t rf_mode);
float tari_us(uint16_t rf_mode);
