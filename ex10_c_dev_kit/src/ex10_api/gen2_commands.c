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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "board/ex10_osal.h"
#include "ex10_api/byte_span.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/ex10_gen2_reply_string.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/gen2_commands.h"

static_assert(sizeof(bool) == 1u, "Invalid bool size");

static size_t bit_pack(uint8_t* encoded,
                       size_t   bit_offset,
                       uint32_t data,
                       size_t   bit_count)
{
    uint8_t byte_offset        = (uint8_t)(bit_offset - (bit_offset % 8u)) / 8u;
    uint8_t bit_start          = (uint8_t)(bit_offset % 8u);
    uint8_t bits_in_first_byte = 8u - bit_start;
    size_t  total_bits         = bit_offset;

    if (data > (1u << bit_count) - 1)
    {
        return 0;
    }

    // more bits remain than fit in the first byte
    // if bits_in_first_byte == 0, this is the same as the while below
    // if > 0, it means that we started in the middle of a byte
    if (bit_count > bits_in_first_byte)
    {
        uint8_t  rshift_bits        = (uint8_t)(bit_count - bits_in_first_byte);
        uint32_t data_in_first_byte = data >> rshift_bits;
        encoded[byte_offset] |= data_in_first_byte;
        // move forward and denote we added the bits
        bit_count -= bits_in_first_byte;
        byte_offset++;
        bit_start = 0;
        total_bits += bits_in_first_byte;
    }
    // we can fit in a whole byte
    while (bit_count >= 8u)
    {
        uint8_t const rshift_bits = (uint8_t)(bit_count - 8u);
        encoded[byte_offset]      = (uint8_t)(data >> rshift_bits);
        // move forward and denote we added the bits
        bit_count -= 8u;
        byte_offset++;
        total_bits += 8u;
    }
    // shove in the rest of the bits left shifted into the next byte
    if (bit_count > 0)
    {
        uint8_t const lshift_bits = (uint8_t)((8u - bit_count) - bit_start);
        encoded[byte_offset] |= (data << lshift_bits) & 0xFF;
        total_bits += bit_count;
    }
    return total_bits;
}

// Pack in a gen2 command using most significant bit.
// NOTE: This function is used for a bit-stream packing, aka most significant
// bit. This is therefore only useful for variable length fields in a gen2
// command. All other fields are least significant bit.
// NOTE: This function is only used for the final bit packing of a bit stream
// (the last sub-8 bits). Above that, the bitstream will be packed byte by
// byte, using the lsb bit pack (since no bit shifts are needed).
static size_t bit_pack_msb_bits(uint8_t* encoded,
                                size_t   bit_offset,
                                uint32_t data,
                                size_t   bit_count)
{
    // Function is only to be used for packing most significant bits when under
    // 8 bits. Otherwise pack in by byte.
    if (bit_count >= 8u)
    {
        return 0;
    }

    // Ensure the msb bits fit
    if ((data >> (8u - bit_count)) > (1u << bit_count) - 1u)
    {
        return 0;
    }

    uint8_t byte_offset        = (uint8_t)(bit_offset - (bit_offset % 8u)) / 8u;
    uint8_t bit_start          = (uint8_t)(bit_offset % 8u);
    uint8_t bits_in_first_byte = 8u - bit_start;
    size_t  total_bits         = bit_offset;
    uint8_t decoded_bits       = 0u;

    // more bits remain than fit in the first byte
    // if bits_in_first_byte == 0, this is the same as the while below
    // if > 0, it means that we started in the middle of a byte
    if (bit_count > bits_in_first_byte)
    {
        uint8_t  rshift_bits        = 8u - bits_in_first_byte;
        uint32_t data_in_first_byte = data >> rshift_bits;

        encoded[byte_offset] |= data_in_first_byte;
        // move forward and denote we added the bits
        bit_count -= bits_in_first_byte;
        byte_offset++;
        bit_start = 0;
        total_bits += bits_in_first_byte;
        decoded_bits += bits_in_first_byte;
    }
    // shove in the rest of the bits starting with most significant bit
    if (bit_count > 0)
    {
        // if bit count is x, create a mask for top x bits
        uint8_t const top_bit_mask = (uint8_t) ~((1u << (8u - bit_count)) - 1u);
        // shift the data first if some bits were placed in the previous encoded
        // byte now with the bits remaining for this byte, mask the top bits
        uint8_t const masked_data = (data << decoded_bits) & top_bit_mask;

        // shift the remaining encode data to the right in the case that we are
        // not starting at bit 0
        encoded[byte_offset] |= masked_data >> bit_start;
        total_bits += bit_count;
    }
    return total_bits;
}

static size_t get_ebv_bit_len(size_t value)
{
    size_t over_7_counter = 0;
    size_t total_bits     = 0;
    // Check for 7 bit overflow since the top bit of a byte in EBV is a flag
    while (value > 0x7F)
    {
        over_7_counter++;
        value -= 0x80;
    }
    if (over_7_counter)
    {
        // There is a higher byte that needs packing
        total_bits = get_ebv_bit_len(over_7_counter);
    }

    return (total_bits + 8);
}

// This recursive call is a must for the EBV due to the complications
// of rolling out a function which needs to be extensible.
// Due to the nature of Gen2, the MSbyte must be appended to the
// command first, which means doing the full encoding before appending.
// The recursive depth is based on the size of the value you start with.
// Since EBV uses the top bit to encode if another byte follows, each byte
// and thus each recursion step encodes 7 bits of the final value.
// This means with x recursive index, a 1 in that index can be found with
// value = (ebv[x - 1] & 0x7F) * (1 << (7 * (x - 1)))
// With 2 bytes / 2 steps, a 1 in the second index will give
// 10000001 00000000          -> 128
// With 3...
// 10000001 10000000 00000000 -> 16384
// This can be extrapolated to see how deep the recursion will go for your given
// value.
static size_t bit_pack_ebv_recursive(uint8_t* encoded_command,
                                     size_t   start_length,
                                     size_t   value,
                                     bool     first_iter)
{
    size_t over_7_counter = 0;
    // Check for 7 bit overflow since the top bit of a byte in EBV is a flag
    while (value > 0x7F)
    {
        over_7_counter++;
        value -= 0x80;
    }
    if (over_7_counter)
    {
        // There is a higher byte that needs packing first
        start_length = bit_pack_ebv_recursive(
            encoded_command, start_length, over_7_counter, false);
    }

    // The bytes corresponding to higher multiples get an extension bit
    // 01111111                   -> 127
    // 10000001 00000000          -> 128
    // 11111111 01111111          -> 16383
    // 10000001 10000000 00000000 -> 16384
    // If in the first iteration, this is the lowest remainder and does not need
    // an extension bit.
    if (!first_iter)
    {
        value |= 0x80;
    }

    // all higher bytes are packed, so pack the remaining value
    start_length = bit_pack(encoded_command, start_length, value, 8u);
    return start_length;
}

// bit_pack_ebv EX: 3460 = 0xD84
// while greater than 0x7f will subtract until remainder is 4
// over_7_counter = 27
// we enter the if, recursive call on the 27
// -> 27 is not > 0x7F
// -> over_7_counter is 0
// -> since the 27 is not the first iter, it gets or'ed
// -> we bit pack 0x9B as 8 bits, which is the MSbyte
// return from recursive call with the new start_length
// the first iter, so no or'ing
// bit pack in 0x04 as 8 bits, which is the LSbyte.
static size_t bit_pack_ebv(uint8_t* encoded_command,
                           size_t   start_length,
                           size_t   value)
{
    return bit_pack_ebv_recursive(encoded_command, start_length, value, true);
}

