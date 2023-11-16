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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "board/ex10_gpio.h"
#include "ex10_api/rf_mode_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

enum DrmStatus
{
    DrmStatusAuto = 0,  // DRM status depends on the RF mode
    DrmStatusOn   = 1,  // DRM always on
    DrmStatusOff  = 2,  // DRM always off
};

struct Ex10RxBasebandFilter
{
    /**
     * Set the DRM status to the given DRM status value.
     *
     * @param drm_enable DRM staus to set.
     */
    void (*set_drm_status)(enum DrmStatus drm_enable);

    /**
     * Get the current Dense Reader Mode (DRM) status.
     *
     * @return enum DrmStatus Current DRM status
     */
    enum DrmStatus (*get_drm_status)(void);

    /**
     * Determine whether the given RF mode is used as a Dense Reader Mode (DRM)
     * or not.
     *
     * @param rf_mode The requested RF mode.
     *
     * @return bool true if the RF mode is a Dense Reader Mode, false if not.
     */
    bool (*rf_mode_is_drm)(enum RfModes rf_mode);

    /**
     * Choose the RX baseband filter type to apply with the given RF mode.
     *
     * @param rf_mode The requested RF mode.
     *
     * @return enum BasebandFilterType BasebandFilterBandpass if Dense Reader
     * Mode(DRM) is applied. Return BasebandFilterHighpass otherwise.
     */
    enum BasebandFilterType (*choose_rx_baseband_filter)(enum RfModes rf_mode);
};

struct Ex10RxBasebandFilter const* get_ex10_rx_baseband_filter(void);

#ifdef __cplusplus
}
#endif
