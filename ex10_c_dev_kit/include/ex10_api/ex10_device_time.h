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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Ex10DeviceTime
{
    /**
     * Grabs the current time in ms from the Ex10 Device.
     *
     * @return uint32_t The number of milliseconds elapsed since the
     *                  device powered up.
     */
    uint32_t (*time_now)(void);

    /**
     * Returns the time elapsed since the start_time parameter through the
     * end_time parameter.
     *
     * @return uint32_t The number of milliseconds elapsed since start_time.
     */
    uint32_t (*window_time_elapsed)(uint32_t start_time, uint32_t end_time);

    /**
     * Returns the time elapsed since the start_time parameter up to the current
     * time.
     *
     * @return uint32_t The number of milliseconds elapsed since start_time.
     */
    uint32_t (*time_elapsed)(uint32_t start_time);

    /**
     * Wait for a specific number of milliseconds.
     *
     * @param msec_to_wait The number of milliseconds to wait.
     */
    struct Ex10Result (*wait_ms)(uint32_t msec_to_wait);
};

struct Ex10DeviceTime* get_ex10_device_time(void);

#ifdef __cplusplus
}
#endif
