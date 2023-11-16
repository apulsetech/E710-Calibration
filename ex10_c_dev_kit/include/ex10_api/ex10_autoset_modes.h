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

#include "ex10_api/application_registers.h"
#include "ex10_api/ex10_inventory_sequence.h"
#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/rf_mode_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

/// The number of RF modes contained within an Autoset mode.
#define AUTOSET_RF_MODE_COUNT ((size_t)4u)

/**
 * @enum AutosetModeId
 * Enumeration of Autoset modes supported by the SDK.
 */
enum AutosetModeId
{
    AutosetMode_Invalid = 0,  ///< Indicates the Autoset mode is invalid.

    AutosetMode_1120 = 1120,  ///< Region: FCC, SKU: E910, E710
    AutosetMode_1122 = 1122,  ///< Region: FCC, SKU: E510
    AutosetMode_1123 = 1123,  ///< Region: FCC, SKU: E310

    AutosetMode_1220 = 1220,  ///< Region: EU1 (ETSI_LOWER),
                              ///< SKU: E910, E710, E510, E310

    AutosetMode_1320 = 1320,  ///< Region: EU2 (ETSI_UPPER), SKU: E910, E710
    AutosetMode_1322 = 1322,  ///< Region: EU2 (ETSI_UPPER), SKU: E510
    AutosetMode_1323 = 1323,  ///< Region: EU2 (ETSI_UPPER), SKU: E310

    AutosetMode_1420 = 1420,  ///< Region: All others,
                              ///< SKU: E910, E710, E510, E310
};

/**
 * @struct AutosetRfModes
 * A container of RF modes aggregated by an Autoset mode.
 */
struct AutosetRfModes
{
    /// The Autoset mode ID.
    enum AutosetModeId autoset_mode_id;
    /// The RF modes contained as an array.
    enum RfModes const* rf_mode_list;
    /// The number of RF modes within the array.
    size_t rf_modes_length;
};

/**
 * @struct Ex10AutosetModes
 * Each Autoset mode accessor references an array of RF modes containing
 * AUTOSET_RF_MODE_COUNT elements.
 *
 * These RF modes, from lowest to highest index, within the array, provide
 * better sensitivity with slower tag access.
 *
 * The Autoset modes are specific to a given region.
 */
struct Ex10AutosetModes
{
    /**
     * Get a pointer to a struct of AutosetRfModes based on the Autoset mode
     * passed in and the region name.
     *
     * @param autoset_mode_id The Autoset mode.
     *
     * @return struct AutosetRfModes const* The RF modes associated with an
     *         Autoset mode. NULL if the function call fails.
     * @note   Invalid region names will return the "Other regions" RF modes.
     */
    struct AutosetRfModes const* (*get_autoset_rf_modes)(
        enum AutosetModeId autoset_mode_id);

    /**
     * Based on the region and SKU, get the associated Autoset mode ID.
     *
     * @param region_id Specifies the region for RF mode optimization.
     * @param sku       Specifies the SKU on which the Autoset RF modes will
     *                  be used.
     *
     * @return enum AutosetModeId The Autoset mode that will perform optimally
     *                            on a specific SKU for a specific region.
     */
    enum AutosetModeId (*get_autoset_mode_id)(enum Ex10RegionId region_id,
                                              enum ProductSku   sku);

    /**
     * Initialize the array of struct InventoryRoundConfigBasic nodes.
     *
     * @param [out] inventory_round_config
     *   An array of struct InventoryRoundConfigBasic nodes to initialize with
     *   Autoset RF modes and related inventory parameters.
     * @param inventory_round_config_size
     *   The number of nodes in the inventory_round_config array.
     *   Used to bounds check the inventory_round_config array size against
     *   the autoset_rf_modes array size.
     * @param autoset_rf_modes
     *   A const pointer struct AutosetRfModes, which contains an array of
     *   RF modes for running inventory rounds.
     * @param antenna       The antenna port to use during the inventory round.
     * @param tx_power_cdbm The transmit power in cdBm to use during the round.
     * @param target        The inventory flag to target during the inventory
     *                      round; i.e. target_A or target_B.
     * @param session       The inventory session for tags to participate in.
     *
     * @return struct Ex10Result
     *         Indicates whether the function was successful or not.
     */
    struct Ex10Result (*init_autoset_basic_inventory_sequence)(
        struct InventoryRoundConfigBasic* inventory_round_config,
        size_t                            inventory_round_config_size,
        struct AutosetRfModes const*      autoset_rf_modes,
        uint8_t                           antenna,
        int16_t                           tx_power_cdbm,
        uint8_t                           target,
        enum InventoryRoundControlSession session);
};

struct Ex10AutosetModes const* get_ex10_autoset_modes(void);

#ifdef __cplusplus
}
#endif
