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

#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "board/gpio_driver.h"
#include "board/time_helpers.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_power_modes.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/rf_mode_definitions.h"
#include "ex10_api/trace.h"

// static bool const verbose = true;
static uint32_t us_per_s = 1000u * 1000u;
static uint32_t ms_per_s = 1000u;

static uint8_t const select_all = 0u;


/// Prints microsecond tick counts as seconds.milliseconds
static void print_microseconds(uint32_t time_us)
{
    ex10_ex_printf(
        "%6u.%03u", time_us / us_per_s, (time_us % us_per_s) / ms_per_s);
}

/**
 * A continuous inventory helper function to call the
 * Ex10Helpers.continuous_inventory() function which in turn calls
 * Ex10Reader.continuous_inventory().
 *
 * @return int32_t An indication of success.
 * @retval >= 0    The number of tags found in the inventory.
 * @retval  < 0    The negated InventoryHelperReturns value.
 */
static int32_t continuous_inventory(
    struct InventoryHelperParams* ihp,
    struct StopConditions const*  stop_conditions)
{
    ex10_ex_printf("continuous inventory, duration: ");
    print_microseconds(stop_conditions->max_duration_us);
    ex10_ex_printf("\n");
    struct InfoFromPackets packet_info = {0u, 0u, 0u, 0u, {0u}};
    ihp->packet_info                   = &packet_info;

    struct ContinuousInventorySummary continuous_inventory_summary = {0};

    struct ContInventoryHelperParams cihp = {
        .inventory_params = ihp,
        .stop_conditions  = stop_conditions,
        .summary_packet   = &continuous_inventory_summary,
    };

    enum InventoryHelperReturns const ret_val =
        get_ex10_helpers()->continuous_inventory(&cihp);

    if (ret_val != InvHelperSuccess)
    {
        ex10_ex_eprintf("Continuous inventory failed: %d\n", (int)ret_val);
        return -(int32_t)ret_val;
    }

    uint32_t const read_rate =
        ex10_calculate_read_rate(continuous_inventory_summary.number_of_tags,
                                 continuous_inventory_summary.duration_us);

    ex10_ex_printf("Tag Read rate:       %6u\n", read_rate);
    ex10_ex_printf("Number of tags read: %6u\n",
                   continuous_inventory_summary.number_of_tags);
    ex10_ex_printf("Numbers of seconds:  ");
    print_microseconds(continuous_inventory_summary.duration_us);
    ex10_ex_printf("\n");
    ex10_ex_printf("RF Mode:             %6u\n", ihp->rf_mode);

    if (continuous_inventory_summary.number_of_tags == 0)
    {
        ex10_ex_eprintf("No tags found in inventory\n");
    }

    return (int32_t)continuous_inventory_summary.number_of_tags;
}

#if defined(EX10_PRINT_IMPL) || defined(EX10_PRINT_ERR_IMPL)
static char const* power_mode_string(enum PowerMode power_mode)
{
    switch (power_mode)
    {
        case PowerModeOff:
            return "PowerModeOff";
        case PowerModeStandby:
            return "PowerModeStandby";
        case PowerModeReadyCold:
            return "PowerModeReadyCold";
        case PowerModeReady:
            return "PowerModeReady";
        case PowerModeInvalid:
            return "PowerModeInvalid";
        default:
            return "PowerMode --unknown--";
    }
}
#else
static char const* power_mode_string(enum PowerMode power_mode)
{
    (void)power_mode;
    return NULL;
}
#endif

static void print_usage(bool           as_default,
                        float          time_s_inventory,
                        float          time_s_low_power,
                        size_t         cycles,
                        enum PowerMode low_power_mode)
{
    char const* default_or_using = as_default ? " default" : "using";
    ex10_ex_eputs(
        "-T time, in seconds, to run inventory,                "
        "%s: %6.1f seconds\n",
        default_or_using,
        time_s_inventory);