static size_t bit_pack_from_pointer(uint8_t*       encoded_command,
                                    size_t         start_length,
                                    const uint8_t* data,
                                    size_t         bit_len)
{
    uint8_t curr_byte = 0;

    while (bit_len >= 8u)
    {
        start_length =
            bit_pack(encoded_command, start_length, data[curr_byte], 8u);
        curr_byte++;
        bit_len -= 8u;
    }
    if (bit_len > 0)
    {
        // Note: Packing from a pointer is used for packing a variable length
        // field in a gen2 command. Variable length fields operate on a most
        // significant bit principle unlike other fields.
        start_length = bit_pack_msb_bits(
            encoded_command, start_length, data[curr_byte], bit_len);
    }
    return start_length;
}

static uint32_t bit_unpack_ebv(const uint8_t* cmd, size_t byte_len)
{
    // Always and the top bit out since that is the extension bit and not the
    // value
    uint32_t unpack_val = 0;
    // 01111111                   -> 127
    // 10000001 00000000          -> 128
    // 11111111 01111111          -> 16383
    // 10000001 10000000 00000000 -> 16384
    for (uint8_t idx = (uint8_t)byte_len; idx > 0; idx--)
    {
        unpack_val += (cmd[idx - 1] & 0x7F) * (1 << (7 * (idx - 1)));
    }
    return unpack_val;
}

static uint8_t const* bit_unpack_msb(const uint8_t* cmd,
                                     size_t         start_length,
                                     size_t         bit_len)
{
    // NOTE: this will be overwritten. remember to copy the data
    static uint8_t unpack_buffer[TxCommandDecodeBufferSize];
    ex10_memzero(unpack_buffer, sizeof(unpack_buffer));

    uint8_t curr_byte    = 0u;
    uint8_t bit_start    = start_length % 8u;
    uint8_t decoded_bits = 0u;
    uint8_t byte_start   = (uint8_t)(start_length - bit_start) / 8u;

    while (bit_len > 0)
    {
        if (bit_start > 0)
        {
            uint8_t shifted_data = (uint8_t)(cmd[byte_start++] << bit_start);
            uint8_t bits_fit     = 8u - bit_start;
            if (bits_fit > bit_len)
            {
                uint8_t bits_over = (uint8_t)(bits_fit - bit_len);
                // mask data coming after our encode length
                uint8_t data_mask = (uint8_t) ~((1 << bits_over) - 1);
                shifted_data &= data_mask;
                bits_fit = (uint8_t)bit_len;
            }
            unpack_buffer[curr_byte] = shifted_data;
            bit_len -= bits_fit;
            decoded_bits = bits_fit;
            bit_start    = 0;
        }
        else
        {
            uint8_t shifted_data = cmd[byte_start] >> decoded_bits;
            uint8_t bits_fit     = 8u - decoded_bits;
            if (bits_fit == 8u)
            {
                byte_start++;
            }
            if (bits_fit > bit_len)
            {
                uint8_t bits_over = (uint8_t)(bits_fit - bit_len);
                // mask data coming after our encode length
                uint8_t data_mask = (uint8_t) ~((1 << bits_over) - 1);
                shifted_data &= data_mask;
                bits_fit = (uint8_t)bit_len;
            }
            unpack_buffer[curr_byte++] |= shifted_data;
            bit_len -= bits_fit;
            bit_start    = bits_fit % 8u;
            decoded_bits = 0;
        }
    }
    return unpack_buffer;
}

static uint8_t const* bit_unpack(const uint8_t* cmd,
                                 size_t         start_length,
                                 size_t         bit_len)
{
    // NOTE: this will be overwritten. remember to copy the data
    static uint8_t unpack_buffer[TxCommandDecodeBufferSize];
    ex10_memzero(unpack_buffer, sizeof(unpack_buffer));
    uint8_t decoded_bits = 0;

    while (bit_len > 0)
    {
        uint32_t final_bit    = start_length + bit_len;
        uint8_t  bit_end      = final_bit % 8u;
        uint8_t  highest_byte = (uint8_t)(final_bit - (final_bit % 8u)) / 8u;
        highest_byte -= ((final_bit % 8u) == 0) ? 1 : 0;

        uint8_t decode_idx = (decoded_bits - (decoded_bits % 8u)) / 8u;

        uint8_t       unpack_bits = 0;
        uint8_t const bottom_bits = 8u - (decoded_bits % 8u);
        uint8_t const bit_len_8   = (uint8_t)bit_len;

        if (bit_end != 0)
        {
            unpack_bits = (bit_end >= bit_len) ? bit_len_8 : bit_end;
        }
        else
        {
            unpack_bits = (bottom_bits >= bit_len) ? bit_len_8 : bottom_bits;
        }

        if (bit_end != 0)
        {
            unpack_buffer[decode_idx] |= (cmd[highest_byte] >> (8u - bit_end)) &
                                         ((1 << unpack_bits) - 1);
        }
        else
        {
            uint8_t data = cmd[highest_byte] & ((1 << unpack_bits) - 1);
            unpack_buffer[decode_idx] |= (data << (decoded_bits % 8u));
        }
        bit_len -= unpack_bits;
        decoded_bits += unpack_bits;
    }
    return unpack_buffer;
}

static uint32_t ebv_length_decode(const uint8_t* cmd, uint32_t curr_bit_len)
{
    uint8_t        ebv_bytes     = 1;
    uint8_t const* unpacked_data = bit_unpack(cmd, curr_bit_len, 8u);
    if (unpacked_data[0] & 0x80)
    {
        ebv_bytes     = 2;
        unpacked_data = bit_unpack(cmd, curr_bit_len + 8u, 8u);
        if (unpacked_data[1] & 0x80)
        {
            ebv_bytes = 3;
        }
    }
    return ebv_bytes;
}

static uint16_t le_bytes_to_uint16(void const* void_ptr)
{
    uint8_t const* bytes = (uint8_t const*)void_ptr;
    uint16_t       value = 0u;
    value |= bytes[0u];
    value <<= 8u;
    value |= bytes[1u];
    return value;
}

// clang-format off
// IPJ_autogen | gen_c_gen2_encode {

static void select_command_encode(
    const void*      args,
    struct BitSpan* return_command)
{
    const uint8_t* arg_base        = args;
    const uint8_t* arg_ptr         = args;
    uint8_t* encoded_command = return_command->data;


    // Pull out the byte span data for for the variable length field mask
    const struct SelectCommandArgs* span_args = (const struct SelectCommandArgs*)arg_base;
    if (span_args->mask == NULL)
    {
        // set encoded command length to 0 to mark an error
        return_command->length = 0;
        return;
    }
    const struct BitSpan* span_data = span_args->mask;
    size_t total_span_bits = span_data->length;
    if ((total_span_bits > 0) && (span_data->data == NULL))
    {
        // set encoded command length to 0 to mark an error
        return_command->length = 0;
        return;
    }

    // Get the total length of the command to ensure the encode buffer is completely cleared
    uint32_t final_bit_len =
    4 + // command
    3 + // target
    3 + // action
    2 + // memory_bank
    get_ebv_bit_len(((const struct SelectCommandArgs*)arg_base)->bit_pointer) + // bit_pointer
    8 + // bit_count
    total_span_bits + // mask
    1 + // truncate
    0;

    uint8_t final_byte_len = (uint8_t)(final_bit_len - (final_bit_len % 8u)) / 8u;
    final_byte_len += ((final_bit_len % 8u) > 0u) ? 1u : 0u;
    ex10_memzero(encoded_command, final_byte_len);

    // Begin encoding
    size_t   curr_bit_len    = 0u;

