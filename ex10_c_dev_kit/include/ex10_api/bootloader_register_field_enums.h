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

// enums used by the register fields

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
// IPJ_autogen | gen_c_boot_ex10_api_reg_field_enums {

enum RemainReason {
    RemainReasonNoReason         = 0x00,
    RemainReasonReadyNAsserted   = 0x01,
    RemainReasonApplicationImageInvalid = 0x02,
    RemainReasonResetCommand     = 0x03,
    RemainReasonCrash            = 0x04,
    RemainReasonWatchdog         = 0x05,
    RemainReasonLockup           = 0x06,
};
// IPJ_autogen }
// clang-format on

#ifdef __cplusplus
}
#endif
