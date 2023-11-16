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

#include "utils/ex10_use_case_example_errors.h"

struct Ex10Result make_ex10_app_error(
    enum Ex10ApplicationResultCode app_result_code)
{
    struct Ex10Result const ex10_result = {
        .error         = true,
        .customer      = true,
        .rfu           = 0,
        .module        = Ex10ModuleApplication,
        .result_code   = {.raw = (uint8_t)app_result_code},
        .device_status = {.raw = 0}};

    return ex10_result;
}

char const* ex10_app_result_code_string(
    enum Ex10ApplicationResultCode app_result_code)
{
    switch (app_result_code)
    {
        case Ex10ApplicationSuccess:
            return "Ex10ApplicationSuccess";
        case Ex10ApplicationErrorBadParamValue:
            return "Ex10ApplicationErrorBadParamValue";
        case Ex10ApplicationTagCount:
            return "Ex10ApplicationTagCount";
        case Ex10StopReasonUnexpected:
            return "Ex10StopReasonUnexpected";
        case Ex10ApplicationReadRate:
            return "Ex10ApplicationReadRate";

        case Ex10ApplicationInvalidPacketType:
            return "Ex10ApplicationInvalidPacketType";
        case Ex10ApplicationUnexpectedPacketType:
            return "Ex10ApplicationUnexpectedPacketType";

        case Ex10ApplicationTagLost:
            return "Ex10ApplicationTagLost";
        case Ex10ApplicationUnwelcomeTag:
            return "Ex10ApplicationUnwelcomeTag";
        case Ex10ApplicationGen2ReplyError:
            return "Ex10ApplicationGen2ReplyError";
        case Ex10ApplicationGen2ReplyExpected:
            return "Ex10ApplicationGen2ReplyExpected";
        case Ex10ApplicationMissingHaltedPacket:
            return "Ex10ApplicationMissingHaltedPacket";
        case Ex10ApplicationGen2TxCommandManagerError:
            return "Ex10ApplicationGen2TxCommandManagerError";

        case Ex10ApplicationCommandLineUnknownSpecifier:
            return "Ex10ApplicationCommandLineUnknownSpecifier";
        case Ex10ApplicationCommandLineBadParamValue:
            return "Ex10ApplicationCommandLineBadParamValue";
        case Ex10ApplicationCommandLineMissingParamValue:
            return "Ex10ApplicationCommandLineMissingParamValue";

        case Ex10ApplicationUnknown:
        default:
            return "Ex10Application_Unknown";
    }
}

void print_ex10_app_result(struct Ex10Result const result)
{
    if ((result.error == true) && (result.customer == true))
    {
        enum Ex10ApplicationResultCode const app_result_code =
            (enum Ex10ApplicationResultCode)result.result_code.raw;

        ex10_ex_eprintf("Ex10Result: error: %u, customer: %u, module: (%u) %s",
                        result.error,
                        result.customer,
                        result.module,
                        get_ex10_module_string(result.module));

        char const* result_code_string =
            ex10_app_result_code_string(app_result_code);
        ex10_ex_eputs(
            ", result_code: (%d) %s\n", app_result_code, result_code_string);
    }
    else
    {
        print_ex10_result(result);
    }
}
