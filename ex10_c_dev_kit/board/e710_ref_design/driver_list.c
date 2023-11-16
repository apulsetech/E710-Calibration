/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2020 - 2022 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#include "board/driver_list.h"
#include "board/gpio_driver.h"
#include "board/spi_driver.h"
#include "board/uart_driver.h"

static bool is_initialized = false;

static struct Ex10DriverList driver_list = {
    .gpio_if = {0},
    .host_if = {0},
    .uart_if = {0},
};

struct Ex10DriverList* get_ex10_board_driver_list(void)
{
    if (is_initialized == false)
    {
        struct Ex10GpioDriver const* gpio_driver = get_ex10_gpio_driver();

        driver_list.gpio_if.initialize        = gpio_driver->gpio_initialize;
        driver_list.gpio_if.cleanup           = gpio_driver->gpio_cleanup;
        driver_list.gpio_if.set_board_power   = gpio_driver->set_board_power;
        driver_list.gpio_if.get_board_power   = gpio_driver->get_board_power;
        driver_list.gpio_if.set_ex10_enable   = gpio_driver->set_ex10_enable;
        driver_list.gpio_if.get_ex10_enable   = gpio_driver->get_ex10_enable;
        driver_list.gpio_if.irq_enable        = gpio_driver->irq_enable;
        driver_list.gpio_if.assert_reset_n    = gpio_driver->assert_reset_n;
        driver_list.gpio_if.deassert_reset_n  = gpio_driver->deassert_reset_n;
        driver_list.gpio_if.assert_ready_n    = gpio_driver->assert_ready_n;
        driver_list.gpio_if.release_ready_n   = gpio_driver->release_ready_n;
        driver_list.gpio_if.busy_wait_ready_n = gpio_driver->busy_wait_ready_n;
        driver_list.gpio_if.ready_n_pin_get   = gpio_driver->ready_n_pin_get;
        driver_list.gpio_if.reset_device      = gpio_driver->reset_device;

        driver_list.gpio_if.register_irq_callback =
            gpio_driver->register_irq_callback;
        driver_list.gpio_if.deregister_irq_callback =
            gpio_driver->deregister_irq_callback;
        driver_list.gpio_if.thread_is_irq_monitor =
            gpio_driver->thread_is_irq_monitor;

        driver_list.gpio_if.irq_monitor_callback_enable =
            gpio_driver->irq_monitor_callback_enable;
        driver_list.gpio_if.irq_monitor_callback_is_enabled =
            gpio_driver->irq_monitor_callback_is_enabled;

        struct Ex10SpiDriver const* spi_driver = get_ex10_spi_driver();

        driver_list.host_if.open  = spi_driver->spi_open;
        driver_list.host_if.close = spi_driver->spi_close;
        driver_list.host_if.read  = spi_driver->spi_read;
        driver_list.host_if.write = spi_driver->spi_write;

        struct Ex10UartDriver const* uart_driver = get_ex10_uart_driver();

        driver_list.uart_if.open  = uart_driver->uart_open;
        driver_list.uart_if.close = uart_driver->uart_close;
        driver_list.uart_if.read  = uart_driver->uart_read;
        driver_list.uart_if.write = uart_driver->uart_write;

        is_initialized = true;
    }

    return &driver_list;
}