    // Adding field command
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0x0a, 4);
    // Adding field target
    arg_ptr      = arg_base + offsetof(struct SelectCommandArgs, target);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 3);
    // Adding field action
    arg_ptr      = arg_base + offsetof(struct SelectCommandArgs, action);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 3);
    // Adding field memory_bank
    arg_ptr      = arg_base + offsetof(struct SelectCommandArgs, memory_bank);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 2);
    // Adding field bit_pointer
    arg_ptr      = arg_base + offsetof(struct SelectCommandArgs, bit_pointer);
    curr_bit_len = bit_pack_ebv(encoded_command, curr_bit_len, ((const uint32_t*)arg_ptr)[0]);
    // Adding field bit_count
    arg_ptr      = arg_base + offsetof(struct SelectCommandArgs, bit_count);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 8);
    // Adding field mask

    curr_bit_len = bit_pack_from_pointer(encoded_command, curr_bit_len, span_data->data, total_span_bits);
    // Adding field truncate
    arg_ptr      = arg_base + offsetof(struct SelectCommandArgs, truncate);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);

    // If the final bit length is not what we expected, set the length to 0
    return_command->length = (curr_bit_len != final_bit_len) ? 0 : curr_bit_len;
}

static void read_command_encode(
    const void*      args,
    struct BitSpan* return_command)
{
    const uint8_t* arg_base        = args;
    const uint8_t* arg_ptr         = args;
    uint8_t* encoded_command = return_command->data;



    // Get the total length of the command to ensure the encode buffer is completely cleared
    uint32_t final_bit_len =
    8 + // command
    2 + // memory_bank
    get_ebv_bit_len(((const struct ReadCommandArgs*)arg_base)->word_pointer) + // word_pointer
    8 + // word_count
    0;

    uint8_t final_byte_len = (uint8_t)(final_bit_len - (final_bit_len % 8u)) / 8u;
    final_byte_len += ((final_bit_len % 8u) > 0u) ? 1u : 0u;
    ex10_memzero(encoded_command, final_byte_len);

    // Begin encoding
    size_t   curr_bit_len    = 0u;

    // Adding field command
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0xc2, 8);
    // Adding field memory_bank
    arg_ptr      = arg_base + offsetof(struct ReadCommandArgs, memory_bank);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 2);
    // Adding field word_pointer
    arg_ptr      = arg_base + offsetof(struct ReadCommandArgs, word_pointer);
    curr_bit_len = bit_pack_ebv(encoded_command, curr_bit_len, ((const uint32_t*)arg_ptr)[0]);
    // Adding field word_count
    arg_ptr      = arg_base + offsetof(struct ReadCommandArgs, word_count);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 8);

    // If the final bit length is not what we expected, set the length to 0
    return_command->length = (curr_bit_len != final_bit_len) ? 0 : curr_bit_len;
}

static void write_command_encode(
    const void*      args,
    struct BitSpan* return_command)
{
    const uint8_t* arg_base        = args;
    const uint8_t* arg_ptr         = args;
    uint8_t* encoded_command = return_command->data;



    // Get the total length of the command to ensure the encode buffer is completely cleared
    uint32_t final_bit_len =
    8 + // command
    2 + // memory_bank
    get_ebv_bit_len(((const struct WriteCommandArgs*)arg_base)->word_pointer) + // word_pointer
    16 + // data
    0;

    uint8_t final_byte_len = (uint8_t)(final_bit_len - (final_bit_len % 8u)) / 8u;
    final_byte_len += ((final_bit_len % 8u) > 0u) ? 1u : 0u;
    ex10_memzero(encoded_command, final_byte_len);

    // Begin encoding
    size_t   curr_bit_len    = 0u;

    // Adding field command
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0xc3, 8);
    // Adding field memory_bank
    arg_ptr      = arg_base + offsetof(struct WriteCommandArgs, memory_bank);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 2);
    // Adding field word_pointer
    arg_ptr      = arg_base + offsetof(struct WriteCommandArgs, word_pointer);
    curr_bit_len = bit_pack_ebv(encoded_command, curr_bit_len, ((const uint32_t*)arg_ptr)[0]);
    // Adding field data
    arg_ptr      = arg_base + offsetof(struct WriteCommandArgs, data);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *((const uint16_t*)arg_ptr), 16);

    // If the final bit length is not what we expected, set the length to 0
    return_command->length = (curr_bit_len != final_bit_len) ? 0 : curr_bit_len;
}

static void kill_command_encode(
    const void*      args,
    struct BitSpan* return_command)
{
    const uint8_t* arg_base        = args;
    const uint8_t* arg_ptr         = args;
    uint8_t* encoded_command = return_command->data;



    // Get the total length of the command to ensure the encode buffer is completely cleared
    uint32_t final_bit_len =
    8 + // command
    16 + // password
    3 + // rfu
    0;

    uint8_t final_byte_len = (uint8_t)(final_bit_len - (final_bit_len % 8u)) / 8u;
    final_byte_len += ((final_bit_len % 8u) > 0u) ? 1u : 0u;
    ex10_memzero(encoded_command, final_byte_len);

    // Begin encoding
    size_t   curr_bit_len    = 0u;

    // Adding field command
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0xc4, 8);
    // Adding field password
    arg_ptr      = arg_base + offsetof(struct KillCommandArgs, password);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *((const uint16_t*)arg_ptr), 16);
    // Adding field rfu
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0, 3);

    // If the final bit length is not what we expected, set the length to 0
    return_command->length = (curr_bit_len != final_bit_len) ? 0 : curr_bit_len;
}

static void lock_command_encode(
    const void*      args,
    struct BitSpan* return_command)
{
    const uint8_t* arg_base        = args;
    const uint8_t* arg_ptr         = args;
    uint8_t* encoded_command = return_command->data;



    // Get the total length of the command to ensure the encode buffer is completely cleared
    uint32_t final_bit_len =
    8 + // command
    1 + // kill_password_read_write_mask
    1 + // kill_password_permalock_mask
    1 + // access_password_read_write_mask
    1 + // access_password_permalock_mask
    1 + // epc_memory_write_mask
    1 + // epc_memory_permalock_mask
    1 + // tid_memory_write_mask
    1 + // tid_memory_permalock_mask
    1 + // file_0_memory_write_mask
    1 + // file_0_memory_permalock_mask
    1 + // kill_password_read_write_lock
    1 + // kill_password_permalock
    1 + // access_password_read_write_lock
    1 + // access_password_permalock
    1 + // epc_memory_write_lock
    1 + // epc_memory_permalock
    1 + // tid_memory_write_lock
    1 + // tid_memory_permalock
    1 + // file_0_memory_write_lock
    1 + // file_0_memory_permalock
    0;

    uint8_t final_byte_len = (uint8_t)(final_bit_len - (final_bit_len % 8u)) / 8u;
    final_byte_len += ((final_bit_len % 8u) > 0u) ? 1u : 0u;
    ex10_memzero(encoded_command, final_byte_len);

    // Begin encoding
    size_t   curr_bit_len    = 0u;

