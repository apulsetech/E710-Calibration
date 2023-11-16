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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ex10_api/ex10_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The size of the Ex10 Event Fifo. To simplify Event Fifo packet parsing and
 * storage the size of each buffer allocated for reading during event fifo
 * interrupts must be this size or larger. The prevents packets read from the
 * Ex10 from being wrapped across 2 separate buffers.
 */
#define EX10_EVENT_FIFO_SIZE ((size_t)4096u)

// IPJ_autogen | gen_c_app_ex10_api_fifo_types {
// clang-format off
#pragma pack(push, 1)

static const uint16_t event_fifo_sha = 0x252f;

/**
 * @struct PacketHeader
 * The EventFifo packet header.
 */
struct PacketHeader
{
    /// The length of the entire packet in 32-bit words.
    uint8_t    packet_length;
    /// The EventFifo packet type. See `enum EventPacketType <./ex10_host_c_support/event_parser.html#_CPPv415EventPacketType>`_.
    uint8_t    packet_type;
    /// A SHA which is generated based on the EventFifo format.
    uint16_t   sha;
    /// Packet timestamp, in microseconds, from the Impinj reader chip.
    uint32_t   us_counter;
};
static_assert(sizeof(struct PacketHeader) == 8,
              "Size of packet header not packed properly");

/**
 * @enum EventPacketType
 * The EvenfFifo packet type. Used to interpret the packet payload.
 */
enum EventPacketType
{
    TxRampUp                   = 0x01,
    TxRampDown                 = 0x02,
    InventoryRoundSummary      = 0x03,
    QChanged                   = 0x04,
    TagRead                    = 0x05,
    Gen2Transaction            = 0x07,
    ContinuousInventorySummary = 0x08,
    HelloWorld                 = 0x09,
    Custom                     = 0x0a,
    PowerControlLoopSummary    = 0x0b,
    AggregateOpSummary         = 0x0d,
    Halted                     = 0x0e,
    InvalidPacket              = 0xf0,
    FifoOverflowPacket         = 0xf1,
    Ex10ResultPacket           = 0xf2,
    WriteProfileData           = 0xfd,
    SjcMeasurement             = 0xfe,
    Debug                      = 0xff,
};

/**
 * @struct TxRampUp
 * Sent when the transmit power is ramped up
 */
struct TxRampUp
{
    uint32_t carrier_frequency;
};

/**
 * @struct TxRampDown
 * Sent when the transmit power is ramped down
 */
struct TxRampDown
{
    uint8_t reason;
    uint8_t packet_rfu_1;
    uint8_t packet_rfu_2;
    uint8_t packet_rfu_3;
};

/**
 * @enum RampDownReason
 * The reason Tx power was ramped down.
 */
enum RampDownReason
{
    RampDownHost       = 1,
    RampDownRegulatory = 2,
};

/**
 * @struct InventoryRoundSummary
 * End of inventory round report
 */
struct InventoryRoundSummary
{
    uint32_t duration_us;
    uint32_t total_slots;
    uint16_t num_slots;
    uint16_t empty_slots;
    uint16_t single_slots;
    uint16_t collided_slots;
    uint8_t reason;
    uint8_t final_q;
    uint8_t min_q_count;
    uint8_t queries_since_valid_epc_count;
};

/**
 * @enum InventorySummaryReason
 * The reason for inventory round completion.
 */
enum InventorySummaryReason
{
    InventorySummaryNone          = 0,
    InventorySummaryDone          = 1,
    InventorySummaryHost          = 2,
    InventorySummaryRegulatory    = 3,
    InventorySummaryEventFifoFull = 4,
    InventorySummaryTxNotRampedUp = 5,
    InventorySummaryInvalidParam  = 6,
    InventorySummaryLmacOverload  = 7,
    InventorySummaryUnsupported   = 8,
};

/**
 * @struct QChanged
 * The modem has adjusted the Q value
 */
struct QChanged
{
    uint16_t num_slots;
    uint16_t empty_slots;
    uint16_t single_slots;
    uint16_t collided_slots;
    uint8_t q_value;
    uint8_t sent_query;
    uint8_t packet_rfu_1;
    uint8_t packet_rfu_2;
};

/**
 * @struct TagRead
 * Contains fixed-length fields from a TagRead packet in the EventFifo
 */