    ex10_ex_eputs(
        "-t time, in seconds, to paused in low power mode,     "
        "%s: %6.1f seconds\n",
        default_or_using,
        time_s_low_power);

    ex10_ex_eputs(
        "-n the number of inventory -> lower power iterations, "
        "%s: %4zu   cycles\n",
        default_or_using,
        cycles);

    ex10_ex_eputs(
        "-p mode, the low power mode to use,                   "
        "%s: %4u   %s",
        default_or_using,
        low_power_mode,
        power_mode_string(low_power_mode));
    ex10_ex_eputs("\n");

    for (enum PowerMode power_mode = PowerModeOff;
         power_mode < PowerModeInvalid;
         ++power_mode)
    {
        ex10_ex_eputs(
            "         %u: %s\n", power_mode, power_mode_string(power_mode));
    }
}

// In unparsable, returns error_value.
static long int parse_int(char const* str, int error_value)
{
    if (str == NULL)
    {
        ex10_ex_eprintf("Missing argument\n");
        return error_value;
    }

    char*          endp  = NULL;
    long int const value = strtol(str, &endp, 0);
    if (*endp != 0)
    {
        ex10_ex_eprintf("Parsing %s as integer failed, pos: %c\n", str, *endp);
        return error_value;
    }

    return value;
}