    // Adding field command
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0xc5, 8);
    // Adding field kill_password_read_write_mask
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, kill_password_read_write_mask);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field kill_password_permalock_mask
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, kill_password_permalock_mask);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field access_password_read_write_mask
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, access_password_read_write_mask);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field access_password_permalock_mask
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, access_password_permalock_mask);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field epc_memory_write_mask
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, epc_memory_write_mask);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field epc_memory_permalock_mask
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, epc_memory_permalock_mask);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field tid_memory_write_mask
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, tid_memory_write_mask);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field tid_memory_permalock_mask
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, tid_memory_permalock_mask);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field file_0_memory_write_mask
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, file_0_memory_write_mask);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field file_0_memory_permalock_mask
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, file_0_memory_permalock_mask);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field kill_password_read_write_lock
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, kill_password_read_write_lock);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field kill_password_permalock
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, kill_password_permalock);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field access_password_read_write_lock
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, access_password_read_write_lock);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field access_password_permalock
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, access_password_permalock);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field epc_memory_write_lock
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, epc_memory_write_lock);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field epc_memory_permalock
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, epc_memory_permalock);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field tid_memory_write_lock
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, tid_memory_write_lock);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field tid_memory_permalock
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, tid_memory_permalock);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field file_0_memory_write_lock
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, file_0_memory_write_lock);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field file_0_memory_permalock
    arg_ptr      = arg_base + offsetof(struct LockCommandArgs, file_0_memory_permalock);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);

    // If the final bit length is not what we expected, set the length to 0
    return_command->length = (curr_bit_len != final_bit_len) ? 0 : curr_bit_len;
}

static void access_command_encode(
    const void*      args,
    struct BitSpan* return_command)
{
    const uint8_t* arg_base        = args;
    const uint8_t* arg_ptr         = args;
    uint8_t* encoded_command = return_command->data;



    // Get the total length of the command to ensure the encode buffer is completely cleared
    uint32_t final_bit_len =
    8 + // command
    16 + // password
    0;

    uint8_t final_byte_len = (uint8_t)(final_bit_len - (final_bit_len % 8u)) / 8u;
    final_byte_len += ((final_bit_len % 8u) > 0u) ? 1u : 0u;
    ex10_memzero(encoded_command, final_byte_len);

    // Begin encoding
    size_t   curr_bit_len    = 0u;

    // Adding field command
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0xc6, 8);
    // Adding field password
    arg_ptr      = arg_base + offsetof(struct AccessCommandArgs, password);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *((const uint16_t*)arg_ptr), 16);

    // If the final bit length is not what we expected, set the length to 0
    return_command->length = (curr_bit_len != final_bit_len) ? 0 : curr_bit_len;
}

static void block_write_command_encode(
    const void*      args,
    struct BitSpan* return_command)
{
    const uint8_t* arg_base        = args;
    const uint8_t* arg_ptr         = args;
    uint8_t* encoded_command = return_command->data;


    // Pull out the byte span data for for the variable length field data
    const struct BlockWriteCommandArgs* span_args = (const struct BlockWriteCommandArgs*)arg_base;
    if (span_args->data == NULL)
    {
        // set encoded command length to 0 to mark an error
        return_command->length = 0;
        return;
    }
    const struct BitSpan* span_data = span_args->data;
    size_t total_span_bits = span_data->length;
    if ((total_span_bits > 0) && (span_data->data == NULL))
    {
        // set encoded command length to 0 to mark an error
        return_command->length = 0;
        return;
    }

    // Get the total length of the command to ensure the encode buffer is completely cleared
    uint32_t final_bit_len =
    8 + // command
    2 + // memory_bank
    get_ebv_bit_len(((const struct BlockWriteCommandArgs*)arg_base)->word_pointer) + // word_pointer
    8 + // word_count
    total_span_bits + // data
    0;

    uint8_t final_byte_len = (uint8_t)(final_bit_len - (final_bit_len % 8u)) / 8u;
    final_byte_len += ((final_bit_len % 8u) > 0u) ? 1u : 0u;
    ex10_memzero(encoded_command, final_byte_len);

    // Begin encoding
    size_t   curr_bit_len    = 0u;

    // Adding field command
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0xc7, 8);
    // Adding field memory_bank
    arg_ptr      = arg_base + offsetof(struct BlockWriteCommandArgs, memory_bank);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 2);
    // Adding field word_pointer
    arg_ptr      = arg_base + offsetof(struct BlockWriteCommandArgs, word_pointer);
    curr_bit_len = bit_pack_ebv(encoded_command, curr_bit_len, ((const uint32_t*)arg_ptr)[0]);
    // Adding field word_count
    arg_ptr      = arg_base + offsetof(struct BlockWriteCommandArgs, word_count);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 8);
    // Adding field data

    curr_bit_len = bit_pack_from_pointer(encoded_command, curr_bit_len, span_data->data, total_span_bits);

    // If the final bit length is not what we expected, set the length to 0
    return_command->length = (curr_bit_len != final_bit_len) ? 0 : curr_bit_len;
}

static void block_permalock_command_encode(
    const void*      args,
    struct BitSpan* return_command)
{
    const uint8_t* arg_base        = args;
    const uint8_t* arg_ptr         = args;
    uint8_t* encoded_command = return_command->data;


    // Pull out the byte span data for for the variable length field mask
    const struct BlockPermalockCommandArgs* span_args = (const struct BlockPermalockCommandArgs*)arg_base;
    if (span_args->mask == NULL)
    {
        // set encoded command length to 0 to mark an error
        return_command->length = 0;
        return;
    }
    const struct BitSpan* span_data = span_args->mask;
    size_t total_span_bits = span_data->length;
    if ((total_span_bits > 0) && (span_data->data == NULL))
    {
        // set encoded command length to 0 to mark an error
        return_command->length = 0;
        return;
    }

    // Get the total length of the command to ensure the encode buffer is completely cleared
    uint32_t final_bit_len =
    8 + // command
    8 + // rfu
    1 + // read_lock
    2 + // memory_bank
    get_ebv_bit_len(((const struct BlockPermalockCommandArgs*)arg_base)->block_pointer) + // block_pointer
    8 + // block_range
    total_span_bits + // mask
    0;

    uint8_t final_byte_len = (uint8_t)(final_bit_len - (final_bit_len % 8u)) / 8u;
    final_byte_len += ((final_bit_len % 8u) > 0u) ? 1u : 0u;
    ex10_memzero(encoded_command, final_byte_len);

    // Begin encoding
    size_t   curr_bit_len    = 0u;

    // Adding field command
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0xc9, 8);
    // Adding field rfu
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0, 8);
    // Adding field read_lock
    arg_ptr      = arg_base + offsetof(struct BlockPermalockCommandArgs, read_lock);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field memory_bank
    arg_ptr      = arg_base + offsetof(struct BlockPermalockCommandArgs, memory_bank);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 2);
    // Adding field block_pointer
    arg_ptr      = arg_base + offsetof(struct BlockPermalockCommandArgs, block_pointer);
    curr_bit_len = bit_pack_ebv(encoded_command, curr_bit_len, ((const uint32_t*)arg_ptr)[0]);
    // Adding field block_range
    arg_ptr      = arg_base + offsetof(struct BlockPermalockCommandArgs, block_range);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 8);
    // Adding field mask

    curr_bit_len = bit_pack_from_pointer(encoded_command, curr_bit_len, span_data->data, total_span_bits);

    // If the final bit length is not what we expected, set the length to 0
    return_command->length = (curr_bit_len != final_bit_len) ? 0 : curr_bit_len;
}

static void authenticate_command_encode(
    const void*      args,
    struct BitSpan* return_command)
{
    const uint8_t* arg_base        = args;
    const uint8_t* arg_ptr         = args;
    uint8_t* encoded_command = return_command->data;


    // Pull out the byte span data for for the variable length field message
    const struct AuthenticateCommandArgs* span_args = (const struct AuthenticateCommandArgs*)arg_base;
    if (span_args->message == NULL)
    {
        // set encoded command length to 0 to mark an error
        return_command->length = 0;
        return;
    }
    const struct BitSpan* span_data = span_args->message;
    size_t total_span_bits = span_data->length;
    if ((total_span_bits > 0) && (span_data->data == NULL))
    {
        // set encoded command length to 0 to mark an error
        return_command->length = 0;
        return;
    }

