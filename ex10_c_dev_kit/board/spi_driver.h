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

struct Ex10SpiDriver
{
    int32_t (*spi_open)(uint32_t spi_speed_hz);

    void (*spi_close)(void);

    /**
     * Used for any transaction requiring only a write with no
     * expectation of data coming back after the write.
     * @param tx_buff   Buffer containing the data to be written
                        data after the write.
     * @param length The number of bytes to write from tx_buff
     *
     * If not all bytes are written properly, a -1 is returned.
     */
    int32_t (*spi_write)(const void* tx_buff, size_t length);

    /**
     * Used for any transaction requiring only a read using
     * dummy bytes out MOSI.
     * @param rx_buff   Buffer in which to place incoming
     *                  data after the write.
     * @param length The number of bytes to read into rx_buff
     *
     * If not all bytes are received properly, a -1 is returned.
     */
    int32_t (*spi_read)(void* rx_buff, size_t length);
};

struct Ex10SpiDriver const* get_ex10_spi_driver(void);

#ifdef __cplusplus
}
#endif
