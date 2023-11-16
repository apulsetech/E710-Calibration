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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "gpio_interface.h"
#include "host_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Ex10CommandTransactor
{
    /**
     * Initialize the command transactor object.
     * This resets the command transactor to the command state.
     * The host interface and gpio modules are opened and owned by the command
     * transactor.
     */
    void (*init)(struct Ex10GpioInterface const* gpio_interface,
                 struct HostInterface const*     host_interface);

    /**
     * Release any resources in use by the command transactor module.
     * This includes the host interface.
     * The host interface and gpio modules are closed and released from
     * ownership by the command transactor.
     */
    void (*deinit)(void);

    /**
     * Sends a command from the host to the Ex10.
     * Blocks until READY_N is asserted by Ex10.
     *
     * @param command_buffer     Contains the Ex10 command.
     * @param command_length     The length of the command in bytes.
     * @param ready_n_timeout_ms The number of milliseconds to wait for the
     *                           Ex10 READY_N line to assert low.
     *
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     *         Errors reported when:
     *          - The gpio and wireline interfaces are not initialized.
     *          - The Ex10 READY_N line does not assert low before the timeout
     *            expires.
     *          - The number of bytes sent does not equal the number of bytes
     *            requested over the wireline interface.
     */
    struct Ex10Result (*send_command)(const void* command_buffer,
                                      size_t      command_length,
                                      uint32_t    ready_n_timeout_ms);

    /**
     * Receives a response from the host.
     * Blocks waiting for READY_N to be asserted by the Ex10.
     *
     * @param response_buffer        The buffer which will be filled in with the
     *                               Ex10 response.
     * @param response_buffer_length The size of the response buffer in bytes.
     *                               This is also the expected Rx length in
     * bytes.
     * @param ready_n_timeout_ms     The number of milliseconds to wait for the
     *                               Ex10 READY_N line to assert low.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     *         Errors reported when:
     *         - The gpio and wireline interfaces are not initialized.
     *         - Expected Rx length exceeds (EX10_SPI_BURST_SIZE + 1u).
     *         - The Ex10 READY_N line does not assert low before the timeout
     *           expires.
     *         - Errors reported from host interface read() command
     *         - Number of bytes received does not match the expected RX size.
     */
    struct Ex10Result (*receive_response)(void*    response_buffer,
                                          size_t   response_buffer_length,
                                          uint32_t ready_n_timeout_ms);

    /** Calls send_command() and receive_response() with one function. */
    struct Ex10Result (*send_and_recv_bytes)(const void* command_buffer,
                                             size_t      command_length,
                                             void*       response_buffer,
                                             size_t      response_buffer_length,
                                             uint32_t    ready_n_timeout_ms);
};

struct Ex10CommandTransactor const* get_ex10_command_transactor(void);

#ifdef __cplusplus
}
#endif
