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
#include <stdint.h>

#include "ex10_api/application_register_definitions.h"
#include "ex10_api/application_register_field_enums.h"
#include "ex10_api/byte_span.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/ex10_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/// The maximum length of a Ex10 bootloader command and response.
#define EX10_BOOTLOADER_MAX_COMMAND_SIZE ((size_t)(2048u + 4u))

/// Use the max bootloader cmd size less 2 bytes (one byte for command code
/// and one byte for the destination).
#define EX10_MAX_IMAGE_CHUNK_SIZE \
    ((size_t)(EX10_BOOTLOADER_MAX_COMMAND_SIZE - 2))

/// The nominal timeout to wait for the Ex10 chip to process a command
/// and assert READY_N low, indicating that it is ready for a new command or
/// that it is ready to send out a response.
#define NOMINAL_READY_N_TIMEOUT_MS ((uint32_t)2500u)

// clang-format off
// IPJ_autogen | gen_c_app_ex10_api_formatting {
// The formatting for the given Ex10 API commands
#pragma pack(push, 1)

struct Ex10ReadFormat
{
    uint16_t address;
    uint16_t length;
};
struct Ex10WriteFormat
{
    uint16_t address;
    uint16_t length;
    uint8_t const* data;
};
struct Ex10ReadFifoFormat
{
    uint8_t fifo_select;
    uint16_t transfer_size;
};
struct Ex10StartUploadFormat
{
    uint8_t upload_code;
    uint8_t const* upload_data;
};
struct Ex10ContinueUploadFormat
{
    uint8_t const* upload_data;
};
struct Ex10ResetFormat
{
    uint8_t destination;
};
struct Ex10TestTransferFormat
{
    uint8_t const* data;
};
struct Ex10WriteInfoPageFormat
{
    uint8_t page_id;
    uint8_t const* data;
    uint16_t crc;
};
struct Ex10TestReadFormat
{
    uint32_t address;
    uint16_t length;
};
struct Ex10InsertFifoEventFormat
{
    uint8_t trigger_irq;
    uint8_t const* packet;
};

#pragma pack(pop)
// IPJ_autogen }
// clang-format on

enum FifoSelection
{
    EventFifo = 0,
};

struct Ex10Commands
{
    /**
     * Read one or more byte spans of memory on the device.
     * Given an addresses and lengths to read, access the device over SPI bus to
     * read the byte spans.
     *
     * @param reg_list           A pointer to a list of register_info structs,
     *                           from which the address and length are used to
     *                           form the Read command(s).
     * @param buffers            A list of buffers that will be filled with
     *                           segments read from the Impinj Reader Chip.
     * @param segment_count      The number of registers to read from, which
     *                           must match the number of nodes in both the
     *                           reg_list[] and buffers[].
     * @param ready_n_timeout_ms The number of milliseconds to wait for the
     *                           Ex10 READY_N line to assert low.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*read)(struct RegisterInfo const* const reg_list[],
                              void*                            buffers[],
                              size_t                           segment_count,
                              uint32_t ready_n_timeout_ms);

    /**
     * Read an Ex10 register blob.
     *
     * @param address          Address to begin reading from.
     * @note                   This must be 32-bit aligned.
     * @param length_in_bytes  Number of bytes to read from the register.
     * @note                   This must be a multiple of 4 bytes.
     * @param read_buffer      The data buffer into which the read data will
     *                         be copied into.
     *
     * @note This function assumes that the read_buffer parameter is at least as
     *       large as the length parameter.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*test_read)(uint32_t address,
                                   uint16_t length_in_bytes,
                                   void*    read_buffer);

    /**
     * Write one or more byte spans on the device.
     * Given an array of byte spans, access the device over SPI to write.
     *
     * @param reg_list           A pointer to a list of register_info
     *                           structs, from which the address and length are
     *                           used to form the Write command(s).
     * @param buffers            A pointer to a list of buffers containing data
     *                           which will be placed into the Write command and
     *                           sent to the Impinj Reader Chip.
     * @param segment_count      The number of registers to write to, which must
     *                           match the number of nodes in both the
     *                           reg_list[] and buffers[] arrays.
     * @param ready_n_timeout_ms The number of milliseconds to wait for the
     *                           Ex10 READY_N line to assert low.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*write)(const struct RegisterInfo* const reg_list[],
                               void const* const                buffers[],
                               size_t                           segment_count,
                               uint32_t ready_n_timeout_ms);

    /**
     * Read the specified number of bytes from device FIFO stream.
     * Given a selected fifo, read bytes from the fifo into a ByteSpan.
     *
     * Multiple ReadFifo commands are made until the byte_span length
     * value is filled to capacity.
     *
     * @param selection Which FIFO to read from.
     *                  In the Ex10 only the EventFifo (0) is supported.
     * @param byte_span A struct with a pointer to a ByteSpan struct to hold
     *                  the requested bytes, along with a length.
     *                  The byte_span data buffer should be able to hold one
     *                  more byte than the number of bytes to read from the
     *                  fifo. This extra byte will hold the response code
     *                  from the device.
     * @return Returns the Ex10Result struct to inform the caller
     *         what type of issues happened during the command.
     *         Can return a host_result error if the passed data address for
     *         reading the fifo is not 32 bit aligned.
     *         Can return a device_response error if the first byte read back
     *         from the device is not a Success code.
     *         Can return a host_result error if the length read back is not
     *         what was expected from the response.
     * @note If 32-bit alignment of the event fifo messages is required then
     *       byte_span->data[1] must be u32 aligned since data[0] will contain
     *       the response code.
     *
     * The byte_span->length should be the number of bytes obtained by reading
     * the EventFifoNumBytes register. It does not include the response code
     * byte.
     *
     * The byte_span->length value will be updated with the number of bytes read
     * using the ReadFifo command into the buffer.
     *
     * @warning If the byte_span value is greater than the number of bytes
     *          contained in the fifo then this function will continue to read
     *          from the fifo until the length value is satisfied. Therefore the
     *          byte_span length value should contain the value read from
     *          EventFifoNumBytes register.
     */
    struct Ex10Result (*read_fifo)(enum FifoSelection selection,
                                   struct ByteSpan*   byte_span);

