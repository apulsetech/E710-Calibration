/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2022 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#include "ex10_api/ex10_power_modes.h"

#include "board/board_spec.h"
#include "board/ex10_gpio.h"
#include "board/time_helpers.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/power_transactor.h"
#include "ex10_api/trace.h"

/**
 * The timeout, in milliseconds, to wait for the Ex10Reader layer to complete
 * packet processing after Ex10Reader.stop_transmitting() is called.
 */
static uint32_t const stop_transmitter_timeout_ms = 500u;

struct Ex10PowerModesPrivate
{
    struct Ex10Reader const*          reader;
    struct Ex10Ops const*             ops;
    struct Ex10Protocol const*        protocol;
    struct Ex10PowerTransactor const* power_transactor;
    struct Ex10RfPower const*         rf_power;
    enum PowerMode                    power_mode;
};

static struct Ex10PowerModesPrivate power_modes = {
    .reader           = NULL,
    .ops              = NULL,
    .protocol         = NULL,
    .power_transactor = NULL,
    .rf_power         = NULL,
    .power_mode       = PowerModeReady,
};

static void init(void)
{
    power_modes.reader           = get_ex10_reader();
    power_modes.ops              = get_ex10_ops();
    power_modes.protocol         = get_ex10_protocol();
    power_modes.power_transactor = get_ex10_power_transactor();
    power_modes.rf_power         = get_ex10_rf_power();
    power_modes.power_mode       = PowerModeReady;
}

static void deinit(void) {}

static struct Ex10Result stop_transmitter_and_wait(void)
{
    struct Ex10Result ex10_result = power_modes.reader->stop_transmitting();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    enum InventoryState inventory_state =
        power_modes.reader->get_continuous_inventory_state()->state;

    uint32_t const start_time_ms = get_ex10_time_helpers()->time_now();
    while ((inventory_state != InvIdle) &&
           (get_ex10_time_helpers()->time_elapsed(start_time_ms) <
            stop_transmitter_timeout_ms))
    {
        inventory_state =
            power_modes.reader->get_continuous_inventory_state()->state;
    }

    if (inventory_state != InvIdle)
    {
        return make_ex10_sdk_error(Ex10ModulePowerModes,
                                   Ex10SdkErrorInvalidState);
    }

    return make_ex10_success();
}

static struct Ex10Result set_gpio_pins(bool pa_bias_enable, bool rf_ps_enable)
{
    struct Ex10GpioHelpers const* gpio_helpers   = get_ex10_gpio_helpers();
    struct GpioPinsSetClear       gpio_set_clear = {0u, 0u, 0u, 0u};

    struct Ex10Result ex10_result =
        gpio_helpers->set_rf_power_supply_enable(&gpio_set_clear, rf_ps_enable);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result =
        gpio_helpers->set_pa_bias_enable(&gpio_set_clear, pa_bias_enable);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = power_modes.ops->set_clear_gpio_pins(&gpio_set_clear);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    return power_modes.ops->wait_op_completion();
}

static struct Ex10Result powerup_and_init_ex10(void)
{
    int const powerup_status =
        power_modes.power_transactor->power_up_to_application();
    if (powerup_status != Application)
    {
        return make_ex10_sdk_error(Ex10ModulePowerModes,
                                   Ex10SdkErrorRunLocation);
    }

    // Hook up the Ex10Protocol interrupt handler callback with the
    // GpioInterface once powered up into the application.
    power_modes.protocol->enable_interrupt_handlers(true);

    struct Ex10Result ex10_result = power_modes.rf_power->init_ex10();

    if (ex10_result.error == false)
    {
        ex10_result = power_modes.reader->init_ex10();
    }

    return ex10_result;
}

static struct Ex10Result set_power_mode_cold(bool radio_power_enable)
{
    struct Ex10Result ex10_result = stop_transmitter_and_wait();

    if (ex10_result.error == false)
    {
        bool const pa_bias_enable = false;
        bool const rf_ps_enable   = false;
        ex10_result               = set_gpio_pins(pa_bias_enable, rf_ps_enable);
    }

    if (ex10_result.error == false)
    {
        ex10_result = power_modes.ops->radio_power_control(radio_power_enable);
    }

