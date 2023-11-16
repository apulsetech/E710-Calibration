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
 * @struct UartInterface
 * The Ex10 UART interface is a serial listener that receives control inputs
 * from a connected host device.
 */
struct UartInterface
{
    /**
     * Initialize the UART interface.
     * This includes setting the bitrate, etc.
     */
    int32_t (*open)(uint32_t bitrate);

    /**
     * Deinitialize the host interface.
     * Frees all resources associated with the hardware device.
     * This allows the device to be acquired for use by other applications.
     */
    void (*close)(void);

    /**
     * Receive a byte stream of data from the controlling device.
     *
     * @param data A pointer to the buffer which will be filled in by the
     *        byte stream received from the controller.
     * @param length The length of the data buffer in bytes.
     *
     * @return The number of bytes received.
     * @retval -1 The uart interface hardware faulted.
     */
    int32_t (*read)(void* data, size_t length);

    /**
     * Send a byte stream of data to the controlling device.
     *
     * @param data A pointer to the byte stream to send to the controlling
     *        device.
     * @param length The number of bytes in the stream to send.
     *
     * @return The number of bytes sent.
     * @retval -1 The uart interface hardware faulted.
     */
    int32_t (*write)(const void* data, size_t length);
};

#ifdef __cplusplus
}
#endif
