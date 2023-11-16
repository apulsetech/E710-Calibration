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

/**
 * @file get_version_info.c
 * @details get version info example will read the parameters specified
 * from the Impinj reader chip and print them out to standard out.
 * The avaliable command line arguments are:
 *
 *   - a: returns application info
 *   - b: returns bootloader info
 *   - c: returns calibration info
 *   - d: returns device info
 *   - s: returns sku
 *   - R: revalidate application image
 */

#include "calibration.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/bootloader_registers.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/version_info.h"


static void calibration_version_info(struct Ex10Protocol const* ex10_protocol)
{
    // Attempt to reset into the Application
    ex10_protocol->reset(Application);

    ex10_ex_printf("Calibration version: ");

    if (ex10_protocol->get_running_location() == Application)
    {
        struct Ex10Calibration const* calibration = get_ex10_calibration();
        ex10_ex_printf("%u\n", calibration->get_cal_version());
    }
    else
    {
        // In the bootloader,
        // the calibration cannot be read via the Read command.
        ex10_ex_printf("Unknown\n");
    }
}

static void sku_info(struct Ex10Protocol const* ex10_protocol)
{
    // Attempt to reset into the Application
    ex10_protocol->reset(Application);

    // Read the sku back from the device
    enum ProductSku const sku_val = get_ex10_version()->get_sku();

    ex10_ex_printf("SKU: ");
    switch (sku_val)
    {
        case SkuE310:
            ex10_ex_printf("E310\n");
            break;
        case SkuE510:
            ex10_ex_printf("E510\n");
            break;
        case SkuE710:
            ex10_ex_printf("E710\n");
            break;
        case SkuE910:
            ex10_ex_printf("E910\n");
            break;
        case SkuUnknown:
        default:
            ex10_ex_printf("Unknown\n");
            break;
    }
}

static void device_info(void)
{
    const char* dev_info = get_ex10_version()->get_device_info();
    ex10_ex_printf("%s\n", dev_info);
}

static void app_version_info(struct Ex10Protocol const* ex10_protocol)
{
    char                       ver_info[VERSION_STRING_SIZE];
    struct ImageValidityFields image_validity;
    struct RemainReasonFields  remain_reason;

    // Attempt to reset into the Application
    ex10_protocol->reset(Application);

    get_ex10_version()->get_application_info(
        ver_info, sizeof(ver_info), &image_validity, &remain_reason);
    ex10_ex_printf("%s\n", ver_info);

    if ((image_validity.image_valid_marker) &&
        !(image_validity.image_non_valid_marker))
    {
        ex10_ex_printf("Application image VALID\n");
    }
    else
    {
        ex10_ex_printf("Application image INVALID\n");
    }

    ex10_ex_printf("Remain in bootloader reason: %s\n",
                   get_ex10_helpers()->get_remain_reason_string(
                       remain_reason.remain_reason));
}

static void bootloader_version_info(void)
{
    char ver_info[VERSION_STRING_SIZE];

    get_ex10_version()->get_bootloader_info(ver_info, sizeof(ver_info));
    ex10_ex_printf("%s\n", ver_info);
}

static void revalidate_application_image(void)
{
    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();
    enum Status const initial_status = ex10_protocol->get_running_location();
    if (initial_status == Application)
    {
        ex10_protocol->reset(Bootloader);
    }

    if (ex10_protocol->get_running_location() != Bootloader)
    {
        ex10_ex_eprintf("reset(Bootloader) failed\n");
        return;
    }

    struct ImageValidityFields const image_validity =
        ex10_protocol->revalidate_image();

    ex10_ex_printf("image_valid_marker    : %u\n",
                   image_validity.image_valid_marker);
    ex10_ex_printf("image_non_valid_marker: %u\n",
                   image_validity.image_non_valid_marker);

    if ((image_validity.image_valid_marker == false) ||
        (image_validity.image_non_valid_marker == true))
    {
        ex10_ex_eprintf("--- image is invalid ---\n");
        return;
    }
    else
    {
        // Reseting into the Application will not work if the image
        // validity markers do not indicate a valid image.
        if (initial_status == Application)
        {
            ex10_protocol->reset(Application);
        }
    }
}

static void print_help(void)
{
    ex10_ex_printf("No args given. Accepted arguments are...\n");
    ex10_ex_printf("a: returns application info,\n");
    ex10_ex_printf("b: returns bootloader info,\n");
    ex10_ex_printf("c: returns calibration info\n");
    ex10_ex_printf("d: returns device info,\n");
    ex10_ex_printf("s: returns sku\n");
    ex10_ex_printf("R: revalidate application image\n");
}

int main(int argc, char* argv[])
{
    ex10_ex_printf("Starting version info example\n");
    int result = 0;

    // Initialize version_list to print all, except revalidate image.
    // The default set of versions to print when no arguments are specified.
    // Leave one extra blank char for the 'R' (revalidate image) parameter,
    // which is not executed by default.
    char         version_list[]    = {'a', 'b', 'c', 'd', 's', ' '};
    size_t const version_list_size = ARRAY_SIZE(version_list);
    size_t       param_length      = version_list_size;

    if (argc > 1)
    {
        // Command arguments specified.
        // argc is all params and 1 count for the script itself
        param_length = (size_t)argc - 1u;
        if (param_length > version_list_size)
        {
            ex10_ex_eprintf(
                "Command line argument count %zu "
                "exceeds allowed count %zu, extra arguments dropped\n",
                param_length,
                version_list_size);
            param_length = version_list_size;
        }

        for (size_t i = 0; i < param_length; i++)
        {
            // skip the script name in argv
            version_list[i] = *(argv[i + 1]);
        }
    }

    struct Ex10Result const ex10_result =
        ex10_bootloader_board_setup(BOOTLOADER_SPI_CLOCK_HZ);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_bootloader_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();

    for (size_t i = 0; i < param_length; i++)
    {
        // The argument input is the image file
        char version_arg = version_list[i];

        switch (version_arg)
        {
            case 'a':
                app_version_info(ex10_protocol);
                break;
            case 'b':
                bootloader_version_info();
                break;
            case 'c':
                calibration_version_info(ex10_protocol);
                break;
            case 'd':
                device_info();
                break;
            case 's':
                sku_info(ex10_protocol);
                break;
            case 'R':
                revalidate_application_image();
                break;
            case ' ':
                // Do nothing for empty param; i.e. space char.
                break;
            default:
                print_help();
                result = -1;
                break;
        }
    }

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending version info example\n");
    return result;
}
