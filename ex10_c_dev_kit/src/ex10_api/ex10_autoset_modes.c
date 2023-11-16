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

#include "ex10_api/ex10_autoset_modes.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_regulatory.h"

// EU1 == ETSI_LOWER
// EU2 == ETSI_UPPER

// clang-format off
// FCC:
// E710, E910
static enum RfModes const autoset_1120_rf_modes[] = {mode_103, mode_148, mode_146, mode_185};
// E510
static enum RfModes const autoset_1122_rf_modes[] = {mode_120, mode_148, mode_146, mode_185};
// E310
static enum RfModes const autoset_1123_rf_modes[] = {mode_125, mode_141, mode_146, mode_185};

// ETSI_LOWER (EU1):
// E710, E910
static enum RfModes const autoset_1220_rf_modes[] = {mode_225, mode_223, mode_241, mode_285};

// ETSI_UPPER (EU2):
// E710, E910
static enum RfModes const autoset_1320_rf_modes[] = {mode_302, mode_345, mode_343, mode_382};
// E510
static enum RfModes const autoset_1322_rf_modes[] = {mode_323, mode_345, mode_343, mode_382};
// E310
static enum RfModes const autoset_1323_rf_modes[] = {mode_325, mode_342, mode_343, mode_382};

// All other regions:
// E710, E910
static enum RfModes const autoset_1420_rf_modes[] = {mode_203, mode_223, mode_241, mode_285};
// clang-format on

static struct AutosetRfModes const autoset_modes_table[] = {
    {
        .autoset_mode_id = AutosetMode_1120,
        .rf_mode_list    = autoset_1120_rf_modes,
        .rf_modes_length = ARRAY_SIZE(autoset_1120_rf_modes),
    },
    {
        .autoset_mode_id = AutosetMode_1122,
        .rf_mode_list    = autoset_1122_rf_modes,
        .rf_modes_length = ARRAY_SIZE(autoset_1122_rf_modes),
    },
    {
        .autoset_mode_id = AutosetMode_1123,
        .rf_mode_list    = autoset_1123_rf_modes,
        .rf_modes_length = ARRAY_SIZE(autoset_1123_rf_modes),
    },
    {
        .autoset_mode_id = AutosetMode_1220,
        .rf_mode_list    = autoset_1220_rf_modes,
        .rf_modes_length = ARRAY_SIZE(autoset_1220_rf_modes),
    },
    {
        .autoset_mode_id = AutosetMode_1320,
        .rf_mode_list    = autoset_1320_rf_modes,
        .rf_modes_length = ARRAY_SIZE(autoset_1320_rf_modes),
    },
    {
        .autoset_mode_id = AutosetMode_1322,
        .rf_mode_list    = autoset_1322_rf_modes,
        .rf_modes_length = ARRAY_SIZE(autoset_1322_rf_modes),
    },
    {
        .autoset_mode_id = AutosetMode_1323,
        .rf_mode_list    = autoset_1323_rf_modes,
        .rf_modes_length = ARRAY_SIZE(autoset_1323_rf_modes),
    },
    {
        .autoset_mode_id = AutosetMode_1420,
        .rf_mode_list    = autoset_1420_rf_modes,
        .rf_modes_length = ARRAY_SIZE(autoset_1420_rf_modes),
    },
};

static struct AutosetRfModes const* get_autoset_rf_modes(
    enum AutosetModeId autoset_mode_id)
{
    for (size_t iter = 0u; iter < ARRAY_SIZE(autoset_modes_table); ++iter)
    {
        if (autoset_modes_table[iter].autoset_mode_id == autoset_mode_id)
        {
            return &autoset_modes_table[iter];
        }
    }

    return NULL;  // AutosetModeId match not found.
}

static bool is_valid_sku(enum ProductSku sku)
{
    return (sku == SkuE910) || (sku == SkuE710) || (sku == SkuE510) ||
           (sku == SkuE310);
}

static bool is_valid_region(enum Ex10RegionId region_id)
{
    return (get_ex10_regulatory()->get_region(region_id) !=
            get_ex10_regulatory()->get_region(REGION_NOT_DEFINED));
}

// clang-format off
#define AUTOSET_SKU_SIZE    (4u)
#define AUTOSET_REGION_SIZE (4u)

