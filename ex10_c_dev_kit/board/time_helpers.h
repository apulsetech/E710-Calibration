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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct Ex10TimeHelpers
 * Helpers for working with board specific timing. This API should be
 * used when timing is needed from the host rather than the
 * ex10 device.
 */
struct Ex10TimeHelpers
{
    /**
     * Grabs the current time to mark as the start of a timer.
     *
     * @return uint32_t The number of milliseconds elapsed since the
     *                  start of the program.
     */
    uint32_t (*time_now)(void);

    /**
     * Returns the time elapsed since the start_time parameter.
     *
     * @return uint32_t The number of milliseconds elapsed since start_time.
     */
    uint32_t (*time_elapsed)(uint32_t start_time);

    /**
     * Wait for a specific number of milliseconds.
     *
     * @param msec_to_wait The number of milliseconds to wait.
     */
    void (*busy_wait_ms)(uint32_t msec_to_wait);

    /**
     * Suspends execution for the specified number of milliseconds.
     *
     * @param msec_to_wait The number of milliseconds to wait, will assert
     *                     if > 1000ms.
     */
    void (*wait_ms)(uint32_t msec_to_wait);
};

struct Ex10TimeHelpers* get_ex10_time_helpers(void);

#ifdef __cplusplus
}
#endif
