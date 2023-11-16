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
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum AllowedBpsRates
{
    Bps_9600,
    Bps_19200,
    Bps_38400,
    Bps_57600,
    Bps_115200
};

struct Ex10UartDriver
{
    /**
     * Open the UART driver for usage.
     *
     * @param bitrate One of the allowed bit rates allowed for the UART device.
     *
     * @return int32_t Indicates success or failure.
     *                 Zero for success, non-zero for failure.
     */
    int32_t (*uart_open)(enum AllowedBpsRates bitrate);

    /// Close the UART driver.
    void (*uart_close)(void);

    /**
     * Used for any transaction requiring only a write with no
     * expectation of data coming back after the write.
     *
     * @param tx_buff Buffer containing the data to be written
     * @param length  The number of bytes to write from tx_buff.
     *
     * @return int32_t The number of bytes written over the UART interface.
     * @retval -1      Indicates not all bytes were written properly
     */
    int32_t (*uart_write)(const void* tx_buff, size_t length);

    /**
     * Used for any transaction requiring only a read from the
     * serial port.
     *
     * @param rx_buff  Buffer in which to place incoming
     *                 data after the write.
     * @param length   The number of bytes to read into rx_buff.
     *
     * @return int32_t The number of bytes read over the UART interface.
     * @retval -1      Indicates not all bytes were read properly.
     */
    int32_t (*uart_read)(void* rx_buff, size_t length);
};

struct Ex10UartDriver const* get_ex10_uart_driver(void);

#ifdef __cplusplus
}
#endif
