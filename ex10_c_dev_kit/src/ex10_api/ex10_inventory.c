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
#include <string.h>

#include "board/board_spec.h"
#include "calibration.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_rf_power.h"

#include "ex10_modules/ex10_ramp_module_manager.h"


static struct Ex10Result run_inventory(
    struct InventoryRoundControlFields const*   inventory_config,
    struct InventoryRoundControl_2Fields const* inventory_config_2,
    bool                                        send_selects)
{
    struct Ex10Ops const*      ops      = get_ex10_ops();
    struct Ex10Protocol const* protocol = get_ex10_protocol();

    // check to make sure that an op isn't running (say if inventory is
    // called twice by accident)
    if (protocol->is_op_currently_running())
    {
        return make_ex10_sdk_error(Ex10ModuleInventory, Ex10SdkErrorOpRunning);
    }

    if (send_selects)
    {
        // Sends any enabled selects in the gen2 tx buffer
        struct Ex10Result ex10_result = ops->send_select();
        if (ex10_result.error)
        {
            return ex10_result;
        }
        ex10_result = ops->wait_op_completion();
        if (ex10_result.error)
        {
            return ex10_result;
        }
    }

    if (inventory_config->tag_focus_enable)
    {
        if (inventory_config->session != SessionS1)
        {
            ex10_printf(
                "Warning: TagFocus feature will only work on session S1; "
                "inventory session requested: %u.\n",
                inventory_config->session);
        }
    }

    // Run a round of inventory and return even if CW is still on
    return ops->start_inventory_round(inventory_config, inventory_config_2);
}

static struct Ex10Result start_inventory(
    uint8_t                                     antenna,
    enum RfModes                                rf_mode,
    int16_t                                     tx_power_cdbm,
    struct InventoryRoundControlFields const*   inventory_config,
    struct InventoryRoundControl_2Fields const* inventory_config_2,
    bool                                        send_selects)
{
    struct Ex10RfPower const*           ex10_rf_power = get_ex10_rf_power();
    struct Ex10RampModuleManager const* ramp_module_manager =
        get_ex10_ramp_module_manager();

    struct Ex10Result ex10_result = ex10_rf_power->set_rf_mode(rf_mode);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    const bool cw_is_on      = ex10_rf_power->get_cw_is_on();
    uint16_t temperature_adc = ramp_module_manager->retrieve_adc_temperature();

    // If CW is already on, there is no need to measure temperature for setting
    // the RF power settings.
    if (cw_is_on == false || temperature_adc == INT16_MAX)
    {
        ex10_result = get_ex10_rf_power()->measure_and_read_adc_temperature(
            &temperature_adc);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        ramp_module_manager->store_adc_temperature(temperature_adc);
    }

    // If the temperature reading was invalid or cw is already on,
    // disable temperature compensation.
    bool const temp_comp_enabled =
        get_ex10_board_spec()->temperature_compensation_enabled(
            temperature_adc);

    struct PowerDroopCompensationFields const droop_comp_fields =
        ex10_rf_power->get_droop_compensation_defaults();

    if (cw_is_on == false)
    {
        // Update the channel time tracking before kicking off the
        // next inventory round. This will be used to update the
        // regulatory timers if the inventory call needs to ramp up
        // again.
        ex10_result = get_ex10_active_region()->update_channel_time_tracking();
        if (ex10_result.error)
        {
            return ex10_result;
        }

        struct CwConfig cw_config;
        ex10_rf_power->build_cw_configs(antenna,
                                        rf_mode,
                                        tx_power_cdbm,
                                        temperature_adc,
                                        temp_comp_enabled,
                                        &cw_config);

        ramp_module_manager->store_pre_ramp_variables(antenna);
        ramp_module_manager->store_post_ramp_variables(
            tx_power_cdbm, get_ex10_active_region()->get_next_channel_khz());

        // Note that cw_on() runs the AggregateOp and waits for Op completion.
        // No need to wait again once cw_on() returns.
        ex10_result = ex10_rf_power->cw_on(&cw_config.gpio,
                                           &cw_config.power,
                                           &cw_config.synth,
                                           &cw_config.timer,
                                           &droop_comp_fields);
        if (ex10_result.error)
        {
            return ex10_result;
        }
    }
    ex10_result =
        run_inventory(inventory_config, inventory_config_2, send_selects);

    // There is a race condition where the sdk checks for cw, the device
    // reports it is ramped up, then it ramps down before select is run.
    // If this occurs, ramp up and and rerun. Any errors after this get
    // returned.
    if (ex10_result.error &&
        ex10_result.device_status.ops_status.op_id == SendSelectOp &&
        ex10_result.device_status.ops_status.error == ErrorInvalidTxState)
    {
        ex10_result = get_ex10_active_region()->update_channel_time_tracking();
        if (ex10_result.error)
        {
            return ex10_result;
        }

        struct CwConfig cw_config;
        ex10_rf_power->build_cw_configs(antenna,
                                        rf_mode,
                                        tx_power_cdbm,
                                        temperature_adc,
                                        temp_comp_enabled,
                                        &cw_config);

        ramp_module_manager->store_pre_ramp_variables(antenna);
        ramp_module_manager->store_post_ramp_variables(
            tx_power_cdbm, get_ex10_active_region()->get_next_channel_khz());

        ex10_result = ex10_rf_power->cw_on(&cw_config.gpio,
                                           &cw_config.power,
                                           &cw_config.synth,
                                           &cw_config.timer,
                                           &droop_comp_fields);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        ex10_result =
            run_inventory(inventory_config, inventory_config_2, send_selects);
    }
    return ex10_result;
}


