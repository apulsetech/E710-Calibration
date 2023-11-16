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
 * @file simple_ramping_example.c
 * @details  This example shows how to use the Ex10Reader to ramp the Impinj
 * reader chip up to power.
 */

#include "board/time_helpers.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/rf_mode_definitions.h"


/* Settings used when running this example */
static const uint8_t     antenna             = 1;
static const uint16_t    rf_mode             = mode_148;
static const uint16_t    transmit_power_cdbm = 3000;
static enum Ex10RegionId region_id           = REGION_FCC;
static const bool        verbose             = true;

static int simple_ramp_example(void)
{
    // Sanity check for lower power output
    uint16_t                  adc      = 0u;
    struct Ex10Ops const*     ops      = get_ex10_ops();
    struct Ex10RfPower const* rf_power = get_ex10_rf_power();
    struct Ex10Reader const*  reader   = get_ex10_reader();

    struct Ex10Result ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    ex10_result =
        rf_power->measure_and_read_aux_adc(AdcResultPowerLoSum, 1u, &adc);
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    if (adc > 200)
    {
        ex10_ex_eprintf("Is power already ramped?\n");
        return -1;
    }

    // Set a low EventFifo threshold, just for ramping events.
    ex10_result = get_ex10_protocol()->set_event_fifo_threshold(0u);
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    // Use high-level reader interface to transmit CW
    uint32_t const frequency_khz = 0u;     // Use frequency from hopping table
    uint32_t const remain_on     = false;  // Use regulatory times
    ex10_result                  = ops->wait_op_completion();
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    ex10_result = reader->cw_test(
        antenna, rf_mode, transmit_power_cdbm, frequency_khz, remain_on);
    // Checking the error code more explicitly since CW test aggregates multiple
    // ops together
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }
    // Sanity check for a relatively high power output, indicating that the
    // ex10 device is transmitting.
    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    ex10_result =
        rf_power->measure_and_read_aux_adc(AdcResultPowerLoSum, 1u, &adc);
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    if (adc < 500)
    {
        ex10_ex_eprintf("Power did not ramp up (%d)\n", adc);
        return -1;
    }

    // Setup a delay to wait for the ramp down
    struct Ex10Region const* region =
        get_ex10_regulatory()->get_region(region_id);
    ex10_result =
        ops->start_timer_op(region->regulatory_timers.nominal_ms * 1000);
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    // Wait out the timer
    ex10_result = ops->wait_timer_op();
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        ex10_discard_packets(true, true, true);
        return -1;
    }

    // Wait for TxRampDown packet
    uint32_t const start_time = get_ex10_time_helpers()->time_now();
    while (true)
    {
        // Fail if TxRampDown not seen after 5 seconds
        if (get_ex10_time_helpers()->time_elapsed(start_time) > 5000)
        {
            ex10_ex_eprintf("TxRampDown not seen after 5 seconds\n");
            return -1;
        }

        struct EventFifoPacket const* packet = reader->packet_peek();
        if (packet)
        {
            if (verbose)
            {
                get_ex10_event_fifo_printer()->print_packets(packet);
            }
            bool const ramp_down = (packet->packet_type == TxRampDown);
            reader->packet_remove();

            if (ramp_down)
            {
                break;
            }
        }
    }
    return 0;
}

int main(void)
{
    ex10_ex_printf("Starting ramp test\n");

    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    int result = simple_ramp_example();

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending ramp test\n");
    return result;
}
