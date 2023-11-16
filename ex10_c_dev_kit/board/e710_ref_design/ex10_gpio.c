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

#include <assert.h>

#include "board/e710_ref_design/ex10_gpio.h"
#include "board/ex10_gpio.h"
#include "board_spec_constants.h"

#include "ex10_api/ex10_print.h"

#define ANTENNA_PIN_COUNT ((size_t)1u)

static_assert(ANTENNA_PIN_COUNT > 0u, "");
static_assert(ANTENNA_PORT_COUNT <= (1u << ANTENNA_PIN_COUNT), "");

/**
 * @struct BoardGpioPins
 * Define the functionality for the GPIO pins in a board-specific manner.
 */
struct BoardGpioPins
{
    /// DIGITAL_IO[19] Selects the active antenna port
    ///   - '0': Antenna port 2
    ///   - '1': Antenna port 1
    uint8_t antenna[ANTENNA_PIN_COUNT];

    /// DIGITAL_IO[7] Selects the Rx baseband Dense Reader Mode (DRM) filter
    ///   - '1': Bandpass (BPF)
    ///   - '2': Highpass (HPF)
    uint8_t baseband_filter;

    /// @{ These DIO pins on the Ex10 development board are unconnected.
    uint8_t dio_0;
    uint8_t dio_1;
    uint8_t dio_6;
    uint8_t dio_8;
    uint8_t dio_13;
    /// @}

    /// DIGITAL_IO[16] PA_BIAS_EN Active High:
    /// Enable the PA bias to APC1, APC2 pins.
    uint8_t pa_bias_enable;

    /// DIGITAL_IO[15] Sets the VCC_PA supply voltage level.
    ///   - '1': >27 dBm High power
    ///   - '0': Low power <= +27 dBm
    uint8_t power_range;

    /// DIGITAL_IO[17] RF_PS_EN Active high:
    /// Enables the VCC_PA power supply
    uint8_t rf_enable;

    /// DIGITAL_IO[18] Selects the Tx SAW filter:
    ///   - '0': 902 - 928 MHz
    ///   - '1': 866 - 868 MHz
    uint8_t saw_filter;
};

static struct BoardGpioPins const board_gpio_pins = {
    .antenna         = {19u},
    .baseband_filter = 7u,
    .dio_0           = 0u,
    .dio_1           = 1u,
    .dio_6           = 6u,
    .dio_8           = 8u,
    .dio_13          = 13u,
    .pa_bias_enable  = 16u,
    .power_range     = 15u,
    .rf_enable       = 17u,
    .saw_filter      = 18u,
};

static void ex10_gpio_get_config(uint32_t               gpio_levels,
                                 struct Ex10GpioConfig* gpio_config)
{
    // clang-format off
    struct Ex10GpioConfig config = {
        .antenna         = ((1u << board_gpio_pins.antenna[0]     ) & gpio_levels) ? 1u : 2u,
        .baseband_filter = ((1u << board_gpio_pins.baseband_filter) & gpio_levels) ? BasebandFilterBandpass : BasebandFilterHighpass,
        .dio_0           = ((1u << board_gpio_pins.dio_0          ) & gpio_levels) ? true : false,
        .dio_1           = ((1u << board_gpio_pins.dio_1          ) & gpio_levels) ? true : false,
        .dio_6           = ((1u << board_gpio_pins.dio_6          ) & gpio_levels) ? true : false,
        .dio_8           = ((1u << board_gpio_pins.dio_8          ) & gpio_levels) ? true : false,
        .dio_13          = ((1u << board_gpio_pins.dio_13         ) & gpio_levels) ? true : false,
        .pa_bias_enable  = ((1u << board_gpio_pins.pa_bias_enable ) & gpio_levels) ? true : false,
        .power_range     = ((1u << board_gpio_pins.power_range    ) & gpio_levels) ? PowerRangeHigh : PowerRangeLow,
        .rf_enable       = ((1u << board_gpio_pins.rf_enable      ) & gpio_levels) ? true : false,
        .rf_filter       = ((1u << board_gpio_pins.saw_filter     ) & gpio_levels) ? LOWER_BAND : UPPER_BAND,
    };
    // clang-format on

