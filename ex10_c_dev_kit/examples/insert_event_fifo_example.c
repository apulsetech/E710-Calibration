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
 * @file insert_event_fifo_example.c
 * @details The insert event fifo example shows how to insert an event in the
 *  event FIFO stream from the host.
 */

#include <string.h>

#include "board/time_helpers.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/command_transactor.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"


static int check_padding_bytes(struct EventFifoPacket const* packet)
{
    // Note: If dynamic_data is pointing at padding bytes, then
    // they should all be zero and pad out to 32-bit alignment.
    if (packet->dynamic_data_length >= sizeof(uint32_t))
    {
        ex10_ex_eprintf("Dynamic data overload in given packet\n");
        return 1;
    }
    if (packet->dynamic_data !=
        &packet->static_data->raw[packet->static_data_length])
    {
        ex10_ex_eprintf(
            "Incorrect dynamic data or incorrect static data length\n");
        return 1;
    }
    uint8_t const* iter = packet->dynamic_data;
    for (; iter < packet->dynamic_data + packet->dynamic_data_length; ++iter)
    {
        if (*iter != 0u)
        {
            ex10_ex_eprintf("Non-zero data bytes found in 0 padding bytes\n");
            return 1;
        }
    }
    // iter now points one byte past the last padding byte,
    // which is the location of the next packet in memory.
    // Check that this location is 32-bit aligned.
    if ((uintptr_t)iter % sizeof(uint32_t) != 0u)
    {
        ex10_ex_eprintf("Dynamic data with padding is not 32-bit aligned\n");
        return 1;
    }

    return 0;
}

