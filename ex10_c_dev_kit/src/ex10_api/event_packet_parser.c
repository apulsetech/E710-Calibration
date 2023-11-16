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


#include <stddef.h>

#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/print_data.h"

#define SHORT_TID_MONZA_4D 0xE2801100
#define SHORT_TID_MONZA_4U 0xE2801104
#define SHORT_TID_MONZA_4QT 0xE2801105
#define SHORT_TID_MONZA_4E 0xE280110C
#define SHORT_TID_MONZA_4I 0xE2801114
#define SHORT_TID_MONZA_5 0xE2801130
#define SHORT_TID_MONZA_5U 0xE2801132

#define GEN2_REPLY_HEADER_LENGTH_BYTES 1
#define GEN2_REPLY_HANDLE_LENGTH_BYTES 2
#define GEN2_REPLY_CRC_LENGTH_BYTES 2

/*
 * Definitions of a tag's reply to ACK
 */
static uint16_t const TAG_WORD_SIZE_IN_BYTES   = 2u;
static uint32_t const TAG_REPLY_PC_WORD_OFFSET = 0u;
static uint32_t const TAG_REPLY_EPC_MEM_OFFSET = 1u;

// Note that these defines are for words in tag backscatter stored in little
// endian format
static uint16_t const TAG_REPLY_PC_WORD_L_SHIFT = 3u;
static uint16_t const TAG_REPLY_PC_WORD_L_MASK  = 0x001F;
static uint16_t const TAG_REPLY_PC_WORD_XI      = 0x02;

static uint16_t const TAG_REPLY_XPC_W1_XEB = 0x80;


static bool is_monza4_monza5(uint8_t const* tid)
{
    uint32_t short_tid = 0u;
    short_tid |= tid[0];
    short_tid <<= 8u;
    short_tid |= tid[1];
    short_tid <<= 8u;
    short_tid |= tid[2];
    short_tid <<= 8u;
    short_tid |= tid[3];

    switch (short_tid)
    {
        case SHORT_TID_MONZA_4D:
        case SHORT_TID_MONZA_4U:
        case SHORT_TID_MONZA_4QT:
        case SHORT_TID_MONZA_4E:
        case SHORT_TID_MONZA_4I:
        case SHORT_TID_MONZA_5:
        case SHORT_TID_MONZA_5U:
            return true;

        default:
            return false;
    }
}

