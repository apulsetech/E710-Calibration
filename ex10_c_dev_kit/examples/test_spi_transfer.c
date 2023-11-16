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
 * @file test_spi_transfer.c
 * @details Execute repeated varied length data transfers via the ex10 host
 * interface using the TransferTest command.
 *
 * The number of iterations is a single argument on the command line.
 * Use zero to run forever.
 *
 * @note
 * One limitation is that the 'time_helper' routines assert when the call to
 * clock() rolls over. When this occurs, you'll see:
 *   ...
 *   Test transfer 17064366
 *   Test transfer 17064367
 *   exercise_transfers.bin: time_helpers.c:20: time_start:
 *   Assertion `time <= * 0xFFFFFFFF' failed. Aborted (core dumped)
 */

#include <stdint.h>

#include "board/board_spec.h"
#include "board/ex10_osal.h"
#include "ex10_api/board_init.h"
#include "ex10_api/command_transactor.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_protocol.h"


static const bool verbose = true;

int main(int argc, char* argv[])
{
    ex10_ex_printf("Starting SPI transfer example\n");

    uint8_t tx_data[EX10_SPI_BURST_SIZE];
    uint8_t rx_data[EX10_SPI_BURST_SIZE];

    struct ConstByteSpan tx = {
        .data   = tx_data,
        .length = EX10_SPI_BURST_SIZE,
    };

    struct ByteSpan rx = {
        .data   = rx_data,
        .length = EX10_SPI_BURST_SIZE,
    };

    uint32_t iterations = 1000u;

    if (argc < 2)
    {
        ex10_ex_printf("using default iteration count: %u\n", iterations);
    }
    else
    {
        iterations = (uint32_t)atoi(argv[1u]);
    }

    struct Ex10Result ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();

    tx.length = 0u;
    for (size_t iteration_number = 0u;
         (iterations == 0u) || (iteration_number < iterations);
         ++iteration_number)
    {
        ex10_memset(
            tx_data, sizeof(tx_data), iteration_number & UINT8_MAX, tx.length);
        rx.length = tx.length;

        if (verbose)
        {
            ex10_ex_printf("Test transfer %zu (size %zu)\n",
                           iteration_number++,
                           tx.length);
        }
        bool const verify = true;
        ex10_result       = ex10_protocol->test_transfer(&tx, &rx, verify);
        if (ex10_result.error)
        {
            ex10_ex_eprintf(
                "Response code from device: %u\n",
                ex10_result.device_status.cmd_host_result.failed_result_code);
            ex10_ex_eprintf("Response code from commands layer: %u\n",
                            ex10_result.device_status.cmd_host_result
                                .failed_host_result_code);
            break;
        }

        tx.length += 1u;
        if (tx.length >= EX10_SPI_BURST_SIZE)
        {
            // Note the wrap-around comparison is 1 less than the
            // EX10_SPI_BURST_SIZE. This allows room for the TransferTest
            // command byte to be inserted.
            tx.length = 0u;
        }
    }

    if (ex10_result.error == false)
    {
        ex10_ex_printf("Pass\n");
    }
    else
    {
        ex10_ex_eprintf("Fail\n");
    }

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending SPI transfer example\n");
    return ex10_result.error;
}
