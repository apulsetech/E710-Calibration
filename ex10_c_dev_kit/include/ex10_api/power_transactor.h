/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2022 Impinj, Inc. All rights reserved.                      *
 *                                                                           *
 *****************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct Ex10PowerTransactor
{
    void (*init)(void);
    void (*deinit)(void);

    /**
     * Power on the Impinj Reader Chip into the Application.
     * If the application programmed into the Impinj Reader Chip is not
     * valid then the reader will be executing the within the bootloader.
     *
     * @return int Indicates whether the call was successful.
     * @retval < 0 Indicates a device IO or operation system error.
     * @retval   1 Indicates that the Impinj Reader Chip is executing in
     *             the bootloader context.
     * @retval   2 Indicates that the Impinj Reader Chip is executing in
     *             the application context (success).
     */
    int (*power_up_to_application)(void);

    /** Power on the Impinj Reader Chip into the Bootloader. */
    void (*power_up_to_bootloader)(void);

    /**
     * Remove power from the Impinj Reader Chip, keeping software interfaces
     * intact and ready for use.
     */
    void (*power_down)(void);
};

struct Ex10PowerTransactor const* get_ex10_power_transactor(void);

#ifdef __cplusplus
}
#endif