    if (ex10_result.error == false)
    {
        ex10_result = power_modes.ops->wait_op_completion();
    }

    return ex10_result;
}

static struct Ex10Result set_power_mode_off(void)
{
    struct Ex10Result const ex10_result = stop_transmitter_and_wait();

    // Before powering down the Impinj Reader Chip, disable interrupt
    // processing, thereby ignoring the IRQ_N falling edge associated with
    // removing power.
    power_modes.protocol->enable_interrupt_handlers(false);
    // deinit reader
    power_modes.reader->deinit();

    power_modes.power_transactor->power_down();

    // Flush all packets contained within the SDK FifoBufferNode nodes.
    // Note: flush_packets must be false since the Ex10 is powered down.
    bool const print_packets      = false;
    bool const flush_packets      = false;
    bool const debug_aggregate_op = false;
    get_ex10_helpers()->discard_packets(
        print_packets, flush_packets, debug_aggregate_op);

    power_modes.power_mode =
        ex10_result.error ? power_modes.power_mode : PowerModeOff;

    return ex10_result;
}

static struct Ex10Result set_power_mode_standby(void)
{
    bool const        ex10_radio_power_enable = false;
    struct Ex10Result ex10_result =
        set_power_mode_cold(ex10_radio_power_enable);
    power_modes.power_mode =
        ex10_result.error ? power_modes.power_mode : PowerModeStandby;
    return ex10_result;
}

static struct Ex10Result set_power_mode_ready_cold(void)
{
    bool const        ex10_radio_power_enable = true;
    struct Ex10Result ex10_result =
        set_power_mode_cold(ex10_radio_power_enable);
    power_modes.power_mode =
        ex10_result.error ? power_modes.power_mode : PowerModeReadyCold;
    return ex10_result;
}

static struct Ex10Result set_power_mode_ready(void)
{
    bool const        ex10_radio_power_enable = true;
    struct Ex10Result ex10_result =
        power_modes.ops->radio_power_control(ex10_radio_power_enable);

    if (ex10_result.error == false)
    {
        ex10_result = power_modes.ops->wait_op_completion();
    }

    if (ex10_result.error == false)
    {
        bool const pa_bias_enable = true;
        bool const rf_ps_enable   = true;
        ex10_result               = set_gpio_pins(pa_bias_enable, rf_ps_enable);
    }

    uint32_t const delay_time_ms =
        get_ex10_board_spec()->get_pa_bias_power_on_delay_ms();
    get_ex10_time_helpers()->busy_wait_ms(delay_time_ms);

    power_modes.power_mode =
        ex10_result.error ? power_modes.power_mode : PowerModeReady;
    return ex10_result;
}

static struct Ex10Result set_power_mode(enum PowerMode power_mode)
{
    if (power_modes.power_mode != power_mode)
    {
        if (power_modes.power_mode == PowerModeOff)
        {
            struct Ex10Result const ex10_result = powerup_and_init_ex10();
            if (ex10_result.error)
            {
                return ex10_result;
            }
        }

        switch (power_mode)
        {
            case PowerModeInvalid:
                break;  // Invalid state, handle as error condition.
            case PowerModeOff:
                return set_power_mode_off();
            case PowerModeStandby:
                return set_power_mode_standby();
            case PowerModeReadyCold:
                return set_power_mode_ready_cold();
            case PowerModeReady:
                return set_power_mode_ready();
            default:
                break;  // Invalid state, handle as error condition.
        }

        // Invalid state encountered.
        // No op was run, but indicate that an error occurred.
        return make_ex10_sdk_error(Ex10ModulePowerModes,
                                   Ex10SdkErrorInvalidState);
    }

    // The power mode is unchanged, do nothing and report all is well.
    return make_ex10_success();
}

static enum PowerMode get_power_mode(void)
{
    return power_modes.power_mode;
}

struct Ex10PowerModes const* get_ex10_power_modes(void)
{
    static struct Ex10PowerModes power_modes_instance = {
        .init           = init,
        .deinit         = deinit,
        .set_power_mode = set_power_mode,
        .get_power_mode = get_power_mode,
    };

    return &power_modes_instance;
}
