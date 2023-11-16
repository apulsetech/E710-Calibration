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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "board/ex10_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct Ex10GpioConfig
 *
 * E710 Evaluation board GPIO levels mapping:
 * Antenna:                   2: '0',           1: '1'
 * Baseband Filter:    Highpass: '0',    Bandpass: '1'
 * PA Bias Enable:      Disable: '0',      Enable: '1'
 * PA Bias Range:           Low: '0',        High: '1'
 * RF Enable:           Disable: '0',      Enable: '1'
 * SAW Filter:      900-930 MHz: '0', 865-868 MHz: '1'
 *
 * @param gpio_config Board agnostic settings which require GPIO levels to be
 *                    set to specific levels.
 */
struct Ex10GpioConfig
{
    uint8_t                 antenna;
    enum BasebandFilterType baseband_filter;
    bool                    dio_0;
    bool                    dio_1;
    bool                    dio_6;
    bool                    dio_8;
    bool                    dio_13;
    bool                    pa_bias_enable;
    enum PowerRange         power_range;
    bool                    rf_enable;
    enum RfFilter           rf_filter;
};

#ifdef __cplusplus
}
#endif