    *gpio_config = config;
}

static struct Ex10Result ex10_gpio_get_levels(
    struct Ex10GpioConfig const* gpio_config,
    uint32_t*                    gpio_levels)
{
    if ((gpio_config == NULL) || (gpio_levels == NULL))
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio, Ex10SdkErrorNullPointer);
    }

    if ((gpio_config->antenna != 1u) && (gpio_config->antenna != 2u))
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio,
                                   Ex10SdkErrorBadParamValue);
    }

    if ((gpio_config->baseband_filter != BasebandFilterHighpass) &&
        (gpio_config->baseband_filter != BasebandFilterBandpass))
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio,
                                   Ex10SdkErrorBadParamValue);
    }

    if ((gpio_config->power_range != PowerRangeLow) &&
        (gpio_config->power_range != PowerRangeHigh))
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio,
                                   Ex10SdkErrorBadParamValue);
    }

    if ((gpio_config->rf_filter != LOWER_BAND) &&
        (gpio_config->rf_filter != UPPER_BAND))
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio,
                                   Ex10SdkErrorBadParamValue);
    }

    uint32_t const antenna_level = (gpio_config->antenna == 1u) ? 1u : 0u;
    uint32_t const baseband_filter_level =
        (gpio_config->baseband_filter == BasebandFilterBandpass) ? 1u : 0u;
    uint32_t const dio_0                = (gpio_config->dio_0 == 1u) ? 1u : 0u;
    uint32_t const dio_1                = (gpio_config->dio_1 == 1u) ? 1u : 0u;
    uint32_t const dio_6                = (gpio_config->dio_6 == 1u) ? 1u : 0u;
    uint32_t const dio_8                = (gpio_config->dio_8 == 1u) ? 1u : 0u;
    uint32_t const dio_13               = (gpio_config->dio_13 == 1u) ? 1u : 0u;
    uint32_t const pa_bias_enable_level = gpio_config->pa_bias_enable ? 1u : 0u;
    uint32_t const power_range_level =
        (gpio_config->power_range == PowerRangeHigh) ? 1u : 0u;
    uint32_t const rf_enable_level = gpio_config->rf_enable ? 1u : 0u;
    uint32_t const rf_filter_level =
        (gpio_config->rf_filter == LOWER_BAND) ? 1u : 0u;

    // clang-format off
    uint32_t const gpio_level_bits =
        (antenna_level         << board_gpio_pins.antenna[0]     ) |
        (baseband_filter_level << board_gpio_pins.baseband_filter) |
        (dio_0                 << board_gpio_pins.dio_0          ) |
        (dio_1                 << board_gpio_pins.dio_1          ) |
        (dio_6                 << board_gpio_pins.dio_6          ) |
        (dio_8                 << board_gpio_pins.dio_8          ) |
        (dio_13                << board_gpio_pins.dio_13         ) |
        (pa_bias_enable_level  << board_gpio_pins.pa_bias_enable ) |
        (power_range_level     << board_gpio_pins.power_range    ) |
        (rf_enable_level       << board_gpio_pins.rf_enable      ) |
        (rf_filter_level       << board_gpio_pins.saw_filter     ) ;
    // clang-format on

    *gpio_levels = gpio_level_bits;

    return make_ex10_success();
}

static uint32_t ex10_gpio_get_output_enables(void)
{
    // clang-format off
    uint32_t const gpio_enable_bits =
        (1u << board_gpio_pins.antenna[0]     ) |
        (1u << board_gpio_pins.baseband_filter) |
        (1u << board_gpio_pins.dio_0          ) |
        (1u << board_gpio_pins.dio_1          ) |
        (1u << board_gpio_pins.dio_6          ) |
        (1u << board_gpio_pins.dio_8          ) |
        (1u << board_gpio_pins.dio_13         ) |
        (1u << board_gpio_pins.pa_bias_enable ) |
        (1u << board_gpio_pins.power_range    ) |
        (1u << board_gpio_pins.rf_enable      ) |
        (1u << board_gpio_pins.saw_filter     ) ;
    // clang-format on

    return gpio_enable_bits;
}

