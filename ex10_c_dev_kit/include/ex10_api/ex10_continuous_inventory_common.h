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

/**
 * @enum StopReason
 * The reason that a continuous inventory was stopped.
 * @note These values in the continuous_inventory_summary.reason field.
 *       If new values are added, they must be consistent with the
 *       documentation. These values must be constrained to 8 bits.
 */
enum StopReason
{
    SRNone,
    SRHost,
    SRMaxNumberOfRounds,
    SRMaxNumberOfTags,
    SRMaxDuration,
    SROpError,
    SRSdkTimeoutError,
    SRDeviceCommandError,
    SRDeviceAggregateBufferOverflow,
    SRDeviceRampCallbackError,
    SRDeviceEventFifoFull,
    SRDeviceInventoryInvalidParam,
    SRDeviceLmacOverload,
    SRDeviceInventorySummaryReasonInvalid,
    SRReasonUnknown,
};

struct StopConditions
{
    uint32_t max_number_of_rounds;
    uint32_t max_number_of_tags;
    uint32_t max_duration_us;
};

#ifdef __cplusplus
}
#endif