static bool inventory_halted(void)
{
    struct HaltedStatusFields halted_status;
    get_ex10_protocol()->read(&halted_status_reg, &halted_status);
    return halted_status.halted;
}

static enum StopReason ex10_result_to_continuous_inventory_error(
    struct Ex10Result ex10_result)
{
    if (ex10_result.error)
    {
        if (ex10_result.module == Ex10ModuleDevice)
        {
            switch (ex10_result.result_code.device)
            {
                case Ex10DeviceSuccess:
                    return SRNone;
                case Ex10DeviceErrorCommandsNoResponse:
                case Ex10DeviceErrorCommandsWithResponse:
                    return SRDeviceCommandError;
                case Ex10DeviceErrorOps:
                    return SROpError;
                case Ex10DeviceErrorOpsTimeout:
                    return SRSdkTimeoutError;
                default:
                    return SRReasonUnknown;
            }
        }
        else
        {
            switch (ex10_result.result_code.sdk)
            {
                case Ex10SdkSuccess:
                case Ex10SdkErrorBadParamValue:
                case Ex10SdkErrorBadParamLength:
                case Ex10SdkErrorBadParamAlignment:
                case Ex10SdkErrorNullPointer:
                case Ex10SdkErrorTimeout:
                case Ex10SdkErrorRunLocation:
                case Ex10SdkErrorOpRunning:
                case Ex10SdkErrorInvalidState:
                case Ex10SdkNoFreeEventFifoBuffers:
                case Ex10SdkFreeEventFifoBuffersLengthMismatch:
                case Ex10InvalidEventFifoPacket:
                case Ex10SdkErrorGpioInterface:
                case Ex10SdkErrorHostInterface:
                case Ex10ErrorGen2BufferLength:
                case Ex10ErrorGen2NumCommands:
                case Ex10ErrorGen2CommandEncode:
                case Ex10ErrorGen2CommandDecode:
                case Ex10ErrorGen2CommandEnableMismatch:
                case Ex10ErrorGen2EmptyCommand:
                case Ex10SdkErrorUnexpectedTxLength:
                case Ex10BelowThreshold:
                case Ex10MemcpyFailed:
                case Ex10MemsetFailed:
                    return SRNone;

                case Ex10SdkErrorAggBufferOverflow:
                    return SRDeviceAggregateBufferOverflow;
                case Ex10AboveThreshold:
                    return SRDeviceRampCallbackError;
                case Ex10SdkEventFifoFull:
                    return SRDeviceEventFifoFull;
                case Ex10InventoryInvalidParam:
                    return SRDeviceInventoryInvalidParam;
                case Ex10SdkLmacOverload:
                    return SRDeviceLmacOverload;
                case Ex10InventorySummaryReasonInvalid:
                    return SRDeviceInventorySummaryReasonInvalid;
                default:
                    return SRReasonUnknown;
            }
        }
    }
    return SRNone;
}

static const struct Ex10Inventory ex10_inventory = {
    .run_inventory    = run_inventory,
    .start_inventory  = start_inventory,
    .inventory_halted = inventory_halted,
    .ex10_result_to_continuous_inventory_error =
        ex10_result_to_continuous_inventory_error,
};

const struct Ex10Inventory* get_ex10_inventory(void)
{
    return &ex10_inventory;
}