static enum AutosetModeId autoset_mode_id_table[AUTOSET_SKU_SIZE][AUTOSET_REGION_SIZE] = {
//  FCC                 EU1                 EU2                 OTHER
    {AutosetMode_1120,  AutosetMode_1220,   AutosetMode_1320,   AutosetMode_1420},  // SKU E910
    {AutosetMode_1120,  AutosetMode_1220,   AutosetMode_1320,   AutosetMode_1420},  // SKU E710
    {AutosetMode_1122,  AutosetMode_1220,   AutosetMode_1322,   AutosetMode_1420},  // SKU E510
    {AutosetMode_1123,  AutosetMode_1220,   AutosetMode_1323,   AutosetMode_1420},  // SKU E310
};

static enum AutosetModeId get_autoset_mode_id(enum Ex10RegionId region_id,
                                              enum ProductSku   sku)
{
    if ((is_valid_region(region_id) == false) || (is_valid_sku(sku) == false))
    {
        return AutosetMode_Invalid;
    }
    size_t const sku_index    = (sku == SkuE910) ? 0u :
                                (sku == SkuE710) ? 1u :
                                (sku == SkuE510) ? 2u : 3u;
    size_t const region_index = (region_id == REGION_FCC)        ? 0u :
                                (region_id == REGION_ETSI_LOWER) ? 1u :
                                (region_id == REGION_ETSI_UPPER) ? 2u : 3u;
    return autoset_mode_id_table[sku_index][region_index];
}
// clang-format on

static struct Ex10Result init_autoset_basic_inventory_sequence(
    struct InventoryRoundConfigBasic* inventory_round_config,
    size_t                            inventory_round_config_size,
    struct AutosetRfModes const*      autoset_rf_modes,
    uint8_t                           antenna,
    int16_t                           tx_power_cdbm,
    uint8_t                           target,
    enum InventoryRoundControlSession session)
{
    if (inventory_round_config == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleAutosetModes,
                                   Ex10SdkErrorNullPointer);
    }

    if (inventory_round_config_size < autoset_rf_modes->rf_modes_length)
    {
        return make_ex10_sdk_error(Ex10ModuleAutosetModes,
                                   Ex10SdkErrorBadParamValue);
    }

    struct InventoryRoundControlFields const inventory_config_template = {
        .initial_q            = 8u,
        .max_q                = 15u,
        .min_q                = 0u,
        .num_min_q_cycles     = 2u,
        .fixed_q_mode         = false,
        .q_increase_use_query = false,
        .q_decrease_use_query = false,
        .session              = session,
        .select               = SelectAll,  // Used in Query command Sel
        .target               = target,
        .halt_on_all_tags     = false,
        .fast_id_enable       = false,
        .tag_focus_enable     = false,
        .auto_access          = false,
        .abort_on_fail        = false,
        .halt_on_fail         = false,
        .always_ack           = false,
        .rfu                  = false,
    };

    struct InventoryRoundControl_2Fields const inventory_config_2_template = {
        .max_queries_since_valid_epc                = 16u,
        .Reserved0                                  = 0u,
        .starting_min_q_count                       = 0u,
        .starting_max_queries_since_valid_epc_count = 0u,
    };

    for (size_t iter = 0u; iter < autoset_rf_modes->rf_modes_length; ++iter)
    {
        struct InventoryRoundConfigBasic* basic_config =
            &inventory_round_config[iter];
        enum RfModes const rf_mode = autoset_rf_modes->rf_mode_list[iter];

        basic_config->antenna       = antenna;
        basic_config->rf_mode       = rf_mode;
        basic_config->tx_power_cdbm = tx_power_cdbm;

        basic_config->inventory_config   = inventory_config_template;
        basic_config->inventory_config_2 = inventory_config_2_template;

        basic_config->send_selects = false;
    }

    return make_ex10_success();
}

struct Ex10AutosetModes const* get_ex10_autoset_modes(void)
{
    static struct Ex10AutosetModes autoset_modes_instance = {
        .get_autoset_rf_modes = get_autoset_rf_modes,
        .get_autoset_mode_id  = get_autoset_mode_id,
        .init_autoset_basic_inventory_sequence =
            init_autoset_basic_inventory_sequence,
    };

    return &autoset_modes_instance;
}