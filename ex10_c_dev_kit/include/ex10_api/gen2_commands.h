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

#include "ex10_api/application_register_definitions.h"
#include "ex10_api/event_fifo_packet_types.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_COMMAND_BYTES ((size_t)0x20u)

/**
 * @enum Gen2Command
 * Gen2 standard commands
 */
enum Gen2Command
{
    Gen2Select = 0,
    Gen2Read,
    Gen2Write,
    Gen2Kill_1,  ///< First step of the kill command, immediate reply.
    Gen2Kill_2,  ///< Second step of the kill command, delayed reply.
    Gen2Lock,
    Gen2Access,
    Gen2BlockWrite,
    Gen2BlockPermalock,
    Gen2Authenticate,
    Gen2MarginRead,
    _GEN2_COMMAND_MAX,
};

enum BlockPermalockReadLock
{
    Read      = 0,
    Permalock = 1,
};

enum SelectTarget
{
    Session0     = 0,
    Session1     = 1,
    Session2     = 2,
    Session3     = 3,
    SelectedFlag = 4,
};

enum SelectAction
{
    Action000 = 0,  ///< match:SL=1, inv->A      non-match:SL=0, inv->B
    Action001 = 1,  ///< match:SL=1, inv->A      non-match:do nothing
    Action010 = 2,  ///< match:do nothing        non-match:SL=0, inv->B
    Action011 = 3,  ///< match:SL=!, (A->B,B->A) non-match:do nothing
    Action100 = 4,  ///< match:SL=0, inv->B      non-match:SL=1, inv->A
    Action101 = 5,  ///< match:SL=0, inv->B      non-match:do nothing
    Action110 = 6,  ///< match:do nothing        non-match:SL=1, inv->A
    Action111 = 7,  ///< match:do nothing        non-match:SL=!, (A->B, B->A)
};

enum SelectMemoryBank
{
    SelectFileType = 0,
    SelectEPC      = 1,
    SelectTID      = 2,
    SelectFile0    = 3,
};

enum SelectType
{
    SelectAll         = 0,
    SelectAll2        = 1,
    SelectNotAsserted = 2,
    SelectAsserted    = 3,
};

enum MemoryBank
{
    Reserved = 0,
    EPC      = 1,
    TID      = 2,
    User     = 3,
};

enum TagErrorCode
{
    Other                  = 0,
    NotSupported           = 1,
    InsufficientPrivileges = 2,
    MemoryOverrun          = 3,
    MemoryLocked           = 4,
    CryptoSuite            = 5,
    CommandNotEncapsulated = 6,
    ResponseBufferOverflow = 7,
    SecurityTimeout        = 8,
    InsufficientPower      = 11,
    NonSpecific            = 15,
    NoError                = 16,
};

/**
 * @enum ResponseType
 * Gen2 response types as defined in the Gen2TxnControls register.
 * To support in_process select "Delayed".
 */
enum ResponseType
{
    None_     = 0x0,
    Immediate = 0x1,
    Delayed   = 0x2,
    InProcess = 0x3
};

/**
 * @struct BitSpan
 * Defines a mutable location of contiguous memory, organized as bits.
 * Each byte in data is utilized, therefore 8 bits from length exist in
 * each byte. The bits and bytes are organized LSB first. This means
 * that bit 0 of byte 0 is the first bit, bit 1 of byte 0 is next, etc.
 */
struct BitSpan
{
    uint8_t* data;    ///< The pointer to beginning of the memory location.
    size_t   length;  ///< The number of bits in the span.
};

/**
 * @struct Gen2CommandSpec
 * A structure allowing the CSDK to parse Gen2 commands.
 *
 * @param command The command type being used.
 * @param args    Specifies the aruments of the given command type.
 *                These take the form of the CommandNameArgs.
 *
 * EX: SelectCommandArgs. This is then accompanied by the command field
 * which lets the code know how to interpret the void* casted data.
 */
struct Gen2CommandSpec
{
    enum Gen2Command command;
    void*            args;
};

/**
 * @struct Gen2Reply
 * A decoded gen2 command from a raw reply. Used in decode_reply() in
 * gen2_commands.c
 *
 * @param reply              Populated with the command the tag is replying to.
 * @param error_code         Shows if a generic gen2 error occurred. These are
 *                           the generic gen2 errors specified in the gen2
 *                           spec under Annex I section I.1.
 * @param data               The response data with the header bit stripped
 *                           off. The header shows the error, so after being
 *                           parsed, this is discarded.
 * @param transaction_status Gives some additional error information which
 *                           can be garnered from the Ex10 device modem.
 */