struct TagRead
{
    uint16_t rssi;
    uint16_t rf_phase_begin;
    uint16_t rf_phase_end;
    uint16_t rx_gain_settings;
    uint8_t type;
    uint8_t tid_offset;
    bool halted_on_tag : 1;
    bool memory_parity_err : 1;
    uint8_t packet_rfu_1 : 6;
    uint8_t packet_rfu_2;
};

/** @enum TagReadType */
enum TagReadType
{
    TagReadTypeEpc              = 1,
    TagReadTypeEpcWithTid       = 2,
    TagReadTypeEpcWithFastIdTid = 3,
};

/**
 * @struct Gen2Transaction
 * The result of a user requested Gen2 transaction
 */
struct Gen2Transaction
{
    uint8_t transaction_id;
    uint8_t status;
    bool valid_path_metrics;
    uint8_t packet_rfu;
    uint16_t rf_phase_begin;
    uint16_t rf_phase_end;
    uint16_t rssi;
    uint16_t num_bits;
};

/** @enum Gen2TransactionStatus */
enum Gen2TransactionStatus
{
    Gen2TransactionStatusOk               = 1,
    Gen2TransactionStatusBadCrc           = 2,
    Gen2TransactionStatusNoReply          = 3,
    Gen2TransactionStatusInvalidReplyType = 4,
    Gen2TransactionStatusCoverCodeFailed  = 5,
    Gen2TransactionStatusMemoryParityErr  = 6,
    Gen2TransactionStatusUnknown          = 255,
    Gen2TransactionStatusUnsupported      = 7,
};

/**
 * @struct ContinuousInventorySummary
 * End of continuous inventory report
 */
struct ContinuousInventorySummary
{
    uint32_t duration_us;
    uint32_t number_of_inventory_rounds;
    uint32_t number_of_tags;
    uint8_t reason;
    uint8_t last_op_id;
    uint8_t last_op_error;
    uint8_t packet_rfu_1;
};

/**
 * @struct HelloWorld
 * Sign on message emitted when the reader chip boots up
 */
struct HelloWorld
{
    uint16_t sku;
    uint8_t reset_reason;
    uint8_t crash_info_conditional;
};

/**
 * @struct Custom
 * Packet from the InsertFifoEvent command. This packet contains dynamic data.
 */
struct Custom
{
    uint32_t payload_len;
};

/**
 * @struct PowerControlLoopSummary
 * Contains summary data from the PowerControlLoopOp
 */
struct PowerControlLoopSummary
{
    uint32_t iterations_taken;
    int16_t final_error;
    int16_t final_tx_fine_gain;
};

/**
 * @struct AggregateOpSummary
 * Contains summary data of what was run in the aggregate Op.
 */
struct AggregateOpSummary
{
    uint16_t op_run_count;
    uint16_t write_count;
    uint16_t insert_fifo_count;
    uint16_t final_buffer_byte_index;
    uint16_t total_jump_count;
    uint8_t last_inner_op_run;
    uint8_t last_inner_op_error;
    uint16_t identifier;
    uint8_t last_inner_command_run;
    uint8_t last_inner_command_error;
};

/**
 * @struct Halted
 * Packet is emitted when the LMAC enters the halted state
 */
struct Halted
{
    uint16_t halted_handle;
    uint8_t reason;
    uint8_t packet_rfu;
};

/** @enum HaltedReason */
enum HaltedReason
{
    HaltedReasonEntered          = 1,
    HaltedReasonReturned         = 2,
};

/**
 * @struct InvalidPacket
 * An invalid packet
 */
struct InvalidPacket
{
    uint32_t packet_rfu;
};

/**
 * @struct FifoOverflowPacket
 * The FIFO overflowed during the previous insertion.
 */
struct FifoOverflowPacket
{
    uint16_t num_bytes_over;
    uint8_t overflowing_packet_type;
    uint8_t rfu;
};

/**
 * @struct Ex10ResultPacket
 * An error packet
 */
struct Ex10ResultPacket
{
    struct Ex10Result ex10_result;
};

/**
 * @struct WriteProfileData
 * The code profiler file data event
 */
struct WriteProfileData
{
    int32_t file_no;
    uint32_t payload_length;
};

/**
 * @struct SjcMeasurement
 * A SJC residue measurement and configuration corresponding to the RxRunSjcOp.
These measurements can arrive during the RxRunSjcOp if enabled, and at the end
of the RxRunSjcOp. The final measurement event packet generated by the FW
corresponds with the last and best SJC solution determined by the RxRunSjcOp.

 */