    // Get the total length of the command to ensure the encode buffer is completely cleared
    uint32_t final_bit_len =
    8 + // command
    2 + // rfu
    1 + // send_rep
    1 + // inc_rep_len
    8 + // csi
    12 + // length
    total_span_bits + // message
    0;

    uint8_t final_byte_len = (uint8_t)(final_bit_len - (final_bit_len % 8u)) / 8u;
    final_byte_len += ((final_bit_len % 8u) > 0u) ? 1u : 0u;
    ex10_memzero(encoded_command, final_byte_len);

    // Begin encoding
    size_t   curr_bit_len    = 0u;

    // Adding field command
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0xd5, 8);
    // Adding field rfu
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0, 2);
    // Adding field send_rep
    arg_ptr      = arg_base + offsetof(struct AuthenticateCommandArgs, send_rep);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field inc_rep_len
    arg_ptr      = arg_base + offsetof(struct AuthenticateCommandArgs, inc_rep_len);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 1);
    // Adding field csi
    arg_ptr      = arg_base + offsetof(struct AuthenticateCommandArgs, csi);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 8);
    // Adding field length
    arg_ptr      = arg_base + offsetof(struct AuthenticateCommandArgs, length);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *((const uint16_t*)arg_ptr), 12);
    // Adding field message

    curr_bit_len = bit_pack_from_pointer(encoded_command, curr_bit_len, span_data->data, total_span_bits);

    // If the final bit length is not what we expected, set the length to 0
    return_command->length = (curr_bit_len != final_bit_len) ? 0 : curr_bit_len;
}

static void margin_read_command_encode(
    const void*      args,
    struct BitSpan* return_command)
{
    const uint8_t* arg_base        = args;
    const uint8_t* arg_ptr         = args;
    uint8_t* encoded_command = return_command->data;


    // Pull out the byte span data for for the variable length field mask
    const struct MarginReadCommandArgs* span_args = (const struct MarginReadCommandArgs*)arg_base;
    if (span_args->mask == NULL)
    {
        // set encoded command length to 0 to mark an error
        return_command->length = 0;
        return;
    }
    const struct BitSpan* span_data = span_args->mask;
    size_t total_span_bits = span_data->length;
    if ((total_span_bits > 0) && (span_data->data == NULL))
    {
        // set encoded command length to 0 to mark an error
        return_command->length = 0;
        return;
    }

    // Get the total length of the command to ensure the encode buffer is completely cleared
    uint32_t final_bit_len =
    16 + // command
    2 + // memory_bank
    get_ebv_bit_len(((const struct MarginReadCommandArgs*)arg_base)->bit_pointer) + // bit_pointer
    8 + // bit_length
    total_span_bits + // mask
    0;

    uint8_t final_byte_len = (uint8_t)(final_bit_len - (final_bit_len % 8u)) / 8u;
    final_byte_len += ((final_bit_len % 8u) > 0u) ? 1u : 0u;
    ex10_memzero(encoded_command, final_byte_len);

    // Begin encoding
    size_t   curr_bit_len    = 0u;

    // Adding field command
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, 0xe001, 16);
    // Adding field memory_bank
    arg_ptr      = arg_base + offsetof(struct MarginReadCommandArgs, memory_bank);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 2);
    // Adding field bit_pointer
    arg_ptr      = arg_base + offsetof(struct MarginReadCommandArgs, bit_pointer);
    curr_bit_len = bit_pack_ebv(encoded_command, curr_bit_len, ((const uint32_t*)arg_ptr)[0]);
    // Adding field bit_length
    arg_ptr      = arg_base + offsetof(struct MarginReadCommandArgs, bit_length);
    curr_bit_len = bit_pack(encoded_command, curr_bit_len, *arg_ptr, 8);
    // Adding field mask

    curr_bit_len = bit_pack_from_pointer(encoded_command, curr_bit_len, span_data->data, total_span_bits);

    // If the final bit length is not what we expected, set the length to 0
    return_command->length = (curr_bit_len != final_bit_len) ? 0 : curr_bit_len;
}


static enum Gen2Command decode_command_type(const struct BitSpan* return_command)
{
    uint8_t  const four_bit_id    = return_command->data[0] >> 4;
    uint8_t  const eight_bit_id   = return_command->data[0];
    uint16_t const sixteen_bit_id = le_bytes_to_uint16(&return_command->data[0]);

    // Start by checking for the 4 bit select
    if(four_bit_id == 0xA)
    {
        return Gen2Select;
    }
    else
    {
        switch(eight_bit_id)
        {
            case 0xc2:
                return Gen2Read;
                break;
            case 0xc3:
                return Gen2Write;
                break;
            case 0xc4:
                return Gen2Kill_1;
                break;
            case 0xc5:
                return Gen2Lock;
                break;
            case 0xc6:
                return Gen2Access;
                break;
            case 0xc7:
                return Gen2BlockWrite;
                break;
            case 0xc9:
                return Gen2BlockPermalock;
                break;
            case 0xd5:
                return Gen2Authenticate;
                break;
            default:
                switch(sixteen_bit_id)
                {
                    case 0xe001:
                        return Gen2MarginRead;
                        break;
                    default:
                        return _GEN2_COMMAND_MAX;
                    break;
                }
                return _GEN2_COMMAND_MAX;
            break;
        }
    }
    return _GEN2_COMMAND_MAX;
}

static void select_command_decode(
    void*      decoded_args,
    const struct BitSpan* encoded_command)
{
    struct SelectCommandArgs* decoded_command = (struct SelectCommandArgs*) decoded_args;
    size_t   curr_bit_len    = 0;
    uint8_t const* unpacked_data;
    // Skip command ID in decode
    curr_bit_len += 4;
    // Pulling field target
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 3);
    decoded_command->target = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 3;
    // Pulling field action
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 3);
    decoded_command->action = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 3;
    // Pulling field memory_bank
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 2);
    decoded_command->memory_bank = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 2;
    // Pulling EBV field bit_pointer
    uint8_t const ebv_byte_len = (uint8_t)ebv_length_decode(encoded_command->data, curr_bit_len);
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, ebv_byte_len*8);
    decoded_command->bit_pointer = bit_unpack_ebv(unpacked_data, ebv_byte_len);
    curr_bit_len += ebv_byte_len*8;
    // Pulling field bit_count
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 8);
    decoded_command->bit_count = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 8;
    // Pulling field mask
    // Variable bit decode uses the rest of the length -1 for truncate
    uint8_t const copy_bits = (uint8_t)(encoded_command->length - curr_bit_len - 1u);

    unpacked_data = bit_unpack_msb(encoded_command->data, curr_bit_len, copy_bits);
    // copy data over to the byte span
    uint8_t copy_bytes = (copy_bits - (copy_bits % 8u)) / 8u;
    copy_bytes += (copy_bits % 8u) ? 1u: 0u;

    static uint8_t variable_buffer[TxCommandDecodeBufferSize];
    ex10_memzero(variable_buffer, sizeof(variable_buffer));

    static struct BitSpan variable_data = {.data=variable_buffer, .length=0};
    // Point the variable data to this local store.
    // Note: This is overridden the next time it is used.
    decoded_command->mask = &variable_data;

    ex10_memcpy(
        variable_data.data, TxCommandDecodeBufferSize, unpacked_data, copy_bytes);
    variable_data.length = copy_bits;
    curr_bit_len += copy_bits;
    // Pulling field truncate
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->truncate = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    (void)curr_bit_len;  // ignore final curr_bit_len increment
}

