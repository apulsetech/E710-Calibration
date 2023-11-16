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
 * @file app_upload_example.c
 * @details The application upload example is used to upload the Impinj reader
 *  chip firmware. It expects the first command line argument to be the yk_image
 *  file to be uploaded.
 */


#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/bootloader_registers.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/version_info.h"


static uint8_t image_array[EX10_MAX_IMAGE_BYTES];

static struct ConstByteSpan read_in_image(char const* image_file_name)
{
    FILE* fp = fopen(image_file_name, "r");
    if (fp == NULL)
    {
        ex10_ex_eprintf("fopen() failed: (%d) %s\n", errno, strerror(errno));
        return (struct ConstByteSpan){.data = NULL, .length = 0u};
    }

    fseek(fp, 0, SEEK_END);
    long int const file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_len <= 0)
    {
        ex10_ex_eprintf("ftell() failed: (%d) %s\n", errno, strerror(errno));
        return (struct ConstByteSpan){.data = NULL, .length = 0u};
    }

    size_t const file_length = (size_t)file_len;

    if (file_length >= sizeof(image_array))
    {
        ex10_ex_eprintf("ftell(): file size %zu > allocated image size %zu\n",
                        file_length,
                        sizeof(image_array));
    }

    size_t const n_read = fread(image_array, sizeof(uint8_t), file_length, fp);
    if (n_read != file_length)
    {
        ex10_ex_eprintf(
            "fread(): n_read %zu != file length %zu\n", n_read, file_length);
    }

    struct ConstByteSpan image = {
        .data   = image_array,
        .length = n_read,
    };

    fclose(fp);
    return image;
}

static int app_upload_example(struct Ex10Protocol const* protocol,
                              char const*                image_file_name)
{
    struct ConstByteSpan image_info = read_in_image(image_file_name);
    if (image_info.data == NULL || image_info.length == 0)
    {
        return errno;
    }

    // Reset into the bootloader and hold it there using the READY_N line.
    // In the case of a non-responsive application this hard reset and entry
    // to the bootloader is needed.
    protocol->reset(Bootloader);

    ex10_ex_printf("Uploading Application image...\n");
    const struct Ex10Result ex10_result =
        protocol->upload_image(UploadFlash, image_info);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("Upload failed.\n");
        print_ex10_result(ex10_result);
        return 1;
    }
    else
    {
        ex10_ex_printf("Upload complete.\n");
    }

    protocol->reset(Application);

    char                       ver_info[VERSION_STRING_SIZE];
    struct ImageValidityFields image_validity;
    get_ex10_version()->get_application_info(
        ver_info, sizeof(ver_info), &image_validity, NULL);

    ex10_ex_printf("%s\n", ver_info);

    if ((image_validity.image_valid_marker) &&
        !(image_validity.image_non_valid_marker))
    {
        ex10_ex_printf("Application image VALID\n");
    }
    else
    {
        ex10_ex_eprintf("Application image INVALID\n");
    }

    return (protocol->get_running_location() != Application);
}

int main(int argc, char* argv[])
{
    ex10_ex_printf("Starting App Upload example\n");

    // Ensure argc is 2 meaning the image file was specified
    // EX: sudo app_upload_example bin_file
    if (argc != 2)
    {
        ex10_ex_eprintf("No file passed in to upload.\n");
        return EINVAL;
    }

    // The argument input is the image file
    char const* file_name = argv[1];

    struct Ex10Result const ex10_result =
        ex10_bootloader_board_setup(BOOTLOADER_SPI_CLOCK_HZ);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_bootloader_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_bootloader_board_teardown();
        return -1;
    }

    struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();

    int result = app_upload_example(ex10_protocol, file_name);

    ex10_bootloader_board_teardown();
    ex10_ex_printf("Ending App Upload example\n");
    return result;
}
