/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2021 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#include <stdint.h>
#include <string.h>

#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_result.h"

// clang-format off
// IPJ_autogen | gen_c_app_ex10_result_strings {

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
char const* get_ex10_sdk_result_code_string(enum Ex10SdkResultCode value)
{
    switch(value)
    {
        case Ex10SdkSuccess:
            return "Ex10SdkSuccess";
        case Ex10SdkErrorBadParamValue:
            return "Ex10SdkErrorBadParamValue";
        case Ex10SdkErrorBadParamLength:
            return "Ex10SdkErrorBadParamLength";
        case Ex10SdkErrorBadParamAlignment:
            return "Ex10SdkErrorBadParamAlignment";
        case Ex10SdkErrorNullPointer:
            return "Ex10SdkErrorNullPointer";
        case Ex10SdkErrorTimeout:
            return "Ex10SdkErrorTimeout";
        case Ex10SdkErrorRunLocation:
            return "Ex10SdkErrorRunLocation";
        case Ex10SdkErrorAggBufferOverflow:
            return "Ex10SdkErrorAggBufferOverflow";
        case Ex10SdkErrorOpRunning:
            return "Ex10SdkErrorOpRunning";
        case Ex10SdkErrorInvalidState:
            return "Ex10SdkErrorInvalidState";
        case Ex10SdkEventFifoFull:
            return "Ex10SdkEventFifoFull";
        case Ex10SdkNoFreeEventFifoBuffers:
            return "Ex10SdkNoFreeEventFifoBuffers";
        case Ex10SdkFreeEventFifoBuffersLengthMismatch:
            return "Ex10SdkFreeEventFifoBuffersLengthMismatch";
        case Ex10SdkLmacOverload:
            return "Ex10SdkLmacOverload";
        case Ex10InventoryInvalidParam:
            return "Ex10InventoryInvalidParam";
        case Ex10InventorySummaryReasonInvalid:
            return "Ex10InventorySummaryReasonInvalid";
        case Ex10InvalidEventFifoPacket:
            return "Ex10InvalidEventFifoPacket";
        case Ex10SdkErrorGpioInterface:
            return "Ex10SdkErrorGpioInterface";
        case Ex10SdkErrorHostInterface:
            return "Ex10SdkErrorHostInterface";
        case Ex10ErrorGen2BufferLength:
            return "Ex10ErrorGen2BufferLength";
        case Ex10ErrorGen2NumCommands:
            return "Ex10ErrorGen2NumCommands";
        case Ex10ErrorGen2CommandEncode:
            return "Ex10ErrorGen2CommandEncode";
        case Ex10ErrorGen2CommandDecode:
            return "Ex10ErrorGen2CommandDecode";
        case Ex10ErrorGen2CommandEnableMismatch:
            return "Ex10ErrorGen2CommandEnableMismatch";
        case Ex10ErrorGen2EmptyCommand:
            return "Ex10ErrorGen2EmptyCommand";
        case Ex10SdkErrorUnexpectedTxLength:
            return "Ex10SdkErrorUnexpectedTxLength";
        case Ex10AboveThreshold:
            return "Ex10AboveThreshold";
        case Ex10BelowThreshold:
            return "Ex10BelowThreshold";
        case Ex10MemcpyFailed:
            return "Ex10MemcpyFailed";
        case Ex10MemsetFailed:
            return "Ex10MemsetFailed";
        default:
            return "Ex10SdkResultCode_Undefined";
    }
}
#else
char const* get_ex10_sdk_result_code_string(enum Ex10SdkResultCode value)
{
    (void)value;
    return NULL;
}
#endif

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
char const* get_ex10_device_result_code_string(enum Ex10DeviceResultCode value)
{
    switch(value)
    {
        case Ex10DeviceSuccess:
            return "Ex10DeviceSuccess";
        case Ex10DeviceErrorCommandsNoResponse:
            return "Ex10DeviceErrorCommandsNoResponse";
        case Ex10DeviceErrorCommandsWithResponse:
            return "Ex10DeviceErrorCommandsWithResponse";
        case Ex10DeviceErrorOps:
            return "Ex10DeviceErrorOps";
        case Ex10DeviceErrorOpsTimeout:
            return "Ex10DeviceErrorOpsTimeout";
        default:
            return "Ex10DeviceResultCode_Undefined";
    }
}
#else
char const* get_ex10_device_result_code_string(enum Ex10DeviceResultCode value)
{
    (void)value;
    return NULL;
}
#endif

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
char const* get_ex10_module_string(enum Ex10Module value)
{
    switch(value)
    {
        case Ex10ModuleUndefined:
            return "Ex10ModuleUndefined";
        case Ex10ModuleDevice:
            return "Ex10ModuleDevice";
        case Ex10ModuleCommandTransactor:
            return "Ex10ModuleCommandTransactor";
        case Ex10ModuleCommands:
            return "Ex10ModuleCommands";
        case Ex10ModuleProtocol:
            return "Ex10ModuleProtocol";
        case Ex10ModuleOps:
            return "Ex10ModuleOps";
        case Ex10ModuleUtils:
            return "Ex10ModuleUtils";
        case Ex10ModuleRfPower:
            return "Ex10ModuleRfPower";
        case Ex10ModuleInventory:
            return "Ex10ModuleInventory";
        case Ex10ModuleReader:
            return "Ex10ModuleReader";
        case Ex10ModuleTest:
            return "Ex10ModuleTest";
        case Ex10ModulePowerModes:
            return "Ex10ModulePowerModes";
        case Ex10ModuleGen2Commands:
            return "Ex10ModuleGen2Commands";
        case Ex10ModuleGen2Response:
            return "Ex10ModuleGen2Response";
        case Ex10ModuleModuleManager:
            return "Ex10ModuleModuleManager";
        case Ex10ModuleFifoBufferList:
            return "Ex10ModuleFifoBufferList";
        case Ex10ModuleBoardInit:
            return "Ex10ModuleBoardInit";
        case Ex10AntennaDisconnect:
            return "Ex10AntennaDisconnect";
        case Ex10ListenBeforeTalk:
            return "Ex10ListenBeforeTalk";
        case Ex10ModuleUseCase:
            return "Ex10ModuleUseCase";
        case Ex10ModuleAutosetModes:
            return "Ex10ModuleAutosetModes";
        case Ex10ModuleApplication:
            return "Ex10ModuleApplication";
        case Ex10ModuleRegion:
            return "Ex10ModuleRegion";
        case Ex10ModuleEx10Gpio:
            return "Ex10ModuleEx10Gpio";
        default:
            return "Ex10Module_Undefined";
    }
}
#else
char const* get_ex10_module_string(enum Ex10Module value)
{
    (void)value;
    return NULL;
}
#endif

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
char const* get_ex10_commands_host_result_code_string(enum Ex10CommandsHostResultCode value)
{
    switch(value)
    {
        case HostResultSuccess:
            return "HostResultSuccess";
        case HostResultReceivedLengthIncorrect:
            return "HostResultReceivedLengthIncorrect";
        case HostResultTestTransferVerifyError:
            return "HostResultTestTransferVerifyError";
        default:
            return "Ex10CommandsHostResultCode_Undefined";
    }
}
#else
char const* get_ex10_commands_host_result_code_string(enum Ex10CommandsHostResultCode value)
{
    (void)value;
    return NULL;
}
#endif
// IPJ_autogen }
// clang-format on
