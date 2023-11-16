/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2020 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct Ex10GpioInterface
 * This interface is a pass-through to the Ex10GpioDriver interface.
 * @see  struct Ex10GpioDriver for a description of these functions.
 * @note Not all of the Ex10GpioDriver functions are exposed by the
 *       Ex10GpioInterface interface.
 */
struct Ex10GpioInterface
{
    int32_t (*initialize)(bool board_power_on, bool ex10_enable, bool reset);
    void (*cleanup)(void);

    int32_t (*set_board_power)(bool power_on);
    bool (*get_board_power)(void);

    int32_t (*set_ex10_enable)(bool enable);
    bool (*get_ex10_enable)(void);

    int32_t (*register_irq_callback)(void (*cb_func)(void));
    int32_t (*deregister_irq_callback)(void);

    void (*irq_monitor_callback_enable)(bool enable);
    bool (*irq_monitor_callback_is_enabled)(void);

    void (*irq_enable)(bool);
    bool (*thread_is_irq_monitor)(void);

    int32_t (*assert_reset_n)(void);
    int32_t (*deassert_reset_n)(void);
    int32_t (*reset_device)(void);

    int32_t (*assert_ready_n)(void);
    int32_t (*release_ready_n)(void);

    int32_t (*busy_wait_ready_n)(uint32_t ready_n_timeout_ms);
    int32_t (*ready_n_pin_get)(void);
};

#ifdef __cplusplus
}
#endif
