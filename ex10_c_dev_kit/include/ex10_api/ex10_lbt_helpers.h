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

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Ex10LbtHelpers
{
    /**
     * Runs continuous inventory using LBT before each ramp.
     * LBT is inserted before each ramp using the pre-ramp callback
     *
     * @param cihp            Helper parameters to assist in running
     *                        continuous inventory.
     *
     * @return return Any potential errors in running.
     */
    enum InventoryHelperReturns (*continuous_inventory_lbt)(
        struct ContInventoryHelperParams* cihp);
};

const struct Ex10LbtHelpers* get_ex10_lbt_helpers(void);

#ifdef __cplusplus
}
#endif
