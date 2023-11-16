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

#include <assert.h>
#include <string.h>
#include <sys/types.h>

#include "board/uart_helpers.h"

static struct UartInterface const* _uart_if = NULL;

static void init(struct Ex10DriverList const* driver_list)
{
    _uart_if = &driver_list->uart_if;
}

static void deinit(void)
{
    _uart_if = NULL;
}

static void send(const void* command_buffer)
{
    assert(_uart_if != NULL);

    size_t command_length = strlen(command_buffer);
    assert(command_length <= max_byte_length);

    ssize_t const bytes_sent = _uart_if->write(command_buffer, command_length);
    assert(bytes_sent > 0);
    assert((size_t)bytes_sent == command_length);
    (void)bytes_sent;
}

static size_t receive(void* rx_buffer, size_t rx_buffer_length)
{
    assert(_uart_if != NULL);
    assert(rx_buffer_length <= max_byte_length);

    ssize_t const bytes_received = _uart_if->read(rx_buffer, rx_buffer_length);
    assert(bytes_received > 0);

    return (size_t)bytes_received;
}

static const struct Ex10UartHelper uart_helper = {
    .init    = init,
    .deinit  = deinit,
    .send    = send,
    .receive = receive,
};

struct Ex10UartHelper const* get_ex10_uart_helper(void)
{
    return &uart_helper;
}
