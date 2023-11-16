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
#include <stddef.h>

#include "ex10_modules/ex10_ramp_module_manager.h"

/**
 * @struct PrivateRampModuleVariables
 * Used at the top level by use cases and examples. This manager offers a
 * generic place where the information needed by the pre and post ramp
 * variables can be stored. This allows the use cases to be decoupled from
 * which ramping callbacks are installed (if any).  It also contains a place
 * to register these pre and post ramp callbacks, and a generic way to call
 * them.
 *
 * If new ramping modules need new parameters, they should be added to this
 * structure and the pre/post store functions to be expanded with them.
 * This will cause the builds to break where these functions are used and
 * will force all of the calling sites to be updated.
 */
struct PrivateModuleVariables
{
    void (*pre_ramp_callback)(struct Ex10Result*);
    void (*post_ramp_callback)(struct Ex10Result*);
    uint16_t adc_temperature;
};

struct PrivatePreRampVariables
{
    enum RfModes rf_mode;
    uint8_t      antenna;
};

struct PrivatePostRampVariables
{
    int16_t  tx_power_cdbm;
    uint32_t frequency_khz;
};

static struct PrivateModuleVariables module_variables = {
    .pre_ramp_callback  = NULL,
    .post_ramp_callback = NULL,
    .adc_temperature    = INT16_MAX,
};

static struct PrivatePreRampVariables pre_ramp_variables = {.antenna = 0};

static struct PrivatePostRampVariables post_ramp_variables = {
    .tx_power_cdbm = 0,
    .frequency_khz = 0};


static void store_adc_temperature(uint16_t adc_temperature)
{
    module_variables.adc_temperature = adc_temperature;
}

static void store_pre_ramp_variables(uint8_t antenna)
{
    pre_ramp_variables.antenna = antenna;
}

static void store_post_ramp_variables(int16_t  tx_power_cdbm,
                                      uint32_t frequency_khz)
{
    post_ramp_variables.tx_power_cdbm = tx_power_cdbm;
    post_ramp_variables.frequency_khz = frequency_khz;
}

static uint16_t retrieve_adc_temperature(void)
{
    return module_variables.adc_temperature;
}
static uint32_t retrieve_post_ramp_frequency_khz(void)
{
    return post_ramp_variables.frequency_khz;
}

static int16_t retrieve_post_ramp_tx_power_cdbm(void)
{
    return post_ramp_variables.tx_power_cdbm;
}

static uint8_t retrieve_pre_ramp_antenna(void)
{
    return pre_ramp_variables.antenna;
}

static struct Ex10Result call_pre_ramp_callback(void)
{
    if (module_variables.pre_ramp_callback != NULL)
    {
        struct Ex10Result ex10_result;
        module_variables.pre_ramp_callback(&ex10_result);
        return ex10_result;
    }

    return make_ex10_success();
}

static struct Ex10Result call_post_ramp_callback(void)
{
    if (module_variables.post_ramp_callback != NULL)
    {
        struct Ex10Result ex10_result;
        module_variables.post_ramp_callback(&ex10_result);
        return ex10_result;
    }
    return make_ex10_success();
}

static struct Ex10Result register_ramp_callbacks(
    void (*pre_cb)(struct Ex10Result*),
    void (*post_cb)(struct Ex10Result*))
{
    // Both pre and post callbacks must be empty
    if (module_variables.pre_ramp_callback != NULL ||
        module_variables.post_ramp_callback != NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleModuleManager,
                                   Ex10SdkErrorInvalidState);
    }
    module_variables.pre_ramp_callback  = pre_cb;
    module_variables.post_ramp_callback = post_cb;
    return make_ex10_success();
}

static void unregister_ramp_callbacks(void)
{
    module_variables.pre_ramp_callback  = NULL;
    module_variables.post_ramp_callback = NULL;
}

static struct Ex10RampModuleManager const ex10_module_manager = {
    .store_adc_temperature            = store_adc_temperature,
    .store_pre_ramp_variables         = store_pre_ramp_variables,
    .store_post_ramp_variables        = store_post_ramp_variables,
    .retrieve_adc_temperature         = retrieve_adc_temperature,
    .retrieve_post_ramp_frequency_khz = retrieve_post_ramp_frequency_khz,
    .retrieve_post_ramp_tx_power_cdbm = retrieve_post_ramp_tx_power_cdbm,
    .retrieve_pre_ramp_antenna        = retrieve_pre_ramp_antenna,
    .call_pre_ramp_callback           = call_pre_ramp_callback,
    .call_post_ramp_callback          = call_post_ramp_callback,
    .register_ramp_callbacks          = register_ramp_callbacks,
    .unregister_ramp_callbacks        = unregister_ramp_callbacks,
};

struct Ex10RampModuleManager const* get_ex10_ramp_module_manager(void)
{
    return &ex10_module_manager;
}
