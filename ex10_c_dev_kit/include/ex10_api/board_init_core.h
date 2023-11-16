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

#include "ex10_api/ex10_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Typical usage is 4MHz when running the Application.
static uint32_t const DEFAULT_SPI_CLOCK_HZ = 4000000u;

/// A reduced clock speed is required when running the Bootloader.
static uint32_t const BOOTLOADER_SPI_CLOCK_HZ = 1000000u;

/**
 * Initialize the Impinj Reader Chip core SDK.  This includes the
 * Ops and protocol layers, and any board bring up.
 *
 * @param region_id    The region to initialize the active region with
 * @param spi_clock_hz The SPI inteface clock speed in Hz
 *
 * @return struct Ex10Result
 *         Indicates whether the function call passed or failed.
 */
struct Ex10Result ex10_core_board_setup(enum Ex10RegionId region_id,
                                        uint32_t          spi_clock_hz);

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
struct Ex10Result ex10_bootloader_core_board_setup(uint32_t spi_clock_hz);

/**
 * Deinitailize the Ex10Protocol object and power down the Impinj Reader Chip
 * when the Impinj Reader Chip was configured in bootloader execution status.
 */
void ex10_bootloader_core_board_teardown(void);

/**
 * Deinitialize the Ex10 Core interface groups of objects and power
 * down the Impinj Reader Chip. This is strictly a software and driver layer
 * release of host resources. No transactions are performed to the Impinj Reader
 * Chip over the host interface within this function call.
 */
void ex10_core_board_teardown(void);


/**
 * Initialize the gpio_driver pins required for Impinj Reader Chip operation.
 * The following schematic pins are set as output direction, with the levels:
 *   PWR_EN     deassert, active high, pin level = 0
 *   ENABLE     deassert, active high, pin level = 0
 *   RESET_N    assert,   active low,  pin level = 0
 *
 * @param gpio_if The instance of the GpioInterface.
 */
void ex10_core_board_gpio_init(struct Ex10GpioInterface const* gpio_if);

#ifdef __cplusplus
}
#endif