static void read_command_decode(
    void*      decoded_args,
    const struct BitSpan* encoded_command)
{
    struct ReadCommandArgs* decoded_command = (struct ReadCommandArgs*) decoded_args;
    size_t   curr_bit_len    = 0;
    uint8_t const* unpacked_data;
    // Skip command ID in decode
    curr_bit_len += 8;
    // Pulling field memory_bank
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 2);
    decoded_command->memory_bank = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 2;
    // Pulling EBV field word_pointer
    uint8_t const ebv_byte_len = (uint8_t)ebv_length_decode(encoded_command->data, curr_bit_len);
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, ebv_byte_len*8);
    decoded_command->word_pointer = bit_unpack_ebv(unpacked_data, ebv_byte_len);
    curr_bit_len += ebv_byte_len*8;
    // Pulling field word_count
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 8);
    decoded_command->word_count = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 8;
    (void)curr_bit_len;  // ignore final curr_bit_len increment
}

static void write_command_decode(
    void*      decoded_args,
    const struct BitSpan* encoded_command)
{
    struct WriteCommandArgs* decoded_command = (struct WriteCommandArgs*) decoded_args;
    size_t   curr_bit_len    = 0;
    uint8_t const* unpacked_data;
    // Skip command ID in decode
    curr_bit_len += 8;
    // Pulling field memory_bank
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 2);
    decoded_command->memory_bank = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 2;
    // Pulling EBV field word_pointer
    uint8_t const ebv_byte_len = (uint8_t)ebv_length_decode(encoded_command->data, curr_bit_len);
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, ebv_byte_len*8);
    decoded_command->word_pointer = bit_unpack_ebv(unpacked_data, ebv_byte_len);
    curr_bit_len += ebv_byte_len*8;
    // Pulling field data
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 16);
    decoded_command->data = ((uint16_t const*)unpacked_data)[0];
    curr_bit_len += 16;
    (void)curr_bit_len;  // ignore final curr_bit_len increment
}

static void kill_command_decode(
    void*      decoded_args,
    const struct BitSpan* encoded_command)
{
    struct KillCommandArgs* decoded_command = (struct KillCommandArgs*) decoded_args;
    size_t   curr_bit_len    = 0;
    uint8_t const* unpacked_data;
    // Skip command ID in decode
    curr_bit_len += 8;
    // Pulling field password
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 16);
    decoded_command->password = ((uint16_t const*)unpacked_data)[0];
    curr_bit_len += 16;
    // Skip rfu in decode
    curr_bit_len += 3;
    (void)curr_bit_len;  // ignore final curr_bit_len increment
}

static void lock_command_decode(
    void*      decoded_args,
    const struct BitSpan* encoded_command)
{
    struct LockCommandArgs* decoded_command = (struct LockCommandArgs*) decoded_args;
    size_t   curr_bit_len    = 0;
    uint8_t const* unpacked_data;
    // Skip command ID in decode
    curr_bit_len += 8;
    // Pulling field kill_password_read_write_mask
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->kill_password_read_write_mask = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field kill_password_permalock_mask
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->kill_password_permalock_mask = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field access_password_read_write_mask
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->access_password_read_write_mask = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field access_password_permalock_mask
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->access_password_permalock_mask = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field epc_memory_write_mask
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->epc_memory_write_mask = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field epc_memory_permalock_mask
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->epc_memory_permalock_mask = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field tid_memory_write_mask
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->tid_memory_write_mask = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field tid_memory_permalock_mask
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->tid_memory_permalock_mask = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field file_0_memory_write_mask
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->file_0_memory_write_mask = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field file_0_memory_permalock_mask
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->file_0_memory_permalock_mask = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field kill_password_read_write_lock
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->kill_password_read_write_lock = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field kill_password_permalock
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->kill_password_permalock = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field access_password_read_write_lock
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->access_password_read_write_lock = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field access_password_permalock
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->access_password_permalock = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field epc_memory_write_lock
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->epc_memory_write_lock = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field epc_memory_permalock
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->epc_memory_permalock = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field tid_memory_write_lock
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->tid_memory_write_lock = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field tid_memory_permalock
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->tid_memory_permalock = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field file_0_memory_write_lock
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->file_0_memory_write_lock = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field file_0_memory_permalock
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->file_0_memory_permalock = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    (void)curr_bit_len;  // ignore final curr_bit_len increment
}

static void access_command_decode(
    void*      decoded_args,
    const struct BitSpan* encoded_command)
{
    struct AccessCommandArgs* decoded_command = (struct AccessCommandArgs*) decoded_args;
    size_t   curr_bit_len    = 0;
    uint8_t const* unpacked_data;
    // Skip command ID in decode
    curr_bit_len += 8;
    // Pulling field password
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 16);
    decoded_command->password = ((uint16_t const*)unpacked_data)[0];
    curr_bit_len += 16;
    (void)curr_bit_len;  // ignore final curr_bit_len increment
}

static void block_write_command_decode(
    void*      decoded_args,
    const struct BitSpan* encoded_command)
{
    struct BlockWriteCommandArgs* decoded_command = (struct BlockWriteCommandArgs*) decoded_args;
    size_t   curr_bit_len    = 0;
    uint8_t const* unpacked_data;
    // Skip command ID in decode
    curr_bit_len += 8;
    // Pulling field memory_bank
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 2);
    decoded_command->memory_bank = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 2;
    // Pulling EBV field word_pointer
    uint8_t const ebv_byte_len = (uint8_t)ebv_length_decode(encoded_command->data, curr_bit_len);
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, ebv_byte_len*8);
    decoded_command->word_pointer = bit_unpack_ebv(unpacked_data, ebv_byte_len);
    curr_bit_len += ebv_byte_len*8;
    // Pulling field word_count
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 8);
    decoded_command->word_count = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 8;
    // Pulling field data
    // Variable bit decode uses the rest of the length
    uint8_t const copy_bits = (uint8_t)(encoded_command->length - curr_bit_len);

    unpacked_data = bit_unpack_msb(encoded_command->data, curr_bit_len, copy_bits);
    // copy data over to the byte span
    uint8_t copy_bytes = (copy_bits - (copy_bits % 8u)) / 8u;
    copy_bytes += (copy_bits % 8u) ? 1u: 0u;

    static uint8_t variable_buffer[TxCommandDecodeBufferSize];
    ex10_memzero(variable_buffer, sizeof(variable_buffer));

    static struct BitSpan variable_data = {.data=variable_buffer, .length=0};
    // Point the variable data to this local store.
    // Note: This is overridden the next time it is used.
    decoded_command->data = &variable_data;

    ex10_memcpy(
        variable_data.data, TxCommandDecodeBufferSize, unpacked_data, copy_bytes);
    variable_data.length = copy_bits;
    curr_bit_len += copy_bits;
    (void)curr_bit_len;  // ignore final curr_bit_len increment
}

