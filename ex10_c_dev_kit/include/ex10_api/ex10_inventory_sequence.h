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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ex10_api/application_register_definitions.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/rf_mode_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct InventoryRoundConfigBasic
 * Define the parameters to be used when performing an inventory round using
 * the Ex10Reader.run_inventory_sequence() interface.
 *
 * An inventory round will sequentially call the StartInventoryRoundOp across
 * regulatory transmitter ramp up/down cycles until the Q algorithm completes.
 * Thereby inventorying all tags within the field of view.
 *
 * This struct is used when sequencing through Autoset RF modes and is contained
 * within the struct InventoryRoundSequence.
 */
struct InventoryRoundConfigBasic
{
    /// The antenna on which to run the inventory round.
    uint8_t antenna;

    /// The RF mode to be used during the inventory round.
    enum RfModes rf_mode;

    /// The transmitter power, in 0.01 dBm, units
    /// to be used during the inventory round.
    int16_t tx_power_cdbm;

    /// @see struct InventoryRoundControlFields
    struct InventoryRoundControlFields inventory_config;

    /// @see struct InventoryRoundControl_2Fields
    struct InventoryRoundControl_2Fields inventory_config_2;

    /// This field will set the Gen2SelectEnable register for this inventory
    /// round. This requires that the Gen2TxBuffer, Gen2Offsets and Gen2Lengths
    /// registers, are set correctly prior to running this inventory round.
    bool send_selects;
};

/**
 * @enum InventoryRoundConfigType
 * Used to identify that the struct InventoryRoundSequence contains nodes of
 * struct InventoryRoundConfigBasic.
 */
enum InventoryRoundConfigType
{
    /// The structure of the InventoryRoundSequence.configs field is unknown.
    INVENTORY_ROUND_CONFIG_UNKNOWN = 0,

    /// Identify the InventoryRoundSequence.configs as containing an array
    /// of type struct InventoryRoundConfigBasic.
    INVENTORY_ROUND_CONFIG_BASIC = 1,
};

/**
 * @struct InventoryRoundSequence
 * A struct which contains an array of inventory round configurations to
 * execute a sequence across.
 */
struct InventoryRoundSequence
{
    /// An identifier that indicates the type of inventory sequence is being
    /// contained within this structure.
    enum InventoryRoundConfigType type_id;
    /// A pointer to the first node of the InventoryRoundConfigBasic objects.
    /// This type is void* to allow for future extensibility. The type of this
    /// pointer is determined with the type_id member.
    void const* configs;
    /// The number of InventoryRoundConfig nodes within the array.
    size_t count;
};

#ifdef __cplusplus
}
#endif
