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

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// The type of prefix to write before each row of data written.
enum DataPrefix
{
    DataPrefixNone = 0,  ///< No prefix written.
    DataPrefixIndex,     ///< An index into the data, starting with zero.
    DataPrefixAddress    ///< The data address.
};

/**
 * Print a span of bytes to the console using ex10_printf().
 *
 * @param data_vptr The bytewise data to print.
 * @param length    The length of the data to print.
 */
void ex10_print_data_line(void const* data_vptr, size_t length)
    __attribute__((visibility("hidden")));

/**
 * Print a span of 32-bit words to the console using ex10_printf().
 *
 * @param data_vptr The bytewise data to print, which must be 32-bit aligned.
 * @param length    The number u32 words to print.
 * @param radix     The output radix of the 32-bit values: 10 or 16.
 * @param is_signed If true,  then display the 32-bit numbers as signed.
 *                  If false, then display the 32-bit numbers as unsigned.
 */
void ex10_print_int32_line(void const* data_vptr,
                           size_t      length,
                           uint8_t     radix,
                           bool        is_signed)
    __attribute__((visibility("hidden")));

/**
 * Print data as bytes to a the console using ex10_printf().
 *
 * @param data      A pointer to the data bytes to print.
 * @param length    The number of data bytes to print.
 * @param prefix    @see enum DataPrefix.
 *
 */
void ex10_print_data(void const* data, size_t length, enum DataPrefix prefix)
    __attribute__((visibility("hidden")));

/**
 * Print data as bytes to a the console using ex10_printf(),
 * always printing a prefix offset, starting with the value of the
 * index parameter.
 *
 * @param data    A pointer to the data bytes to print.
 * @param length  The number of data bytes to print.
 * @param index   The integer value to print in the leading column, followed
 *                by a colon. The index value increments with each printed row.
 *
 */
void ex10_print_data_indexed(void const* data, size_t length, size_t index)
    __attribute__((visibility("hidden")));

/**
 * Print data as 32-bit words to the console using ex10_printf().
 * The data does not need to be aligned.
 *
 * @param data      A pointer to the 32-bit words to print.
 * @param length    The number of 32-bit words to print.
 * @param radix     10 or 16 to define the numeric base.
 * @param is_signed Defines the interpretation of each 32-bit
 *                  word when the radix is 10.
 * @param index     The integer value to print in the leading column, followed
 *                  by a colon. The index value increments with each printed
 *                  by the number of bytes printed per row.
 *
 */
void ex10_print_int32(void const* data,
                      size_t      length,
                      uint8_t     radix,
                      bool        is_signed,
                      size_t      index) __attribute__((visibility("hidden")));

#ifdef __cplusplus
}
#endif
