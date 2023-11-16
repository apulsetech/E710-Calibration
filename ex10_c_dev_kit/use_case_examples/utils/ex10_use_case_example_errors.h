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

#ifdef __cplusplus
extern "C" {
#endif

#include "ex10_api/gen2_tx_command_manager.h"

#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_result.h"

/**
 * @enum Ex10ApplicationResultCode
 * Use case examples enumerated errors.
 */
enum Ex10ApplicationResultCode
{
    Ex10ApplicationSuccess = 0,
    Ex10ApplicationErrorBadParamValue,
    Ex10ApplicationTagCount,
    Ex10StopReasonUnexpected,
    Ex10ApplicationReadRate,
    Ex10ApplicationBadSelect,

    Ex10ApplicationInvalidPacketType,
    Ex10ApplicationUnexpectedPacketType,

    Ex10ApplicationTagLost,
    Ex10ApplicationUnwelcomeTag,
    Ex10ApplicationGen2ReplyError,
    Ex10ApplicationGen2ReplyExpected,
    Ex10ApplicationMissingHaltedPacket,
    Ex10ApplicationGen2TxCommandManagerError,

    Ex10ApplicationCommandLineUnknownSpecifier,
    Ex10ApplicationCommandLineBadParamValue,
    Ex10ApplicationCommandLineMissingParamValue,

    Ex10ApplicationUnknown,
};

struct Ex10Result make_ex10_app_error(
    enum Ex10ApplicationResultCode app_result_code);

struct Ex10Result make_ex10_command_line_help_requested(void);

char const* ex10_app_result_code_string(
    enum Ex10ApplicationResultCode app_result_code);

void print_ex10_app_result(struct Ex10Result const ex10_result);

#ifdef __cplusplus
}
#endif
