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

#pragma once

#include <stdint.h>

#include "ex10_api/application_register_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
// IPJ_autogen | gen_c_app_ex10_result {

/**
  Generic SDK result codes. These values will be placed into the 'result_code'
  field of the struct Ex10Result value.
 */
enum Ex10SdkResultCode
{
    Ex10SdkSuccess                                 =  0,
    /**
      Invalid parameter value
     */
    Ex10SdkErrorBadParamValue                      =  1,
    /**
      Invalid parameter length
     */
    Ex10SdkErrorBadParamLength                     =  2,
    /**
      Parameter address or length misalignment
     */
    Ex10SdkErrorBadParamAlignment                  =  3,
    /**
      Pointer parameter is null
     */
    Ex10SdkErrorNullPointer                        =  4,
    /**
      Timeout occurred while waiting for an event to occur or
      action to complete, for example waiting for READY_N to deassert.
     */
    Ex10SdkErrorTimeout                            =  5,
    /**
      The running location, application or bootloader, did not match the
      requested operation.
     */
    Ex10SdkErrorRunLocation                        =  6,
    /**
      Aggregate buffer overflow
     */
    Ex10SdkErrorAggBufferOverflow                  =  7,
    /**
      Attempting to start an Op when another Op is running
     */
    Ex10SdkErrorOpRunning                          =  8,
    /**
      Invalid or unexpected state encountered
     */
    Ex10SdkErrorInvalidState                       =  9,
    /**
      The Ex10 EventFifo overflowed; Packet processing halted.
     */
    Ex10SdkEventFifoFull                           = 10,
    /**
      The Ex10 Free event FIFO buffer list is empty.
     */
    Ex10SdkNoFreeEventFifoBuffers                  = 11,
    /**
      Ex10 Free event FIFO buffers length is incompatible for required
      operation. Example: the free event FIFO buffers may be defined
      too short to accommodate the expected response from the ReadFifo command.
     */
    Ex10SdkFreeEventFifoBuffersLengthMismatch      = 12,
    /**
      The Ex10 LMAC processor was over-loaded; packet processing halted.
     */
    Ex10SdkLmacOverload                            = 13,
    /**
      A register setting in the Ex10 contained an invalid value when starting
      the StartInventoryRoundOp. When this happens the Ex10 will respond with
      an EventFifo InventoryRoundSummary packet whose summary_reason is equal
      to InventorySummaryInvalidParam.
     */
    Ex10InventoryInvalidParam                      = 14,
    /**
      The InventoryRoundSummary.reason value is not known to the SDK.
     */
    Ex10InventorySummaryReasonInvalid              = 15,
    /**
      Invalid Event FIFO packet detected
     */
    Ex10InvalidEventFifoPacket                     = 16,
    /**
      An error occurred on the GPIO interface when communicating
      with the Ex10 device.
     */
    Ex10SdkErrorGpioInterface                      = 17,
    /**
      An error occurred on the host (i.e. SPI) interface when communicating
      with the Ex10 device.
     */
    Ex10SdkErrorHostInterface                      = 18,
    /**
      The encoded Gen commands will not fit in the buffer.
     */
    Ex10ErrorGen2BufferLength                      = 19,
    /**
      Too many Gen2 Tx Commands.
     */
    Ex10ErrorGen2NumCommands                       = 20,
    /**
      Could not encode the Gen2 command.
     */
    Ex10ErrorGen2CommandEncode                     = 21,
    /**
      Could not decode the Gen2 command.
     */
    Ex10ErrorGen2CommandDecode                     = 22,
    /**
      Unexpected select, halted, or access enable command.
     */
    Ex10ErrorGen2CommandEnableMismatch             = 23,
    /**
      The Gen command at the returned index is not valid.
     */
    Ex10ErrorGen2EmptyCommand                      = 24,
    /**
      Unexpected transmission length occurred on the host (i.e. SPI)
      interface when communicating with the Ex10 device.
     */
    Ex10SdkErrorUnexpectedTxLength                 = 25,
    /**
      The result was above the measurement threshold.
     */
    Ex10AboveThreshold                             = 26,
    /**
      The result was below the measurement threshold.
     */
    Ex10BelowThreshold                             = 27,
    /**
      The call to ex10_memcpy() failed
     */
    Ex10MemcpyFailed                               = 28,
    /**
      The call to ex10_memzero() failed
     */
    Ex10MemsetFailed                               = 29,
};
/**
  When an error occurs in the Ex10 interface,
  the result code will be one of these values.
 */