static struct TagReadFields get_tag_read_fields(void const*      dynamic_data,
                                                size_t           data_length,
                                                enum TagReadType type,
                                                uint8_t          tid_offset)
{
    struct TagReadFields err_tag_read = {
        .pc         = NULL,
        .epc        = NULL,
        .epc_length = 0,
        .stored_crc = NULL,
        .xpc_w1     = NULL,
        .xpc_w2     = NULL,
        .tid        = NULL,
        .tid_length = 0,
    };

    if (dynamic_data == NULL)
    {
        ex10_eprintf("Dynamic data pointer is NULL.\n");
        return err_tag_read;
    }

    uint16_t const* tag_data = (uint16_t const*)dynamic_data;

    // The first 16-bit word is the PC
    uint16_t const* pc = &tag_data[TAG_REPLY_PC_WORD_OFFSET];

    size_t data_length_from_pc =
        (((*pc) >> TAG_REPLY_PC_WORD_L_SHIFT) & TAG_REPLY_PC_WORD_L_MASK) *
        TAG_WORD_SIZE_IN_BYTES;

    // Length of ACK response (equals to length from the PC word + PC + CRC).
    size_t min_data_length = data_length_from_pc + 2 * TAG_WORD_SIZE_IN_BYTES;

    // Make sure the received data length matches the calculated ACK response.
    if (data_length < min_data_length)
    {
        ex10_eprintf(
            "Tag response length calculated from the PC is %zu words,"
            " while received data length is %zu\n",
            min_data_length,
            data_length);

        return err_tag_read;
    }

    uint16_t const* xpc_w1     = NULL;
    uint16_t const* xpc_w2     = NULL;
    size_t          xpc_length = 0;

    // If XI indicator is set, XPC word 1 was backscattered after the
    // PC word and the EPC will follow XPC word(s):
    // ---------------------------------------
    // | PC | XPC_W1 |    EPC    | StoredCRC |
    // ---------------------------------------
    if ((*pc) & TAG_REPLY_PC_WORD_XI)
    {
        xpc_w1 = (uint16_t const*)(pc + 1);
        xpc_length += TAG_WORD_SIZE_IN_BYTES;

        // If XEB indicator is set, additional XPC word 2 was backscattered
        // after XPC word 1 and the EPC will follow XPC word 2:
        // ------------------------------------------------
        // | PC | XPC_W1 | XPC_W2 |    EPC    | StoredCRC |
        // ------------------------------------------------
        if ((*xpc_w1) & TAG_REPLY_XPC_W1_XEB)
        {
            xpc_w2 = (uint16_t const*)(pc + 2);
            xpc_length += TAG_WORD_SIZE_IN_BYTES;
        }
    }

    // Setting pointer to the start of EPC
    uint8_t const* epc =
        (uint8_t const*)&tag_data[TAG_REPLY_EPC_MEM_OFFSET] + xpc_length;

    // Now that we have parsed the XPC word(s) adjust the length
    data_length_from_pc -= xpc_length;

    size_t         epc_length = 0;
    uint8_t const* stored_crc = NULL;
    uint8_t const* tid        = NULL;
    size_t         tid_length = 0;

    switch (type)
    {
        case TagReadTypeEpc:
            // For EPC only, the dynamic data is the normal response to an
            // Ack command:
            // ---------------------------------
            // | PC |     EPC      | StoredCRC |
            // ---------------------------------
            epc_length = data_length_from_pc;
            stored_crc = &epc[epc_length];

            // As only EPC was received, nullifying all TID related fields
            tid        = NULL;
            tid_length = 0;
            break;

        case TagReadTypeEpcWithTid:
            // clang-format off
            // In this case, the dynamic data is the response to an Ack,
            // followed by a Gen2 Read Reply:
            // ------------------------------------------------------------------------------
            // | PC |     EPC      | StoredCRC | Header | Memory Words (TID) | Handle | CRC |
            // ------------------------------------------------------------------------------
            // StoredCRC is calculated over the PC + EPC.
            //       CRC is calculated over the Header + TID + Handle.
            // clang-format on
            min_data_length +=
                TID_LENGTH_BYTES + GEN2_REPLY_HEADER_LENGTH_BYTES +
                GEN2_REPLY_HANDLE_LENGTH_BYTES + GEN2_REPLY_CRC_LENGTH_BYTES;
            if (data_length < min_data_length)
            {
                ex10_eprintf(
                    "Minimal length calculated from the PC is %zu words,"
                    " while received data length is %zu\n",
                    min_data_length,
                    data_length);

                return err_tag_read;
            }

            epc_length = data_length_from_pc;
            stored_crc = &epc[epc_length];

            tid = (uint8_t const*)(tag_data) + tid_offset +
                  GEN2_REPLY_HEADER_LENGTH_BYTES;
            tid_length = TID_LENGTH_BYTES;
            break;

        case TagReadTypeEpcWithFastIdTid:
            tid        = (uint8_t const*)(tag_data) + tid_offset;
            tid_length = TID_LENGTH_BYTES;

            if (is_monza4_monza5(tid))
            {
                // For Monza4 and Monza5 tags, the dynamic data has the
                // following format:
                // ----------------------------------------------------
                // | PC |     EPC      | StoredCRC |    TID     | CRC |
                // ----------------------------------------------------
                // CRC is the calculated over the entire packet starting with
                // the PC until the end of the TID.
                epc_length = data_length_from_pc - TID_LENGTH_BYTES - 2;
                stored_crc = &epc[epc_length];
            }
            else
            {
                // For Monza6 and newer Impinj tags, the dynamic data has the
                // following format:
                // ----------------------------------------
                // | PC |     EPC      |    TID     | CRC |
                // ----------------------------------------
                // CRC is the calculated over the entire packet starting with
                // the PC until the end of the TID.
                epc_length = data_length_from_pc - TID_LENGTH_BYTES;
                stored_crc = NULL;
            }
            break;

        default:
            ex10_eprintf("Unexpected tag read data type: %d\n", type);
            return err_tag_read;
    }


    return (struct TagReadFields){
        .pc         = pc,
        .epc        = epc,
        .epc_length = epc_length,
        .stored_crc = stored_crc,
        .xpc_w1     = xpc_w1,
        .xpc_w2     = xpc_w2,
        .tid        = tid,
        .tid_length = tid_length,
    };
}

