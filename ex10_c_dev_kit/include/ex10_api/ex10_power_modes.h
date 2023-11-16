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

#pragma once

#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum PowerMode
 *
 * The power modes control provides top level control of the Impinj Reader Chip,
 * its peripherals connected through the GPIO pins (labelled DIGITAL_IO on the
 * schematic), and the Impinj Reader Chip internal analog power.
 *
 *  |                    |   Off  | Standby |  Ready  | ReadyCold |
 *  |:------------------:|:------:|:-------:|:-------:|:---------:|
 *  | PA_BIAS_ENABLE     |    X   |    0    |    1    |     0     |
 *  | RF_PS_ENABLE       |    X   |    0    |    1    |     0     |
 *  | Ex10 Analog Power  |    X   |    0    |    1    |     1     |
 *  | PWR_EN             |    0   |    1    |    1    |     1     |
 *
 * X = Don't Care
 *
 * @see struct Ex10GpioConfig, struct Ex10GpioHelpers
 */
enum PowerMode
{
    /// The invalid state is set if the power mode API call fails to change
    /// state. This state is set if any of the calls to the Ops layer fail.
    PowerModeInvalid = 0,
    /// Power is removed from the Impinj Reader Chip and its peripherals.
    PowerModeOff = 1,
    /// The Impinj Reader Chip is powered, however, its internal analog power
    /// supplies are disabled. The external Power Amplifer is not powered.
    PowerModeStandby = 2,
    /// The Impinj Reader Chip and is fully powered up; including its internal
    /// analog circuitry. External peripherals are not powered on.
    PowerModeReadyCold = 3,
    /// The Impinj Reader Chip and its internal analog circuitry are powered.
    /// The external Power Amplifer is powered.
    /// The Impinj Reader Chip is ready to inventory RFID tags.
    PowerModeReady = 4,
};

/**
 * @struct Ex10PowerModes
 * The Ex10 reader interface.
 *
 * @note It is recommended that the initial Ex10Reader.init_ex10() function
 *       sets the Impinj Reader Chip into a state consistent with
 *       ``PowerModeReady``.
 *       - This is consistent with how the Ex10Reader.init_ex10() initializes
 *         the Impinj Reader Chip.
 *       - This is consistent with the
 *         Ex10BoardSpec.get_default_gpio_output_levels() implementation.
 *       - This will not conflict with stored settings startup sequencing.
 *       - It is expected that since the Impinj Reader Chip is being powered
 *         up then there are tags to be inventoried; PowerModeReady is ready
 *         to start inventorying tags.
 *       - If the Ex10Reader module is changed to complete its initialization,
 *         via the Ex10Reader.init_ex10() function, into a mode other than
 *         PowerModeReady, then the Ex10PowerModes.init() behavior will need
 *         to change accordingly.
 */
struct Ex10PowerModes
{
    /** Initialize the Ex10PowerModes object. */
    void (*init)(void);

    /**
     * Release any resources used by the Ex10PowerModes object.
     */
    void (*deinit)(void);

    /**
     * Set the power mode.
     *
     * @param power_mode The power mode to set. @see enum PowerMode
     *
     * @return struct Ex10Result
     *         Indicates whether the transition to the requested power mode
     *         was successful or not.
     */
    struct Ex10Result (*set_power_mode)(enum PowerMode power_mode);

    /**
     * Get the Ex10PowerModes power mode.
     *
     * @return enum PowerMode The current operating power mode.
     */
    enum PowerMode (*get_power_mode)(void);
};

struct Ex10PowerModes const* get_ex10_power_modes(void);

#ifdef __cplusplus
}
#endif
