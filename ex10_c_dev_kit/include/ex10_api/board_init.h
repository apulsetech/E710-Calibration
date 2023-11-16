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

#include "board/uart_driver.h"
#include "ex10_api/board_init_core.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_power_modes.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/gen2_commands.h"
#include "ex10_api/version_info.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the gpio_driver pins required for Impinj Reader Chip operation.
 * The following schematic pins are set as output direction, with the levels:
 *   PWR_EN     deassert, active high, pin level = 0
 *   ENABLE     deassert, active high, pin level = 0
 *   RESET_N    assert,   active low,  pin level = 0
 *
 * @param gpio_if The instance of the GpioInterface.
 */
void ex10_board_gpio_init(struct Ex10GpioInterface const* gpio_if);

/**
 * Initialize the Impinj Reader Chip and its associated Ex10 host interfaces
 * in a typical configuration for use in a reader application.
 *
 * @param spi_clock_hz The SPI interface clock speed in Hz.
 * @param region_id    The region in which the reader will operate.
 *                     @see enum Ex10RegionId
 *
 * @return struct Ex10Result
 *         Indicates whether the function call passed or failed.
 */
struct Ex10Result ex10_typical_board_setup(uint32_t          spi_clock_hz,
                                           enum Ex10RegionId region_id);

/**
 * Initialize the Impinj Reader Chip and its associated Ex10 host interfaces
 * in a minimal configuration for communication with the bootloader. This
 * initialization profile is primarily used for uploading new firmware images
 * into the Impinj Reader Chip.
 *
 * @param spi_clock_hz The SPI interface clock speed in Hz.
 *                     This value must be 1 MHz or less.
 *
 * @return struct Ex10Result
 *         Indicates whether the function call passed or failed.
 */
struct Ex10Result ex10_bootloader_board_setup(uint32_t spi_clock_hz);

/**
 * On the Impinj Reader Chip development board this function wraps the UART
 * device driver initialization. This function should only be called after
 * the successful call to ex10_typical_board_setup() when the host UART
 * is to be used to control the development board operation.
 *
 * @param bitrate One of the valid values in enum AllowedBpsRates.
 */
void ex10_typical_board_uart_setup(enum AllowedBpsRates bitrate);

/**
 * Deinitialize the Ex10Protocol object and power down the Impinj Reader Chip
 * when the Impinj Reader Chip was configured in bootloader execution status.
 */
void ex10_bootloader_board_teardown(void);

/**
 * Power down the Impinj Reader Chip. This is strictly a software and
 * driver layer release of host resources. No transactions are
 * performed to the Impinj Reader Chip over the host interface within
 * this function call.
 */
void ex10_typical_board_teardown(void);

/**
 * Deinitialize the UART driver used to control the development board.
 */
void ex10_typical_board_uart_teardown(void);

#ifdef __cplusplus
}
#endif
