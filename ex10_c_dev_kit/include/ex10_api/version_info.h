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

#include <stddef.h>

#include "ex10_api/bootloader_registers.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The version string allocation size in bytes. An allocation of this size will
 * will be sufficient when the character buffer is passed into the functions:
 *   get_bootloader_info()
 *   get_application_info()
 */
#define VERSION_STRING_SIZE ((size_t)120u)

/**
 * @struct Ex10Version
 * Version information interface.
 */
struct Ex10Version
{
    /** @{
     * Get the bootloader or application information as a null terminated
     * string.
     * @note The string will contain newlines and whitespace formatting.
     *       The string does not contain a final newline.
     * @note In order to retrieve the bootloader information,
     *       the device will be reset into the bootloader.
     *       If the call to this function is made from the Application context
     *       then the device will be reset back to the Application context once
     *       the bootloader version information is read from the device.
     *
     * @param ex10_protocol The Ex10 protocol object.
     * @param buffer        A character buffer in which the version information
     *                      will be written.
     * @param buffer_length The length of the buffer in bytes.
     *
     * @return ssize_t The number of bytes filled into the buffer. This value
     *                 does not include the null terminator character.
     */
    ssize_t (*get_bootloader_info)(char* buffer, size_t buffer_length);

    ssize_t (*get_application_info)(
        char*                       buffer,
        size_t                      buffer_length,
        struct ImageValidityFields* image_validity_buf,
        struct RemainReasonFields*  remain_reason_buf);
    /** @} */

    /**
     * A helper to read the SKU from the ProductSkuFields register
     * return A uint16_t of the SKU value
     */
    enum ProductSku (*get_sku)(void);

    /**
     * Returns the device info
     */
    const char* (*get_device_info)(void);
};

struct Ex10Version const* get_ex10_version(void);

/** @} */
#ifdef __cplusplus
}
#endif
