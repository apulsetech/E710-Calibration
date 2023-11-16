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

#include <stdbool.h>

#include "board/board_spec.h"
#include "calibration.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/ex10_rf_power.h"

#include "ex10_modules/ex10_antenna_disconnect.h"
#include "ex10_modules/ex10_ramp_module_manager.h"


struct ReversePowerParams
{
    uint16_t return_loss_cdb;
    int16_t  max_margin_cdb;
    uint16_t last_threshold;
    uint16_t last_measurement;
};
/* Default parameters for reverse power threshold as used by the reader
 * callbacks. */

static struct ReversePowerParams rev_power_params = {
    .return_loss_cdb  = 1000,
    .max_margin_cdb   = -400,
    .last_threshold   = 0,
    .last_measurement = 0,
};

static void antenna_disconnect_post_ramp_callback(
    struct Ex10Result* ex10_result);

static struct Ex10Result init(void)
{
    return get_ex10_ramp_module_manager()->register_ramp_callbacks(
        NULL, antenna_disconnect_post_ramp_callback);
}

static struct Ex10Result deinit(void)
{
    get_ex10_ramp_module_manager()->unregister_ramp_callbacks();

    return make_ex10_success();
}

static void set_return_loss_cdb(uint16_t return_loss_cdb)
{
    rev_power_params.return_loss_cdb = return_loss_cdb;
}

static void set_max_margin_cdb(int16_t max_margin_cdb)
{
    rev_power_params.max_margin_cdb = max_margin_cdb;
}

/**
 * Determines if the return loss threshold was exceeded. User passed
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
static bool get_return_loss_threshold_exceeded(void)
{
    struct Ex10Calibration const*       cal = get_ex10_calibration();
    struct Ex10RampModuleManager const* ramp_module_manager =
        get_ex10_ramp_module_manager();

    /* Ensure the function exist in this cal version */
    if (cal->reverse_power_to_adc == NULL)
    {
        ex10_eprintf(
            "reverse_power_to_adc() function is NULL in "
            "the used calibration version\n");

        // To ensure that the error is noticed, returning 'true' here
        return true;
    }

    /* The insertion loss is the loss we expect based on the board. A user
     * should modify this based on the board used. Since the PDET cal table is
     * performed between the antenna port power and the LO pin, the difference
     * in insertion loss between antenna and LO pin, vs antenna and RX pin,
     * will need to be considered here to use the LO PDET cal.
     */
    int16_t const thresh =
        ramp_module_manager->retrieve_post_ramp_tx_power_cdbm() -
        (int16_t)rev_power_params.return_loss_cdb -
        ((int16_t)BOARD_INSERTION_LOSS_RX - (int16_t)BOARD_INSERTION_LOSS_LO) -
        rev_power_params.max_margin_cdb;

    /* Use the expected threshold found above to find...
     * 1. the reverse power detector to use
     * 2. the corresponding adc target on said reverse power detector
     */
    bool                                temp_comp_enabled = true;
    enum AuxAdcControlChannelEnableBits reverse_power_enables =
        ChannelEnableBitsNone;
    /* Note here, the reverse_power_to_adc function sets reverse_power_enables
     * to the appropriate power detector based on the expected threshold.
     * aka we chose a detector in range of our expected readings
     */
    uint16_t const reverse_power_adc_threshold = cal->reverse_power_to_adc(
        thresh,
        ramp_module_manager->retrieve_post_ramp_frequency_khz(),
        ramp_module_manager->retrieve_adc_temperature(),
        temp_comp_enabled,
        get_ex10_active_region()->get_rf_filter(),
        &reverse_power_enables);

    /* Ensure the function exist in this cal version */
    if (reverse_power_adc_threshold == CAL_FUNC_NOT_SUPPORTED)
    {
        ex10_eprintf(
            "This board does not support reverse power detection. The "
            "board is either uncalibrated or uses a calibration version "
            "which is too old. To get "
            "rid of this warning, please unregister this callback\n");
        return false;
    }

    enum AuxAdcResultsAdcResult rx_adc_result;
    if (reverse_power_enables == ChannelEnableBitsPowerRx0)
    {
        rx_adc_result = AdcResultPowerRx0;
    }
    else if (reverse_power_enables == ChannelEnableBitsPowerRx1)
    {
        rx_adc_result = AdcResultPowerRx1;
    }
    else if (reverse_power_enables == ChannelEnableBitsPowerRx2)
    {
        rx_adc_result = AdcResultPowerRx2;
    }
    else
    {
        return true;
    }

    uint16_t          reverse_power_adc = 0u;
    struct Ex10Result ex10_result =
        get_ex10_rf_power()->measure_and_read_aux_adc(
            rx_adc_result, 1u, &reverse_power_adc);
    if (ex10_result.error)
    {
        return true;
    }
    // Stash the theshold and last measurement away
    // in case they are needed later
    rev_power_params.last_threshold   = reverse_power_adc_threshold;
    rev_power_params.last_measurement = reverse_power_adc;

    if (reverse_power_adc >= reverse_power_adc_threshold)
    {
        return true;
    }

    return false;
}

static uint16_t get_last_reverse_power_adc_threshold(void)
{
    return rev_power_params.last_threshold;
}

static uint16_t get_last_reverse_power_adc(void)
{
    return rev_power_params.last_measurement;
}

static void antenna_disconnect_post_ramp_callback(
    struct Ex10Result* ex10_result)
{
    /* Call the reader function to check for proper rx power detection
     * Note: The tx power and frequency requires use of the reader layer to
     * update the stored values.
     */
    const bool thresh_exceeded = get_return_loss_threshold_exceeded();

    /* Problem in rx path, so stop transmitting */
    *ex10_result = make_ex10_success();
    if (thresh_exceeded)
    {
        get_ex10_rf_power()->stop_op_and_ramp_down();
        *ex10_result =
            make_ex10_sdk_error(Ex10AntennaDisconnect, Ex10AboveThreshold);
    }
    return;
}


static const struct Ex10AntennaDisconnect ex10_antenna_disconnect = {
    .init                = init,
    .deinit              = deinit,
    .set_return_loss_cdb = set_return_loss_cdb,
    .set_max_margin_cdb  = set_max_margin_cdb,
    .get_last_reverse_power_adc_threshold =
        get_last_reverse_power_adc_threshold,
    .get_last_reverse_power_adc         = get_last_reverse_power_adc,
    .get_return_loss_threshold_exceeded = get_return_loss_threshold_exceeded,
    .antenna_disconnect_post_ramp_callback =
        antenna_disconnect_post_ramp_callback,
};

const struct Ex10AntennaDisconnect* get_ex10_antenna_disconnect(void)
{
    return &ex10_antenna_disconnect;
}