// IPJ_autogen | gen_c_app_ex10_api_packet_parser {
// clang-format off

/* Make sure packet struct sizes are as expected */
static_assert(sizeof(struct TxRampUp) == 4,
              "Size of TxRampUp packet type incorrect");
static_assert(sizeof(struct TxRampDown) == 4,
              "Size of TxRampDown packet type incorrect");
static_assert(sizeof(struct InventoryRoundSummary) == 20,
              "Size of InventoryRoundSummary packet type incorrect");
static_assert(sizeof(struct QChanged) == 12,
              "Size of QChanged packet type incorrect");
static_assert(sizeof(struct TagRead) == 12,
              "Size of TagRead packet type incorrect");
static_assert(sizeof(struct Gen2Transaction) == 12,
              "Size of Gen2Transaction packet type incorrect");
static_assert(sizeof(struct ContinuousInventorySummary) == 16,
              "Size of ContinuousInventorySummary packet type incorrect");
static_assert(sizeof(struct HelloWorld) == 4,
              "Size of HelloWorld packet type incorrect");
static_assert(sizeof(struct Custom) == 4,
              "Size of Custom packet type incorrect");
static_assert(sizeof(struct PowerControlLoopSummary) == 8,
              "Size of PowerControlLoopSummary packet type incorrect");
static_assert(sizeof(struct AggregateOpSummary) == 16,
              "Size of AggregateOpSummary packet type incorrect");
static_assert(sizeof(struct Halted) == 4,
              "Size of Halted packet type incorrect");
static_assert(sizeof(struct InvalidPacket) == 4,
              "Size of InvalidPacket packet type incorrect");
static_assert(sizeof(struct FifoOverflowPacket) == 4,
              "Size of FifoOverflowPacket packet type incorrect");
static_assert(sizeof(struct Ex10ResultPacket) == 8,
              "Size of Ex10ResultPacket packet type incorrect");
static_assert(sizeof(struct WriteProfileData) == 8,
              "Size of WriteProfileData packet type incorrect");
static_assert(sizeof(struct SjcMeasurement) == 12,
              "Size of SjcMeasurement packet type incorrect");
static_assert(sizeof(struct Debug) == 4,
              "Size of Debug packet type incorrect");

static size_t get_static_payload_length(enum EventPacketType current_type)
{
    switch (current_type)
    {
        case TxRampUp:
            return sizeof(struct TxRampUp);
        case TxRampDown:
            return sizeof(struct TxRampDown);
        case InventoryRoundSummary:
            return sizeof(struct InventoryRoundSummary);
        case QChanged:
            return sizeof(struct QChanged);
        case TagRead:
            return sizeof(struct TagRead);
        case Gen2Transaction:
            return sizeof(struct Gen2Transaction);
        case ContinuousInventorySummary:
            return sizeof(struct ContinuousInventorySummary);
        case HelloWorld:
            return sizeof(struct HelloWorld);
        case Custom:
            return sizeof(struct Custom);
        case PowerControlLoopSummary:
            return sizeof(struct PowerControlLoopSummary);
        case AggregateOpSummary:
            return sizeof(struct AggregateOpSummary);
        case Halted:
            return sizeof(struct Halted);
        case InvalidPacket:
            return sizeof(struct InvalidPacket);
        case FifoOverflowPacket:
            return sizeof(struct FifoOverflowPacket);
        case Ex10ResultPacket:
            return sizeof(struct Ex10ResultPacket);
        case WriteProfileData:
            return sizeof(struct WriteProfileData);
        case SjcMeasurement:
            return sizeof(struct SjcMeasurement);
        case Debug:
            return sizeof(struct Debug);
        default:
            return 0;
    }
}

static bool get_packet_type_valid(enum EventPacketType current_type)
{
    switch (current_type)
    {
        case TxRampUp:
            return true;
        case TxRampDown:
            return true;
        case InventoryRoundSummary:
            return true;
        case QChanged:
            return true;
        case TagRead:
            return true;
        case Gen2Transaction:
            return true;
        case ContinuousInventorySummary:
            return true;
        case HelloWorld:
            return true;
        case Custom:
            return true;
        case PowerControlLoopSummary:
            return true;
        case AggregateOpSummary:
            return true;
        case Halted:
            return true;
        case InvalidPacket:
            return false;
        case FifoOverflowPacket:
            return true;
        case Ex10ResultPacket:
            return true;
        case WriteProfileData:
            return true;
        case SjcMeasurement:
            return true;
        case Debug:
            return true;
        default:
            return false;
    }
}

