/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2021 Impinj, Inc. All rights reserved.                      *
 *                                                                           *
 *****************************************************************************/

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct ByteSpan
 * Defines a mutable location of contiguous memory, organized as bytes.
 */
struct ByteSpan
{
    uint8_t* data;    ///< The pointer to beginning of the memory location.
    size_t   length;  ///< The number of bytes in the contiguous memory.
};

/**
 * @struct ConstByteSpan
 * Defines a immutable location of contiguous memory, organized as bytes.
 */
struct ConstByteSpan
{
    uint8_t const* data;  ///< The pointer to beginning of the memory location.
    size_t         length;  ///< The number of bytes in the contiguous memory.
};

#ifdef __cplusplus
}
#endif
