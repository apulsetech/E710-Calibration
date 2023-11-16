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

#include <assert.h>
#include <stdint.h>

#include "board/ex10_osal.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_api_strings.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/fifo_buffer_list.h"

static_assert(sizeof(struct Ex10Result) == 8,
              "Incorrect size of struct Ex10Result, not packed properly");
static_assert(sizeof(union Ex10DeviceStatus) == 4,
              "Incorrect size of union Ex10DeviceStatus, not packed properly");
static_assert(sizeof(union Ex10ResultCode) == 1,
              "Incorrect size of union Ex10ResultCode, not packed properly");
static_assert(
    sizeof(struct Ex10CommandsHostResult) == 4,
    "Incorrect size of struct Ex10CommandsHostResult, not packed properly");

// Stored error initialized to "Success" value
static struct Ex10Result ex10_error_list = {.error       = false,
                                            .customer    = false,
                                            .rfu         = 0,
                                            .module      = Ex10ModuleUndefined,
                                            .result_code = {.raw = 0},
                                            .device_status = {.raw = 0}};

static void ex10_error_list_push(struct Ex10Result ex10_result)
{
    // If an error is already logged, it will not be overwritten
    if (ex10_error_list.error)
    {
        return;
    }

    ex10_error_list = ex10_result;
}

struct Ex10Result ex10_error_list_pull(void)
{
    struct Ex10Result ex10_result = ex10_error_list;

    ex10_error_list = make_ex10_success();

    return ex10_result;
}

struct Ex10Result make_ex10_success(void)
{
    struct Ex10Result const result = {.error         = false,
                                      .customer      = false,
                                      .rfu           = 0,
                                      .module        = Ex10ModuleUndefined,
                                      .result_code   = {.raw = 0},
                                      .device_status = {.raw = 0}};

    return result;
}

struct Ex10Result make_ex10_sdk_error(enum Ex10Module        module,
                                      enum Ex10SdkResultCode sdk_result_code)
{
    struct Ex10Result const result = {.error         = true,
                                      .customer      = false,
                                      .rfu           = 0,
                                      .module        = module,
                                      .result_code   = {.sdk = sdk_result_code},
                                      .device_status = {.raw = 0}};

    ex10_error_list_push(result);

    return result;
}

struct Ex10Result make_ex10_sdk_error_with_status(
    enum Ex10Module        module,
    enum Ex10SdkResultCode sdk_result_code,
    uint32_t               status)
{
    struct Ex10Result const result = {.error         = true,
                                      .customer      = false,
                                      .rfu           = 0,
                                      .module        = module,
                                      .result_code   = {.sdk = sdk_result_code},
                                      .device_status = {.raw = status}};

    ex10_error_list_push(result);

    return result;
}

struct Ex10Result make_ex10_commands_no_resp_error(
    struct CommandResultFields cmd_result)
{
    struct Ex10Result const result = {
        .error         = true,
        .customer      = false,
        .rfu           = 0,
        .module        = Ex10ModuleDevice,
        .result_code   = {.device = Ex10DeviceErrorCommandsNoResponse},
        .device_status = {.cmd_result = cmd_result}};

    ex10_error_list_push(result);

    return result;
}

struct Ex10Result make_ex10_commands_w_resp_error(
    enum ResponseCode               failed_result_code,
    enum CommandCode                failed_command_code,
    enum Ex10CommandsHostResultCode failed_host_result_code)
{
    struct Ex10CommandsHostResult cmd_host_result = {
        .failed_result_code      = failed_result_code,
        .failed_command_code     = failed_command_code,
        .failed_host_result_code = failed_host_result_code};

    struct Ex10Result const result = {
        .error         = true,
        .customer      = false,
        .rfu           = 0,
        .module        = Ex10ModuleDevice,
        .result_code   = {.device = Ex10DeviceErrorCommandsWithResponse},
        .device_status = {.cmd_host_result = cmd_host_result}};

    ex10_error_list_push(result);

    return result;
}

struct Ex10Result make_ex10_ops_error(struct OpsStatusFields ops_status)
{
    struct Ex10Result const result = {
        .error         = true,
        .customer      = false,
        .rfu           = 0,
        .module        = Ex10ModuleDevice,
        .result_code   = {.device = Ex10DeviceErrorOps},
        .device_status = {.ops_status = ops_status}};

    ex10_error_list_push(result);

    return result;
}

struct Ex10Result make_ex10_ops_timeout_error(struct OpsStatusFields ops_status)
{
    struct Ex10Result const result = {
        .error         = true,
        .customer      = false,
        .rfu           = 0,
        .module        = Ex10ModuleDevice,
        .result_code   = {.device = Ex10DeviceErrorOpsTimeout},
        .device_status = {.ops_status = ops_status}};

    ex10_error_list_push(result);

    return result;
}

static void eprint_ops_status(struct OpsStatusFields ops_status)
{
    ex10_eprintf("op_id: (0x%02X) %s, busy: %u, error: (%u) %s\n",
                 ops_status.op_id,
                 ex10_get_op_id_string(ops_status.op_id),
                 ops_status.busy,
                 ops_status.error,
                 ex10_get_ops_status_string(ops_status.error));
}

