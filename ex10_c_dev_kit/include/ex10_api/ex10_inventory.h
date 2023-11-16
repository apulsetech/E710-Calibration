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

#include "ex10_api/ex10_continuous_inventory_common.h"
#include "ex10_api/ex10_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct InventoryParams
 */
struct InventoryParams
{
    uint8_t                              antenna;
    enum RfModes                         rf_mode;
    int16_t                              tx_power_cdbm;
    struct InventoryRoundControlFields   inventory_config;
    struct InventoryRoundControl_2Fields inventory_config_2;
    bool                                 send_selects;
};

struct Ex10Inventory
{
    /**
     * Sends all enabled selects if directed by the user and starts an
     * inventory round.
     *
     * This function runs the SendSelectOp, if send_selects is true,
     * followed by the StartInventoryRoundOp.
     *
     * @param inventory_config      @see struct InventoryRoundControlFields
     * @param inventory_config_2    @see struct InventoryRoundControl_2Fields
     * @param run_select_op         Whether or not to run the send select op
                                    and send any enabled select commands in the
                                    gen2 command buffer.
     * @return Info about any encountered errors.
     */
    struct Ex10Result (*run_inventory)(
        struct InventoryRoundControlFields const*   inventory_config,
        struct InventoryRoundControl_2Fields const* inventory_config_2,
        bool                                        send_selects);

    /**
     * Starts an Inventory by running the StartInventoryRoundOp.
     * This function calls Ex10Inventory.run_inventory() after performing
     * the preconditions:
     * - Ramp up the transmitter, if necessary.
     * - Update the temperature measurement.
     * - Set the RF mode.
     *
     * @param antenna               The antenna to use.
     * @param rf_mode               The RF mode to use.
     * @param tx_power_cdbm         The transmitter power, in centi-dB.
     * @param inventory_config      @see struct InventoryRoundControlFields
     * @param inventory_config_2    @see struct InventoryRoundControl_2Fields
     * @param send_selects          When set to true, select commands, which are
     *        programmed into the Gen2 command buffer, and enabled,
     *        are sent each time the StartInventoryRoundOp is run.
     */
    struct Ex10Result (*start_inventory)(
        uint8_t                                     antenna,
        enum RfModes                                rf_mode,
        int16_t                                     tx_power_cdbm,
        struct InventoryRoundControlFields const*   inventory_config,
        struct InventoryRoundControl_2Fields const* inventory_config_2,
        bool                                        send_selects);

    /**
     * Checks to see if the LMAC is currently in the halted state.
     * (the LMAC could have ramped down due to regulatory for example)
     *
     * @return True if the LMAC is halted, False if not
     */
    bool (*inventory_halted)(void);

    /**
     * Checks for errors within ex10_result and attempts to map them to
     * continuous inventory errors. If the error does not correlate to a known
     * continuous inventory error, the continuous inventory error will be
     * SRReasonUnknown.
     *
     * @return The continuous inventory error mapped from the passed Ex10Result.
     */
    enum StopReason (*ex10_result_to_continuous_inventory_error)(
        struct Ex10Result ex10_result);
};

const struct Ex10Inventory* get_ex10_inventory(void);

#ifdef __cplusplus
}
#endif
