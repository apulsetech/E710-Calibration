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

#include <ctype.h>

#include "utils/ex10_inventory_command_line.h"

#include "board/ex10_osal.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_regulatory.h"
#include "ex10_regulatory/ex10_default_region_names.h"
#include "utils/ex10_use_case_example_errors.h"

static bool           help_requested  = false;
static enum Verbosity verbosity_level = SILENCE;

static struct Ex10CommandLineArgument const arguments[] = {
    {
        .specifiers      = {"-r", "--region"},
        .specifiers_size = 2u,
        .type            = Ex10CommandLineArgumentString,
        .description =
            "Set the regulatory region; i.e. 'FCC', 'ETSI_LOWER', ...",
    },
    {
        .specifiers      = {"--read_rate"},
        .specifiers_size = 1u,
        .type            = Ex10CommandLineArgumentUint32,
        .description     = "Set the minimum read rate limit, tags/second",
    },
    {
        .specifiers      = {"-a", "--antenna"},
        .specifiers_size = 2u,
        .type            = Ex10CommandLineArgumentUint32,
        .description     = "Select an antenna port",
    },
    {
        .specifiers      = {"-f", "--frequency_khz"},
        .specifiers_size = 2u,
        .type            = Ex10CommandLineArgumentUint32,
        .description     = "Set the LO frequency in kHz",
    },
    {
        .specifiers      = {"-R", "--remain_on"},
        .specifiers_size = 2u,
        .type            = Ex10CommandLineArgumentSetTrue,
        .description     = "Ignore regulatory timers",
    },
    {
        .specifiers      = {"-p", "--tx_power_cdbm"},
        .specifiers_size = 2u,
        .type            = Ex10CommandLineArgumentInt32,
        .description     = "Set the transmitter power in dBm",
    },
    {
        .specifiers      = {"-m", "--mode"},
        .specifiers_size = 2u,
        .type            = Ex10CommandLineArgumentUint32,
        .description     = "Set the autoset or RF mode (depends on use case)",
    },
    {
        .specifiers      = {"-t", "--target"},
        .specifiers_size = 2u,
        .type            = Ex10CommandLineArgumentChar,
        .description     = "Set the inventory target to run",
    },
    {
        .specifiers      = {"-q", "--initial_q"},
        .specifiers_size = 2u,
        .type            = Ex10CommandLineArgumentUint32,
        .description     = "Set the inventory initial Q value",
    },
    {
        .specifiers      = {"-s", "--session"},
        .specifiers_size = 2u,
        .type            = Ex10CommandLineArgumentUint32,
        .description     = "Set the inventory session to use",
    },
    {
        .specifiers      = {"-v", "--verbose"},
        .specifiers_size = 2u,
        .type            = Ex10CommandLineArgumentUint32,
        .description =
            "Set the verbosity level: 0: silent, 1: print tag packets, "
            "2: print all packets",
    },
    {
        .specifiers      = {"-h", "--help"},
        .specifiers_size = 2u,
        .type            = Ex10CommandLineArgumentSetTrue,
        .description     = "Print this help message",
    },
};

static bool is_valid_target_spec(char target_spec)
{
    return target_spec == 'A' || target_spec == 'B' || target_spec == 'D';
}

/**
 * Fill in the array of default_values based on the values contained in
 * the inventory_options parameter.
 *
 * @param       inventory_options The initialized value set in main().
 * @param [out] values            The values array to be initialized w/defaults.
 * @return struct Ex10Result      Indicates success or failure.
 * @note   Failure indicates a mismatch in the arguments specifier string;
 *         which is a programming error, not a run-time error.
 */
static struct Ex10Result fill_in_default_values(
    struct InventoryOptions const*      inventory_options,
    union Ex10CommandLineArgumentValue* values)
{
    bool   parsed_ok = true;
    size_t index     = 0u;

