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

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ex10_api/application_registers.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_lbt_helpers.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/ex10_utils.h"

#include "ex10_modules/ex10_antenna_disconnect.h"
#include "ex10_modules/ex10_listen_before_talk.h"

static enum InventoryHelperReturns continuous_inventory_lbt(
    struct ContInventoryHelperParams* cihp)
{
    struct Ex10Reader const*  reader  = get_ex10_reader();
    struct Ex10Helpers const* helpers = get_ex10_helpers();

    struct InventoryHelperParams* ihp = cihp->inventory_params;

    if ((cihp->stop_conditions->max_number_of_rounds == 0) &&
        (cihp->stop_conditions->max_number_of_tags == 0) &&
        (cihp->stop_conditions->max_duration_us == 0))
    {
        return InvHelperStopConditions;
    }

    // Clear the tag info so all data is from the next round
    helpers->clear_info_from_packets(ihp->packet_info);
    helpers->discard_packets(false, true, false);

    // Example of how to overwrite default LBT parameters
    // max_rssi_measurements = 5000 is for certification testing,
    // functionally equivalent to 10 second timeout
    get_ex10_listen_before_talk()->set_max_rssi_measurements(5000);

    // Overwrite any existing pre-ramp functionality with the LBT function
    // NOTE: if the pre-ramp fails LBT, it will cancel the continuous inventory
    get_ex10_antenna_disconnect()->deinit();
    get_ex10_listen_before_talk()->init();

    // Starting continuous inventory
    struct Ex10Result const ex10_result =
        reader->continuous_inventory(ihp->antenna,
                                     ihp->rf_mode,
                                     ihp->tx_power_cdbm,
                                     ihp->inventory_config,
                                     ihp->inventory_config_2,
                                     ihp->send_selects,
                                     cihp->stop_conditions,
                                     ihp->dual_target,
                                     false);
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return InvHelperOpStatusError;
    }

    return InvHelperSuccess;
}

static const struct Ex10LbtHelpers ex10_lbt_helpers = {
    .continuous_inventory_lbt = continuous_inventory_lbt,
};

const struct Ex10LbtHelpers* get_ex10_lbt_helpers(void)
{
    return &ex10_lbt_helpers;
}
