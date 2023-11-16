/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2023 Impinj, Inc. All rights reserved.                      *
 *                                                                           *
 *****************************************************************************/

#include <float.h>
#include <stdlib.h>
#include <string.h>

#include "ex10_api/ex10_print.h"
#include "ex10_command_line.h"
#include "utils/ex10_use_case_example_errors.h"

/**
 * Search through the array of specifiers and determine if any of the
 * string specifiers match the parameter passed in.
 *
 * @param argument  A pointer to the argument to search for the specifier match.
 * @param specifier A NULL terminated string to search for in the member
 *                  specifiers array.
 *
 * @return bool true if parameter specifier matches one of the strings in
 *              the specifiers member list.
 *              false if no match was found.
 */
static bool has_specifier(struct Ex10CommandLineArgument const* argument,
                          char const*                           specifier)
{
    for (size_t index = 0u; index < argument->specifiers_size; ++index)
    {
        if (argument->specifiers[index] != NULL)
        {
            if (strncmp(argument->specifiers[index],
                        specifier,
                        MAX_SPECIFIER_LENGTH) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

struct Ex10Result ex10_parse_command_line(
    char const* const*                    argv,
    int                                   argc,
    struct Ex10CommandLineArgument const* arguments,
    size_t                                arguments_size,
    union Ex10CommandLineArgumentValue*   values)
{
    // Note: skipping argv[0]; it is the program being run.
    for (int arg_iter = 1u; arg_iter < argc;)
    {
        size_t const index = ex10_lookup_argument_index(
            arguments, arguments_size, argv[arg_iter]);
        if (index < arguments_size)
        {
            switch (arguments[index].type)
            {
                case Ex10CommandLineArgumentUninitialized:
                    // This case should never happen.
                    break;
                case Ex10CommandLineArgumentChar:
                    ++arg_iter;
                    if (arg_iter >= argc)
                    {
                        return make_ex10_app_error(
                            Ex10ApplicationCommandLineMissingParamValue);
                    }
                    values[index].character = *argv[arg_iter];
                    break;
                case Ex10CommandLineArgumentInt32:
                    ++arg_iter;
                    if (arg_iter >= argc)
                    {
                        return make_ex10_app_error(
                            Ex10ApplicationCommandLineMissingParamValue);
                    }
                    values[index].int32 =
                        parse_int32(argv[arg_iter], INT32_MAX);
                    if (values[index].int32 == INT32_MAX)
                    {
                        return make_ex10_app_error(
                            Ex10ApplicationCommandLineBadParamValue);
                    }
                    break;
                case Ex10CommandLineArgumentUint32:
                    ++arg_iter;
                    if (arg_iter >= argc)
                    {
                        return make_ex10_app_error(
                            Ex10ApplicationCommandLineMissingParamValue);
                    }
                    values[index].uint32 =
                        parse_uint32(argv[arg_iter], UINT32_MAX);
                    if (values[index].uint32 == UINT32_MAX)
                    {
                        return make_ex10_app_error(
                            Ex10ApplicationCommandLineBadParamValue);
                    }
                    break;
                case Ex10CommandLineArgumentFloat:
                    ++arg_iter;
                    if (arg_iter >= argc)
                    {
                        return make_ex10_app_error(
                            Ex10ApplicationCommandLineMissingParamValue);
                    }
                    values[index].floater =
                        parse_float(argv[arg_iter], FLT_MAX);
                    if (values[index].floater >= FLT_MAX - FLT_EPSILON)
                    {
                        return make_ex10_app_error(
                            Ex10ApplicationCommandLineBadParamValue);
                    }
                    break;
                case Ex10CommandLineArgumentBool:
                    ++arg_iter;
                    if (arg_iter >= argc)
                    {
                        return make_ex10_app_error(
                            Ex10ApplicationCommandLineMissingParamValue);
                    }
                    values[index].int32 =
                        parse_int32(argv[arg_iter], INT32_MAX);
                    if (values[index].int32 == INT32_MAX)
                    {
                        return make_ex10_app_error(
                            Ex10ApplicationCommandLineBadParamValue);
                    }
                    values[index].boolean = (values[index].int32 != 0);
                    break;
                case Ex10CommandLineArgumentSetTrue:
                    values[index].boolean = true;
                    break;
                case Ex10CommandLineArgumentSetFalse:
                    values[index].boolean = false;
                    break;
                case Ex10CommandLineArgumentString:
                    ++arg_iter;
                    if (arg_iter >= argc)
                    {
                        return make_ex10_app_error(
                            Ex10ApplicationCommandLineMissingParamValue);
                    }
                    values[index].string = argv[arg_iter];
                    break;
                default:
                    break;
            }
        }
        else
        {
            return make_ex10_app_error(
                Ex10ApplicationCommandLineUnknownSpecifier);
        }
        ++arg_iter;
    }

    return make_ex10_success();
}

int32_t parse_int32(char const* str, int32_t error_value)
{
    if (str == NULL)
    {
        ex10_ex_eprintf("Argument str == NULL\n");
        return error_value;
    }

    char*          endp  = NULL;
    long int const value = strtol(str, &endp, 0);
    if (*endp != 0)
    {
        ex10_ex_eprintf(
            "Parsing %s as signed integer failed, pos: %c\n", str, *endp);
        return error_value;
    }

    return (int32_t)value;
}

uint32_t parse_uint32(char const* str, uint32_t error_value)
{
    if (str == NULL)
    {
        ex10_ex_eprintf("Argument str == NULL\n");
        return error_value;
    }

    char*                   endp  = NULL;
    long unsigned int const value = strtoul(str, &endp, 0);
    if (*endp != 0)
    {
        ex10_ex_eprintf(
            "Parsing %s as unsigned integer failed, pos: %c\n", str, *endp);
        return error_value;
    }

    return (uint32_t)value;
}

float parse_float(char const* str, float error_value)
{
    if (str == NULL)
    {
        ex10_ex_eprintf("Argument str == NULL\n");
        return error_value;
    }

    char*       endp  = NULL;
    float const value = strtof(str, &endp);
    if (*endp != 0)
    {
        ex10_ex_eprintf("Parsing %s as float failed, pos: %c\n", str, *endp);
        return error_value;
    }
    return value;
}

void ex10_eprint_command_line_usage(
    struct Ex10CommandLineArgument const* arguments,
    size_t                                arguments_size)
{
    for (size_t index = 0u; index < arguments_size; ++index)
    {
        struct Ex10CommandLineArgument const* node = &arguments[index];
        for (size_t spec_index = 0u; spec_index < node->specifiers_size;
             ++spec_index)
        {
            char const* specifier = node->specifiers[spec_index];
            if (specifier && *specifier)
            {
                if (spec_index > 0u)
                {
                    ex10_ex_eputs(", ");
                }
                ex10_ex_eputs("%s", specifier);
            }
        }
        ex10_ex_eputs("\n");
        if (node->description && *node->description)
        {
            ex10_ex_eputs("    %s", node->description);
        }
        ex10_ex_eputs("\n");
    }
}

void ex10_eprint_command_line_values(
    struct Ex10CommandLineArgument const*     arguments,
    size_t                                    arguments_size,
    union Ex10CommandLineArgumentValue const* values)
{
#if defined(EX10_ENABLE_PRINT_EX_ERR)
    for (size_t index = 0u; index < arguments_size; ++index)
    {
        struct Ex10CommandLineArgument const* node    = &arguments[index];
        int                                   n_write = 0;
        for (size_t spec_index = 0u; spec_index < node->specifiers_size;
             ++spec_index)
        {
            char const* specifier = node->specifiers[spec_index];
            if (specifier && *specifier)
            {
                if (spec_index > 0u)
                {
                    n_write += ex10_ex_eputs(", ");
                }
                n_write += ex10_ex_eputs("%s", specifier);
            }
        }

        while (n_write < (int)MAX_SPECIFIER_LENGTH)
        {
            n_write += ex10_ex_eputs(" ");
        }
        ex10_ex_eputs(": ");

        switch (node->type)
        {
            default:
            case Ex10CommandLineArgumentUninitialized:
                ex10_ex_eputs("undefined");
                break;
            case Ex10CommandLineArgumentInt32:
                ex10_ex_eputs("%d", values[index].int32);
                break;
            case Ex10CommandLineArgumentUint32:
                ex10_ex_eputs("%d", values[index].uint32);
                break;
            case Ex10CommandLineArgumentFloat:
                ex10_ex_eputs("%.2f", values[index].floater);
                break;
            case Ex10CommandLineArgumentBool:
            case Ex10CommandLineArgumentSetTrue:
            case Ex10CommandLineArgumentSetFalse:
                ex10_ex_eputs("%s", values[index].boolean ? "true" : "false");
                break;
            case Ex10CommandLineArgumentString:
                ex10_ex_eputs("%s", values[index].string);
                break;
        }
        ex10_ex_eputs("\n");
    }
#else
    (void)arguments;
    (void)arguments_size;
    (void)values;
#endif
}

size_t ex10_lookup_argument_index(
    struct Ex10CommandLineArgument const* arguments,
    size_t                                arguments_size,
    char const*                           specifier)
{
    for (size_t index = 0u; index < arguments_size; ++index)
    {
        if (has_specifier(&arguments[index], specifier))
        {
            return index;
        }
    }
    ex10_ex_eprintf("Specifier '%s' not found\n", specifier);
    return arguments_size;
}

union Ex10CommandLineArgumentValue const* ex10_lookup_value(
    struct Ex10CommandLineArgument const* arguments,
    size_t                                arguments_size,
    char const*                           specifier,
    union Ex10CommandLineArgumentValue*   parsed_values)
{
    size_t const index =
        ex10_lookup_argument_index(arguments, arguments_size, specifier);
    return (index < arguments_size) ? &parsed_values[index] : NULL;
}