// clang-format on
// IPJ_autogen }

static struct EventFifoPacket parse_event_packet(struct ConstByteSpan* bytes)
{
    struct PacketHeader const* packet_header =
        (struct PacketHeader const*)bytes->data;

    union PacketData const* static_data =
        (union PacketData const*)(bytes->data + sizeof(struct PacketHeader));

    size_t const packet_length_bytes =
        packet_header->packet_length * sizeof(uint32_t);

    // get_static_payload_length() returns 0 for unknown packets.
    size_t const static_data_length =
        get_static_payload_length(packet_header->packet_type);

    size_t const min_packet_length =
        sizeof(struct PacketHeader) + static_data_length;

    size_t         dynamic_data_length = 0u;
    uint8_t const* dynamic_data        = NULL;

    bool const is_valid = get_packet_type_valid(packet_header->packet_type) &&
                          (packet_header->sha == event_fifo_sha) &&
                          (packet_length_bytes >= min_packet_length) &&
                          (packet_length_bytes <= bytes->length) &&
                          (bytes->length > 0);

    if (is_valid && (static_data_length > 0u))
    {
        dynamic_data_length = packet_length_bytes - min_packet_length;
        dynamic_data        = bytes->data + min_packet_length;
    }

    else if (is_valid == false)
    {
        if (packet_header->sha != event_fifo_sha)
        {
            ex10_eprintf(
                "Event Fifo packet with invalid SHA detected "
                "Expected: 0x%04x Received: 0x%04x.\n",
                event_fifo_sha,
                packet_header->sha);
            ex10_eputs("\nThis is normally because the Impinj Reader Chip ");
            ex10_eputs("SDK and Firmware versions are different.\n");
            ex10_eputs("Updating the firmware to the version matching the ");
            ex10_eputs("SDK will often resolve this.\n");
        }
        else
        {
            // Debug rogue packets:
            ex10_eprintf(
                "Unknown packet: type: %u, static_length: %zu sha: 0x%04x\n",
                packet_header->packet_type,
                static_data_length,
                packet_header->sha);

            ex10_print_data(bytes->data, bytes->length, DataPrefixIndex);
        }

        // Indicate that this event packet could not be parsed properly.
        // Set bytes->length to zero so the caller will terminate
        // its packet processing within the current EventFifoBuffer.
        bytes->length = 0u;

        struct EventFifoPacket const invalid_packet = {
            .packet_type         = InvalidPacket,
            .us_counter          = 0,
            .static_data         = NULL,
            .static_data_length  = 0u,
            .dynamic_data        = NULL,
            .dynamic_data_length = 0u,
            .is_valid            = false,
        };

        return invalid_packet;
    }

    struct EventFifoPacket const packet = {
        .packet_type         = packet_header->packet_type,
        .us_counter          = packet_header->us_counter,
        .static_data         = static_data,
        .static_data_length  = static_data_length,
        .dynamic_data        = dynamic_data,
        .dynamic_data_length = dynamic_data_length,
        .is_valid            = is_valid,
    };

    bytes->data += packet_length_bytes;
    bytes->length -= packet_length_bytes;

    return packet;
}

static struct PacketHeader make_packet_header(
    enum EventPacketType event_packet_type)
{
    size_t const packet_length_words =
        (sizeof(struct PacketHeader) +
         get_static_payload_length(event_packet_type)) /
        sizeof(uint32_t);

    struct PacketHeader const packet_header = {
        .packet_length = (uint8_t)packet_length_words,
        .packet_type   = (uint8_t)event_packet_type,
        .sha           = event_fifo_sha,
        .us_counter    = 0u,
    };

    return packet_header;
}

static const struct Ex10EventParser ex10_event_parser = {
    .get_tag_read_fields       = get_tag_read_fields,
    .get_static_payload_length = get_static_payload_length,
    .get_packet_type_valid     = get_packet_type_valid,
    .parse_event_packet        = parse_event_packet,
    .make_packet_header        = make_packet_header,
};

struct Ex10EventParser const* get_ex10_event_parser(void)
{
    return &ex10_event_parser;
}
