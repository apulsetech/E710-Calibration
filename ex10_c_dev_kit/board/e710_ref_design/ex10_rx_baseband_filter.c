/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2023 Impinj, Inc. All rights reserved.                      *
 *                                                                           *
 *****************************************************************************/

#include "board/ex10_rx_baseband_filter.h"

static enum DrmStatus drm_status = DrmStatusAuto;

static void set_drm_status(enum DrmStatus drm_enable)
{
    drm_status = drm_enable;
}

static enum DrmStatus get_drm_status(void)
{
    return drm_status;
}

static bool rf_mode_is_drm(enum RfModes rf_mode)
{
    if (drm_status == DrmStatusAuto)
    {
        // Define baseband filter to use depending on rf_mode
        // 241 and 244 are duplicates of modes 5 and 7
        const uint16_t drm_modes[] = {
            5, 7, 141, 146, 186, 241, 244, 286, 342, 343, 383, 4141, 4146};

        for (size_t idx = 0; idx < sizeof(drm_modes) / sizeof(drm_modes[0]);
             idx++)
        {
            if (rf_mode == drm_modes[idx])
            {
                // DRM on
                return true;
            }
        }
        // DRM off
        return false;
    }
    else if (drm_status == DrmStatusOn)
    {
        return true;
    }
    else if (drm_status == DrmStatusOff)
    {
        return false;
    }

    // If the drm_status is neither ON/OFF/AUTO, DRM is off
    return false;
}

static enum BasebandFilterType choose_rx_baseband_filter(enum RfModes rf_mode)
{
    return rf_mode_is_drm(rf_mode) ? BasebandFilterBandpass
                                   : BasebandFilterHighpass;
}

static struct Ex10RxBasebandFilter const ex10_baseband_filter = {
    .set_drm_status            = set_drm_status,
    .get_drm_status            = get_drm_status,
    .rf_mode_is_drm            = rf_mode_is_drm,
    .choose_rx_baseband_filter = choose_rx_baseband_filter,
};

struct Ex10RxBasebandFilter const* get_ex10_rx_baseband_filter(void)
{
    return &ex10_baseband_filter;
}
