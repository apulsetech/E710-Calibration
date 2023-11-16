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
//
// Definitions and descriptions of the Ex10 RF modes
//
#include <stddef.h>

#include "ex10_api/ex10_macros.h"
#include "ex10_api/rf_mode_definitions.h"

// clang-format off
// IPJ_autogen | generate_host_api_rf_modes_info | use_private_modes:False, use_private_details:False {
static const struct RfModeInfo rf_mode_infos[] = {
    {.rf_mode = mode_1,
     .blf_khz     = 640,
     .miller      = 2,
     .lpf_bw_khz  = 1200,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 7.5},
    {.rf_mode = mode_3,
     .blf_khz     = 320,
     .miller      = 2,
     .lpf_bw_khz  = 600,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_5,
     .blf_khz     = 320,
     .miller      = 4,
     .lpf_bw_khz  = 400,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_7,
     .blf_khz     = 250,
     .miller      = 4,
     .lpf_bw_khz  = 390,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_11,
     .blf_khz     = 640,
     .miller      = 1,
     .lpf_bw_khz  = 1600,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 7.5},
    {.rf_mode = mode_12,
     .blf_khz     = 320,
     .miller      = 2,
     .lpf_bw_khz  = 600,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 15.0},
    {.rf_mode = mode_13,
     .blf_khz     = 160,
     .miller      = 8,
     .lpf_bw_khz  = 300,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_15,
     .blf_khz     = 640,
     .miller      = 4,
     .lpf_bw_khz  = 800,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 7.5},
    {.rf_mode = mode_102,
     .blf_khz     = 640,
     .miller      = 1,
     .lpf_bw_khz  = 1600,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 7.5},
    {.rf_mode = mode_103,
     .blf_khz     = 640,
     .miller      = 1,
     .lpf_bw_khz  = 1600,
     .tx_mod      = DSB,
     .pie         = 1.5,
     .tari_us     = 6.25},
    {.rf_mode = mode_120,
     .blf_khz     = 640,
     .miller      = 2,
     .lpf_bw_khz  = 1200,
     .tx_mod      = DSB,
     .pie         = 1.5,
     .tari_us     = 6.25},
    {.rf_mode = mode_123,
     .blf_khz     = 320,
     .miller      = 2,
     .lpf_bw_khz  = 600,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_124,
     .blf_khz     = 640,
     .miller      = 2,
     .lpf_bw_khz  = 1200,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 7.5},
    {.rf_mode = mode_125,
     .blf_khz     = 320,
     .miller      = 2,
     .lpf_bw_khz  = 600,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 15.0},
    {.rf_mode = mode_141,
     .blf_khz     = 320,
     .miller      = 4,
     .lpf_bw_khz  = 400,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_146,
     .blf_khz     = 250,
     .miller      = 4,
     .lpf_bw_khz  = 390,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_147,
     .blf_khz     = 640,
     .miller      = 4,
     .lpf_bw_khz  = 800,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 7.5},
    {.rf_mode = mode_148,
     .blf_khz     = 640,
     .miller      = 4,
     .lpf_bw_khz  = 800,
     .tx_mod      = PR_ASK,
     .pie         = 1.5,
     .tari_us     = 7.5},
    {.rf_mode = mode_185,
     .blf_khz     = 160,
     .miller      = 8,
     .lpf_bw_khz  = 300,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_202,
     .blf_khz     = 426,
     .miller      = 1,
     .lpf_bw_khz  = 1065,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 15.0},
    {.rf_mode = mode_203,
     .blf_khz     = 426,
     .miller      = 1,
     .lpf_bw_khz  = 1065,
     .tx_mod      = PR_ASK,
     .pie         = 1.5,
     .tari_us     = 12.5},
    {.rf_mode = mode_222,
     .blf_khz     = 320,
     .miller      = 2,
     .lpf_bw_khz  = 600,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_223,
     .blf_khz     = 320,
     .miller      = 2,
     .lpf_bw_khz  = 600,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 15.0},
    {.rf_mode = mode_225,
     .blf_khz     = 426,
     .miller      = 2,
     .lpf_bw_khz  = 798,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 15.0},
    {.rf_mode = mode_226,
     .blf_khz     = 426,
     .miller      = 2,
     .lpf_bw_khz  = 798,
     .tx_mod      = PR_ASK,
     .pie         = 1.5,
     .tari_us     = 12.5},
    {.rf_mode = mode_241,
     .blf_khz     = 320,
     .miller      = 4,
     .lpf_bw_khz  = 400,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_244,
     .blf_khz     = 250,
     .miller      = 4,
     .lpf_bw_khz  = 390,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_285,
     .blf_khz     = 160,
     .miller      = 8,
     .lpf_bw_khz  = 300,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_302,
     .blf_khz     = 640,
     .miller      = 1,
     .lpf_bw_khz  = 1600,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 7.5},
    {.rf_mode = mode_323,
     .blf_khz     = 640,
     .miller      = 2,
     .lpf_bw_khz  = 1200,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 7.5},
    {.rf_mode = mode_324,
     .blf_khz     = 320,
     .miller      = 2,
     .lpf_bw_khz  = 600,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_325,
     .blf_khz     = 320,
     .miller      = 2,
     .lpf_bw_khz  = 600,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 15.0},
    {.rf_mode = mode_342,
     .blf_khz     = 320,
     .miller      = 4,
     .lpf_bw_khz  = 400,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_343,
     .blf_khz     = 250,
     .miller      = 4,
     .lpf_bw_khz  = 390,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
    {.rf_mode = mode_344,
     .blf_khz     = 640,
     .miller      = 4,
     .lpf_bw_khz  = 800,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 7.5},
    {.rf_mode = mode_345,
     .blf_khz     = 640,
     .miller      = 4,
     .lpf_bw_khz  = 800,
     .tx_mod      = PR_ASK,
     .pie         = 1.5,
     .tari_us     = 7.5},
    {.rf_mode = mode_382,
     .blf_khz     = 160,
     .miller      = 8,
     .lpf_bw_khz  = 300,
     .tx_mod      = PR_ASK,
     .pie         = 2.0,
     .tari_us     = 20.0},
};
// IPJ_autogen }
// clang-format on

static const struct RfModeInfo* find_rf_mode(uint16_t rf_mode)
{
    const size_t num_elements = ARRAY_SIZE(rf_mode_infos);
    for (size_t index = 0; index < num_elements; index++)
    {
        if (rf_mode_infos[index].rf_mode == rf_mode)
        {
            return &rf_mode_infos[index];
        }
    }
    return NULL;
}

int get_blf_khz(uint16_t rf_mode)
{
    const struct RfModeInfo* rf_mode_info = find_rf_mode(rf_mode);

    if (rf_mode_info == NULL)
    {
        return -1;
    }
    return rf_mode_info->blf_khz;
}

int get_miller(uint16_t rf_mode)
{
    const struct RfModeInfo* rf_mode_info = find_rf_mode(rf_mode);

    if (rf_mode_info == NULL)
    {
        return -1;
    }
    return rf_mode_info->miller;
}

int get_lpf_bw_khz(uint16_t rf_mode)
{
    const struct RfModeInfo* rf_mode_info = find_rf_mode(rf_mode);

    if (rf_mode_info == NULL)
    {
        return -1;
    }
    return rf_mode_info->lpf_bw_khz;
}

int get_tx_mod(uint16_t rf_mode)
{
    const struct RfModeInfo* rf_mode_info = find_rf_mode(rf_mode);

    if (rf_mode_info == NULL)
    {
        return -1;
    }
    return rf_mode_info->tx_mod;
}

float get_pie(uint16_t rf_mode)
{
    const struct RfModeInfo* rf_mode_info = find_rf_mode(rf_mode);

    if (rf_mode_info == NULL)
    {
        return -1.0;
    }
    return rf_mode_info->pie;
}

float tari_us(uint16_t rf_mode)
{
    const struct RfModeInfo* rf_mode_info = find_rf_mode(rf_mode);

    if (rf_mode_info == NULL)
    {
        return -1.0;
    }
    return rf_mode_info->tari_us;
}
