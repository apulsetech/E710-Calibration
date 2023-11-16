/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2020 - 2022 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/
// Data structure definition and initialization
// for all bootloader registers. This contains the basic
// register info and a pointer to the register data.

#pragma once

#include <stddef.h>

#include "bootloader_register_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
// IPJ_autogen | gen_c_boot_ex10_api_reg_instances {

static struct RegisterInfo const ram_image_return_value_reg = {
    .name = "RamImageReturnValue",
    .address = 0x0030,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define RAM_IMAGE_RETURN_VALUE_REG_LENGTH  ((size_t)4u)
#define RAM_IMAGE_RETURN_VALUE_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const fref_freq_bootloader_reg = {
    .name = "FrefFreq",
    .address = 0x0034,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define FREF_FREQ_REG_LENGTH  ((size_t)4u)
#define FREF_FREQ_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const remain_reason_reg = {
    .name = "RemainReason",
    .address = 0x0038,
    .length = 0x0001,
    .num_entries = 1,
    .access = ReadOnly,
};
#define REMAIN_REASON_REG_LENGTH  ((size_t)1u)
#define REMAIN_REASON_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const image_validity_reg = {
    .name = "ImageValidity",
    .address = 0x0039,
    .length = 0x0001,
    .num_entries = 1,
    .access = ReadOnly,
};
#define IMAGE_VALIDITY_REG_LENGTH  ((size_t)1u)
#define IMAGE_VALIDITY_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const bootloader_version_string_reg = {
    .name = "BootloaderVersionString",
    .address = 0x003A,
    .length = 0x0020,
    .num_entries = 1,
    .access = ReadOnly,
};
#define BOOTLOADER_VERSION_STRING_REG_LENGTH  ((size_t)32u)
#define BOOTLOADER_VERSION_STRING_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const bootloader_build_number_reg = {
    .name = "BootloaderBuildNumber",
    .address = 0x005A,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define BOOTLOADER_BUILD_NUMBER_REG_LENGTH  ((size_t)4u)
#define BOOTLOADER_BUILD_NUMBER_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const bootloader_git_hash_reg = {
    .name = "BootloaderGitHash",
    .address = 0x005E,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define BOOTLOADER_GIT_HASH_REG_LENGTH  ((size_t)4u)
#define BOOTLOADER_GIT_HASH_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const crash_info_reg = {
    .name = "CrashInfo",
    .address = 0x0100,
    .length = 0x0100,
    .num_entries = 1,
    .access = ReadOnly,
};
#define CRASH_INFO_REG_LENGTH  ((size_t)256u)
#define CRASH_INFO_REG_ENTRIES ((size_t)1u)
// IPJ_autogen }
// clang-format on

#ifdef __cplusplus
}
#endif
