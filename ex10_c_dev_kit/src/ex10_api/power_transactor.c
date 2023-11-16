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

#include "ex10_api/power_transactor.h"
#include "board/driver_list.h"
#include "board/time_helpers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/commands.h"

static struct Ex10DriverList const* _driver_list = NULL;

/**
 * The delay time required from the time VDD power is applied to the
 * Impinj Reader Chip to the time the ENABLE line may be set high.
 * See RAIN RFID Reader Chip Datasheet, Section 2.2.2 IO conditions:
 *   ENABLE must be driven high to enable the reader chip. It should only be
 *   driven high after a stable 24 MHz clock signal is present at the FREF pin.
 */
static uint32_t const VDD_TO_ENABLE_TIME_MS = 5u;

/**
 * The delay time required from the ENABLE line going high to the
 * RESET_N line being released from the low state.
 * See RAIN RFID Reader Chip Datasheet, Section 2.2.2 IO conditions:
 *   RESET_N must be allowed to be driven low by the chip entering startup.
 *   If it is driven low to reset the part, it must be released >500 us after
 *   the ENABLE pin is driven high.
 */
static uint32_t const ENABLE_TO_RESET_RELEASE_TIME_MS = 10u;

/**
 * The amount of time required for the Impinj Reader Chip to remain in the
 * unpowered state when power is removed.
 * See RAIN RFID Reader Chip Datasheet, Section 2.3.2 Power Down Sequence
 *   The chip should be left powered down for at least 50 ms before it is
 *   powered up again.
 */
static uint32_t const VDD_CORE_POWER_DOWN_TIME_MS = 50u;


/**
 * Bring up the Impinj Reader Chip using the sequence described in
 * the documentation titled
 *   Impinj Reader Chip Wireline API, Resetting into the Application
 * This will manipulate the host GPIO pins connected to the Ex10 pins
 * ENABLE, RESET_N and READY_N along with the VDD supply voltage to properly
 * reset the Ex10 into the application, if there is a valid application loaded
 * in the Ex10 device.
 *
 * This function will return when READY_N has asserted low (0) indicating that
 * the Impinj Reader Chip is ready to receive commands.
 */
static void ex10_power_up_to_application(void)
{
    _driver_list->gpio_if.assert_reset_n();
    _driver_list->gpio_if.set_board_power(true);
    get_ex10_time_helpers()->wait_ms(VDD_TO_ENABLE_TIME_MS);
    _driver_list->gpio_if.set_ex10_enable(true);
    get_ex10_time_helpers()->wait_ms(ENABLE_TO_RESET_RELEASE_TIME_MS);
    _driver_list->gpio_if.deassert_reset_n();

    _driver_list->gpio_if.busy_wait_ready_n(NOMINAL_READY_N_TIMEOUT_MS);
}

/**
 * Bring up the Impinj Reader Chip using the sequence described in
 * the documentation titled
 *   Impinj Reader Chip Wireline API, Resetting into the Bootloader
 * This will manipulate the host GPIO pins connected to the Ex10 pins
 * ENABLE, RESET_N and READY_N along with the VDD supply voltage to properly
 * reset the Ex10 into the bootloader.
 *
 * This function will return when READY_N has asserted low (0) indicating that
 * the Impinj Reader Chip is ready to receive commands.
 */
static void ex10_power_up_to_bootloader(void)
{
    _driver_list->gpio_if.assert_reset_n();
    _driver_list->gpio_if.set_board_power(true);
    get_ex10_time_helpers()->wait_ms(VDD_TO_ENABLE_TIME_MS);
    _driver_list->gpio_if.set_ex10_enable(true);
    get_ex10_time_helpers()->wait_ms(ENABLE_TO_RESET_RELEASE_TIME_MS);
    _driver_list->gpio_if.assert_ready_n();
    get_ex10_time_helpers()->wait_ms(1);
    _driver_list->gpio_if.deassert_reset_n();
    get_ex10_time_helpers()->wait_ms(ENABLE_TO_RESET_RELEASE_TIME_MS);
    _driver_list->gpio_if.release_ready_n();
    get_ex10_time_helpers()->wait_ms(1);

    _driver_list->gpio_if.busy_wait_ready_n(NOMINAL_READY_N_TIMEOUT_MS);
}

