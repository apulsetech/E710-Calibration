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

#include <stdlib.h>

#include "ex10_api/board_init.h"
#include "ex10_api/board_init_core.h"

#include "board/driver_list.h"
#include "board/fifo_buffer_pool.h"
#include "board/uart_helpers.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/power_transactor.h"

void ex10_board_gpio_init(struct Ex10GpioInterface const* gpio_if)
{
    ex10_core_board_gpio_init(gpio_if);
}

struct Ex10Result ex10_typical_board_setup(uint32_t          spi_clock_hz,
                                           enum Ex10RegionId region_id)
{
    struct Ex10Reader const*     reader      = get_ex10_reader();
    struct Ex10PowerModes const* power_modes = get_ex10_power_modes();

    // Setup the Ex10 Core components
    struct Ex10Result ex10_result =
        ex10_core_board_setup(region_id, spi_clock_hz);
    if (ex10_result.error)
    {
        return ex10_result;
    }
    // Note that the region is setup in the core board setup
    // but this region_id is still passed to the ex10_reader
    // to maintain the existing API
    reader->init(region_id);
    power_modes->init();

    ex10_result = reader->init_ex10();
    if (ex10_result.error == true)
    {
        return ex10_result;
    }

    reader->read_calibration();

    return make_ex10_success();
}

struct Ex10Result ex10_bootloader_board_setup(uint32_t spi_clock_hz)
{
    return ex10_bootloader_core_board_setup(spi_clock_hz);
}

void ex10_typical_board_uart_setup(enum AllowedBpsRates bitrate)
{
    struct Ex10DriverList const* driver_list = get_ex10_board_driver_list();
    driver_list->uart_if.open(bitrate);
    get_ex10_uart_helper()->init(driver_list);
}

void ex10_bootloader_board_teardown(void)
{
    ex10_bootloader_core_board_teardown();
}

void ex10_typical_board_teardown(void)
{
    get_ex10_reader()->deinit();
    // tear down the ex10 core
    ex10_core_board_teardown();
}

void ex10_typical_board_uart_teardown(void)
{
    get_ex10_uart_helper()->deinit();
    get_ex10_board_driver_list()->uart_if.close();
}