static void block_permalock_command_decode(
    void*      decoded_args,
    const struct BitSpan* encoded_command)
{
    struct BlockPermalockCommandArgs* decoded_command = (struct BlockPermalockCommandArgs*) decoded_args;
    size_t   curr_bit_len    = 0;
    uint8_t const* unpacked_data;
    // Skip command ID in decode
    curr_bit_len += 8;
    // Skip rfu in decode
    curr_bit_len += 8;
    // Pulling field read_lock
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->read_lock = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field memory_bank
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 2);
    decoded_command->memory_bank = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 2;
    // Pulling EBV field block_pointer
    uint8_t const ebv_byte_len = (uint8_t)ebv_length_decode(encoded_command->data, curr_bit_len);
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, ebv_byte_len*8);
    decoded_command->block_pointer = bit_unpack_ebv(unpacked_data, ebv_byte_len);
    curr_bit_len += ebv_byte_len*8;
    // Pulling field block_range
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 8);
    decoded_command->block_range = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 8;
    // Pulling field mask
    // Variable bit decode uses the rest of the length
    uint8_t const copy_bits = (uint8_t)(encoded_command->length - curr_bit_len);

    unpacked_data = bit_unpack_msb(encoded_command->data, curr_bit_len, copy_bits);
    // copy data over to the byte span
    uint8_t copy_bytes = (copy_bits - (copy_bits % 8u)) / 8u;
    copy_bytes += (copy_bits % 8u) ? 1u: 0u;

    static uint8_t variable_buffer[TxCommandDecodeBufferSize];
    ex10_memzero(variable_buffer, sizeof(variable_buffer));

    static struct BitSpan variable_data = {.data=variable_buffer, .length=0};
    // Point the variable data to this local store.
    // Note: This is overridden the next time it is used.
    decoded_command->mask = &variable_data;

    ex10_memcpy(
        variable_data.data, TxCommandDecodeBufferSize, unpacked_data, copy_bytes);
    variable_data.length = copy_bits;
    curr_bit_len += copy_bits;
    (void)curr_bit_len;  // ignore final curr_bit_len increment
}

static void authenticate_command_decode(
    void*      decoded_args,
    const struct BitSpan* encoded_command)
{
    struct AuthenticateCommandArgs* decoded_command = (struct AuthenticateCommandArgs*) decoded_args;
    size_t   curr_bit_len    = 0;
    uint8_t const* unpacked_data;
    // Skip command ID in decode
    curr_bit_len += 8;
    // Skip rfu in decode
    curr_bit_len += 2;
    // Pulling field send_rep
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->send_rep = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field inc_rep_len
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 1);
    decoded_command->inc_rep_len = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 1;
    // Pulling field csi
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 8);
    decoded_command->csi = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 8;
    // Pulling field length
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 12);
    decoded_command->length = ((uint16_t const*)unpacked_data)[0];
    curr_bit_len += 12;
    // Pulling field message
    // Variable bit decode uses the rest of the length
    uint8_t const copy_bits = (uint8_t)(encoded_command->length - curr_bit_len);

    unpacked_data = bit_unpack_msb(encoded_command->data, curr_bit_len, copy_bits);
    // copy data over to the byte span
    uint8_t copy_bytes = (copy_bits - (copy_bits % 8u)) / 8u;
    copy_bytes += (copy_bits % 8u) ? 1u: 0u;

    static uint8_t variable_buffer[TxCommandDecodeBufferSize];
    ex10_memzero(variable_buffer, sizeof(variable_buffer));

    static struct BitSpan variable_data = {.data=variable_buffer, .length=0};
    // Point the variable data to this local store.
    // Note: This is overridden the next time it is used.
    decoded_command->message = &variable_data;

    ex10_memcpy(
        variable_data.data, TxCommandDecodeBufferSize, unpacked_data, copy_bytes);
    variable_data.length = copy_bits;
    curr_bit_len += copy_bits;
    (void)curr_bit_len;  // ignore final curr_bit_len increment
}

static void margin_read_command_decode(
    void*      decoded_args,
    const struct BitSpan* encoded_command)
{
    struct MarginReadCommandArgs* decoded_command = (struct MarginReadCommandArgs*) decoded_args;
    size_t   curr_bit_len    = 0;
    uint8_t const* unpacked_data;
    // Skip command ID in decode
    curr_bit_len += 16;
    // Pulling field memory_bank
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 2);
    decoded_command->memory_bank = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 2;
    // Pulling EBV field bit_pointer
    uint8_t const ebv_byte_len = (uint8_t)ebv_length_decode(encoded_command->data, curr_bit_len);
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, ebv_byte_len*8);
    decoded_command->bit_pointer = bit_unpack_ebv(unpacked_data, ebv_byte_len);
    curr_bit_len += ebv_byte_len*8;
    // Pulling field bit_length
    unpacked_data = bit_unpack(encoded_command->data, curr_bit_len, 8);
    decoded_command->bit_length = ((uint8_t const*)unpacked_data)[0];
    curr_bit_len += 8;
    // Pulling field mask
    // Variable bit decode uses the rest of the length -16 for tag_handle
    uint8_t const copy_bits = (uint8_t)(encoded_command->length - curr_bit_len - 16u);

    unpacked_data = bit_unpack_msb(encoded_command->data, curr_bit_len, copy_bits);
    // copy data over to the byte span
    uint8_t copy_bytes = (copy_bits - (copy_bits % 8u)) / 8u;
    copy_bytes += (copy_bits % 8u) ? 1u: 0u;

    static uint8_t variable_buffer[TxCommandDecodeBufferSize];
    ex10_memzero(variable_buffer, sizeof(variable_buffer));

    static struct BitSpan variable_data = {.data=variable_buffer, .length=0};
    // Point the variable data to this local store.
    // Note: This is overridden the next time it is used.
    decoded_command->mask = &variable_data;

    ex10_memcpy(
        variable_data.data, TxCommandDecodeBufferSize, unpacked_data, copy_bytes);
    variable_data.length = copy_bits;
    curr_bit_len += copy_bits;
    (void)curr_bit_len;  // ignore final curr_bit_len increment
}
// IPJ_autogen }
// clang-format on

static void (*gen2_command_encode_table[])(const void*, struct BitSpan*) = {
    select_command_encode,
    read_command_encode,
    write_command_encode,
    kill_command_encode,
    kill_command_encode,
    lock_command_encode,
    access_command_encode,
    block_write_command_encode,
    block_permalock_command_encode,
    authenticate_command_encode,
    margin_read_command_encode,
};

static struct Ex10Result encode_gen2_command(
    const struct Gen2CommandSpec* cmd_spec,
    struct BitSpan*               encoded_command)
{
    if ((cmd_spec == NULL) || (cmd_spec->args == NULL) ||
        (encoded_command == NULL) || (encoded_command->data == NULL))
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorNullPointer);
    }

    // Ensure the command is supported
    if (cmd_spec->command >= _GEN2_COMMAND_MAX)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorBadParamValue);
    }

    // The command will map to the encoder function pointer
    gen2_command_encode_table[cmd_spec->command](cmd_spec->args,
                                                 encoded_command);

    // If an error occurred during encoding, the encoded command length will be
    // set to 0
    if (encoded_command->length == 0)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10ErrorGen2CommandEncode);
    }

    return make_ex10_success();
}

static void (*gen2_command_decode_table[])(void*, const struct BitSpan*) = {
    select_command_decode,
    read_command_decode,
    write_command_decode,
    kill_command_decode,
    kill_command_decode,
    lock_command_decode,
    access_command_decode,
    block_write_command_decode,
    block_permalock_command_decode,
    authenticate_command_decode,
    margin_read_command_decode,
};

static struct Ex10Result decode_gen2_command(
    struct Gen2CommandSpec* cmd_spec,
    const struct BitSpan*   encoded_command)
{
    if ((cmd_spec == NULL) || (cmd_spec->args == NULL) ||
        (encoded_command == NULL) || (encoded_command->data == NULL))
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorNullPointer);
    }

    // Find the type of command from the encoded function
    cmd_spec->command = decode_command_type(encoded_command);
    if (cmd_spec->command >= _GEN2_COMMAND_MAX)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorBadParamValue);
    }
    // Pull out the args based on the command type
    gen2_command_decode_table[cmd_spec->command](cmd_spec->args,
                                                 encoded_command);
    return make_ex10_success();
}