static float parse_float(char const* str, float error_value)
{
    if (str == NULL)
    {
        ex10_ex_eprintf("Missing argument\n");
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

static int cycle_through_inventory_and_power_modes(
    enum PowerMode low_power_mode,
    uint32_t       time_us_inventory,
    uint32_t       time_ms_low_power,
    size_t         cycles)
{
    struct Ex10TimeHelpers const* time_helpers = get_ex10_time_helpers();
    struct Ex10GpioDriver const*  gpio_driver  = get_ex10_gpio_driver();

    // Host GPIO pins can be used to trigger instrumentation on specific
    // events:
    // - RPi GPIO pin 2, debug_pin(0):
    //   Falling edge indicates PowerModeOn operation running inventory.
    //   Rising  edge indicates the end of inventory.
    // - RPi GPIO pin 3, debug_pin(1):
    //   Falling edge indicates low power mode operation as specified
    //   on the command line.
    //   Rising edge indicates the end of low power mode.
    gpio_driver->debug_pin_set(0u, false);
    gpio_driver->debug_pin_set(1u, true);

    enum PowerMode power_mode = PowerModeInvalid;
    for (unsigned int iter = 0u; iter < cycles; ++iter)
    {
        struct InventoryRoundControlFields inventory_config = {
            .initial_q            = 8u,
            .max_q                = 15u,
            .min_q                = 0u,
            .num_min_q_cycles     = 1u,
            .fixed_q_mode         = false,
            .q_increase_use_query = false,
            .q_decrease_use_query = false,
            .session              = SessionS2,
            .select               = select_all,
            .target               = target_A,
            .halt_on_all_tags     = false,
            .tag_focus_enable     = false,
            .fast_id_enable       = false,
        };

        struct InventoryRoundControl_2Fields const inventory_config_2 = {
            .max_queries_since_valid_epc = 16u,
        };

        struct InventoryHelperParams inventory_params = {
            .antenna               = 1u,
            .rf_mode               = mode_11,
            .tx_power_cdbm         = 3000u,
            .inventory_config      = &inventory_config,
            .inventory_config_2    = &inventory_config_2,
            .send_selects          = false,
            .remain_on             = false,
            .dual_target           = true,
            .inventory_duration_ms = 0,
            .packet_info           = NULL,
            .verbose               = false,
        };

        // The goal is to inventory all tags within the field of view,
        // limiting the inventory rounds to 10 seconds if there are many
        // tags.
        struct StopConditions const stop_conditions = {
            .max_duration_us      = time_us_inventory,
            .max_number_of_rounds = 0u,
            .max_number_of_tags   = 0u,
        };

        ex10_ex_printf(
            "---------- iteration: %2u / %2zu:\n", iter + 1u, cycles);
        struct Ex10PowerModes const* ex10_power_modes = get_ex10_power_modes();
        power_mode = ex10_power_modes->get_power_mode();
        ex10_ex_printf("inventory power mode: %u, %s\n",
                       power_mode,
                       power_mode_string(power_mode));

        int const result =
            continuous_inventory(&inventory_params, &stop_conditions);

        // At least one tag must be inventoried and no errors encountered.
        if (result <= 0)
        {
            return -1;
        }

        gpio_driver->debug_pin_toggle(0u);
        ex10_power_modes->set_power_mode(low_power_mode);
        gpio_driver->debug_pin_toggle(1u);
        power_mode = ex10_power_modes->get_power_mode();
        ex10_ex_printf("low power mode: %u, %s\n",
                       power_mode,
                       power_mode_string(power_mode));
        time_helpers->busy_wait_ms(time_ms_low_power);

        gpio_driver->debug_pin_toggle(1u);
        ex10_power_modes->set_power_mode(PowerModeReady);
        gpio_driver->debug_pin_toggle(0u);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    ex10_ex_printf("Starting power modes example\n");

    float          time_s_inventory = 2.0;
    float          time_s_low_power = 2.0;
    enum PowerMode low_power_mode   = PowerModeOff;
    size_t         cycles           = 2u;

    char const* opt_spec = "T:t:p:n:h?";
    for (int opt_char = getopt(argc, argv, opt_spec); opt_char != -1;
         opt_char     = getopt(argc, argv, opt_spec))
    {
        switch (opt_char)
        {
            case 'T':
                time_s_inventory = parse_float(optarg, time_s_inventory);
                break;
            case 't':
                time_s_low_power = parse_float(optarg, time_s_low_power);
                break;
            case 'p':
            {
                long int const p_mode = parse_int(optarg, (int)low_power_mode);
                low_power_mode        = (enum PowerMode)p_mode;
            }
            break;
            case 'n':
            {
                long int const cycles_i = parse_int(optarg, (int)cycles);
                cycles                  = (size_t)cycles_i;
            }
            break;
            case 'h':
            case '?':
                print_usage(true,
                            time_s_inventory,
                            time_s_low_power,
                            cycles,
                            low_power_mode);
                return 0;
            default:
                ex10_ex_eprintf("Uknown argument specified: %c\n",
                                (char)opt_char);
                return -EINVAL;
        }
    }

    print_usage(
        false, time_s_inventory, time_s_low_power, cycles, low_power_mode);

    uint32_t const time_us_inventory =
        (uint32_t)lround(time_s_inventory * us_per_s);
    uint32_t const time_ms_low_power =
        (uint32_t)lround(time_s_low_power * ms_per_s);

    // Note: PowerModeReady can be used as a "low power mode". In this case
    // inventory will not be run, but the mode will be "Ready".
    bool const power_mode_ok =
        (low_power_mode >= PowerModeOff) && (low_power_mode <= PowerModeReady);
    if (power_mode_ok == false)
    {
        ex10_ex_eprintf("Invalid PowerMode: %d\n", low_power_mode);
        return -EINVAL;
    }

    if (time_us_inventory == 0)
    {
        ex10_ex_eprintf("Invalid time_us_inventory\n");
        return -EINVAL;
    }

    if (time_ms_low_power == 0)
    {
        ex10_ex_eprintf("Invalid time_ms_low_power\n");
        return -EINVAL;
    }

    if (cycles == 0)
    {
        ex10_ex_eprintf("Invalid cycles\n");
        return -EINVAL;
    }

    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    int result = cycle_through_inventory_and_power_modes(
        low_power_mode, time_us_inventory, time_ms_low_power, cycles);

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending power modes example\n");
    return result;
}