    /**
     * Erase and write to an info page.
     *
     * @param page_code  Tells the bootloader which info page to work with.
     * @param image_data A struct with a pointer to a struct ConstByteSpan which
     *                   contains the image_data to be uploaded and length.
     * @param crc16      crc16-ccitt calculated over image_data bytes
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*write_info_page)(uint8_t                     page_code,
                                         const struct ConstByteSpan* image_data,
                                         uint16_t                    crc16);

    /**
     * Initiate an image upload while in the bootloader.
     *
     * @param code       Tells the bootloader where to upload the App image in
     *                    memory.
     * @param image_data A struct with a pointer to a struct ConstByteSpan which
     *                   contains the image_data to be uploaded and length.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*start_upload)(uint8_t                     code,
                                      const struct ConstByteSpan* image_data);

    /**
     * Continue the FW upload process. Should have been preceded by a start
     * upload or continue upload command.
     *
     * @param image_data A struct with a pointer to a struct ConstByteSpan which
     *                   contains the image_data to be uploaded and length.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*continue_upload)(
        const struct ConstByteSpan* image_data);

    /**
     * Complete an image upload after all image data has been sent. Should have
     * been preceded by a start upload or continue upload command.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*complete_upload)(void);

    /**
     * Forces re-validation of the app image from the bootloader.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*revalidate_main_image)(void);

    /**
     * Soft reset of the device.
     *
     * @param destination Where to go after the reset:
     *                    1 - Bootloader
     *                    2 - Application
     * @note This command has no response.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*reset)(enum Status destination);

    /**
     * Run the Transfer Test command and get the response.
     *
     * @param send A ConstByteSpan to send as the transfer test payload.
     * @param recv A ByteSpan of received bytes from the transfer test. ByteSpan
     *             length must be => length of the ByteSpan to send.
     * @param verify Check the test response and return error if incorrect.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*test_transfer)(struct ConstByteSpan const* send,
                                       struct ByteSpan*            recv,
                                       bool                        verify);

    /**
     * Emplace an EventFifo packet of data into a data buffer.
     *
     * @param event_packet   The EventFifo data to place into the data buffer.
     * @param command_buffer The destination buffer into which the EventFifo
     *                       data will be formatted and written.
     * @param padding_bytes  The number of padding bytes to append to the end
     *                       of the EventFifo packet payload. Padding bytes are
     *                       required to maintain EventFifo packet 32-bit
     *                       alignment.
     * @param packet_bytes   The total number of bytes, including padding bytes,
     *                       required by the EventFifo packet.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*create_fifo_event)(
        struct EventFifoPacket const* event_packet,
        uint8_t*                      command_buffer,
        size_t const                  padding_bytes,
        size_t const                  packet_bytes);

    /**
     * Insert an arbitrary EventFifo packet into the Event Fifo stream.
     *
     * @param trigger_irq  A bool that indicates a fifo above threshold
     *                     interrupt will be generated.
     * @param event_packet The EventFifo packet to append to the end of the Ex10
     *                     message queue.
     *
     * @note This command has no response
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*insert_fifo_event)(
        const bool                    trigger_irq,
        struct EventFifoPacket const* event_packet);
};

struct Ex10Commands const* get_ex10_commands(void);

#ifdef __cplusplus
}
#endif