struct Gen2Reply
{
    enum Gen2Command           reply;
    enum TagErrorCode          error_code;
    uint16_t*                  data;
    enum Gen2TransactionStatus transaction_status;
};

/**
 * Gen2 commands which can be sent to tags.
 */
struct SelectCommandArgs
{
    enum SelectTarget     target;
    enum SelectAction     action;
    enum SelectMemoryBank memory_bank;
    uint32_t              bit_pointer;
    uint8_t               bit_count;
    struct BitSpan*       mask;
    bool                  truncate;
} __attribute__((aligned(4)));

struct ReadCommandArgs
{
    enum MemoryBank memory_bank;
    uint32_t        word_pointer;
    uint8_t         word_count;
} __attribute__((aligned(4)));

struct MarginReadCommandArgs
{
    enum MemoryBank memory_bank;
    uint32_t        bit_pointer;
    uint8_t         bit_length;
    struct BitSpan* mask;
} __attribute__((aligned(4)));

struct WriteCommandArgs
{
    enum MemoryBank memory_bank;
    uint32_t        word_pointer;
    uint16_t        data;
} __attribute__((aligned(4)));

struct KillCommandArgs
{
    uint16_t password;
} __attribute__((aligned(4)));

struct LockCommandArgs
{
    bool kill_password_read_write_mask;
    bool kill_password_permalock_mask;
    bool access_password_read_write_mask;
    bool access_password_permalock_mask;
    bool epc_memory_write_mask;
    bool epc_memory_permalock_mask;
    bool tid_memory_write_mask;
    bool tid_memory_permalock_mask;
    bool file_0_memory_write_mask;
    bool file_0_memory_permalock_mask;
    bool kill_password_read_write_lock;
    bool kill_password_permalock;
    bool access_password_read_write_lock;
    bool access_password_permalock;
    bool epc_memory_write_lock;
    bool epc_memory_permalock;
    bool tid_memory_write_lock;
    bool tid_memory_permalock;
    bool file_0_memory_write_lock;
    bool file_0_memory_permalock;
} __attribute__((aligned(4)));

struct AccessCommandArgs
{
    uint16_t password;
} __attribute__((aligned(4)));

struct BlockWriteCommandArgs
{
    enum MemoryBank memory_bank;
    uint32_t        word_pointer;
    uint8_t         word_count;
    struct BitSpan* data;
} __attribute__((aligned(4)));

struct BlockPermalockCommandArgs
{
    enum BlockPermalockReadLock read_lock;
    enum MemoryBank             memory_bank;
    uint32_t                    block_pointer;
    uint8_t                     block_range;
    struct BitSpan*             mask;  ///< For the Read case, pass pointer
                                       ///< to a valid BitSpan with length=0
} __attribute__((aligned(4)));

struct AuthenticateCommandArgs
{
    bool            send_rep;
    bool            inc_rep_len;
    uint8_t         csi;
    uint16_t        length;
    struct BitSpan* message;
    uint16_t        rep_len_bits;  ///< if send_rep is True, this is the
                                   ///< expected number of bits in the tag's
                                   ///< response (length in bits of 'Response'
                                   ///< field in the In-process reply packet).
} __attribute__((aligned(4)));

/**
 * Gen2 replies which are received by the device.
 */
struct AccessCommandReply
{
    uint16_t tag_handle;
    uint8_t  response_crc[2];
} __attribute__((aligned(4)));
static_assert(sizeof(struct AccessCommandReply) == 4,
              "Size of AccessCommandResponse type incorrect");

struct KillCommandReply
{
    uint16_t tag_handle;
    uint16_t response_crc;
} __attribute__((aligned(4)));
static_assert(sizeof(struct KillCommandReply) == 4,
              "Size of KillCommandReply type incorrect");

struct DelayedReply
{
    uint16_t tag_handle;
    uint16_t response_crc;
} __attribute__((aligned(4)));
static_assert(sizeof(struct DelayedReply) == 4,
              "Size of DelayedReply type incorrect");

/**
 * An array of Gen2TxnControlsFields structs which are used as initial template
 * information by the transaction_config function.
 */
