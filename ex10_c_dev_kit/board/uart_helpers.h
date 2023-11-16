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

#include "board/driver_list.h"

#ifdef __cplusplus
extern "C" {
#endif

// Longest message is 2kB of raw data.  Expressed in ASCII format, with
// delimiters/whitespace at least triples that number
static const size_t max_byte_length = 7u * 1024u;

struct Ex10UartHelper
{
    void (*init)(struct Ex10DriverList const* driver_list);
    void (*deinit)(void);
    void (*send)(void const* command_buffer);
    size_t (*receive)(void* rx_buffer, size_t rx_buffer_length);
};

const struct Ex10UartHelper* get_ex10_uart_helper(void);

#ifdef __cplusplus
}
#endif