static void set_clear_gpio_pins(struct GpioPinsSetClear* gpio_pins_set_clear,
                                uint32_t                 level_bits,
                                uint32_t                 mask_bits)
{
    gpio_pins_set_clear->output_level_set |= level_bits & mask_bits;
    gpio_pins_set_clear->output_level_clear |= (~level_bits) & mask_bits;
    gpio_pins_set_clear->output_enable_set |= mask_bits;
    gpio_pins_set_clear->output_enable_clear |= 0u;  // no pins are disabled
}

static void set_clear_gpio_single_pin(
    struct GpioPinsSetClear* gpio_pins_set_clear,
    bool                     level,
    uint8_t                  gpio_pin)
{
    uint32_t const level_32 = level ? 1u : 0u;
    set_clear_gpio_pins(
        gpio_pins_set_clear, level_32 << gpio_pin, 1u << gpio_pin);
}

static struct Ex10Result set_antenna_port(
    struct GpioPinsSetClear* gpio_pins_set_clear,
    uint8_t                  antenna)
{
    if (gpio_pins_set_clear == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio, Ex10SdkErrorNullPointer);
    }

    if ((antenna > ANTENNA_PORT_COUNT) || (antenna == 0))
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio,
                                   Ex10SdkErrorBadParamValue);
    }

    uint32_t const level      = (antenna == 1u) ? 1u : 0u;
    uint32_t const level_bits = level << board_gpio_pins.antenna[0];
    uint32_t const mask_bits  = 1u << board_gpio_pins.antenna[0];

    set_clear_gpio_pins(gpio_pins_set_clear, level_bits, mask_bits);

    return make_ex10_success();
}

static struct Ex10Result set_rx_baseband_filter(
    struct GpioPinsSetClear* gpio_pins_set_clear,
    enum BasebandFilterType  baseband_filter)
{
    if (gpio_pins_set_clear == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio, Ex10SdkErrorNullPointer);
    }

    if ((baseband_filter != BasebandFilterHighpass) &&
        (baseband_filter != BasebandFilterBandpass))
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio,
                                   Ex10SdkErrorBadParamValue);
    }

    uint32_t const level =
        (baseband_filter == BasebandFilterBandpass) ? 1u : 0u;

    set_clear_gpio_single_pin(
        gpio_pins_set_clear, level, board_gpio_pins.baseband_filter);

    return make_ex10_success();
}

static struct Ex10Result set_pa_bias_enable(
    struct GpioPinsSetClear* gpio_pins_set_clear,
    bool                     pa_bias_enable)
{
    if (gpio_pins_set_clear == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio, Ex10SdkErrorNullPointer);
    }

    set_clear_gpio_single_pin(
        gpio_pins_set_clear, pa_bias_enable, board_gpio_pins.pa_bias_enable);

    return make_ex10_success();
}

static struct Ex10Result set_pa_power_range(
    struct GpioPinsSetClear* gpio_pins_set_clear,
    enum PowerRange          power_range)
{
    if (gpio_pins_set_clear == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio, Ex10SdkErrorNullPointer);
    }

    if ((power_range != PowerRangeLow) && (power_range != PowerRangeHigh))
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio,
                                   Ex10SdkErrorBadParamValue);
    }

    uint32_t const level = (power_range == PowerRangeHigh) ? 1u : 0u;

    set_clear_gpio_single_pin(
        gpio_pins_set_clear, level, board_gpio_pins.power_range);

    return make_ex10_success();
}

static struct Ex10Result set_rf_power_supply_enable(
    struct GpioPinsSetClear* gpio_pins_set_clear,
    bool                     rf_ps_enable)
{
    if (gpio_pins_set_clear == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio, Ex10SdkErrorNullPointer);
    }

    set_clear_gpio_single_pin(
        gpio_pins_set_clear, rf_ps_enable, board_gpio_pins.rf_enable);

    return make_ex10_success();
}

