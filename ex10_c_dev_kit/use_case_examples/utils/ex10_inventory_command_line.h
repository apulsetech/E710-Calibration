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

#include <assert.h>

#include "ex10_api/ex10_result.h"
#include "ex10_command_line.h"

#include "ex10_api/ex10_autoset_modes.h"

#ifdef __cplusplus
extern "C" {
#endif

enum Verbosity
{
    SILENCE             = 0,
    PRINT_SCOPED_EVENTS = 1,
    PRINT_EVERYTHING    = 2,
};

union InventoryMode {
    enum RfModes       rf_mode_id;
    enum AutosetModeId autoset_mode_id;
    uint32_t           raw;
};

// If these static assert()s fail,
// then adjust the InventoryMode.raw member type accordingly.
static_assert(sizeof(enum RfModes) == sizeof(uint32_t), "");
static_assert(sizeof(enum AutosetModeId) == sizeof(uint32_t), "");

struct InventoryOptions
{
    char const*                       region_name;
    uint32_t                          read_rate;
    uint8_t                           antenna;
    uint32_t                          frequency_khz;
    bool                              remain_on;
    int16_t                           tx_power_cdbm;
    union InventoryMode               mode;
    char                              target_spec;
    uint8_t                           initial_q;
    enum InventoryRoundControlSession session;
};

/**
 * Parse the command line arguments specific to running inventory.
 *
 * @param [out] inventory_params
 *              The parsing operation will fill in the members this struct.
 * @param argv  The char*[]   list of arguments passed in to main().
 * @param argc  The number of char*[] arguments passed in to main().
 *
 * @return struct Ex10Result
 *  Indicates whether there was a problem parsing the command line.
 */
struct Ex10Result ex10_inventory_parse_command_line(
    struct InventoryOptions* inventory_params,
    char const* const*       argv,
    int                      argc);

void ex10_eprint_inventory_command_line_usage(void);

void ex10_print_inventory_command_line_settings(
    struct InventoryOptions const* inventory_params);

struct Ex10Result ex10_check_inventory_command_line_settings(
    struct InventoryOptions const* inventory_params);

bool           ex10_command_line_help_requested(void);
enum Verbosity ex10_command_line_verbosity(void);

#ifdef __cplusplus
}
#endif
