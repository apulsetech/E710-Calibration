/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2023 Impinj, Inc. All rights reserved.                      *
 *                                                                           *
 *****************************************************************************/

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ex10_api/ex10_result.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Ex10AntennaDisconnectListenBeforeTalk
{
    /**
     * Initialize the Antenna Disconnect and Listen Before Talk Module
     *  and register the ramp callbacks
     */
    struct Ex10Result (*init)(void);

    /**
     *  De-initialize the Antenna Disconnect and Listen Before Talk Module
     *  and unregister the ramp callbacks
     */
    struct Ex10Result (*deinit)(void);

    /**
     * These functions set the parameters for the antenna disconnect
     * module.  For the descriptions of these functions see the
     * stand alone antenna disconnect module.
     */
    void (*set_return_loss_cdb)(uint16_t return_loss_cdb);
    void (*set_max_margin_cdb)(int16_t max_margin_cdb);
    uint16_t (*get_last_reverse_power_adc_threshold)(void);
    uint16_t (*get_last_reverse_power_adc)(void);


    /**
     * These function set the parameters for the listen before talk
     * module.  For the descriptions of these functions see the
     * stand alone listen before talk module.
     */
    void (*set_rssi_count)(uint8_t rssi_count_exp);
    void (*set_passes_required)(uint8_t passes_required);
    void (*set_lbt_pass_threshold_cdbm)(int32_t lbt_pass_threshold_cdbm);
    void (*set_max_rssi_measurements)(uint32_t set_max_rssi_measurements);
    void (*set_measurement_delay_us)(uint16_t measurement_delay_us);
    int16_t (*get_last_rssi_measurement)(void);
    uint32_t (*get_last_frequency_khz)(void);
    uint32_t (*get_total_num_rssi_measurements)(void);
};

const struct Ex10AntennaDisconnectListenBeforeTalk*
    get_ex10_antenna_disconnect_and_listen_before_talk(void);

#ifdef __cplusplus
}
#endif