// clang-format off
static const struct Gen2TxnControlsFields transaction_configs[] = {
    /* Select */
    {.response_type   = None_,
     .has_header_bit  = false,
     .use_cover_code  = false,
     .append_handle   = false,
     .append_crc16    = true,
     .is_kill_command = false,
     .Reserved0       = 0u,
     .rx_length       = 0u},
    /* Read */
    {.response_type   = Immediate,
     .has_header_bit  = true,
     .use_cover_code  = false,
     .append_handle   = true,
     .append_crc16    = true,
     .is_kill_command = false,
     .Reserved0       = 0u,
     .rx_length       = 33u},
    /* Write */
    {.response_type   = Delayed,
     .has_header_bit  = true,
     .use_cover_code  = true,
     .append_handle   = true,
     .append_crc16    = true,
     .is_kill_command = false,
     .Reserved0       = 0u,
     .rx_length       = 33u},
    /* Kill_1 */
    {.response_type   = Immediate,
     .has_header_bit  = false,
     .use_cover_code  = false,      // Taken care of by is_kill_command
     .append_handle   = true,
     .append_crc16    = true,
     .is_kill_command = true,
     .Reserved0       = 0u,
     .rx_length       = 32u},
    /* Kill_2 */
     {.response_type   = Delayed,
      .has_header_bit  = true,
      .use_cover_code  = false,     // Taken care of by is_kill_command
      .append_handle   = true,
      .append_crc16    = true,
      .is_kill_command = true,
      .Reserved0       = 0u,
      .rx_length       = 33u},
    /* Lock */
    {.response_type   = Delayed,
     .has_header_bit  = true,
     .use_cover_code  = false,
     .append_handle   = true,
     .append_crc16    = true,
     .is_kill_command = false,
     .Reserved0       = 0u,
     .rx_length       = 33u},
    /* Access */
    {.response_type   = Immediate,
     .has_header_bit  = false,
     .use_cover_code  = true,
     .append_handle   = true,
     .append_crc16    = true,
     .is_kill_command = false,
     .Reserved0       = 0u,
     .rx_length       = 32u},
    /* BlockWrite */
    {.response_type   = Delayed,
     .has_header_bit  = true,
     .use_cover_code  = false,
     .append_handle   = true,
     .append_crc16    = true,
     .is_kill_command = false,
     .Reserved0       = 0u,
     .rx_length       = 33u},
    /* BlockPermalock */
    {.response_type   = Delayed,
     .has_header_bit  = true,
     .use_cover_code  = false,
     .append_handle   = true,
     .append_crc16    = true,
     .is_kill_command = false,
     .Reserved0       = 0u,
     .rx_length       = 33u},
    /* Authenticate */
    {.response_type   = InProcess,
     .has_header_bit  = false,
     .use_cover_code  = false,
     .append_handle   = true,
     .append_crc16    = true,
     .is_kill_command = false,
     .Reserved0       = 0u,
     .rx_length       = 41u},
    /* MarginRead */
    {.response_type   = Immediate,
     .has_header_bit  = true,
     .use_cover_code  = false,
     .append_handle   = true,
     .append_crc16    = true,
     .is_kill_command = false,
     .Reserved0       = 0u,
     .rx_length       = 33u},
};
// clang-format on

/**
 * @struct Ex10Gen2Commands
 * Gen2 commands encoder/decoder interface.
 */
struct Ex10Gen2Commands
{
    /**
     * Encodes a Gen2 command for use in the Gen2TxBuffer.
     *
     * @param cmd_spec A struct containing:
     *                 1) command - The command type as an enum  Gen2Command
     *                 2) args - A pointer to an arg struct for the specified
     *                    command, which contains values for the fields in
     *                    the CommandEncoder table.
     * @param [out] encoded_command A bit span containing info about the
     *                              encoded command. This contains the length
     *                              of the command and data for the
     *                              left-justified Gen2 command.
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     * @note For select mask only: Everything is loaded in left-justified.
     * This means the bytes are loaded in via lsb, but the bits
     * are loaded in left to right as well, making them bitwise msb.
     * This was done for reader clarity to make it look like a
     * bitstream
     * EX:
     * load_mask = 0xa2f5
     * load_bits = 12
     * bits_loaded = 0xa2f
     *
     * load_mask = 0x10
     * load_bits = 4
     * bits_loaded = 0b0001
     */
    struct Ex10Result (*encode_gen2_command)(
        const struct Gen2CommandSpec* cmd_spec,
        struct BitSpan*               encoded_command);