    index = ex10_lookup_argument_index(
        arguments, ARRAY_SIZE(arguments), "--region");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].string = inventory_options->region_name;
    }

    index = ex10_lookup_argument_index(
        arguments, ARRAY_SIZE(arguments), "--read_rate");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].uint32 = inventory_options->read_rate;
    }

    index = ex10_lookup_argument_index(
        arguments, ARRAY_SIZE(arguments), "--antenna");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].uint32 = inventory_options->antenna;
    }

    index = ex10_lookup_argument_index(
        arguments, ARRAY_SIZE(arguments), "--frequency_khz");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].uint32 = inventory_options->frequency_khz;
    }

    index = ex10_lookup_argument_index(
        arguments, ARRAY_SIZE(arguments), "--remain_on");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].boolean = inventory_options->remain_on;
    }

    index = ex10_lookup_argument_index(
        arguments, ARRAY_SIZE(arguments), "--tx_power_cdbm");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].int32 = inventory_options->tx_power_cdbm;
    }

    index =
        ex10_lookup_argument_index(arguments, ARRAY_SIZE(arguments), "--mode");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].uint32 = inventory_options->mode.raw;
    }

    index = ex10_lookup_argument_index(
        arguments, ARRAY_SIZE(arguments), "--target");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].character = inventory_options->target_spec;
    }

    index = ex10_lookup_argument_index(
        arguments, ARRAY_SIZE(arguments), "--initial_q");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].uint32 = inventory_options->initial_q;
    }

    index = ex10_lookup_argument_index(
        arguments, ARRAY_SIZE(arguments), "--session");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].uint32 = inventory_options->session;
    }

    /// @note "--verbose" and "--help" are not part of struct InventoryOptions.
    ///       These values also need to be initialized with default values.
    index = ex10_lookup_argument_index(
        arguments, ARRAY_SIZE(arguments), "--verbose");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].uint32 = SILENCE;
    }

    index =
        ex10_lookup_argument_index(arguments, ARRAY_SIZE(arguments), "--help");
    parsed_ok = (index < ARRAY_SIZE(arguments)) ? parsed_ok : false;
    if (index < ARRAY_SIZE(arguments))
    {
        values[index].boolean = false;
    }

    return parsed_ok ? make_ex10_success()
                     : make_ex10_app_error(
                           Ex10ApplicationCommandLineUnknownSpecifier);
}

struct Ex10Result ex10_inventory_parse_command_line(
    struct InventoryOptions* inventory_options,
    char const* const*       argv,
    int                      argc)
{
    union Ex10CommandLineArgumentValue parsed_values[ARRAY_SIZE(arguments)];
    ex10_memzero(parsed_values, sizeof(parsed_values));

    // Set the parsed_values initial values to values passed in via the
    // struct InventoryOptions.
    struct Ex10Result const ex10_result_defaults =
        fill_in_default_values(inventory_options, parsed_values);

    // Override parsed_values settings based on what main() has passed in
    // via the command line argv[], argc.
    struct Ex10Result const ex10_result_parsed = ex10_parse_command_line(
        argv, argc, arguments, ARRAY_SIZE(arguments), parsed_values);

    union Ex10CommandLineArgumentValue const* value = NULL;

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--region", parsed_values);
    if (value != NULL)
    {
        inventory_options->region_name = value->string;
    }

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--read_rate", parsed_values);
    if (value != NULL)
    {
        inventory_options->read_rate = value->uint32;
    }

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--antenna", parsed_values);
    if (value != NULL)
    {
        inventory_options->antenna = (uint8_t)value->uint32;
    }

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--frequency_khz", parsed_values);
    if (value != NULL)
    {
        inventory_options->frequency_khz = value->uint32;
    }

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--remain_on", parsed_values);
    if (value != NULL)
    {
        inventory_options->remain_on = value->boolean;
    }

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--tx_power_cdbm", parsed_values);
    if (value != NULL)
    {
        inventory_options->tx_power_cdbm = (int16_t)value->int32;
    }

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--mode", parsed_values);
    if (value != NULL)
    {
        inventory_options->mode.raw = (uint16_t)value->uint32;
    }

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--target", parsed_values);
    if (value != NULL)
    {
        inventory_options->target_spec = value->character;
    }

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--initial_q", parsed_values);
    if (value != NULL)
    {
        inventory_options->initial_q = (uint8_t)value->uint32;
    }

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--session", parsed_values);
    if (value != NULL)
    {
        inventory_options->session =
            (enum InventoryRoundControlSession)value->uint32;
    }

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--verbose", parsed_values);
    if (value != NULL)
    {
        verbosity_level = (enum Verbosity)value->uint32;
    }

    value = ex10_lookup_value(
        arguments, ARRAY_SIZE(arguments), "--help", parsed_values);
    if (value != NULL)
    {
        help_requested = value->boolean;
    }

    // Capitalize the target specification, to limit options checking to
    // 'A', 'B', or 'D'.
    if (isprint(inventory_options->target_spec))
    {
        inventory_options->target_spec =
            (char)toupper(inventory_options->target_spec);
    }

    struct Ex10Result const ex10_result_checked =
        ex10_check_inventory_command_line_settings(inventory_options);

    // clang-format off
    struct Ex10Result const ex10_result =
        ex10_result_defaults.error ? ex10_result_defaults :
        ex10_result_parsed.error   ? ex10_result_parsed   :
        ex10_result_checked.error  ? ex10_result_checked  :
        make_ex10_success();
    // clang-format on

    if (ex10_result.error || help_requested)
    {
        ex10_eprint_inventory_command_line_usage();
    }
    return ex10_result;
}