struct SjcMeasurement
{
    int8_t cdac_i;
    int8_t cdac_q;
    uint8_t rx_atten;
    uint8_t flags;
    int32_t residue_i;
    int32_t residue_q;
};

/**
 * @struct Debug
 * Packet for debugging purposes. This packet contains dynamic data.
 */
struct Debug
{
    uint32_t payload_len;
};

/**
 * @union PacketData
 * Union of all possible EventFifoPacket data fields.
 */
union PacketData
{
    struct TxRampUp                   tx_ramp_up;
    struct TxRampDown                 tx_ramp_down;
    struct InventoryRoundSummary      inventory_round_summary;
    struct QChanged                   q_changed;
    struct TagRead                    tag_read;
    struct Gen2Transaction            gen2_transaction;
    struct ContinuousInventorySummary continuous_inventory_summary;
    struct HelloWorld                 hello_world;
    struct Custom                     custom;
    struct PowerControlLoopSummary    power_control_loop_summary;
    struct AggregateOpSummary         aggregate_op_summary;
    struct Halted                     halted;
    struct InvalidPacket              invalid_packet;
    struct FifoOverflowPacket         fifo_overflow_packet;
    struct Ex10ResultPacket           ex10_result_packet;
    struct WriteProfileData           write_profile_data;
    struct SjcMeasurement             sjc_measurement;
    struct Debug                      debug;
    uint8_t const raw[1u];
};


#pragma pack(pop)
// clang-format on
// IPJ_autogen }

struct EventFifoPacket
{
    enum EventPacketType    packet_type;
    uint32_t                us_counter;
    union PacketData const* static_data;
    size_t                  static_data_length;
    uint8_t const*          dynamic_data;
    size_t                  dynamic_data_length;
    bool                    is_valid;
};

/**
 * Contains variable-length fields from a TagRead packet in the EventFifo.
 * The TagReadFields contains pointers and lengths into the TagRead EventFifo
 * packet type TagRead.
 *
 * @see get_tag_read_fields() in event_packet_parser.h.
 */
struct TagReadFields
{
    uint16_t const* pc;
    uint16_t const* xpc_w1;
    uint16_t const* xpc_w2;

    uint8_t const* epc;
    size_t         epc_length;
    uint8_t const* stored_crc;

    uint8_t const* tid;
    size_t         tid_length;
};

#define TID_LENGTH_BYTES ((size_t)12)

/**
 * The maximum EPC length in bytes as contained within the struct TagReadData.
 * The maximum EPC length is calculated as:
 * The PC is read and prepended into the EPC buffer:        0x002 bytes
 * From the Gen2 specification: 0x210 - 0x020 = 0x1F0 bits, 0x03E bytes
 * Allow for 2 XPC words to be backscattered                0x004 bytes
 * Total EPC buffer allocated:                              0x044 bytes
 */
#define EPC_BUFFER_BYTE_LENGTH ((size_t)0x044u)

/**
 * @struct TagReadData
 * During an inventory, when the EventFifo TagRead packet is received, this
 * structure is used as the destination for copying the data from the EventFifo
 * packet.
 *
 * The data pointed to be the struct TagReadFields is copied out into this
 * struct. This allows the TagRead EventFifo packet to be contained af the
 * EventFifo packet has been released to free memory.
 */
struct TagReadData
{
    /// The tag's Protocol-Control word.
    /// @note This matches the value stored in the EPC bytes[0:1].
    uint16_t pc;

    /// Tag's XPC word 1
    uint16_t xpc_w1;
    /// Indicates if XPC W1 was detected (indicated by XI=1 in PC word)
    bool xpc_w1_is_valid;

    /// Tag's XPC word 2
    uint16_t xpc_w2;
    /// Indicates if XPC W1 was detected (indicated by XEB=1 in XPC W1 word)
    bool xpc_w2_is_valid;

    /// The tag PC + EPC data backscattered by the tag.
    uint8_t epc[EPC_BUFFER_BYTE_LENGTH];

    /// The number of bytes captured into the TagReadData.epc[] buffer.
    size_t epc_length;

    /// The Stored CRC value backscattered by the tag.
    uint16_t stored_crc;

    /// Indicates whether a Stored CRC value was backscattered by the tag.
    bool stored_crc_is_valid;

    /// The TID data backscattered by the tag if FastId is enabled.
    uint8_t tid[TID_LENGTH_BYTES];

    /// The number of bytes contained in the TagReadData.tid[] buffer.
    size_t tid_length;
};

#ifdef __cplusplus
}
#endif
