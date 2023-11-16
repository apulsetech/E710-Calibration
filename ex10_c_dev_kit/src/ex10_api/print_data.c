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

#include <ctype.h>
#include <stdint.h>

#include "ex10_api/ex10_print.h"
#include "ex10_api/print_data.h"

void ex10_print_data_line(void const* data_vptr, size_t length)
{
    uint8_t const* data = (uint8_t const*)data_vptr;
    for (size_t iter = 0u; iter < length; ++iter, ++data)
    {
        if ((iter % 4u == 0u) && (iter > 0u))
        {
            ex10_printf(" ");
        }
        ex10_printf("%02X", *data);
    }
}

void ex10_print_int32_line(void const* data_vptr,
                           size_t      length,
                           uint8_t     radix,
                           bool        is_signed)
{
    uint8_t const* iter = (uint8_t const*)data_vptr;
    for (size_t index = 0u; index < length; ++index, iter += sizeof(uint32_t))
    {
        uint32_t value = 0;
        value |= iter[3];
        value <<= 8u;
        value |= iter[2];
        value <<= 8u;
        value |= iter[1];
        value <<= 8u;
        value |= iter[0];

        if (radix == 10)
        {
            if (is_signed)
            {
                ex10_printf("%8d ", (int32_t)value);
            }
            else
            {
                ex10_printf("%8u ", value);
            }
        }
        else if (radix == 16)
        {
            ex10_printf("%08X ", value);
        }
    }
}

void ex10_print_data(void const* data, size_t length, enum DataPrefix prefix)
{
    size_t const         bytes_per_line = 16u;
    uint8_t const*       data_ptr       = (uint8_t const*)data;
    uint8_t const* const end_ptr        = data_ptr + length;

    for (size_t iter = 0u; data_ptr < end_ptr;
         iter += bytes_per_line, data_ptr += bytes_per_line)
    {
        size_t const bytes_remaining = length - iter;
        size_t const bytes_to_write  = (bytes_remaining < bytes_per_line)
                                          ? bytes_remaining
                                          : bytes_per_line;
        switch (prefix)
        {
            case DataPrefixNone:
                break;
            case DataPrefixIndex:
                ex10_printf("%04zx: ", iter);
                break;
            case DataPrefixAddress:
                ex10_printf("%p: ", data_ptr);
                break;
            default:
                break;
        }

        ex10_print_data_line(data_ptr, bytes_to_write);
        ex10_printf("\n");
    }
}

void ex10_print_data_indexed(void const* data, size_t length, size_t index)
{
    size_t const         bytes_per_line = 16u;
    uint8_t const*       data_ptr       = (uint8_t const*)data;
    uint8_t const* const end_ptr        = data_ptr + length;

    for (size_t iter = 0u; data_ptr < end_ptr;
         iter += bytes_per_line, data_ptr += bytes_per_line)
    {
        size_t const bytes_remaining = length - iter;
        size_t const bytes_to_write  = (bytes_remaining < bytes_per_line)
                                          ? bytes_remaining
                                          : bytes_per_line;
        ex10_printf("%04zx: ", iter + index);
        ex10_print_data_line(data_ptr, bytes_to_write);
        ex10_printf("\n");
    }
}

void ex10_print_int32(void const* data,
                      size_t      length_32,
                      uint8_t     radix,
                      bool        is_signed,
                      size_t      index)
{
    size_t const words_per_line = 4u;

    for (size_t index_32 = 0u; index_32 < length_32; index_32 += words_per_line)
    {
        size_t const words_remaining = length_32 - index_32;
        size_t const words_to_write  = (words_remaining < words_per_line)
                                          ? words_remaining
                                          : words_per_line;
        size_t const   offset   = index_32 * sizeof(uint32_t);
        uint8_t const* data_ptr = (uint8_t const*)data + offset;

        ex10_printf("%04zx: ", index + offset);
        ex10_print_int32_line(data_ptr, words_to_write, radix, is_signed);
        ex10_printf("\n");
    }
}
