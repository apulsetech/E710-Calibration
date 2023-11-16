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

#include "board/time_helpers.h"
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>


static uint32_t ex10_time_now(void)
{
    struct timespec now;
    // use the MONOTONIC clock because we want relative wall clock time
    // (as opposed to date/time).  And we use the RAW version so that
    // if NTP is adjusting the time we don't get fooled by it.
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);

    // Limit the seconds to 20 bits so that there is room for the
    // multiplication result in the uint32_t value.  This effectivly
    // creates a rollover event at 2 ^ 20 seconds instead of 2 ^ 32.
    uint32_t time_ms = (now.tv_sec & 0x000FFFFF) * 1000;

    uint32_t const ns_to_ms = (1000u * 1000u);
    time_ms += ((unsigned long int)now.tv_nsec + ns_to_ms / 2u) / ns_to_ms;

    return time_ms;
}

static uint32_t ex10_time_elapsed(uint32_t start_time)
{
    uint32_t const time = ex10_time_now();

    // If we rolled over, calculate the time from the start time to
    // max uint32_t and then add the time from 0 to time.
    uint32_t const time_elapsed = (time < start_time)
                                      ? (time + (UINT32_MAX - start_time))
                                      : (time - start_time);

    return time_elapsed;
}

static void ex10_busy_wait_ms(uint32_t msec_to_wait)
{
    uint32_t const start_time = ex10_time_now();
    while (ex10_time_elapsed(start_time) < msec_to_wait)
    {
        continue;
    }
}

// This suspends the caller from execution (at least) msec_to_wait
static void ex10_wait_ms(uint32_t msec_to_wait)
{
    // usleep's 'useconds' argument must be less than 1000000
    assert(msec_to_wait < 1000);
    usleep(msec_to_wait * 1000);
}

static struct Ex10TimeHelpers ex10_time_helpers = {
    .time_now     = ex10_time_now,
    .time_elapsed = ex10_time_elapsed,
    .busy_wait_ms = ex10_busy_wait_ms,
    .wait_ms      = ex10_wait_ms,
};

struct Ex10TimeHelpers* get_ex10_time_helpers(void)
{
    return &ex10_time_helpers;
}
