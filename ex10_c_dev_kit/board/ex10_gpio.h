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

/**
 * @file board/ex10_gpio.h
 *
 * Interface which translates between 'struct Ex10GpioConfig' settings and the
 * GPIO settings that are mapped into the Ex10 GpioOutputEnables and
 * GpioOutputLevels registers.
 */
#pragma once

#include <stdint.h>

/// The region provides information related to GPIO types.
/// Specifically, the Tx RF Filter (the SAW filter) of type enum RfFilter.
#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/ex10_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @note The Ex10 uses GPIO 5 as the interrupt line (IRQ_N) to the host.
///       This signal can not be changed. Attempting to set this pin via
///       the SetGpioOp or SetClearGpioPinsOp are ignored.
static uint32_t const GPIO_PIN_IRQ_N = 5u;

/**
 * @enum BasebandFilterType
 * Mode dependent external Rx filter selection.
 */
enum BasebandFilterType
{
    BasebandFilterHighpass = 0,
    BasebandFilterBandpass = 1,
};

/**
 * @enum PowerRange
 * Controls the applied external PA voltage.
 * The High setting is used for +30 dBm operation.
 */
enum PowerRange
{
    PowerRangeLow  = 0,
    PowerRangeHigh = 1,
};

/**
 * @struct GpioPinsSetClear
 * Used to control output state of each DIGITAL_IO pin of the Impinj Read Chip.
 */
struct GpioPinsSetClear
{
    /// Set a group of DIGITAL_IO pins to have their outputs set to Logic 1.
    /// This value will only be preset on pins which have their outputs enabled.
    uint32_t output_level_set;
    /// Set a group of DIGITAL_IO pins to have their outputs set to Logic 0.
    /// This value will only be preset on pins which have their outputs enabled.
    uint32_t output_level_clear;
    /// Set a group of DIGITAL_IO pins to be enabled as output pins.
    uint32_t output_enable_set;
    /// Disable a group of DIGITAL_IO pins from being enabled as output pins.
    /// Pins which have their output disabled will have the Hi-Z state.
    uint32_t output_enable_clear;
};

/**
 * @struct Ex10GpioConfig
 * (forward declaration)
 *
 * Each board level implementation will need to provide its own specific
 * declaration of the Ex10GpioConfig depending on what functionality the
 * board provides.
 */
struct Ex10GpioConfig;

/**
 * @struct Ex10GpioHelpers
 *
 * Converts a structure of GPIO functional settings into
 * struct GpioPinsSetClear which can be used to set the SetClearGpioPinsOp
 * registers.
 *
 * Each of these function calls combines the struct GpioPinsSetClear parameter,
 * using bit-wise OR, with the gpio_pins_set_clear values passed in. In this
 * manner successive calls can be combined into a single run of
 * SetClearGpioPinsOp.
 */
struct Ex10GpioHelpers
{
    /**
     * Converts a structure of GPIO settings into a uint32_t bitmask of GPIO
     * settings.
     * @param       gpio_config  The desired GPIO configuration.
     * @param [out] gpio_levels  A bitmask which can be written to the
     *                           GpioOutputLevels register.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*get_levels)(struct Ex10GpioConfig const* gpio_config,
                                    uint32_t*                    gpio_levels);

    /**
     * Converts a bitmask of GPIO settings into a structure of GPIO settings.
     * @param       gpio_levels  A bitmask of GPIO settings which could be used
     *                           with the GpioOutputLevels register.
     * @param [out] gpio_config  A structure of GPIO settings which match
     *                           settings in the gpio_levels bitmask.
     */
    void (*get_config)(uint32_t               gpio_levels,
                       struct Ex10GpioConfig* gpio_config);

    /**
     * @return uint32_t A bit field of GPIO pins which must be enabled as
     * outputs for proper board operation.
     */
    uint32_t (*get_output_enables)(void);