static int insert_fifo_example(void)
{
    struct Ex10Reader const* reader = get_ex10_reader();
    // Set the EventFifo threshold to something larger than the size
    // of all the test packets to be inserted. The IRQ will not be
    // triggered until the final InsertFifoEvent command.
    get_ex10_protocol()->set_event_fifo_threshold(2048u);

    // Do not insert a packet, just a request an EventFifo interrupt.
    // This will cause the initial HelloWorld packet to be read from Ex10.
    reader->insert_fifo_event(true, NULL);

    // Wait for the HelloWorld packet to be read from the Ex10.
    get_ex10_time_helpers()->busy_wait_ms(20);

    // Check HelloWorld, the first packet after a reset:
    struct EventFifoPacket const* packet = reader->packet_peek();
    if (!packet)
    {
        ex10_ex_printf(
            "Expected to receive more than 1 packet, but none were found\n");
        return 1;
    }
    if (packet->packet_type != HelloWorld)
    {
        ex10_ex_eprintf("The first packet after a reset expected: %u, read: %u",
                        HelloWorld,
                        packet->packet_type);
        return 1;
    }

    get_ex10_event_fifo_printer()->print_packets(packet);
    reader->packet_remove();

    // Try to read more packets. The packet queue should be empty.
    packet = reader->packet_peek();
    if (packet)
    {
        ex10_ex_printf(
            "Expected to receive an empty packet queue, but was not empty\n");
        return 1;
    }

    union PacketData static_data = {
        .custom.payload_len = 0u,
    };

    // InsertFifoEvent event_packet_0:
    struct EventFifoPacket const event_packet_0 = {
        .packet_type         = Custom,
        .us_counter          = 0u,  // Will be set by Ex10 to Ex10 time.
        .static_data         = &static_data,
        .static_data_length  = sizeof(static_data.custom),
        .dynamic_data        = NULL,
        .dynamic_data_length = 0u,
        .is_valid            = true};

    reader->insert_fifo_event(false, &event_packet_0);

    // InsertFifoEvent event_packet_1:
    uint8_t const test_pattern_1[] = {0x12, 0x34, 0x56, 0x78};
    static_data.custom.payload_len = sizeof(test_pattern_1) / sizeof(uint32_t);

    struct EventFifoPacket const event_packet_1 = {
        .packet_type         = Custom,
        .us_counter          = 0u,  // Will be set by Ex10 to Ex10 time.
        .static_data         = &static_data,
        .static_data_length  = sizeof(static_data.custom),
        .dynamic_data        = test_pattern_1,
        .dynamic_data_length = sizeof(test_pattern_1),
        .is_valid            = true};

    reader->insert_fifo_event(false, &event_packet_1);

    // InsertFifoEvent event_packet_2:
    // clang-format off
    uint8_t const test_pattern_2[] = {0x12, 0x34, 0x56, 0x78,
                                      0xfe, 0xdc, 0xba, 0x98,
                                      0xf0, 0x00, 0x1b, 0xa1};
    // clang-format on
    static_data.custom.payload_len = sizeof(test_pattern_2) / sizeof(uint32_t);

    struct EventFifoPacket const event_packet_2 = {
        .packet_type         = Custom,
        .us_counter          = 0u,  // Will be set by Ex10 to Ex10 time.
        .static_data         = &static_data,
        .static_data_length  = sizeof(static_data.custom),
        .dynamic_data        = test_pattern_2,
        .dynamic_data_length = sizeof(test_pattern_2),
        .is_valid            = true};

    reader->insert_fifo_event(false, &event_packet_2);

    // Test the ContinuousInventorySummary packet
    struct ContinuousInventorySummary const summary = {
        .duration_us                = 10 * 1000u * 1000u,
        .number_of_inventory_rounds = 0x12345678u,
        .number_of_tags             = 0xABCDEF12u,
        .reason                     = SRMaxDuration,
        .last_op_id                 = StartInventoryRoundOp,
        .last_op_error              = ErrorUnknownError,
    };

    struct EventFifoPacket const summary_packet = {
        .packet_type         = ContinuousInventorySummary,
        .us_counter          = 0u,
        .static_data         = (union PacketData const*)&summary,
        .static_data_length  = sizeof(struct ContinuousInventorySummary),
        .dynamic_data        = NULL,
        .dynamic_data_length = 0u,
        .is_valid            = true,
    };

    // This time request the Ex10 interrupt get triggered.
    reader->insert_fifo_event(true, &summary_packet);

    // give enough time for the interrupt handler to retrieve any packets
    get_ex10_time_helpers()->busy_wait_ms(20);

    // Check event_packet_0:
    packet = reader->packet_peek();
    if (!packet)
    {
        ex10_ex_printf(
            "Expected to receive more than 1 packet, but none were found\n");
        return 1;
    }
    get_ex10_event_fifo_printer()->print_packets(packet);
    if (packet->packet_type != Custom ||
        packet->static_data->custom.payload_len != 0u ||
        packet->static_data_length != sizeof(static_data.custom) ||
        packet->dynamic_data_length != 0u ||
        packet->dynamic_data !=
            &packet->static_data->raw[packet->static_data_length])
    {
        ex10_ex_eprintf("Check event_packet_0 failed\n");
        ex10_ex_eputs("Packet type expected: %u, read: %u\n",
                      Custom,
                      packet->packet_type);
        ex10_ex_eputs(
            "Static data custom payload length expected: %u, read: %u\n",
            0u,
            packet->static_data->custom.payload_len);
        ex10_ex_eputs("Static data length expected: %u, read: %u\n",
                      sizeof(static_data.custom),
                      packet->static_data_length);
        ex10_ex_eputs("Dynamic data length expected: %u, read: %u\n",
                      0u,
                      packet->dynamic_data_length);
        ex10_ex_eputs("Dynamic data expected: %u, read: %u\n",
                      &packet->static_data->raw[packet->static_data_length],
                      packet->dynamic_data);
        return 1;
    }

    reader->packet_remove();

    // Check event_packet_1:
    packet = reader->packet_peek();
    if (!packet)
    {
        ex10_ex_printf(
            "Expected to receive more than 1 packet, but none were found\n");
        return 1;
    }
    get_ex10_event_fifo_printer()->print_packets(packet);
    if (packet->packet_type != Custom ||
        packet->static_data->custom.payload_len !=
            sizeof(test_pattern_1) / sizeof(uint32_t) ||
        packet->static_data_length != sizeof(static_data.custom) ||
        packet->dynamic_data_length != sizeof(test_pattern_1))
    {
        ex10_ex_eprintf("Check event_packet_1 failed\n");
        ex10_ex_eputs("Packet type expected: %u, read: %u\n",
                      Custom,
                      packet->packet_type);
        ex10_ex_eputs(
            "Static data custom payload length expected: %u, read: %u\n",
            sizeof(test_pattern_1) / sizeof(uint32_t),
            packet->static_data->custom.payload_len);
        ex10_ex_eputs("Static data length expected: %u, read: %u\n",
                      sizeof(static_data.custom),
                      packet->static_data_length);
        ex10_ex_eputs("Dynamic data length expected: %u, read: %u\n",
                      sizeof(test_pattern_1),
                      packet->dynamic_data_length);
        return 1;
    }
    if (memcmp(packet->dynamic_data,
               test_pattern_1,
               packet->dynamic_data_length) != 0)
    {
        ex10_ex_eprintf("Failed to copy event_packet_1\n");
        return 1;
    }
    reader->packet_remove();

    // Check event_packet_2:
    packet = reader->packet_peek();
    if (!packet)
    {
        ex10_ex_printf(
            "Expected to receive more than 1 packet, but none were found\n");
        return 1;
    }
    get_ex10_event_fifo_printer()->print_packets(packet);
    if (packet->packet_type != Custom ||
        packet->static_data->custom.payload_len !=
            sizeof(test_pattern_2) / sizeof(uint32_t) ||
        packet->static_data_length != sizeof(static_data.custom) ||
        packet->dynamic_data_length != sizeof(test_pattern_2))
    {
        ex10_ex_eprintf("Check event_packet_2 failed\n");
        ex10_ex_eputs("Packet type expected: %u, read: %u\n",
                      Custom,
                      packet->packet_type);
        ex10_ex_eputs(
            "Static data custom payload length expected: %u, read: %u\n",
            sizeof(test_pattern_2) / sizeof(uint32_t),
            packet->static_data->custom.payload_len);
        ex10_ex_eputs("Static data length expected: %u, read: %u\n",
                      sizeof(static_data.custom),
                      packet->static_data_length);
        ex10_ex_eputs("Dynamic data length expected: %u, read: %u\n",
                      sizeof(test_pattern_2),
                      packet->dynamic_data_length);
        return 1;
    }
    if (memcmp(packet->dynamic_data,
               test_pattern_2,
               packet->dynamic_data_length) != 0)
    {
        ex10_ex_eprintf("Failed to copy event_packet_2\n");
        return 1;
    }
    reader->packet_remove();

    // Check for the ContinuousInventorySummary packet:
    packet = reader->packet_peek();
    if (!packet)
    {
        ex10_ex_printf(
            "Expected to receive more than 1 packet, but none were found\n");
        return 1;
    }
    get_ex10_event_fifo_printer()->print_packets(packet);
    if (packet->packet_type != ContinuousInventorySummary)
    {
        ex10_ex_eprintf("Check ContinuousInventorySummary failed\n");
        ex10_ex_eputs("Packet type expected: %u, read: %u\n",
                      ContinuousInventorySummary,
                      packet->packet_type);
        return 1;
    }
    if (packet->static_data_length !=
        sizeof(static_data.continuous_inventory_summary))
    {
        ex10_ex_eprintf("Check ContinuousInventorySummary failed\n");
        ex10_ex_eputs("Static data length expected: %u, read: %u\n",
                      sizeof(static_data.continuous_inventory_summary),
                      packet->static_data_length);
        return 1;
    }
    if (check_padding_bytes(packet) != 0)
    {
        ex10_ex_printf("Error occurred while checking padding bytes\n");
        return 1;
    }

    struct ContinuousInventorySummary const* packet_summary =
        &packet->static_data->continuous_inventory_summary;

    if (packet_summary->duration_us != summary.duration_us ||
        packet_summary->number_of_inventory_rounds !=
            summary.number_of_inventory_rounds ||
        packet_summary->number_of_tags != summary.number_of_tags ||
        packet_summary->reason != summary.reason ||
        packet_summary->last_op_id != summary.last_op_id ||
        packet_summary->last_op_error != summary.last_op_error)
    {
        ex10_ex_eprintf(
            "Packet summary does not match continuous inventory summary\n");
        ex10_ex_eputs("Duration us expected: %u, read: %u\n",
                      summary.duration_us,
                      packet_summary->duration_us);
        ex10_ex_eputs("Number of inventory rounds expected: %u, read: %u\n",
                      summary.number_of_inventory_rounds,
                      packet_summary->number_of_inventory_rounds);
        ex10_ex_eputs("Number of tag read expected: %u, read: %u\n",
                      summary.number_of_tags,
                      packet_summary->number_of_tags);
        ex10_ex_eputs("Summary reason expected: %u, read: %u\n",
                      summary.reason,
                      packet_summary->reason);
        ex10_ex_eputs("Last op id expected: %u, read: %u\n",
                      summary.last_op_id,
                      packet_summary->last_op_id);
        ex10_ex_eputs("Last op error status expected: %u, read: %u\n",
                      summary.last_op_error,
                      packet_summary->last_op_error);
        return 1;
    }
    reader->packet_remove();

    return 0;
}

int main(void)
{
    ex10_ex_printf("Starting insert event FIFO example\n");

    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return 1;
    }

    int result = insert_fifo_example();

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending insert event FIFO example\n");
    return result;
}
