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

#include "board/ex10_osal.h"

int ex10_cond_timed_wait_us(ex10_cond_t*  cond,
                            ex10_mutex_t* mutex,
                            uint32_t      timeout_us)
{
    uint32_t const  ns_per_us = 1000u;
    uint32_t const  us_per_s  = 1000u * 1000u;
    struct timespec tv        = {
        .tv_sec  = (time_t)(timeout_us / us_per_s),
        .tv_nsec = (timeout_us % us_per_s) * ns_per_us,
    };

    return pthread_cond_timedwait(cond, mutex, &tv);
}

int ex10_memcpy(void*       dst_ptr,
                size_t      dst_size,
                const void* src_ptr,
                size_t      src_size)
{
    if (src_size <= dst_size)
    {
        uint8_t*       dst_byte_ptr = (uint8_t*)dst_ptr;
        uint8_t const* src_byte_ptr = (uint8_t const*)src_ptr;
        for (size_t index = 0u; index < src_size; ++index)
        {
            dst_byte_ptr[index] = src_byte_ptr[index];
        }
        return 0;
    }
    else
    {
        ex10_memzero(dst_ptr, dst_size);
        return EINVAL;
    }
}

int ex10_memset(void* dst_ptr, size_t dst_size, int value, size_t count)
{
    if (count <= dst_size)
    {
        uint8_t* dst_byte_ptr = (uint8_t*)dst_ptr;
        for (size_t index = 0u; index < count; ++index)
        {
            dst_byte_ptr[index] = (uint8_t)value;
        }
        return 0;
    }
    else
    {
        ex10_memzero(dst_ptr, dst_size);
        return EINVAL;
    }
}

void ex10_memzero(void* dst_ptr, size_t dst_size)
{
    uint8_t* dst_byte_ptr = (uint8_t*)dst_ptr;
    for (size_t index = 0u; index < dst_size; ++index)
    {
        dst_byte_ptr[index] = 0u;
    }
}
