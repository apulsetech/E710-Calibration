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

/**
 * @file prbs_example.c
 * @details This example shows how to setup the Impinj reader chip to ramp up
 *  and transmit a PRBS pattern for testing purposes.
 */

#include "board/time_helpers.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/rf_mode_definitions.h"


/* Settings used when running this example */
static const uint32_t     prbs_time_on_ms = 8 * 1000;  // Duration in millisec
static const uint8_t      antenna         = 1;
static const enum RfModes rf_mode         = mode_148;
static const uint16_t     transmit_power_cdbm = 3000;

static int prbs_example(void)
{
    bool                     transmitting = false;
    uint32_t                 start_time   = get_ex10_time_helpers()->time_now();
    struct Ex10Reader const* reader       = get_ex10_reader();
    while (get_ex10_time_helpers()->time_elapsed(start_time) < prbs_time_on_ms)
    {
        if (!transmitting)
        {
            uint32_t const frequency_khz = 0u;  // frequency from hopping table
            uint32_t const remain_on     = false;  // Use regulatory times
            struct Ex10Result ex10_result =
                reader->prbs_test(antenna,
                                  rf_mode,
                                  transmit_power_cdbm,
                                  frequency_khz,
                                  remain_on);

            if (ex10_result.error)
            {
                ex10_discard_packets(true, true, true);
                return -1;
            }
            transmitting = true;
        }

        struct EventFifoPacket const* packet = reader->packet_peek();
        if (packet)
        {
            if (packet->packet_type == TxRampDown)
            {
                transmitting = false;
            }
            reader->packet_remove();
        }
    }

    reader->stop_transmitting();
    return 0;
}

int main(void)
{
    ex10_ex_printf("Starting PRBS test\n");

    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    int result = prbs_example();

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending PRBS test\n");
    return result;
}