/**
 * Power down the Impinj Reader Chip. This follows a best practice sequence
 * involving the ENABLE, READY_N and RESET_N pins.
 *
 * @attention The caller (Ex10Protocol) is responsible for disabling interrupts
 *            prior to calling this function. Failure to do so will result in
 *            the IRQ_N thread hang.
 *
 * @note The IRQ_N delegate thread is not destroyed by this function call.
 */
static void ex10_power_down(void)
{
    // When setting ENABLE to 0, RESET_N has to be high to properly start the
    // power down sequence.
    _driver_list->gpio_if.set_ex10_enable(false);

    // Allow the power management state machine to complete power-down,
    // i.e. to transition from ON to Power Down to OFF.
    get_ex10_time_helpers()->wait_ms(1);

    _driver_list->gpio_if.assert_reset_n();

    _driver_list->gpio_if.set_board_power(false);

    // The wait time ensures that VDD_CORE will power down before the next
    // time ENABLE is asserted.
    get_ex10_time_helpers()->wait_ms(VDD_CORE_POWER_DOWN_TIME_MS);
}

static void init(void)
{
    _driver_list = get_ex10_board_driver_list();
}

static void deinit(void)
{
    // There is nothing to do. The drivers are closed in board_init.c,
    // board_teardown functions. Do not set _driver_list due to RFR's
    // multi-threaded multiple calls on deinit().
}

static int power_up_to_application(void)
{
    _driver_list->gpio_if.irq_enable(false);
    ex10_power_up_to_application();
    _driver_list->host_if.close();
    int error = _driver_list->host_if.open(BOOTLOADER_SPI_CLOCK_HZ);
    _driver_list->gpio_if.irq_enable(true);

    /**
     * @note There are 2 scenarios for calling this function:
     *  - Initial board power up and initialization.
     *    When the board is first powered up the interrupts are disabled.
     *  - PowerMode transition from PowerModeOff to any other state.
     *    When the board was powered down in Ex10Protocol.power_down() the
     *    the GPIO driver IRQ_N was disabled via the
     *    Ex10GpioInterface.irq_enable(false) call. See power_down() below.
     *  Calling Ex10GpioInterface.irq_enable(true) is consistent with both
     *  scenarios.
     */
    enum Status const running_location =
        get_ex10_protocol()->get_running_location();

    if ((error == 0) && (running_location == Application))
    {
        // Application execution confirmed. Set the SPI clock rate to 4 MHz.
        _driver_list->gpio_if.irq_enable(false);
        _driver_list->host_if.close();
        error = _driver_list->host_if.open(DEFAULT_SPI_CLOCK_HZ);
        _driver_list->gpio_if.irq_enable(true);
    }

    if (error == 0)
    {
        // This will return 1 if in bootloader, 2 if in application.
        return (int)running_location;
    }

    return error;  // Return the error code encountered.
}

static void power_up_to_bootloader(void)
{
    _driver_list->gpio_if.irq_enable(false);
    ex10_power_up_to_bootloader();
    _driver_list->gpio_if.irq_enable(true);
}

static void power_down(void)
{
    _driver_list->gpio_if.irq_enable(false);
    ex10_power_down();
    _driver_list->gpio_if.irq_enable(true);
}

static const struct Ex10PowerTransactor power_transactor = {
    .init                    = init,
    .deinit                  = deinit,
    .power_up_to_application = power_up_to_application,
    .power_up_to_bootloader  = power_up_to_bootloader,
    .power_down              = power_down,
};

struct Ex10PowerTransactor const* get_ex10_power_transactor(void)
{
    return &power_transactor;
}