    /**
     * Decodes an encoded Gen2 command. Takes in an encoded command via a
     * BitSpan and create populates a Gen2CommandSpec.
     *
     * @param [out] cmd_spec A struct containing:
     *                 1) command - The command type as an enum  Gen2Command
     *                 2) args - A pointer to an arg struct for the specified
     *                    command, which contains values for the fields in
     *                    the CommandEncoder table.
     * @param encoded_command A bit span containing info about the
     *                        encoded command. This contains the length
     *                        of the command and data for the left-justified
     *                        Gen2 command.
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*decode_gen2_command)(
        struct Gen2CommandSpec* cmd_spec,
        const struct BitSpan*   encoded_command);

    /**
     * Decodes a Gen2 command reply into a Gen2Reply struct.
     * @note: The data from the reply is placed into the 'data' field of the
     * decoded_reply. This data is everything after the header bit. Normally, a
     * reply contains 1 bit of header followed by data. The Ex10 device looks
     * for an error in the header. If it sees the error bit, that bit is
     * stored by itself in byte 0 of the raw data from the device. All data
     * after this is stored starting at byte 1 of the reply array. If this tag's
     * reply does not contain a header bit, the data will start from byte 0.
     * This function checks if a tag reply is expected to contain a header bit
     * and based on this knows whether the rest of the reply data starts at byte
     * 0 or 1 of the raw data from the device. The appropriate offset is chosen
     * and placed into the decoded_reply.data field.
     *
     * @param command       The Gen2 command that caused this reply.
     * @param gen2_pkt      A Gen2Transaction packet parsed using the packet
     *                      parser.
     * @param decoded_reply A Gen2Reply struct pointer to store decoded output.
     * @return bool false if the command has no known reply or if an error
     * is parsed in the return (showing the tag reported back and error).
     */
    bool (*decode_reply)(enum Gen2Command              command,
                         const struct EventFifoPacket* gen2_pkt,
                         struct Gen2Reply*             decoded_reply);

    /**
     * Checks the Gen2Reply for error and prints the error message.
     *
     * @param reply A Gen2Reply.
     * @return bool true if the reply has an error.
     */
    bool (*check_error)(struct Gen2Reply reply);

    /**
     * ex10_print the given Gen2Reply.
     *
     * @param reply A Gen2Reply.
     */
    void (*print_reply)(struct Gen2Reply reply);

    /**
     * Determine the Gen2TxnControls register settings needed for this command.
     * @param[in]  cmd_spec A command spec used with 'encode_command'.
     * @param[out] txn_control A Gen2TxnControlsFields struct which maps
     * directly to the Gen2TxnControls register.
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*get_gen2_tx_control_config)(
        const struct Gen2CommandSpec* cmd_spec,
        struct Gen2TxnControlsFields* txn_control);

    /**
     * EBV bit encoding/decoding is detailed in the gen2 spec under Annex A.
     *
     * @param  value The value to get the ebv bit count for
     * @return The bit count if the value were to be encoded.
     */
    size_t (*get_ebv_bit_len)(size_t value);

    /**
     * Packs in bits using least significant byte and least significant bit
     * Most fields of a gen2 command utilize least significant bit
     */
    size_t (*bit_pack)(uint8_t* encoded,
                       size_t   bit_offset,
                       uint32_t data,
                       size_t   bit_count);

    /**
     * EBV bit encoding/decoding is detailed in the gen2 spec under Annex A.
     *
     * @param  encoded_command The encoded command thus far.
     * @param  start_length    The starting bit length to pack more onto.
     * @param  value           The value to encode and pack.
     * @return Returns the new starting bit length post encoding.
     */
    size_t (*bit_pack_ebv)(uint8_t* encoded_command,
                           size_t   start_length,
                           size_t   value);

    const uint8_t* (*bit_unpack)(const uint8_t* cmd,
                                 size_t         start_length,
                                 size_t         bit_len);
    uint32_t (*bit_unpack_ebv)(const uint8_t* cmd, size_t byte_len);

    const uint8_t* (*bit_unpack_msb)(const uint8_t* cmd,
                                     size_t         start_length,
                                     size_t         bit_len);

    uint32_t (*ebv_length_decode)(const uint8_t* cmd, uint32_t curr_bit_len);

    /**
     * Convert a pair of little endian bytes to uint16.
     */
    uint16_t (*le_bytes_to_uint16)(void const* void_ptr);
};

struct Ex10Gen2Commands const* get_ex10_gen2_commands(void);

#ifdef __cplusplus
}
#endif