static struct Ex10Result get_gen2_tx_control_config(
    const struct Gen2CommandSpec* cmd_spec,
    struct Gen2TxnControlsFields* txn_control)
{
    if ((cmd_spec == NULL) || (cmd_spec->args == NULL) || (txn_control == NULL))
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorNullPointer);
    }

    if (cmd_spec->command >= _GEN2_COMMAND_MAX)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorBadParamValue);
    }

    /* Look up in transaction_configs table. */
    struct Gen2TxnControlsFields txn_config =
        transaction_configs[cmd_spec->command];

    /* Special handling for various commands */
    if (cmd_spec->command == Gen2Read)
    {
        /* RxLength changes based on the number of words specified */
        struct ReadCommandArgs const* args =
            (struct ReadCommandArgs const*)(cmd_spec->args);
        txn_config.rx_length += (args->word_count * 16u);
    }
    else if (cmd_spec->command == Gen2BlockPermalock)
    {
        struct BlockPermalockCommandArgs const* args =
            (struct BlockPermalockCommandArgs const*)(cmd_spec->args);

        /* 2 different responses possible based on the read_lock option */
        if (args->read_lock == Read)
        {
            /* rx_length changes based on the number of words specified */
            txn_config.rx_length += args->block_range * 16u;

            txn_config.response_type = Immediate;
        }
        else if (args->read_lock == Permalock)
        {
            txn_config.response_type = Delayed;
        }
    }
    else if (cmd_spec->command == Gen2Authenticate)
    {
        struct AuthenticateCommandArgs const* args =
            (struct AuthenticateCommandArgs const*)(cmd_spec->args);

        if (args->send_rep)
        {
            txn_config.rx_length += args->rep_len_bits;

            if (args->inc_rep_len)
            {
                txn_config.rx_length += 16u;
            }
        }
    }
    else if (cmd_spec->command == Gen2Kill_1 || cmd_spec->command == Gen2Kill_2)
    {
        txn_config.is_kill_command = 1u;
    }

    // Copy the data to the user struct
    *txn_control = txn_config;

    return make_ex10_success();
}

static void general_reply_decode(uint16_t          num_bits,
                                 const uint8_t*    data,
                                 struct Gen2Reply* reply)
{
    size_t word_count = (num_bits - (num_bits % 16)) / 16;
    word_count += (num_bits % 16) ? 1 : 0;
    uint16_t* decoded_word = reply->data;

    for (size_t iter = 0u; iter < word_count; iter++)
    {
        *decoded_word = *(data++);
        *decoded_word <<= 8u;
        *decoded_word++ |= *(data++);
    }
}

static bool decode_reply(enum Gen2Command              command,
                         const struct EventFifoPacket* gen2_pkt,
                         struct Gen2Reply*             decoded_reply)
{
    if ((gen2_pkt == NULL) || (decoded_reply == NULL) ||
        (decoded_reply->data == NULL))
    {
        return false;
    }

    uint16_t num_bits = gen2_pkt->static_data->gen2_transaction.num_bits;

    decoded_reply->reply      = command;
    decoded_reply->error_code = NoError;
    decoded_reply->transaction_status =
        gen2_pkt->static_data->gen2_transaction.status;

    if (decoded_reply->transaction_status != Gen2TransactionStatusOk)
    {
        decoded_reply->error_code = Other;
        return false;
    }

    // Ensure this is a supported command and check if it contains an error
    // header
    bool reply_has_error_header = false;
    switch (command)
    {
        case Gen2Read:
        case Gen2MarginRead:
        case Gen2BlockPermalock:
        case Gen2Write:
        case Gen2Kill_2:
        case Gen2Lock:
        case Gen2BlockWrite:
            reply_has_error_header = true;
            break;
        case Gen2Authenticate:
        case Gen2Kill_1:
        case Gen2Access:
            reply_has_error_header = false;
            break;
        case Gen2Select:         // not supported for decode
        case _GEN2_COMMAND_MAX:  // not supported for decode
        default:
            ex10_eprintf("No known decoder for command %u\n", command);
            return false;
    }

    // Check header bit for errors
    if (reply_has_error_header)
    {
        // There will always be 1 bit for the error header and at
        // least one more byte. If there is an error shown in the
        // first bit, then the next byte is the error that occurred.
        // If there is not, there will always be at least one more
        // byte of data.
        if (num_bits < 9)
        {
            ex10_eprintf("Expected at least 9 bits but received only %u bits\n",
                         num_bits);
            return false;
        }
        bool header_error = gen2_pkt->dynamic_data[0] & 0x01;

        if (header_error)
        {
            decoded_reply->error_code = gen2_pkt->dynamic_data[1];
            if (decoded_reply->error_code != NoError)
            {
                return false;
            }
        }

        // subtract header bit
        // Note: does not remove 9 bits in the case of and error
        // since we just return.
        num_bits -= 1;
    }

    // No error and a proper command. Now parse the data
    const uint8_t* data_after_header = (reply_has_error_header)
                                           ? &gen2_pkt->dynamic_data[1]
                                           : &gen2_pkt->dynamic_data[0];

    // Special case for  in process replies
    bool is_in_process_reply = (command == Gen2Authenticate) ? true : false;
    if (is_in_process_reply)
    {
        /*
         * The reply is an In-process Tag reply
         * Add 7 bits for the barker code
         */
        num_bits += 7;
    }

    general_reply_decode(num_bits, data_after_header, decoded_reply);
    return true;
}

static bool check_error(struct Gen2Reply reply)
{
    if (reply.error_code != NoError)
    {
        ex10_eputs("Gen2Transaction error:\n");
        ex10_eputs("    Reply command: %u\n", reply.reply);
        ex10_eputs("    TagErrorCode: %s (%u)\n",
                   get_ex10_gen2_error_string(reply.error_code),
                   reply.error_code);
        if (reply.reply == Gen2Read)
        {
            ex10_eputs("    data[0]: 0x%04x\n", reply.data[0]);
        }
        return true;
    }
    return false;
}

static void print_reply(struct Gen2Reply reply)
{
    ex10_printf("Gen2Reply: command: (%u) %s\n",
                reply.reply,
                get_ex10_gen2_command_string(reply.reply));
    ex10_printf(
        "           status: (%u) %s\n",
        reply.transaction_status,
        get_ex10_gen2_transaction_status_string(reply.transaction_status));
    ex10_printf("           error code: (%u) %s\n",
                reply.error_code,
                get_ex10_gen2_error_string(reply.error_code));
    if (reply.reply == Gen2Read)
    {
        ex10_printf("           data[0]: 0x%04X\n", reply.data[0]);
    }
}

struct Ex10Gen2Commands const* get_ex10_gen2_commands(void)
{
    static struct Ex10Gen2Commands gen2_commands_instance = {
        .encode_gen2_command        = encode_gen2_command,
        .decode_gen2_command        = decode_gen2_command,
        .decode_reply               = decode_reply,
        .check_error                = check_error,
        .print_reply                = print_reply,
        .get_gen2_tx_control_config = get_gen2_tx_control_config,
        .get_ebv_bit_len            = get_ebv_bit_len,
        .bit_pack                   = bit_pack,
        .bit_pack_ebv               = bit_pack_ebv,
        .bit_unpack                 = bit_unpack,
        .bit_unpack_ebv             = bit_unpack_ebv,
        .bit_unpack_msb             = bit_unpack_msb,
        .ebv_length_decode          = ebv_length_decode,
        .le_bytes_to_uint16         = le_bytes_to_uint16,
    };

    return &gen2_commands_instance;
}
