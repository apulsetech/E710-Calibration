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

#include "board_spec_constants.h"
#include "ex10_api/board_init_core.h"

#include "board/driver_list.h"
#include "board/ex10_random.h"
#include "board/fifo_buffer_pool.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/power_transactor.h"

void ex10_core_board_gpio_init(struct Ex10GpioInterface const* gpio_if)
{
    bool const board_power_on = false;
    bool const ex10_enable    = false;
    bool const reset          = true;  // sets pin level zero.
    gpio_if->initialize(board_power_on, ex10_enable, reset);
}

struct Ex10Result ex10_core_board_setup(enum Ex10RegionId region_id,
                                        uint32_t          spi_clock_hz)
{
    // Seed the random number generator (in the board init layer) prior to
    // initializing the region table.
    get_ex10_random()->setup_random();

    struct Ex10DriverList const* driver_list = get_ex10_board_driver_list();
    // Initialize the modules first:
    get_ex10_power_transactor()->init();
    ex10_core_board_gpio_init(&driver_list->gpio_if);
    const int32_t result = driver_list->host_if.open(spi_clock_hz);
    if (result < 0)
    {
        return make_ex10_sdk_error_with_status(
            Ex10ModuleBoardInit, Ex10SdkErrorHostInterface, (uint32_t)result);
    }

    struct Ex10Protocol const* protocol = get_ex10_protocol();
    struct Ex10Ops const*      ops      = get_ex10_ops();
    struct Ex10RfPower const*  rf_power = get_ex10_rf_power();

    protocol->init(driver_list);
    ops->init();

    // Initialize FIFO buffer list, to be used to hold the content of Ex10
    // device Event FIFO contents
    struct FifoBufferPool const* event_fifo_buffer_pool =
        get_ex10_event_fifo_buffer_pool();

    struct FifoBufferList const* fifo_buffer_list = get_ex10_fifo_buffer_list();

    struct Ex10Result ex10_result =
        fifo_buffer_list->init(event_fifo_buffer_pool->fifo_buffer_nodes,
                               event_fifo_buffer_pool->fifo_buffers,
                               event_fifo_buffer_pool->buffer_count);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Initialize result buffer list, to be used for reporting errors that
    // occurred in interrupt context
    struct FifoBufferPool const* result_buffer_pool =
        get_ex10_result_buffer_pool();

    struct FifoBufferList const* result_buffer_list =
        get_ex10_result_buffer_list();

    ex10_result =
        result_buffer_list->init(result_buffer_pool->fifo_buffer_nodes,
                                 result_buffer_pool->fifo_buffers,
                                 result_buffer_pool->buffer_count);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Power up the Ex10 Reader Chip. This may return with Bootloader status.
    // If this happens then only proceed with Ex10Protocol init_ex10().
    const int power_up_status =
        get_ex10_power_transactor()->power_up_to_application();
    // If the power_up_status is < 0 then there was an error initializing the
    // host interface. Nothing further can be done.
    if (power_up_status < 0)
    {
        return make_ex10_sdk_error_with_status(Ex10ModuleBoardInit,
                                               Ex10SdkErrorHostInterface,
                                               (uint32_t)power_up_status);
    }

    // If we reset into the bootloader then return an error
    if (power_up_status == Bootloader)
    {
        return make_ex10_sdk_error(Ex10ModuleBoardInit,
                                   Ex10SdkErrorRunLocation);
    }

    ex10_result = protocol->init_ex10();
    if (ex10_result.error)
    {
        return ex10_result;
    }
    // Progress through the Ex10 modules' initialization of the
    // Impinj Reader Chip.
    ex10_result = rf_power->init_ex10();
    if (ex10_result.error)
    {
        return ex10_result;
    }
    // setup the active region
    get_ex10_active_region()->set_region(region_id, TCXO_FREQ_KHZ);
    // Set up sjc now that ops is fully initialized
    get_ex10_sjc()->init(protocol);

    return make_ex10_success();
}


struct Ex10Result ex10_bootloader_core_board_setup(uint32_t spi_clock_hz)
{
    struct FifoBufferList const* fifo_buffer_list = get_ex10_fifo_buffer_list();

    fifo_buffer_list->init(NULL, NULL, 0u);

    struct FifoBufferList const* result_buffer_list =
        get_ex10_result_buffer_list();

    result_buffer_list->init(NULL, NULL, 0u);

    struct Ex10DriverList const* driver_list = get_ex10_board_driver_list();
    struct Ex10Protocol const*   protocol    = get_ex10_protocol();

    get_ex10_power_transactor()->init();
    ex10_core_board_gpio_init(&driver_list->gpio_if);

    int32_t const result = driver_list->host_if.open(spi_clock_hz);
    if (result < 0)
    {
        return make_ex10_sdk_error_with_status(
            Ex10ModuleBoardInit, Ex10SdkErrorHostInterface, (uint32_t)result);
    }

    protocol->init(driver_list);
    get_ex10_power_transactor()->power_up_to_bootloader();

    enum Status const running_location =
        get_ex10_protocol()->get_running_location();

    // If we have not reset into the bootloader, return an error
    if (running_location != Bootloader)
    {
        return make_ex10_sdk_error(Ex10ModuleBoardInit,
                                   Ex10SdkErrorRunLocation);
    }

    return make_ex10_success();
}

void ex10_bootloader_core_board_teardown(void)
{
    get_ex10_protocol()->deinit();
    get_ex10_power_transactor()->power_down();
    get_ex10_power_transactor()->deinit();

    struct Ex10DriverList const* driver_list = get_ex10_board_driver_list();
    driver_list->gpio_if.cleanup();
    driver_list->host_if.close();
}

void ex10_core_board_teardown(void)
{
    get_ex10_ops()->release();
    get_ex10_protocol()->deinit();

    get_ex10_power_transactor()->power_down();
    get_ex10_power_transactor()->deinit();

    struct Ex10DriverList const* driver_list = get_ex10_board_driver_list();
    driver_list->gpio_if.cleanup();
    driver_list->host_if.close();
}