void ex10_eprint_inventory_command_line_usage(void)
{
    ex10_eprint_command_line_usage(arguments, ARRAY_SIZE(arguments));
}

void ex10_print_inventory_command_line_settings(
    struct InventoryOptions const* inventory_options)
{
    ex10_ex_printf("Use case example settings:\n");
    ex10_ex_printf("Region      : %s\n", inventory_options->region_name);
    ex10_ex_printf("Read rate   : %u tags/s\n", inventory_options->read_rate);
    ex10_ex_printf("Antenna     : %u\n", inventory_options->antenna);
    ex10_ex_printf("Frequency   : %u kHz\n", inventory_options->frequency_khz);
    ex10_ex_printf("Remain on   : %u\n", inventory_options->remain_on);
    ex10_ex_printf("Tx power    : %u cdBm\n", inventory_options->tx_power_cdbm);
    ex10_ex_printf("Auto/RF mode: %u\n", inventory_options->mode.raw);
    ex10_ex_printf("Target      : %c\n", inventory_options->target_spec);
    ex10_ex_printf("Session     : %u\n", inventory_options->session);
    ex10_ex_printf("Initial Q   : %u\n", inventory_options->initial_q);
}

struct Ex10Result ex10_check_inventory_command_line_settings(
    struct InventoryOptions const* inventory_options)
{
    bool parsed_ok = true;
    if (inventory_options->antenna < 1 || inventory_options->antenna > 2)
    {
        ex10_ex_eprintf("Invalid antenna: %u\n", inventory_options->antenna);
        parsed_ok = false;
    }

    if (inventory_options->session < SessionS0 ||
        inventory_options->session > SessionS3)
    {
        ex10_ex_eprintf("Invalid session: %u\n", inventory_options->session);
        parsed_ok = false;
    }

    if (is_valid_target_spec(inventory_options->target_spec) == false)
    {
        ex10_ex_eprintf("Invalid target specified: %c\n",
                        inventory_options->target_spec);
        parsed_ok = false;
    }

    enum Ex10RegionId const region_id =
        get_ex10_default_region_names()->get_region_id(
            inventory_options->region_name);
    if (region_id == REGION_NOT_DEFINED)
    {
        parsed_ok = false;
        ex10_ex_eprintf("Invalid region specified: %s\n",
                        inventory_options->region_name);
    }

    if (parsed_ok == false)
    {
        return make_ex10_app_error(Ex10ApplicationCommandLineBadParamValue);
    }

    return make_ex10_success();
}

bool ex10_command_line_help_requested(void)
{
    return help_requested;
}

enum Verbosity ex10_command_line_verbosity(void)
{
    return verbosity_level;
}
