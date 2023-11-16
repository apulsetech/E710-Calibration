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

#include <stddef.h>
#include <stdint.h>

#include "ex10_api/application_register_definitions.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/rf_mode_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Ex10EventFifoPrinter
{
    /**
     * Print EventFifo packets based on their type to stdout.
     */
    void (*print_packets)(struct EventFifoPacket const* packet);

    /**
     * Prints the TagReadData FIFO contents after compensating the RSSI
     * using the calibration layer and the passed in parameters.
     */
    void (*print_event_tag_read_compensated_rssi)(
        struct EventFifoPacket const* packet,
        enum RfModes                  rf_mode,
        uint8_t                       antenna,
        enum RfFilter                 rf_filter,
        uint16_t                      adc_temperature);

    /**
     * Print the TagReadData struct contents to stdout.
     */
    void (*print_tag_read_data)(struct TagReadData const* tag_read_data);
};

const struct Ex10EventFifoPrinter* get_ex10_event_fifo_printer(void);

#ifdef __cplusplus
}
#endif
