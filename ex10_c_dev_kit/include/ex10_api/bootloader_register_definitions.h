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
//
// Field definitions for all the bootloader registers in an EX10 device.
//
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "bootloader_register_field_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off

// IPJ_autogen | gen_c_boot_ex10_api_reg {
// Structs which break down the fields and sizing
// within each bootloader register
#pragma pack(push)

struct RamImageReturnValueFields {
    uint32_t data : 32;
};

struct FrefFreqBootloaderFields {
    uint32_t fref_freq_khz : 32;
};

struct RemainReasonFields {
    enum RemainReason remain_reason : 8;
};

struct ImageValidityFields {
    bool image_valid_marker : 1;
    bool image_non_valid_marker : 1;
    int8_t rfu : 6;
};

struct BootloaderVersionStringFields {
    uint8_t* data;
};

struct BootloaderBuildNumberFields {
    uint8_t* data;
};

struct BootloaderGitHashFields {
    uint8_t* data;
};

struct CrashInfoFields {
    uint8_t* data;
};


#pragma pack(pop)
// IPJ_autogen }
// clang-format on

#ifdef __cplusplus
}
#endif
