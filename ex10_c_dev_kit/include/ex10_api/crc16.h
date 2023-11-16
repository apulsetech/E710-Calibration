/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2020 - 2021 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compute the crc16-ccitt value of the bytes in a given buffer
 *
 * @param buffer  The data to be to be used for the CRC16 calculation
 * @param length  Number of bytes included in crc16 calculation
 */
uint16_t ex10_compute_crc16(void const* buffer, size_t length)
    __attribute__((visibility("hidden")));

/**
 * Compute the crc16-ccitt value of the bytes in a given buffer.
 *
 * Use this function on data that is only available in chunks.
 * For the first call, use UINT16_MAX for crc_value.  For subsequent
 * calls, pass the crc_value from the previous call in with the next
 * chunk of data.
 *
 * @param buffer    The data to be to be used for the CRC16 calculation
 * @param length    Number of bytes included in crc16 calculation
 * @param crc_value The inital (UINT16_MAX) or intermediate CRC
 */
uint16_t ex10_compute_crc16_partial(void const* buffer,
                                    size_t      length,
                                    uint16_t    crc_value)
    __attribute__((visibility("hidden")));

#ifdef __cplusplus
}
#endif
