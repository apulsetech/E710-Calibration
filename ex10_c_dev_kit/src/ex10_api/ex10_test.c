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

#include "ex10_api/application_registers.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/ex10_test.h"
#include "ex10_modules/ex10_ramp_module_manager.h"


static struct Ex10Result cw_test(
    uint8_t                                    antenna,
    enum RfModes                               rf_mode,
    int16_t                                    tx_power_cdbm,
    uint32_t                                   frequency_khz,
    struct PowerDroopCompensationFields const* droop_comp,
    uint16_t                                   temperature_adc,
    bool                                       temp_comp_enabled)
{
    if (frequency_khz)
    {
        get_ex10_active_region()->set_single_frequency(frequency_khz);
    }
    struct CwConfig cw_config;
    get_ex10_rf_power()->build_cw_configs(antenna,
                                          rf_mode,
                                          tx_power_cdbm,
                                          temperature_adc,
                                          temp_comp_enabled,
                                          &cw_config);

    struct Ex10Result ex10_result = get_ex10_rf_power()->set_rf_mode(rf_mode);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    struct Ex10RampModuleManager const* ramp_module_manager =
        get_ex10_ramp_module_manager();
    ramp_module_manager->store_pre_ramp_variables(antenna);
    ramp_module_manager->store_post_ramp_variables(tx_power_cdbm,
                                                   frequency_khz);

    return get_ex10_rf_power()->cw_on(&cw_config.gpio,
                                      &cw_config.power,
                                      &cw_config.synth,
                                      &cw_config.timer,
                                      droop_comp);
}

static struct Ex10Result prbs_test(uint8_t      antenna,
                                   enum RfModes rf_mode,
                                   int16_t      tx_power_cdbm,
                                   uint32_t     frequency_khz,
                                   uint16_t     temperature_adc,
                                   bool         temp_comp_enabled)
{
    struct PowerDroopCompensationFields droop_comp =
        get_ex10_rf_power()->get_droop_compensation_defaults();
    struct Ex10Result ex10_result = cw_test(antenna,
                                            rf_mode,
                                            tx_power_cdbm,
                                            frequency_khz,
                                            &droop_comp,
                                            temperature_adc,
                                            temp_comp_enabled);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Wait until fully ramped up
    const struct Ex10Ops* ops = get_ex10_ops();
    ex10_result               = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    return ops->start_prbs();
}

static struct Ex10Result ber_test(uint8_t      antenna,
                                  enum RfModes rf_mode,
                                  int16_t      tx_power_cdbm,
                                  uint32_t     frequency_khz,
                                  uint16_t     num_bits,
                                  uint16_t     num_packets,
                                  bool         delimiter_only,
                                  uint16_t     temperature_adc,
                                  bool         temp_comp_enabled)
{
    get_ex10_active_region()->disable_regulatory_timers();
    struct PowerDroopCompensationFields droop_comp =
        get_ex10_rf_power()->get_droop_compensation_defaults();
    struct Ex10Result ex10_result = cw_test(antenna,
                                            rf_mode,
                                            tx_power_cdbm,
                                            frequency_khz,
                                            &droop_comp,
                                            temperature_adc,
                                            temp_comp_enabled);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Wait until fully ramped up
    const struct Ex10Ops* ops = get_ex10_ops();
    ex10_result               = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    return ops->start_ber_test(num_bits, num_packets, delimiter_only);
}

static struct Ex10Result etsi_burst(
    struct InventoryRoundControlFields const*   inventory_config,
    struct InventoryRoundControl_2Fields const* inventory_config_2,
    struct GpioPinsSetClear const*              gpio_pins_set_clear,
    enum RfModes                                rf_mode,
    struct PowerConfigs*                        power_config,
    struct RfSynthesizerControlFields const*    synth_control,
    struct Ex10RegulatoryTimers const*          timer_config,
    uint16_t                                    on_time_ms,
    uint16_t                                    off_time_ms)
{
    struct Ex10Ops const*      ops      = get_ex10_ops();
    struct Ex10Protocol const* protocol = get_ex10_protocol();
    struct Ex10RfPower const*  rf_power = get_ex10_rf_power();

    // Prevent redundant CW on
    if (rf_power->get_cw_is_on())
    {
        return make_ex10_success();
    }

    struct Ex10Result ex10_result =
        ops->set_clear_gpio_pins(gpio_pins_set_clear);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->lock_synthesizer(synth_control->r_divider,
                                        synth_control->n_divider);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ops->set_rf_mode(rf_mode);
    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Ramp up once to run the power control loop
    ex10_result = rf_power->ramp_transmit_power(power_config, timer_config);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // The CW on process used here waits for power control to
    // finish. This process ignores if the power ramp failed
    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Ramp down and prepare for the ETSI burst
    ex10_result = ops->tx_ramp_down();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Set the timers for proper etsi burst
    struct Ex10RegulatoryTimers const etsi_burst_timers = {
        .nominal_ms          = on_time_ms,
        .extended_ms         = on_time_ms + 5,
        .regulatory_ms       = 0,
        .off_same_channel_ms = off_time_ms,
    };
    rf_power->set_regulatory_timers(&etsi_burst_timers);

    // Set the inventory params for etsi burst
    protocol->write(&inventory_round_control_reg, inventory_config);
    protocol->write(&inventory_round_control_2_reg, inventory_config_2);

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Start the op
    return ops->run_etsi_burst();
}

static struct Ex10Result etsi_burst_test(
    struct InventoryRoundControlFields const*   inventory_config,
    struct InventoryRoundControl_2Fields const* inventory_config_2,
    uint8_t                                     antenna,
    enum RfModes                                rf_mode,
    int16_t                                     tx_power_cdbm,
    uint16_t                                    on_time_ms,
    uint16_t                                    off_time_ms,
    uint32_t                                    frequency_khz,
    uint16_t                                    temperature_adc,
    bool                                        temp_comp_enabled)
{
    if (frequency_khz)
    {
        get_ex10_active_region()->set_single_frequency(frequency_khz);
    }
    struct CwConfig cw_config;
    get_ex10_rf_power()->build_cw_configs(antenna,
                                          rf_mode,
                                          tx_power_cdbm,
                                          temperature_adc,
                                          temp_comp_enabled,
                                          &cw_config);

    return etsi_burst(inventory_config,
                      inventory_config_2,
                      &cw_config.gpio,
                      cw_config.rf_mode,
                      &cw_config.power,
                      &cw_config.synth,
                      &cw_config.timer,
                      on_time_ms,
                      off_time_ms);
}

static const struct Ex10Test ex10_test = {
    .cw_test         = cw_test,
    .prbs_test       = prbs_test,
    .ber_test        = ber_test,
    .etsi_burst_test = etsi_burst_test,
};

const struct Ex10Test* get_ex10_test(void)
{
    return &ex10_test;
}
