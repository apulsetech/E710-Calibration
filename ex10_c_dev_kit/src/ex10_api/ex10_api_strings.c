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

#include "ex10_api/ex10_api_strings.h"
#include "ex10_api/ex10_print.h"

// clang-format off

// IPJ_autogen | gen_c_app_ex10_api_strings {

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
char const* ex10_get_response_string(enum ResponseCode response_code)
{
    switch(response_code)
    {
        case Success:
            return "Success";
        case CommandInvalid:
            return "CommandInvalid";
        case ArgumentInvalid:
            return "ArgumentInvalid";
        case ResponseOverflow:
            return "ResponseOverflow";
        case CommandMalformed:
            return "CommandMalformed";
        case AddressWriteFailure:
            return "AddressWriteFailure";
        case ImageInvalid:
            return "ImageInvalid";
        case LengthInvalid:
            return "LengthInvalid";
        case UploadStateInvalid:
            return "UploadStateInvalid";
        case BadCrc:
            return "BadCrc";
        case FlashInvalidPage:
            return "FlashInvalidPage";
        case FlashPageLocked:
            return "FlashPageLocked";
        case FlashEraseFailure:
            return "FlashEraseFailure";
        case FlashProgramFailure:
            return "FlashProgramFailure";
        case StoredSettingsMalformed:
            return "StoredSettingsMalformed";
        case NotEnoughSpace:
            return "NotEnoughSpace";
        default:
            return "ResponseCode_Undefined";
    }
}
#else
char const* ex10_get_response_string(enum ResponseCode response_code)
{
    (void)response_code;
    return NULL;
}
#endif

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
char const* ex10_get_command_string(enum CommandCode command_code)
{
    switch(command_code)
    {
        case CommandRead:
            return "CommandRead";
        case CommandWrite:
            return "CommandWrite";
        case CommandReadFifo:
            return "CommandReadFifo";
        case CommandStartUpload:
            return "CommandStartUpload";
        case CommandContinueUpload:
            return "CommandContinueUpload";
        case CommandCompleteUpload:
            return "CommandCompleteUpload";
        case CommandReValidateMainImage:
            return "CommandReValidateMainImage";
        case CommandReset:
            return "CommandReset";
        case CommandTestTransfer:
            return "CommandTestTransfer";
        case CommandWriteInfoPage:
            return "CommandWriteInfoPage";
        case CommandTestRead:
            return "CommandTestRead";
        case CommandInsertFifoEvent:
            return "CommandInsertFifoEvent";
        default:
            return "CommandCode_Undefined";
    }
}
#else
char const* ex10_get_command_string(enum CommandCode command_code)
{
    (void)command_code;
    return NULL;
}
#endif

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
char const* ex10_get_op_id_string(enum OpId op_id)
{
    switch(op_id)
    {
        case Idle:
            return "Idle";
        case LogTestOp:
            return "LogTestOp";
        case MeasureAdcOp:
            return "MeasureAdcOp";
        case TxRampUpOp:
            return "TxRampUpOp";
        case TxRampDownOp:
            return "TxRampDownOp";
        case SetTxCoarseGainOp:
            return "SetTxCoarseGainOp";
        case SetTxFineGainOp:
            return "SetTxFineGainOp";
        case RadioPowerControlOp:
            return "RadioPowerControlOp";
        case SetRfModeOp:
            return "SetRfModeOp";
        case SetRxGainOp:
            return "SetRxGainOp";
        case LockSynthesizerOp:
            return "LockSynthesizerOp";
        case EventFifoTestOp:
            return "EventFifoTestOp";
        case RxRunSjcOp:
            return "RxRunSjcOp";
        case SetGpioOp:
            return "SetGpioOp";
        case SetClearGpioPinsOp:
            return "SetClearGpioPinsOp";
        case StartInventoryRoundOp:
            return "StartInventoryRoundOp";
        case RunPrbsDataOp:
            return "RunPrbsDataOp";
        case SendSelectOp:
            return "SendSelectOp";
        case SetDacOp:
            return "SetDacOp";
        case SetATestMuxOp:
            return "SetATestMuxOp";
        case PowerControlLoopOp:
            return "PowerControlLoopOp";
        case MeasureRssiOp:
            return "MeasureRssiOp";
        case UsTimerStartOp:
            return "UsTimerStartOp";
        case UsTimerWaitOp:
            return "UsTimerWaitOp";
        case AggregateOp:
            return "AggregateOp";
        case ListenBeforeTalkOp:
            return "ListenBeforeTalkOp";
        case BerTestOp:
            return "BerTestOp";
        case EtsiBurstOp:
            return "EtsiBurstOp";
        case HpfOverrideTestOp:
            return "HpfOverrideTestOp";
        case SetDcOffsetOp:
            return "SetDcOffsetOp";
        default:
            return "OpId_Undefined";
    }
}
#else
char const* ex10_get_op_id_string(enum OpId op_id)
{
    (void)op_id;
    return NULL;
}
#endif

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
char const* ex10_get_ops_status_string(enum OpsStatus ops_status)
{
    switch(ops_status)
    {
        case ErrorNone:
            return "ErrorNone";
        case ErrorUnknownOp:
            return "ErrorUnknownOp";
        case ErrorUnknownError:
            return "ErrorUnknownError";
        case ErrorInvalidParameter:
            return "ErrorInvalidParameter";
        case ErrorPllNotLocked:
            return "ErrorPllNotLocked";
        case ErrorPowerControlTargetFailed:
            return "ErrorPowerControlTargetFailed";
        case ErrorInvalidTxState:
            return "ErrorInvalidTxState";
        case ErrorRadioPowerNotEnabled:
            return "ErrorRadioPowerNotEnabled";
        case ErrorAggregateBufferOverflow:
            return "ErrorAggregateBufferOverflow";
        case ErrorAggregateInnerOpError:
            return "ErrorAggregateInnerOpError";
        case ErrorSjcCdacRangeError:
            return "ErrorSjcCdacRangeError";
        case ErrorSjcResidueThresholdExceeded:
            return "ErrorSjcResidueThresholdExceeded";
        case ErrorDroopCompensationTooManyAdcChannels:
            return "ErrorDroopCompensationTooManyAdcChannels";
        case ErrorEventFailedToSend:
            return "ErrorEventFailedToSend";
        case ErrorAggregateEx10CommandError:
            return "ErrorAggregateEx10CommandError";
        case ErrorUnsupportedCommand:
            return "ErrorUnsupportedCommand";
        case ErrorBerRxHung:
            return "ErrorBerRxHung";
        default:
            return "OpsStatus_Undefined";
    }
}
#else
char const* ex10_get_ops_status_string(enum OpsStatus ops_status)
{
    (void)ops_status;
    return NULL;
}
#endif
// IPJ_autogen }
// clang-format on
