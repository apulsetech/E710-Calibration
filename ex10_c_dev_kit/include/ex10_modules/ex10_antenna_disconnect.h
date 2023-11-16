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

#include <stddef.h>
#include <stdint.h>

#include "ex10_api/ex10_result.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Ex10AntennaDisconnect
{
    /**
     * Initialize the Antenna Disconnect Module and register the ramp
     * callbacks
     */
    struct Ex10Result (*init)(void);

    /**
     *  De-initialize the Antenna Disconnect Module and unregister the ramp
     *  callbacks
     */
    struct Ex10Result (*deinit)(void);

    /**
     * @param return_loss_cdb The return loss at the board output in cdB.
     */

    void (*set_return_loss_cdb)(uint16_t return_loss_cdb);
    /**
     * @param max_margin_cdb The maximum margin to use in the calculation
     */
    void (*set_max_margin_cdb)(int16_t max_margin_cdb);

    /**
     * Retrieves the last reverse power threshold used in the antenna
     * disconnect callback.  This represents only the threshold that
     * was calculated during the callback.
     *
     * @return uint16_t The reverse power threshold in ADC counts
     */
    uint16_t (*get_last_reverse_power_adc_threshold)(void);

    /**
     * Retrieves the most recent reverse power measurement made duing the
     * post ramp up callback.
     *
     * @return uint16_t The reverse power measured in ADC counts
     */
    uint16_t (*get_last_reverse_power_adc)(void);

    /**
     * Determines if the return loss threshold was exceeded. Using user passed
     * parameters and board specific losses, this function checks if the
     * current measurement on the reverse power detectors is above the
     * allowed value.
     *
     * Note that the parameters are passed into the module
     *
     * @return Whether the threshold was exceeded. A true means that the reverse
     * power detector measured higher than expected, and a false means we are
     * within expectations.
     */
    bool (*get_return_loss_threshold_exceeded)(void);

    /**
     * This function pointer is the post ramp callback use by the Antenna
     * Disconnect ramping module.  It is intended to be used with a super
     * ramping module that includes the Antenna Disconnect behavior with another
     * module (the init() functionality should be used if using only this
     * module)
     */
    void (*antenna_disconnect_post_ramp_callback)(
        struct Ex10Result* ex10_result);
};

const struct Ex10AntennaDisconnect* get_ex10_antenna_disconnect(void);

#ifdef __cplusplus
}
#endif
