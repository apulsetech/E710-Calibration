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

#include "ex10_api/byte_span.h"
#include "ex10_api/event_fifo_packet_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Ex10EventParser
{
    /**
     * Returns a struct that accesses the variable length fields of a TagRead
     * packet from the Event FIFO.
     *
     * If an error occurs during dynamic data parsing, the returned struct
     * will have all fields nullified.
     *
     * @param packet The data to parse for information about the tag.
     */
    struct TagReadFields (*get_tag_read_fields)(void const*      dynamic_data,
                                                size_t           data_length,
                                                enum TagReadType type,
                                                uint8_t          tid_offset);

    /**
     * Returns the minimum acceptable payload length for data after
     * the packet header. This accounts for the static data legnth and does not
     * include the dynamic data length.
     *
     * @param packet_type The Event packet type.
     */
    size_t (*get_static_payload_length)(enum EventPacketType packet_type);

    /**
     * Returns whether the passed packet type is valid.
     *
     * @param packet_type The Event packet type.
     */
    bool (*get_packet_type_valid)(enum EventPacketType packet_type);

    /**
     * Parse an Event Fifo packet from a stream of bytes.
     *
     * @param bytes [in/out] The packet stream to parse.
     * Upon successful completion of parsing the packet the members values are
     * updated.
     *
     * @return struct EventFifoPacket The parsed event fifo packet.
     */
    struct EventFifoPacket (*parse_event_packet)(struct ConstByteSpan* bytes);

    /**
     * Create a mostly formatted packet header based on the type.
     * The packet_length parameter byte[0] will be the number of static payload
     * bytes expected for the packet type of interest.
     *
     * @note The actual packet length is the number of u32 words and encompasses
     * the entire packet including the header and dynamic data.
     *
     * @note The us_counter will be set to zero.
     *
     * @return struct PacketHeader The newly formed packet header with a static
     * packet length in bytes and needs to be modified to work properly.
     */
    struct PacketHeader (*make_packet_header)(
        enum EventPacketType event_packet_type);
};

struct Ex10EventParser const* get_ex10_event_parser(void);

#ifdef __cplusplus
}
#endif
