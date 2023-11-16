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

#include <stdint.h>

#include "board/ex10_osal.h"
#include "ex10_api/aggregate_op_builder.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/ex10_device_time.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_protocol.h"


static uint32_t ex10_time_now(void)
{
    struct TimestampFields time_us;
    get_ex10_protocol()->read(&timestamp_reg, &time_us);

    return (time_us.current_timestamp_us / 1000);
}

static uint32_t ex10_window_time_elapsed(uint32_t start_time, uint32_t end_time)
{
    // If we rolled over, calculate the time from the start time to
    // max uint32_t and then add the time from 0 to end_time.
    uint32_t const time_elapsed = (end_time < start_time)
                                      ? (end_time + (UINT32_MAX - start_time))
                                      : (end_time - start_time);

    return time_elapsed;
}

static uint32_t ex10_time_elapsed(uint32_t start_time)
{
    uint32_t const time_now = ex10_time_now();
    return ex10_window_time_elapsed(start_time, time_now);
}

// This suspends the caller from execution (at least) msec_to_wait
static struct Ex10Result ex10_wait_ms(uint32_t msec_to_wait)
{
    uint32_t us_to_wait = msec_to_wait * 1000;
    // check for overflow
    if (us_to_wait < msec_to_wait)
    {
        return make_ex10_sdk_error(Ex10ModuleUndefined,
                                   Ex10SdkErrorBadParamValue);
    }

    struct OpsStatusFields ops_status;
    struct Ex10Result      ex10_result =
        get_ex10_protocol()->read_ops_status_reg(&ops_status);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // If there is no op running, the start and wait timer ops are used, else
    // read the timestamp register.
    if (ops_status.busy)
    {
        uint32_t const current_time = ex10_time_now();
        while (ex10_time_elapsed(current_time) < msec_to_wait)
        {
            continue;
        }
    }
    else
    {
        struct Ex10Ops const* ops = get_ex10_ops();
        ops->start_timer_op(us_to_wait);
        ex10_result = ops->wait_op_completion();
        if (ex10_result.error)
        {
            return ex10_result;
        }
        ops->wait_timer_op();
        ex10_result = ops->wait_op_completion();
        if (ex10_result.error)
        {
            return ex10_result;
        }
    }
    return make_ex10_success();
}

static struct Ex10DeviceTime ex10_device_time = {
    .time_now            = ex10_time_now,
    .window_time_elapsed = ex10_window_time_elapsed,
    .time_elapsed        = ex10_time_elapsed,
    .wait_ms             = ex10_wait_ms,
};

struct Ex10DeviceTime* get_ex10_device_time(void)
{
    return &ex10_device_time;
}