static struct Ex10Result set_tx_rf_filter(
    struct GpioPinsSetClear* gpio_pins_set_clear,
    enum RfFilter            tx_rf_filter)
{
    if (gpio_pins_set_clear == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio, Ex10SdkErrorNullPointer);
    }

    if ((tx_rf_filter != LOWER_BAND) && (tx_rf_filter != UPPER_BAND))
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio,
                                   Ex10SdkErrorBadParamValue);
    }

    uint32_t const level = (tx_rf_filter == LOWER_BAND) ? 1u : 0u;
    set_clear_gpio_single_pin(
        gpio_pins_set_clear, level, board_gpio_pins.saw_filter);

    return make_ex10_success();
}

static struct Ex10Result set_dio_unused_pins(
    struct GpioPinsSetClear* gpio_pins_set_clear,
    uint32_t                 dio_bits)
{
    if (gpio_pins_set_clear == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleEx10Gpio, Ex10SdkErrorNullPointer);
    }

    uint32_t const level_bits = dio_bits;
    uint32_t const mask_bits =
        (1u << board_gpio_pins.dio_0) | (1u << board_gpio_pins.dio_1) |
        (1u << board_gpio_pins.dio_6) | (1u << board_gpio_pins.dio_8) |
        (1u << board_gpio_pins.dio_13);

    set_clear_gpio_pins(gpio_pins_set_clear, level_bits, mask_bits);

    return make_ex10_success();
}

static void gpio_print_pin_bits(uint32_t gpio_pin_bits)
{
    // clang-format off
    uint8_t                 const antenna         = ((1u << board_gpio_pins.antenna[0]     ) & gpio_pin_bits) ? 1u : 2u;
    enum BasebandFilterType const baseband_filter = ((1u << board_gpio_pins.baseband_filter) & gpio_pin_bits) ? BasebandFilterBandpass : BasebandFilterHighpass;
    bool                    const pa_bias_enable  = ((1u << board_gpio_pins.pa_bias_enable ) & gpio_pin_bits) ? true : false;
    enum PowerRange         const power_range     = ((1u << board_gpio_pins.power_range    ) & gpio_pin_bits) ? PowerRangeHigh : PowerRangeLow;
    bool                    const rf_ps_enable    = ((1u << board_gpio_pins.rf_enable      ) & gpio_pin_bits) ? true : false;
    enum RfFilter           const tx_rf_filter    = ((1u << board_gpio_pins.saw_filter     ) & gpio_pin_bits) ? LOWER_BAND : UPPER_BAND;
    // clang-format on

    char const* bbf =
        (baseband_filter == BasebandFilterBandpass) ? "BPF" : "HPF";
    char const* rff = (tx_rf_filter == LOWER_BAND) ? "LOWER" : "UPPER";

    ex10_printf("gpio bits: 0x%06x\n", gpio_pin_bits);
    ex10_printf("Antenna:   %u\n", antenna);
    ex10_printf("BB filter: %u %s\n", baseband_filter, bbf);
    ex10_printf("PA enable: %u\n", pa_bias_enable);
    ex10_printf("PA range:  %u\n", power_range);
    ex10_printf("RF enable: %u\n", rf_ps_enable);
    ex10_printf("RF filter: %u %s\n", tx_rf_filter, rff);
}

static struct Ex10GpioHelpers const ex10_gpio_helpers = {
    .get_levels                 = ex10_gpio_get_levels,
    .get_config                 = ex10_gpio_get_config,
    .get_output_enables         = ex10_gpio_get_output_enables,
    .set_antenna_port           = set_antenna_port,
    .set_rx_baseband_filter     = set_rx_baseband_filter,
    .set_pa_bias_enable         = set_pa_bias_enable,
    .set_pa_power_range         = set_pa_power_range,
    .set_rf_power_supply_enable = set_rf_power_supply_enable,
    .set_tx_rf_filter           = set_tx_rf_filter,
    .set_dio_unused_pins        = set_dio_unused_pins,
    .print_pin_bits             = gpio_print_pin_bits,
};

struct Ex10GpioHelpers const* get_ex10_gpio_helpers(void)
{
    return &ex10_gpio_helpers;
}