enum Ex10DeviceResultCode
{
    /**
      No errors have occurred; the transaction was successful.
     */
    Ex10DeviceSuccess                              =  0,
    /**
      An error occurred when a command was sent which does not have a response.
      Example: The Ex10 Write Command has no response in the transaction.
     */
    Ex10DeviceErrorCommandsNoResponse              =  1,
    /**
      An error occurred when a command was sent which has a response.
      Example: The Ex10 Read Command has a response in the transaction.
     */
    Ex10DeviceErrorCommandsWithResponse            =  2,
    /**
      An error occurred when starting an Op.
     */
    Ex10DeviceErrorOps                             =  3,
    /**
      An error occurred when starting an Op and waiting for the Op to complete.
     */
    Ex10DeviceErrorOpsTimeout                      =  4,
};
enum Ex10Module
{
    Ex10ModuleUndefined                            =  0,
    Ex10ModuleDevice                               =  1,
    Ex10ModuleCommandTransactor                    =  2,
    Ex10ModuleCommands                             =  3,
    Ex10ModuleProtocol                             =  4,
    Ex10ModuleOps                                  =  5,
    Ex10ModuleUtils                                =  6,
    Ex10ModuleRfPower                              =  7,
    Ex10ModuleInventory                            =  8,
    Ex10ModuleReader                               =  9,
    Ex10ModuleTest                                 = 10,
    Ex10ModulePowerModes                           = 11,
    Ex10ModuleGen2Commands                         = 12,
    Ex10ModuleGen2Response                         = 13,
    Ex10ModuleModuleManager                        = 14,
    Ex10ModuleFifoBufferList                       = 15,
    Ex10ModuleBoardInit                            = 16,
    Ex10AntennaDisconnect                          = 17,
    Ex10ListenBeforeTalk                           = 18,
    Ex10ModuleUseCase                              = 19,
    Ex10ModuleAutosetModes                         = 20,
    Ex10ModuleApplication                          = 21,
    Ex10ModuleRegion                               = 22,
    Ex10ModuleEx10Gpio                             = 23,
};
/**
  When the host interface encounters and error, it will be one of these types.
 */
enum Ex10CommandsHostResultCode
{
    HostResultSuccess                              =  0,
    HostResultReceivedLengthIncorrect              =  1,
    HostResultTestTransferVerifyError              =  2,
};
// IPJ_autogen }
// clang-format on

#pragma pack(push, 1)

/**
 * @struct Ex10CommandsHostResult Format of ``device_status`` field when
 * reporting on status of an Ex10 command "with response" that resulted in an
 * error. For a "command with response", error status can be originated in 2
 * places:
 *
 * - After sending the command, an error response may be received by the Ex10
 *   SDK.
 *
 * - After sending the command, the Ex10 SDK may encounter errors when decoding
 *   the received response.
 *
 */
struct Ex10CommandsHostResult
{
    enum ResponseCode               failed_result_code : 8;
    enum CommandCode                failed_command_code : 8;
    enum Ex10CommandsHostResultCode failed_host_result_code : 8;
    uint8_t                         rfu : 8;
};

// clang-format off
/**
 * @struct Ex10ResultCode A result code specific to the component where this
 *                        status originated
 */
union Ex10ResultCode
{
    enum Ex10SdkResultCode    sdk : 8;
    enum Ex10DeviceResultCode device : 8;
    uint8_t                   raw : 8;
};

/**
 * @struct Ex10DeviceStatus holds status retrieved from Ex10 Reader Chip
 *                          on failed command or operation
 */
union Ex10DeviceStatus
{
    struct CommandResultFields    cmd_result;
    struct Ex10CommandsHostResult cmd_host_result;
    struct OpsStatusFields        ops_status;
    uint32_t                      raw : 32;
};
// clang-format on

/**
 * @struct Ex10Result Structure used by Ex10 SDK for error reporting
 *
 * Bit allocation:
 *  | Bits     |   Name        |  Description                                  |
 *  |:--------:|:-------------:|:---------------------------------------------:|
 *  | [63]     | error         | 1: Indicates Error, 0: Non-error              |
 *  | [62]     | customer      | 1: Customer defined error, 0: Impinj defined  |
 *  | [61:48]  | rfu           | Reserved for future use                       |
 *  | [47:40]  | module        | The module from which the status originated   |
 *  | [39:32]  | result_code   | A result code specific to the origination,    |
 *  |          |               | and module                                    |
 *  | [31:0]   | device_status | Additional error-specific information         |
 */
struct Ex10Result
{
    bool     error : 1;
    bool     customer : 1;
    uint16_t rfu : 14;

    enum Ex10Module module : 8;

    union Ex10ResultCode result_code;

    union Ex10DeviceStatus device_status;
};

#pragma pack(pop)

struct Ex10Result make_ex10_success(void);

struct Ex10Result make_ex10_sdk_error(enum Ex10Module        module,
                                      enum Ex10SdkResultCode result_code);

struct Ex10Result make_ex10_sdk_error_with_status(
    enum Ex10Module        module,
    enum Ex10SdkResultCode sdk_result_code,
    uint32_t               status);

struct Ex10Result make_ex10_commands_no_resp_error(
    struct CommandResultFields cmd_result);

struct Ex10Result make_ex10_commands_w_resp_error(
    enum ResponseCode               failed_result_code,
    enum CommandCode                failed_command_code,
    enum Ex10CommandsHostResultCode failed_host_result_code);

struct Ex10Result make_ex10_ops_error(struct OpsStatusFields ops_status);

struct Ex10Result make_ex10_ops_timeout_error(
    struct OpsStatusFields ops_status);

struct Ex10Result make_ex10_ops_module_error(struct OpsStatusFields ops_status,
                                             enum Ex10Module        module);

struct FifoBufferNode* make_ex10_result_fifo_packet(
    struct Ex10Result ex10_result,
    uint32_t          us_counter);

char const* get_ex10_sdk_result_code_string(enum Ex10SdkResultCode value);
char const* get_ex10_device_result_code_string(enum Ex10DeviceResultCode value);
char const* get_ex10_module_string(enum Ex10Module value);
char const* get_ex10_commands_host_result_code_string(
    enum Ex10CommandsHostResultCode value);

void print_ex10_result(struct Ex10Result const result);

struct Ex10Result ex10_error_list_pull(void);

#ifdef __cplusplus
}
#endif
