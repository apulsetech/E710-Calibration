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

#include <sys/types.h>

#include "ex10_api/application_register_definitions.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/rf_mode_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct Ex10RampModuleManager
 * This is the pre and post ramp callback module manager.  It provides
 * an interface where the modules can register themselves for callback,
 * and the use cases can call a standard function that will then determine
 * if any callback is registered.
 *
 * This module also is a place where state variables can be stored for
 * the callbacks to use.  The use cases are responsible for maintaining
 * these variables to be accurate when the callbacks are called.  This allows
 * the callback modules and the use cases to be decoupled from each other.
 */
struct Ex10RampModuleManager
{
    /**
     * These functions are used to capture the values needed for the pre
     * and post ramp callbacks.  It is the responsibility of the use case/reader
     * layers to set these before the callbacks are called.
     *
     * These are the current settings needed by the supported modules
     * and the list of parameters may grow in the future.  If so, these
     * functions should grow to add those new parameters.  This is because
     * the use cases that are setting these variables don't know what modules
     * are enabled, so they should store all of the parameter.  And if a new
     * parameter is added for a new module, it will change the function
     * signature and cause the use cases to fail to compile until they add the
     * new parameter.
     */
    void (*store_pre_ramp_variables)(uint8_t antenna);
    void (*store_post_ramp_variables)(int16_t  tx_power_cdbm,
                                      uint32_t frequency_khz);
    void (*store_adc_temperature)(uint16_t adc_temperature);

    /**
     * These retrieve functions are here so that the callbacks can
     * grab the parameters that they need to do their functions
     * all of these values are set by calling the store functions above
     * and the use cases are responsible for making sure that the
     * correct values are stored.
     */

    uint16_t (*retrieve_adc_temperature)(void);
    uint8_t (*retrieve_pre_ramp_antenna)(void);
    uint32_t (*retrieve_post_ramp_frequency_khz)(void);
    int16_t (*retrieve_post_ramp_tx_power_cdbm)(void);

    /**
     * Use cases are to call this function immediately before calling cw_on
     */
    struct Ex10Result (*call_pre_ramp_callback)(void);
    /**
     * Use cases are to call this function immediately after calling cw_on
     */
    struct Ex10Result (*call_post_ramp_callback)(void);

    /**
     * Registers the function pointer the rf power module to call before
     * and after ramping
     *
     * @param pre_cb The function pointer to register. The function takes
     * a pointer to Ex10Result as a parameter.  In the case of an error,
     * any error handling should be done by the callback context since it
     * knows what is appropriate to do for the error. If the Ex10Result.error
     * is set to true, it will cause the cw_on to abort the rampup and return
     * Ex10Result.
     * @param post_cb The function pointer to register. The function takes
     * a pointer to Ex10Result as a parameter.  In the case of an error,
     * any error handling should be done by the callback context since it
     * knows what is appropriate to do for the error. If the Ex10Result.error
     * is set to true, it will cause the cw_on to abort the rampup and return
     * Ex10Result.
     *
     * @return Returns Ex10Result with success if successful or an SDK error
     *         if there are already a callbacks installed.
     */
    struct Ex10Result (*register_ramp_callbacks)(
        void (*pre_cb)(struct Ex10Result*),
        void (*post_cb)(struct Ex10Result*));
    /**
     * Removes the callback and sets it back to NULL
     */
    void (*unregister_ramp_callbacks)(void);
};

struct Ex10RampModuleManager const* get_ex10_ramp_module_manager(void);

#ifdef __cplusplus
}
#endif
