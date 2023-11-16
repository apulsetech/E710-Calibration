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

/// The type to use when specifying a channel table size.
typedef uint16_t channel_size_t;

/// The type to use when indexing into a channel table.
/// @note The channel table may be shuffled and therefore an increase in
/// channel index most likely is not one channel higher in frequency.
typedef uint16_t channel_index_t;

/// The type to use when calculation a channel index offset.
/// Channel offsets may indicate an increase (>0) or decrease (<0) in
/// the channel index; and therefore frequency.
typedef int16_t channel_offset_t;

static const channel_index_t channel_index_invalid = UINT16_MAX;

#ifdef __cplusplus
}
#endif
