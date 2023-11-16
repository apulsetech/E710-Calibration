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

#include <stdio.h>
#include <string.h>

#include "board/ex10_osal.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/version_info.h"

static const char* get_device_info(void)
{
    struct DeviceInfoFields dev_info;
    struct Ex10Result       ex10_result =
        get_ex10_protocol()->get_device_info(&dev_info);

    uint16_t device_revision = 0u;
    device_revision |= dev_info.device_revision_hi;
    device_revision <<= 8u;
    device_revision |= dev_info.device_revision_lo;

    static char device_info_str[50];

    if (ex10_result.error)
    {
        snprintf(device_info_str,
                 sizeof(device_info_str),
                 "Failed to retrieve device version\n");
    }
    else
    {
        snprintf(device_info_str,
                 sizeof(device_info_str),
                 "Device:\n  eco: %d\n  rev: %d\n  id:  %d",
                 dev_info.eco_revision,
                 device_revision,
                 dev_info.device_identifier);
    }
    return device_info_str;
}

static ssize_t fill_version_string(char*          buffer,
                                   size_t         buffer_length,
                                   char const*    app_or_bl,
                                   char const*    version_buffer,
                                   uint8_t const* git_hash_buffer,
                                   size_t         git_hash_length,
                                   uint32_t       build_no)
{
    size_t  offset = 0u;
    ssize_t retval = 0;
    if (buffer_length > (size_t)offset + 1u)
    {
        retval = snprintf(&buffer[offset],
                          (size_t)(buffer_length - offset),
                          "%s:\n  version: %s\n",
                          app_or_bl,
                          version_buffer);
        if (retval < 0)
        {
            return retval;
        }
        else
        {
            offset += (size_t)retval;
        }
    }

    char const git_hash_tag[] = "  git hash: ";
    // The length of the git hash string includes the string null terminator +
    // 2 characters for each hash byte. The null gets replaced with newline.
    size_t const git_hash_str_len =
        sizeof(git_hash_tag) + (2u * git_hash_length);

    if (buffer_length > offset + git_hash_str_len)
    {
        ex10_memcpy(&buffer[offset],
                    buffer_length - offset,
                    git_hash_tag,
                    sizeof(git_hash_tag) - 1u);
        offset += sizeof(git_hash_tag) - 1u;
        for (size_t iter = 0u; iter < git_hash_length; ++iter)
        {
            retval = snprintf(&buffer[offset],
                              buffer_length - offset,
                              "%02x",
                              git_hash_buffer[iter]);
            if (retval < 0)
            {
                return retval;
            }
            else
            {
                offset += (size_t)retval;
            }
        }
        buffer[offset++] = '\n';
    }

    if (buffer_length > offset + 1u)
    {
        retval = snprintf(&buffer[offset],
                          (size_t)(buffer_length - offset),
                          "  build no: %d",
                          build_no);
        if (retval < 0)
        {
            return retval;
        }
        else
        {
            offset += (size_t)retval;
        }
    }

    return (ssize_t)offset;
}

static ssize_t get_application_info(
    char*                       buffer,
    size_t                      buffer_length,
    struct ImageValidityFields* image_validity_buf,
    struct RemainReasonFields*  remain_reason_buf)
{
    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();

    struct Ex10FirmwareVersion application_version;
    struct Ex10Result          ex10_result =
        ex10_protocol->get_application_version(&application_version);
    if (ex10_result.error)
    {
        return -1;
    }

    if (image_validity_buf)
    {
        struct ImageValidityFields const image_validity =
            ex10_protocol->get_image_validity();
        *image_validity_buf = image_validity;
    }

    if (remain_reason_buf)
    {
        ex10_result = ex10_protocol->get_remain_reason(remain_reason_buf);
        if (ex10_result.error)
        {
            return -1;
        }
    }

    return fill_version_string(buffer,
                               buffer_length,
                               "Application",
                               application_version.version_string,
                               application_version.git_hash_bytes,
                               git_hash_reg.length,
                               application_version.build_number);
}

static ssize_t get_bootloader_info(char* buffer, size_t buffer_length)
{
    ex10_memzero(buffer, buffer_length);
    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();

    struct Ex10FirmwareVersion bootloader_version;
    struct Ex10Result const    ex10_result =
        ex10_protocol->get_bootloader_version(&bootloader_version);

    if (ex10_result.error)
    {
        return -1;
    }

    return fill_version_string(buffer,
                               buffer_length,
                               "Bootloader",
                               bootloader_version.version_string,
                               bootloader_version.git_hash_bytes,
                               git_hash_reg.length,
                               bootloader_version.build_number);
}

static enum ProductSku get_sku(void)
{
    return get_ex10_protocol()->get_sku();
}

static struct Ex10Version const ex10_version = {
    .get_bootloader_info  = get_bootloader_info,
    .get_application_info = get_application_info,
    .get_sku              = get_sku,
    .get_device_info      = get_device_info,
};

struct Ex10Version const* get_ex10_version(void)
{
    return &ex10_version;
}