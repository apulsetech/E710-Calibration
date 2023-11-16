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

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Impinj Reader Chip SDK Operating System Access Layer (OSAL).
 * A centralized point of code for inserting operating system primitives.
 */
#define EX10_OS_TYPE_POSIX 1
#define EX10_OS_TYPE_BARE_METAL 2
#define EX10_OS_TYPE_SIM 3

#if !defined(EX10_OSAL_TYPE)
#error "EX10_OSAL_TYPE symbol not defined"
#endif  // EX10_OSAL_TYPE

#if (EX10_OSAL_TYPE == EX10_OS_TYPE_POSIX)
#include "board/ex10_osal_posix.h"

#elif (EX10_OSAL_TYPE == EX10_OS_TYPE_BARE_METAL)

typedef int32_t ex10_mutex_t;
#define EX10_MUTEX_INITIALIZER (0)

typedef int32_t ex10_cond_t;
#define EX10_COND_INITIALIZER (0)

#elif (EX10_OSAL_TYPE == EX10_OS_TYPE_SIM)

typedef int32_t ex10_mutex_t;
#define EX10_MUTEX_INITIALIZER (0)

typedef int32_t ex10_cond_t;
#define EX10_COND_INITIALIZER (0)

#else
#error "EX10_OSAL_TYPE symbol not recognized"
#endif  // EX10_OSAL_TYPE

#if !defined(EX10_OSAL_DECLARED)

int ex10_mutex_lock(ex10_mutex_t* mutex);
int ex10_mutex_unlock(ex10_mutex_t* mutex);
int ex10_cond_signal(ex10_cond_t* mutex);
int ex10_cond_wait(ex10_cond_t* cond, ex10_mutex_t* mutex);
int ex10_cond_timed_wait_us(ex10_cond_t*  cond,
                            ex10_mutex_t* mutex,
                            uint32_t      timeout_us);

int ex10_memcpy(void*       dst_ptr,
                size_t      dst_size,
                const void* src_ptr,
                size_t      src_size);

int  ex10_memset(void* dst_ptr, size_t dest_size, int ch, size_t count);
void ex10_memzero(void* dst_ptr, size_t dest_size);

#endif  // EX10_OSAL_INLINE

#ifdef __cplusplus
}
#endif
