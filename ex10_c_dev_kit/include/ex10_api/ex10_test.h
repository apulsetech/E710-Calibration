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

#include "ex10_api/application_register_definitions.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/rf_mode_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Ex10Test
{
    /**
     * Transmit a continuous wave until regulatory timers expire.
     *
     * @see inventory() for parameter descriptions.
     *
     * @return Info about any encountered errors.
     */
    struct Ex10Result (*cw_test)(
        uint8_t                                    antenna,
        enum RfModes                               rf_mode,
        int16_t                                    tx_power_cdbm,
        uint32_t                                   frequency_khz,
        struct PowerDroopCompensationFields const* droop_comp,
        uint16_t                                   temperature_adc,
        bool                                       temp_comp_enabled);

    /**
     * Transmit a pseudo-random bit stream until regulatory timers expire.
     * User must call stop_transmitting() to end this operation.
     *
     * @see inventory() for parameter descriptions.
     * @return Info about any encountered errors.
     */
    struct Ex10Result (*prbs_test)(uint8_t      antenna,
                                   enum RfModes rf_mode,
                                   int16_t      tx_power_cdbm,
                                   uint32_t     frequency_khz,
                                   uint16_t     temperature_adc,
                                   bool         temp_comp_enabled);

    /**
     * Run a packetized BER test.
     *
     * @param num_bits       Number of bits per packet
     * @param num_packets    Number of packets to receive
     * @param delimiter_only Use a delimiter instead of a query to trigger a
     * packet.
     * @see inventory() for other parameter descriptions.
     * @return Info about any encountered errors.
     */
    struct Ex10Result (*ber_test)(uint8_t      antenna,
                                  enum RfModes rf_mode,
                                  int16_t      tx_power_cdbm,
                                  uint32_t     frequency_khz,
                                  uint16_t     num_bits,
                                  uint16_t     num_packets,
                                  bool         delimiter_only,
                                  uint16_t     temperature_adc,
                                  bool         temp_comp_enabled);

    /**
     * Start up a continuous cycle of ramp up, inventory, ramp down.
     * User must call stop_transmitting() to end this operation.
     *
     * @see inventory() for parameter descriptions.
     * @return Info about any encountered errors.
     */
    struct Ex10Result (*etsi_burst_test)(
        struct InventoryRoundControlFields const*   inventory_config,
        struct InventoryRoundControl_2Fields const* inventory_config_2,
        uint8_t                                     antenna,
        enum RfModes                                rf_mode,
        int16_t                                     tx_power_cdbm,
        uint16_t                                    on_time_ms,
        uint16_t                                    off_time_ms,
        uint32_t                                    frequency_khz,
        uint16_t                                    temperature_adc,
        bool                                        temp_comp_enabled);
};

const struct Ex10Test* get_ex10_test(void);

#ifdef __cplusplus
}
#endif
