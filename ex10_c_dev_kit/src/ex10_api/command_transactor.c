/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2020 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#include "ex10_api/command_transactor.h"
#include "board/board_spec.h"

#include "ex10_api/byte_span.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/print_data.h"
#include "ex10_api/trace.h"

static struct ConstByteSpan last_command = {.data = NULL, .length = 0u};

struct CommandTransactor
{
    struct Ex10GpioInterface const* gpio_interface;
    struct HostInterface const*     host_interface;
};

static struct CommandTransactor command_transactor = {.gpio_interface = NULL,
                                                      .host_interface = NULL};

static void init(struct Ex10GpioInterface const* gpio_interface,
                 struct HostInterface const*     host_interface)
{
    command_transactor.gpio_interface = gpio_interface;
    command_transactor.host_interface = host_interface;
}

static void deinit(void)
{
    command_transactor.gpio_interface = NULL;
    command_transactor.host_interface = NULL;
}

static struct Ex10Result send_command(const void* command_buffer,
                                      size_t      command_length,
                                      uint32_t    ready_n_timeout_ms)
{
    if ((command_buffer == NULL) ||
        (command_transactor.gpio_interface == NULL) ||
        (command_transactor.host_interface == NULL))
    {
        return make_ex10_sdk_error(Ex10ModuleCommandTransactor,
                                   Ex10SdkErrorNullPointer);
    }

    last_command.data   = (uint8_t const*)command_buffer;
    last_command.length = command_length;

    tracepoint(pi_ex10sdk, CMD_send, command_buffer, command_length);

    int const ret_val = command_transactor.gpio_interface->busy_wait_ready_n(
        ready_n_timeout_ms);
    if (ret_val != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommandTransactor,
                                   Ex10SdkErrorTimeout);
    }

    int32_t const bytes_sent = command_transactor.host_interface->write(
        command_buffer, command_length);
    if ((bytes_sent < 0) || ((uint32_t)bytes_sent != command_length))
    {
        return make_ex10_sdk_error(Ex10ModuleCommandTransactor,
                                   Ex10SdkErrorUnexpectedTxLength);
    }

    return make_ex10_success();
}

static struct Ex10Result receive_response(void*    response_buffer,
                                          size_t   response_buffer_length,
                                          uint32_t ready_n_timeout_ms)
{
    if ((response_buffer == NULL) ||
        (command_transactor.gpio_interface == NULL) ||
        (command_transactor.host_interface == NULL))
    {
        return make_ex10_sdk_error(Ex10ModuleCommandTransactor,
                                   Ex10SdkErrorNullPointer);
    }

    // Note: The Ex10 can put 1 more byte in the response buffer than it
    // can accept in the command buffer.
    if (response_buffer_length > EX10_SPI_BURST_SIZE + 1u)
    {
        ex10_eprintf(
            "receive_response: last command: 0x%02X: "
            "response_buffer_length: %zu > EX10_SPI_BURST_SIZE: %zu\n",
            *last_command.data,
            response_buffer_length,
            EX10_SPI_BURST_SIZE);

        return make_ex10_sdk_error(Ex10ModuleCommandTransactor,
                                   Ex10SdkErrorBadParamValue);
    }

    int const ret_val = command_transactor.gpio_interface->busy_wait_ready_n(
        ready_n_timeout_ms);
    if (ret_val != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommandTransactor,
                                   Ex10SdkErrorTimeout);
    }

    int32_t const bytes_received = command_transactor.host_interface->read(
        response_buffer, response_buffer_length);

    if (bytes_received < 0)
    {
        return make_ex10_sdk_error_with_status(Ex10ModuleCommandTransactor,
                                               Ex10SdkErrorHostInterface,
                                               (uint32_t)bytes_received);
    }

    if ((size_t)bytes_received != response_buffer_length)
    {
        enum CommandCode command_code = (enum CommandCode) * last_command.data;

        ex10_eprintf("Response length: %d, expected: %d \n",
                     bytes_received,
                     response_buffer_length);
        ex10_eputs("command: 0x%02X\n", command_code);
        ex10_print_data(
            last_command.data, last_command.length, DataPrefixIndex);

        ex10_eputs("response:\n");
        ex10_print_data(
            response_buffer, (size_t)bytes_received, DataPrefixIndex);


        return make_ex10_commands_w_resp_error(
            Success, command_code, HostResultReceivedLengthIncorrect);
    }

    tracepoint(pi_ex10sdk, CMD_recv, response_buffer, response_buffer_length);

    return make_ex10_success();
}

static struct Ex10Result send_and_recv_bytes(const void* command_buffer,
                                             size_t      command_length,
                                             void*       response_buffer,
                                             size_t      response_buffer_length,
                                             uint32_t    ready_n_timeout_ms)
{
    struct Ex10Result ex10_result =
        send_command(command_buffer, command_length, ready_n_timeout_ms);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    return receive_response(
        response_buffer, response_buffer_length, ready_n_timeout_ms);
}

static const struct Ex10CommandTransactor ex10_command_transactor = {
    .init                = init,
    .deinit              = deinit,
    .send_command        = send_command,
    .receive_response    = receive_response,
    .send_and_recv_bytes = send_and_recv_bytes,
};

struct Ex10CommandTransactor const* get_ex10_command_transactor(void)
{
    return &ex10_command_transactor;
}
