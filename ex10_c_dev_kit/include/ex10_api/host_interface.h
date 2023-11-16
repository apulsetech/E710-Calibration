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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct HostInterface
 * The Ex10 host interface is a serial device master which controls the Ex10.
 *
 * Ex10 supports the following protocol:
 * SPI
 *   CPOL = 0 (clock idles low)
 *   CPHA = 1 (data launches on rising edge and is captured on falling edge)
 *   Clock Rates:
 *     4 MHz maximum clock rate when controlling the Application.
 *     1 MHz maximum clock rate when controlling the Bootloader.
 *   Notes:
 *   - When sending data to the Ex10, using the write() method, the received
 *     data on the MISO line contains no valid information.
 *   - When receiving data from the Ex10, using the read() method, the data
 *     sent on the MOSI line is ignored by the Ex10.
 *
 */
struct HostInterface
{
    /**
     * Initialize the host interface.
     * This includes setting the clock speed, polarity, bit width, etc.
     *
     * @return int A indicator of success or failure.
     * @retval     0 The call was successful.
     * @retval    <0 The call was unsuccessful.
     *               Check errno for the failure reason.
     */
    int32_t (*open)(uint32_t interface_speed);

    /**
     * Deinitialize the host interface.
     * Frees all resources associated with the hardware device.
     * This allows the device to be acquired for use by other applications.
     */
    void (*close)(void);

    /**
     * Receive a byte stream of data from the Ex10 device.
     *
     * @param data A pointer to the buffer which will be filled in by the
     *        byte stream received from the Ex10.
     * @param length The length of the data buffer in bytes.
     *
     * @return The number of bytes received from the Ex10 device.
     * @retval -1 The host serial interface hardware faulted.
     */
    int32_t (*read)(void* data, size_t length);

    /**
     * Send a byte stream of data to the Ex10 device.
     *
     * @param data A pointer to the byte stream to send to the Ex10.
     * @param length The number of bytes in the stream to send.
     *
     * @return The number of bytes sent to the Ex10 device.
     * @retval -1 The host serial interface hardware faulted.
     */
    int32_t (*write)(const void* data, size_t length);
};

#ifdef __cplusplus
}
#endif