struct Ex10Result make_ex10_ops_module_error(struct OpsStatusFields ops_status,
                                             enum Ex10Module        module)
{
    struct Ex10Result const result = {
        .error         = true,
        .customer      = false,
        .rfu           = 0,
        .module        = module,
        .result_code   = {.device = Ex10DeviceErrorOps},
        .device_status = {.ops_status = ops_status}};

    ex10_error_list_push(result);

    return result;
}

struct FifoBufferNode* make_ex10_result_fifo_packet(
    struct Ex10Result ex10_result,
    uint32_t          us_counter)
{
    struct FifoBufferNode* result_buffer =
        get_ex10_result_buffer_list()->free_list_get();

    if (result_buffer != NULL)
    {
        struct PacketHeader packet_header =
            get_ex10_event_parser()->make_packet_header(Ex10ResultPacket);

        packet_header.us_counter = us_counter;

        struct Ex10ResultPacket ex10_result_packet = {
            .ex10_result = ex10_result,
        };

        uint8_t* raw_packet_bytes  = result_buffer->raw_buffer.data;
        size_t   raw_packet_length = result_buffer->raw_buffer.length;

        int copy_result = ex10_memcpy(raw_packet_bytes,
                                      raw_packet_length,
                                      &packet_header,
                                      sizeof(packet_header));
        raw_packet_bytes += sizeof(packet_header);
        raw_packet_length -= sizeof(packet_header);

        if (copy_result == 0)
        {
            copy_result = ex10_memcpy(raw_packet_bytes,
                                      raw_packet_length,
                                      &ex10_result_packet,
                                      sizeof(ex10_result_packet));
        }

        result_buffer->fifo_data.data = result_buffer->raw_buffer.data;
        result_buffer->fifo_data.length =
            sizeof(packet_header) + sizeof(ex10_result_packet);

        // If the copy_result failed, then release the result_buffer back
        // to the free list, and return NULL.
        if (copy_result != 0)
        {
            get_ex10_result_buffer_list()->free_list_put(result_buffer);
            result_buffer = NULL;
        }
    }

    return result_buffer;
}

// clang-format off
#define NEWLINE_INDENT "\n            "
void print_ex10_result(struct Ex10Result const result)
{
    ex10_eputs("Ex10Result: error: %u, customer: %u"
                 NEWLINE_INDENT "module: (%u) %s",
                 result.error,
                 result.customer,
                 result.module,
                 get_ex10_module_string(result.module));

    if (result.module == Ex10ModuleDevice)
    {
        ex10_eputs(
            NEWLINE_INDENT "result_code: (%u) %s",
            result.result_code.device,
            get_ex10_device_result_code_string(result.result_code.device));

        switch (result.result_code.device)
        {
        case Ex10DeviceErrorCommandsNoResponse:
            ex10_eputs(
                NEWLINE_INDENT
                "failed_result_code:  (0x%02X) %s"
                NEWLINE_INDENT
                "failed_command_code: (0x%02X) %s"
                NEWLINE_INDENT
                "commands_since_first_error: %u\n",
                result.device_status.cmd_result.failed_result_code,
                ex10_get_response_string(
                    result.device_status.cmd_result.failed_result_code),
                result.device_status.cmd_result.failed_command_code,
                ex10_get_command_string(
                    result.device_status.cmd_result.failed_command_code),
                result.device_status.cmd_result.commands_since_first_error);
            break;

        case Ex10DeviceErrorCommandsWithResponse:
            ex10_eputs(
                NEWLINE_INDENT
                "failed_result_code:   (0x%02X) %s"
                NEWLINE_INDENT
                "failed_command_code:  (0x%02X) %s"
                NEWLINE_INDENT
                "failed_host_result_code: (%u) %s\n",
                result.device_status.cmd_host_result.failed_result_code,
                ex10_get_response_string(
                    result.device_status.cmd_host_result.failed_result_code),
                result.device_status.cmd_host_result.failed_command_code,
                ex10_get_command_string(
                    result.device_status.cmd_host_result.failed_command_code),
                result.device_status.cmd_host_result.failed_host_result_code,
                get_ex10_commands_host_result_code_string(
                    result.device_status.cmd_host_result.failed_host_result_code));
            break;

        case Ex10DeviceErrorOps:
        case Ex10DeviceErrorOpsTimeout:
            ex10_eputs(NEWLINE_INDENT);
            eprint_ops_status(result.device_status.ops_status);
            break;

        default:
            ex10_eputs("\n");
            break;
        }
    }
    else if ((result.module == Ex10ModuleProtocol)          ||
             (result.module == Ex10ModuleCommandTransactor) ||
             (result.module == Ex10ModuleBoardInit))
    {
        switch (result.result_code.sdk)
        {
        case Ex10SdkErrorGpioInterface:
        case Ex10SdkErrorHostInterface:
            ex10_eputs(", errno: 0x%x\n", result.device_status.raw);
            break;

        default:
            ex10_eputs("\n");
            break;
        }
    }
    else if (result.result_code.device == Ex10DeviceErrorOps)
    {
        ex10_eputs(NEWLINE_INDENT);
        eprint_ops_status(result.device_status.ops_status);
    }
    else
    {
        ex10_eputs(NEWLINE_INDENT "result code: (%u) %s \n",
                     result.result_code.sdk,
                     get_ex10_sdk_result_code_string(result.result_code.sdk));
    }
}
// clang-format on
