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

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

// Check for agreement on OSAL_TYPE.
static_assert(EX10_OSAL_TYPE == EX10_OS_TYPE_POSIX,
              "EX10_OSAL_TYPE == EX10_OS_TYPE_POSIX");

// The posix OSAL implementation is declared within this header file.
// Do not double declare the function names.
#define EX10_OSAL_DECLARED (1)

#define EX10_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define EX10_COND_INITIALIZER PTHREAD_COND_INITIALIZER

typedef pthread_mutex_t ex10_mutex_t;
typedef pthread_cond_t  ex10_cond_t;

static inline int ex10_mutex_lock(ex10_mutex_t* mutex)
{
    return pthread_mutex_lock(mutex);
}

static inline int ex10_mutex_unlock(ex10_mutex_t* mutex)
{
    return pthread_mutex_unlock(mutex);
}

static inline int ex10_cond_signal(ex10_cond_t* cond)
{
    return pthread_cond_signal(cond);
}

static inline int ex10_cond_wait(ex10_cond_t* cond, ex10_mutex_t* mutex)
{
    return pthread_cond_wait(cond, mutex);
}

int ex10_cond_timed_wait_us(ex10_cond_t*  cond,
                            ex10_mutex_t* mutex,
                            uint32_t      timeout_us);

int ex10_memcpy(void*       dst_ptr,
                size_t      dst_size,
                const void* src_ptr,
                size_t      src_size);

int  ex10_memset(void* dst_ptr, size_t dst_size, int value, size_t count);
void ex10_memzero(void* dst_ptr, size_t dst_size);