    /**
     * Set the antenna port selection bit(s) within the GpioPinsSetClear
     * structure.
     *
     * @param [in/out] gpio_pins_set_clear The GPIO output level and enable
     * settings.
     *
     * @param antenna The antenna port to be selected when the
     *                SetClearGpioPinsOp is run.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_antenna_port)(
        struct GpioPinsSetClear* gpio_pins_set_clear,
        uint8_t                  antenna);

    /**
     * Set the Rx Baseband Filter bit within the GpioPinsSetClear structure.
     *
     * @param [in/out] gpio_pins_set_clear The GPIO output level and enable
     * settings.
     *
     * @param rx_baseband_filter The Rx Baseband Filter setting,
     *                           which is based on RF mode.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_rx_baseband_filter)(
        struct GpioPinsSetClear* gpio_pins_set_clear,
        enum BasebandFilterType  rx_baseband_filter);

    /**
     * Set the PA Bias Enable bit within the GpioPinsSetClear structure.
     *
     * @param [in/out] gpio_pins_set_clear The GPIO output level and enable
     * settings.
     *
     * @param pa_bias_enable true to enabel the PA Bias Enable,
     *                       false to disable the PA Bias Enable.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_pa_bias_enable)(
        struct GpioPinsSetClear* gpio_pins_set_clear,
        bool                     pa_bias_enable);

    /**
     * Set the PA Power Range bit within the GpioPinsSetClear structure.
     * There are 2 PA bias range values:
     *   - high power: PA bias >  27 dBm
     *   - low  power: PA bias <= 27 dBm
     *
     * @param [in/out] gpio_pins_set_clear The GPIO output level and enable
     * settings.
     *
     * @param pa_power_range The specific PA bias range value to apply.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_pa_power_range)(
        struct GpioPinsSetClear* gpio_pins_set_clear,
        enum PowerRange          pa_power_range);

    /**
     * Set the RF Power Supply enable bit within the GpioPinsSetClear
     * structure.
     *
     * @param [in/out] gpio_pins_set_clear The GPIO output level and enable
     * settings.
     *
     * @param rf_ps_enable true to enable the external RF power supply,
     *                     false to disable the external RF power supply.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_rf_power_supply_enable)(
        struct GpioPinsSetClear* gpio_pins_set_clear,
        bool                     rf_ps_enable);

    /**
     * Set the Tx RF Filter (SAW) selection bit within the GpioPinsSetClear
     * structure.
     *
     * @param [in/out] gpio_pins_set_clear The GPIO output level and enable
     * settings.
     *
     * @param tx_rf_filter The Tx RF filter selection value.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_tx_rf_filter)(
        struct GpioPinsSetClear* gpio_pins_set_clear,
        enum RfFilter            tx_rf_filter);

    /**
     * Set the unused DIGITAL_IO pins of the Ex10 device to specific values
     * within the GpioPinsSetClear structure.
     *
     * @param [in/out] gpio_pins_set_clear The GPIO output level and enable
     * settings.
     *
     * @param dio_bits A bit-wise set of DIGITAL_IO pins whose values are to
     *                 be set high (1) or low (0). This will set all of the
     *                 unused DIGITAL_IO lines to a specific bit pattern.
     * @details Setting dio_bits = (1u << 0u) | (1u << 1u); will set
     *              gpio_pins_set_clear.output_level_set  = 3
     *              gpio_pins_set_clear.output_enable_set = 3
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_dio_unused_pins)(
        struct GpioPinsSetClear* gpio_pins_set_clear,
        uint32_t                 dio_bits);

    /**
     * Print the GPIO 32-bit field in a friendly manner. Useful for debug.
     *
     * @param fp            A FILE stream pointer to send the printable output.
     * @param gpio_pin_bits The GPIO bit field.
     */
    void (*print_pin_bits)(uint32_t gpio_pin_bits);
};

struct Ex10GpioHelpers const* get_ex10_gpio_helpers(void);

#ifdef __cplusplus
}
#endif
