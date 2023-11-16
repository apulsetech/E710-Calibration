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

#include <stdbool.h>

#include "board/board_spec.h"
#include "calibration.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/ex10_rf_power.h"

#include "ex10_modules/ex10_antenna_disconnect.h"
#include "ex10_modules/ex10_antenna_disconnect_and_listen_before_talk.h"
#include "ex10_modules/ex10_listen_before_talk.h"
#include "ex10_modules/ex10_ramp_module_manager.h"


static struct Ex10Result init(void)
{
    return get_ex10_ramp_module_manager()->register_ramp_callbacks(
        get_ex10_listen_before_talk()->lbt_pre_ramp_callback,
        get_ex10_antenna_disconnect()->antenna_disconnect_post_ramp_callback);
}

static struct Ex10Result deinit(void)
{
    get_ex10_ramp_module_manager()->unregister_ramp_callbacks();

    return make_ex10_success();
}

static void set_return_loss_cdb(uint16_t return_loss_cdb)
{
    get_ex10_antenna_disconnect()->set_return_loss_cdb(return_loss_cdb);
}

static void set_max_margin_cdb(int16_t max_margin_cdb)
{
    get_ex10_antenna_disconnect()->set_max_margin_cdb(max_margin_cdb);
}

static uint16_t get_last_reverse_power_adc_threshold(void)
{
    return get_ex10_antenna_disconnect()
        ->get_last_reverse_power_adc_threshold();
}

static uint16_t get_last_reverse_power_adc(void)
{
    return get_ex10_antenna_disconnect()->get_last_reverse_power_adc();
}

static void set_rssi_count(uint8_t rssi_count_exp)
{
    get_ex10_listen_before_talk()->set_rssi_count(rssi_count_exp);
}

static void set_passes_required(uint8_t passes_required)
{
    get_ex10_listen_before_talk()->set_passes_required(passes_required);
}

static void set_lbt_pass_threshold_cdbm(int32_t lbt_pass_threshold_cdbm)
{
    get_ex10_listen_before_talk()->set_lbt_pass_threshold_cdbm(
        lbt_pass_threshold_cdbm);
}

static void set_max_rssi_measurements(uint32_t set_max_rssi_measurements)
{
    get_ex10_listen_before_talk()->set_max_rssi_measurements(
        set_max_rssi_measurements);
}

static void set_measurement_delay_us(uint16_t measurement_delay_us)
{
    get_ex10_listen_before_talk()->set_measurement_delay_us(
        measurement_delay_us);
}

static int16_t get_last_rssi_measurement(void)
{
    return get_ex10_listen_before_talk()->get_last_rssi_measurement();
}

static uint32_t get_last_frequency_khz(void)
{
    return get_ex10_listen_before_talk()->get_last_frequency_khz();
}

static uint32_t get_total_num_rssi_measurements(void)
{
    return get_ex10_listen_before_talk()->get_total_num_rssi_measurements();
}

static const struct Ex10AntennaDisconnectListenBeforeTalk ex10_ad_and_lbt = {
    .init                = init,
    .deinit              = deinit,
    .set_return_loss_cdb = set_return_loss_cdb,
    .set_max_margin_cdb  = set_max_margin_cdb,
    .get_last_reverse_power_adc_threshold =
        get_last_reverse_power_adc_threshold,
    .get_last_reverse_power_adc      = get_last_reverse_power_adc,
    .set_rssi_count                  = set_rssi_count,
    .set_passes_required             = set_passes_required,
    .set_lbt_pass_threshold_cdbm     = set_lbt_pass_threshold_cdbm,
    .set_max_rssi_measurements       = set_max_rssi_measurements,
    .set_measurement_delay_us        = set_measurement_delay_us,
    .get_last_rssi_measurement       = get_last_rssi_measurement,
    .get_last_frequency_khz          = get_last_frequency_khz,
    .get_total_num_rssi_measurements = get_total_num_rssi_measurements,
};

const struct Ex10AntennaDisconnectListenBeforeTalk*
    get_ex10_antenna_disconnect_and_listen_before_talk(void)
{
    return &ex10_ad_and_lbt;
}
