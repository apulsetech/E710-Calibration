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

#include "ex10_api/ex10_gen2_reply_string.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_print.h"

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
char const* get_ex10_gen2_error_string(enum TagErrorCode error_code)
{
    switch (error_code)
    {
        case Other:
            return "Other";
        case NotSupported:
            return "NotSupported";
        case InsufficientPrivileges:
            return "InsufficientPrivileges";
        case MemoryOverrun:
            return "MemoryOverrun";
        case MemoryLocked:
            return "MemoryLocked";
        case CryptoSuite:
            return "CryptoSuite";
        case CommandNotEncapsulated:
            return "CommandNotEncapsulated";
        case ResponseBufferOverflow:
            return "ResponseBufferOverflow";
        case SecurityTimeout:
            return "SecurityTimeout";
        case InsufficientPower:
            return "InsufficientPower";
        case NonSpecific:
            return "NonSpecific";
        case NoError:
            return "NoError";
        default:
            return "Undefined";
    }
}
#else
char const* get_ex10_gen2_error_string(enum TagErrorCode error_code)
{
    (void)error_code;
    return NULL;
}
#endif

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
char const* get_ex10_gen2_transaction_status_string(
    enum Gen2TransactionStatus transaction_status)
{
    switch (transaction_status)
    {
        case Gen2TransactionStatusOk:
            return "Gen2TransactionStatusOk";
        case Gen2TransactionStatusBadCrc:
            return "Gen2TransactionStatusBadCrc";
        case Gen2TransactionStatusNoReply:
            return "Gen2TransactionStatusNoReply";
        case Gen2TransactionStatusInvalidReplyType:
            return "Gen2TransactionStatusInvalidReplyType";
        case Gen2TransactionStatusCoverCodeFailed:
            return "Gen2TransactionStatusCoverCodeFailed";
        case Gen2TransactionStatusMemoryParityErr:
            return "Gen2TransactionStatusMemoryParityErr";
        case Gen2TransactionStatusUnsupported:
            return "Gen2TransactionStatusUnsupported";
        case Gen2TransactionStatusUnknown:
            return "Gen2TransactionStatusUnknown";
        default:
            return "Gen2TransactionStatusUndefined";
    }
}
#else
char const* get_ex10_gen2_transaction_status_string(
    enum Gen2TransactionStatus transaction_status)
{
    (void)transaction_status;
    return NULL;
}
#endif

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
char const* get_ex10_gen2_command_string(enum Gen2Command gen2_command)
{
    switch (gen2_command)
    {
        case Gen2Select:
            return "Gen2Select";
        case Gen2Read:
            return "Gen2Read";
        case Gen2Write:
            return "Gen2Write";
        case Gen2Kill_1:
            return "Gen2Kill_1";
        case Gen2Lock:
            return "Gen2Lock";
        case Gen2Access:
            return "Gen2Access";
        case Gen2BlockWrite:
            return "Gen2BlockWrite";
        case Gen2BlockPermalock:
            return "Gen2BlockPermalock";
        case Gen2Authenticate:
            return "Gen2Authenticate";
        case Gen2MarginRead:
            return "Gen2MarginRead";
        default:
            return "Gen2TransactionStatusUndefined";
    }
}
#else
char const* get_ex10_gen2_command_string(enum Gen2Command gen2_command)
{
    (void)gen2_command;
    return NULL;
}
#endif
