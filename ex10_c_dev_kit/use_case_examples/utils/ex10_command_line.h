/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2022 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ex10_api/ex10_result.h"
#include "utils/ex10_inventory_command_line.h"

#ifdef __cplusplus
extern "C" {
#endif

enum Ex10CommandLineArgumentType
{
    Ex10CommandLineArgumentUninitialized = 0,
    Ex10CommandLineArgumentChar,
    Ex10CommandLineArgumentInt32,
    Ex10CommandLineArgumentUint32,
    Ex10CommandLineArgumentFloat,
    Ex10CommandLineArgumentBool,
    Ex10CommandLineArgumentSetTrue,
    Ex10CommandLineArgumentSetFalse,
    Ex10CommandLineArgumentString,
};

union Ex10CommandLineArgumentValue {
    char        character;
    int32_t     int32;
    uint32_t    uint32;
    float       floater;
    bool        boolean;
    char const* string;
};

#define MAX_SPECIFIER_COUNT ((size_t)(2u))
#define MAX_SPECIFIER_LENGTH ((size_t)(24u))

struct Ex10CommandLineArgument
{
    // An array of strings, which when encountered will trigger this argument
    // node to be parsed. char const* const specifiers[] = {"--region", "-r"};
    // When either of these strings are encountered, the following argument will
    // be parsed as a region name.
    char const* specifiers[MAX_SPECIFIER_COUNT];

    /// The number of strings in the specifiers array.
    size_t specifiers_size;

    // Specify how the command line argument should be parsed.
    enum Ex10CommandLineArgumentType type;

    /// If non-NULL, then provides a description for use by
    /// ex10_print_command_line_usage().
    char const* description;
};

/**
 * Parse the command line arguments from main().
 *
 * Each node in the arguments array specifies how a command line argument is to
 * be parsed into this array. If a command line specifier is encountered, then
 * the associated node in this array is set to the parsed value. If no value is
 * found which matches the specifier, then the default value is used
 * (i.e. the value element within the values array is unchanged).
 *
 * @param argv            The argument list passed into main.
 * @param argc            The number of arguments to parse.
 * @param arguments       The specifiers and container of parsed arguments.
 * @param arguments_size  The number of nodes in the parameter arguments.
 * @param [in out] values The parsed values array. The number of nodes in this
 *                        array must match the arguments_size parameter.
 */
struct Ex10Result ex10_parse_command_line(
    char const* const*                    argv,
    int                                   argc,
    struct Ex10CommandLineArgument const* arguments,
    size_t                                arguments_size,
    union Ex10CommandLineArgumentValue*   values);

/** @{
 * String parsing functions to various types.
 *
 * @param str           A null terminated C language ASCII character string.
 * @param error_value   When type parsing fails, use this value.
 * @return              The parsed value type.
 */
int32_t  parse_int32(char const* str, int32_t error_value);
uint32_t parse_uint32(char const* str, uint32_t error_value);
float    parse_float(char const* str, float error_value);
/** @} */

/**
 * Based on the struct Ex10CommandLineArgument array,
 * print the usage description to the console.
 *
 * @param arguments      The specifiers and container of parsed arguments.
 * @param arguments_size The number of nodes in the parameter arguments.
 */
void ex10_eprint_command_line_usage(
    struct Ex10CommandLineArgument const* arguments,
    size_t                                arguments_size);

/**
 * Based on the struct Ex10CommandLineArgument array,
 * print the values to the console.
 *
 * @param arguments      The specifiers and container of parsed arguments.
 * @param arguments_size The number of nodes in the parameter arguments.
 */
void ex10_eprint_command_line_values(
    struct Ex10CommandLineArgument const*     arguments,
    size_t                                    arguments_size,
    union Ex10CommandLineArgumentValue const* values);

/**
 * Find a node in the array of struct Ex10CommandLineArgument arguments which
 * contains a matching specifier.
 *
 * @param arguments      The pointer to the arguments array to search through.
 * @param arguments_size The number of nodes in the arguments array.
 * @param specifier      The specifier text to match.
 * @return size_t        The index into the arguments array for this the
 *                       specifier is found.
 * @retval               arguments_size if no specifier match is found.
 */
size_t ex10_lookup_argument_index(
    struct Ex10CommandLineArgument const* arguments,
    size_t                                arguments_size,
    char const*                           specifier);

union Ex10CommandLineArgumentValue const* ex10_lookup_value(
    struct Ex10CommandLineArgument const* arguments,
    size_t                                arguments_size,
    char const*                           specifier,
    union Ex10CommandLineArgumentValue*   parsed_values);

#ifdef __cplusplus
}
#endif
