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

#include <string.h>

#include "board/board_spec.h"
#include "board/ex10_osal.h"

#include "ex10_api/byte_span.h"
#include "ex10_api/command_transactor.h"
#include "ex10_api/commands.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_print.h"

/**
 * The Ex10 API commands
 *
 * This file implements the commands found in the ex10_api documentation.
 *
 * It sits directly above the connection to the device, executes commands
 * from the client and returns data and responses from the device.
 *
 * All inputs and returns are bytes (or pointers to same) at this layer
 * of the design.
 *
 * @note In the SDK there are 2 threads accessing this layer:
 *       - The Ex10Protocol.interrupt_handler()
 *       - The "main()" process thread.
 *       It is essential that a mutex be used to resolved conflicting access
 *       to resources shared across these threads. These resources include:
 *       - Access to the host and gpio interfaces.
 *       - Access to the command_buffer[] and response_buffer[] objects.
 */

static uint8_t command_buffer[EX10_SPI_BURST_SIZE];
static uint8_t response_buffer[EX10_SPI_BURST_SIZE];

static uint16_t const command_code_length  = sizeof(uint8_t);
static uint16_t const response_code_length = sizeof(uint8_t);

static struct Ex10Result check_buffer_reg(void const*                buffers,
                                          struct RegisterInfo const* reg)
{
    if (buffers == NULL || reg == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10SdkErrorNullPointer);
    }

    if (((uint32_t)reg->address + (uint32_t)reg->length * reg->num_entries) >
        UINT16_MAX)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands,
                                   Ex10SdkErrorBadParamValue);
    }

    return make_ex10_success();
}

static struct Ex10Result command_read(
    struct RegisterInfo const* const reg_list[],
    void*                            buffers[],
    size_t                           segment_count,
    uint32_t                         ready_n_timeout_ms)
{
    if (reg_list == NULL || buffers == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10SdkErrorNullPointer);
    }

    command_buffer[0] = (uint8_t)CommandRead;

    uint16_t command_offset  = command_code_length;   // Format command_buffer
    uint16_t response_length = response_code_length;  // Target response_buffer
    size_t   segment_index   = 0u;  // Read command segment iterator
    uint16_t segment_offset  = 0u;  // Offset into each read command segment
    size_t   buffer_index    = 0u;  // buffers[] iterator copying from response
    uint16_t buffer_offset   = 0u;  // buffers[] offset, >0 during partial copy

    struct Ex10Result ex10_result = make_ex10_success();

    while (segment_index < segment_count)
    {
        // Indentation block inputs, outputs:
        //   command_offset, response_length, segment_index, segment_offset
        {
            struct RegisterInfo const* const reg = reg_list[segment_index];
            ex10_result = check_buffer_reg(buffers[segment_index], reg);
            if (ex10_result.error)
            {
                return ex10_result;
            }

            uint16_t const segment_length    = reg->length * reg->num_entries;
            uint16_t const segment_remaining = segment_length - segment_offset;

            uint16_t const response_avail =
                sizeof(response_buffer) - response_length;
            uint16_t const read_length = (segment_remaining < response_avail)
                                             ? segment_remaining
                                             : response_avail;
            response_length += read_length;

            uint16_t const read_address      = reg->address + segment_offset;
            command_buffer[command_offset++] = (uint8_t)(read_address >> 0u);
            command_buffer[command_offset++] = (uint8_t)(read_address >> 8u);
            command_buffer[command_offset++] = (uint8_t)(read_length >> 0u);
            command_buffer[command_offset++] = (uint8_t)(read_length >> 8u);

            segment_offset += read_length;
            if (segment_offset >= segment_length)
            {
                segment_offset = 0u;
                segment_index += 1u;
            }
        }

        bool const read_segments_ready = segment_index >= segment_count;
        bool const command_buffer_full =
            (command_offset + sizeof(struct Ex10ReadFormat) >
             sizeof(command_buffer));
        bool const response_buffer_full =
            response_length >= sizeof(response_buffer);

        if (read_segments_ready || command_buffer_full || response_buffer_full)
        {
            ex10_result = get_ex10_command_transactor()->send_and_recv_bytes(
                command_buffer,
                command_offset,
                response_buffer,
                response_length,
                ready_n_timeout_ms);
            if (ex10_result.error)
            {
                return ex10_result;
            }

            enum ResponseCode device_response =
                (enum ResponseCode)response_buffer[0];

            if (device_response != Success)
            {
                return make_ex10_commands_w_resp_error(
                    device_response, CommandRead, HostResultSuccess);
            }

            // Copy the data from the response buffer to the buffers[] passed
            // in from the client.
            uint16_t response_offset = response_code_length;
            do
            {
                struct RegisterInfo const* const reg = reg_list[buffer_index];
                uint16_t const segment_length = reg->length * reg->num_entries;
                uint16_t const segment_remaining =
                    segment_length - buffer_offset;
                uint16_t const response_avail =
                    response_length - response_offset;
                uint16_t const segment_copy_length =
                    (segment_remaining < response_avail) ? segment_remaining
                                                         : response_avail;
                uint8_t* buffer_ptr =
                    (uint8_t*)(buffers[buffer_index]) + buffer_offset;
                int const copy_result =
                    ex10_memcpy(buffer_ptr,
                                sizeof(response_buffer) - response_offset,
                                &response_buffer[response_offset],
                                segment_copy_length);
                if (copy_result != 0)
                {
                    return make_ex10_sdk_error(Ex10ModuleCommands,
                                               Ex10MemcpyFailed);
                }
                response_offset += segment_copy_length;
                buffer_offset += segment_copy_length;
                if (buffer_offset == segment_length)
                {
                    buffer_offset = 0u;
                    buffer_index += 1u;
                }
            } while (response_offset < response_length);

            // Initialize command and response buffers to empty.
            // Note: command_buffer[0] is already set to CommandRead.
            command_offset  = command_code_length;
            response_length = response_code_length;
        }
    }

    return ex10_result;
}

static struct Ex10Result command_test_read(uint32_t address,
                                           uint16_t length_in_bytes,
                                           void*    read_buffer)
{
    // The TestRead command takes in chunks of 4 bytes.
    // If the address or length are misaligned then Ex10 will fail the command.
    if (address % sizeof(uint32_t) != 0u ||
        length_in_bytes % sizeof(uint32_t) != 0u)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands,
                                   Ex10SdkErrorBadParamAlignment);
    }

    // Response starts as one response byte. Ensure response fits.
    size_t const response_length = response_code_length + length_in_bytes;
    if (response_length > sizeof(response_buffer))
    {
        return make_ex10_sdk_error(Ex10ModuleCommands,
                                   Ex10SdkErrorBadParamLength);
    }

    // Response starts as one command byte. Ensure command fits.
    size_t const command_length =
        command_code_length + sizeof(struct Ex10TestReadFormat);

    if (command_length > sizeof(command_buffer))
    {
        return make_ex10_sdk_error(Ex10ModuleCommands,
                                   Ex10SdkErrorBadParamLength);
    }

    // Place the command in the output
    uint8_t* command_ptr = command_buffer;
    *command_ptr++       = (uint8_t)CommandTestRead;

    uint16_t const u32_words_read = length_in_bytes / sizeof(uint32_t);

    // put the u32 length and address into the output
    *command_ptr++ = (uint8_t)(address >> 0u);
    *command_ptr++ = (uint8_t)(address >> 8u);
    *command_ptr++ = (uint8_t)(address >> 16u);
    *command_ptr++ = (uint8_t)(address >> 24u);
    *command_ptr++ = (uint8_t)(u32_words_read >> 0u);
    *command_ptr++ = (uint8_t)(u32_words_read >> 8u);

    // send the command and read in the response
    struct Ex10Result ex10_result =
        get_ex10_command_transactor()->send_and_recv_bytes(
            command_buffer,
            command_length,
            response_buffer,
            response_length,
            NOMINAL_READY_N_TIMEOUT_MS);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    enum ResponseCode device_response = (enum ResponseCode)response_buffer[0u];
    if (device_response != Success)
    {
        return make_ex10_commands_w_resp_error(
            device_response, CommandTestRead, HostResultSuccess);
    }

    // Copy the data read from the Ex10 into the user supplied read_buffer.
    // Do not copy the response code.
    int const copy_result = ex10_memcpy(read_buffer,
                                        length_in_bytes,
                                        &response_buffer[response_code_length],
                                        response_length - response_code_length);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10MemcpyFailed);
    }
    return make_ex10_success();
}

static struct Ex10Result command_write(
    struct RegisterInfo const* const reg_list[],
    void const* const                buffers[],
    size_t                           segment_count,
    uint32_t                         ready_n_timeout_ms)
{
    if (reg_list == NULL || buffers == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10SdkErrorNullPointer);
    }

    uint8_t*             command_ptr = &command_buffer[0u];
    uint8_t const* const command_end = &command_buffer[sizeof(command_buffer)];
    *command_ptr++                   = (uint8_t)CommandWrite;

    uint16_t const write_desc_length =
        sizeof(struct Ex10WriteFormat) - sizeof(uint8_t const*);

    struct Ex10Result ex10_result = make_ex10_success();
    for (size_t iter = 0; iter < segment_count; ++iter)
    {
        ex10_result = check_buffer_reg(buffers[iter], reg_list[iter]);
        if (ex10_result.error)
        {
            return ex10_result;
        }

        uint16_t const reg_entry_length =
            reg_list[iter]->length * reg_list[iter]->num_entries;
        uint16_t const reg_entry_address = reg_list[iter]->address;

        uint16_t buffer_offset = 0u;
        while (buffer_offset < reg_entry_length)
        {
            // If there is room to fill the command buffer with more data,
            // do it. Otherwise, empty the command buffer by writing it to
            // the host interface.
            if (command_ptr + write_desc_length + 16u <= command_end)
            {
                uint16_t const payload_address =
                    reg_entry_address + buffer_offset;
                uint16_t const command_avail =
                    (uint16_t)(command_end - (command_ptr + write_desc_length));
                uint16_t const buffer_remain =
                    (uint16_t)(reg_entry_length - buffer_offset);
                uint16_t const payload_length = (command_avail < buffer_remain)
                                                    ? command_avail
                                                    : buffer_remain;

                *command_ptr++ = (uint8_t)(payload_address >> 0u);
                *command_ptr++ = (uint8_t)(payload_address >> 8u);
                *command_ptr++ = (uint8_t)(payload_length >> 0u);
                *command_ptr++ = (uint8_t)(payload_length >> 8u);

                uint8_t const* buffer_ptr = (uint8_t const*)buffers[iter];
                int const      copy_result =
                    ex10_memcpy(command_ptr,
                                (size_t)(command_end - command_ptr),
                                buffer_ptr + buffer_offset,
                                payload_length);
                if (copy_result != 0)
                {
                    return make_ex10_sdk_error(Ex10ModuleCommands,
                                               Ex10MemcpyFailed);
                }
                command_ptr += payload_length;
                buffer_offset += payload_length;
            }
            else
            {
                size_t const command_length =
                    (size_t)(command_ptr - command_buffer);
                ex10_result = get_ex10_command_transactor()->send_command(
                    command_buffer, command_length, ready_n_timeout_ms);
                if (ex10_result.error)
                {
                    return ex10_result;
                }
                command_ptr = &command_buffer[command_code_length];
            }
        }
    }

    size_t const command_length = (size_t)(command_ptr - command_buffer);
    if (command_length > command_code_length)
    {
        // Do not send a Write command unless there were segments copied
        // into the command buffer; an error will be recorded in the Ex10
        // CommandResult register.
        ex10_result = get_ex10_command_transactor()->send_command(
            command_buffer, command_length, ready_n_timeout_ms);
        if (ex10_result.error)
        {
            return ex10_result;
        }
    }

    return ex10_result;
}

static struct Ex10Result command_read_fifo(enum FifoSelection fifo_select,
                                           struct ByteSpan*   bytes)
{
    if (bytes == NULL || bytes->data == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10SdkErrorNullPointer);
    }

    struct Ex10Result ex10_result = make_ex10_success();

    // The byte that follows byte[0] must be 32-bit aligned.
    // byte[0] will contain the initial response code.
    // byte[1] will contain the first byte of event packet data.
    if ((uintptr_t)bytes->data % sizeof(uint32_t) != 0u)
    {
        /*ex10_eprintf(
                     "Passed address of %p was not 32 bit aligned\n",
                     bytes->data);*/
        return make_ex10_sdk_error(Ex10ModuleCommands,
                                   Ex10SdkErrorBadParamAlignment);
    }

    size_t   fifo_bytes_remaining = bytes->length;
    uint8_t* data_ptr             = bytes->data;
    bytes->length                 = 0;
    while (fifo_bytes_remaining > 0u)
    {
        // The EX10_SPI_BURST_SIZE is also the maximum response length
        // with the response code byte. i.e. The total available bytes in
        // the response can be one byte more than the maximum fifo_len.
        size_t const fifo_len = (fifo_bytes_remaining > EX10_SPI_BURST_SIZE - 1)
                                    ? EX10_SPI_BURST_SIZE - 1
                                    : fifo_bytes_remaining;
        size_t const resp_len = fifo_len + 1u;

        uint8_t const command[1u + sizeof(struct Ex10ReadFifoFormat)] = {
            (uint8_t)CommandReadFifo,
            (uint8_t)fifo_select,
            (uint8_t)(fifo_len >> 0u),
            (uint8_t)(fifo_len >> 8u)};
        ex10_result = get_ex10_command_transactor()->send_command(
            command, sizeof(command), NOMINAL_READY_N_TIMEOUT_MS);
        if (ex10_result.error)
        {
            return ex10_result;
        }

        // The result code from the response will overwrite the last byte of
        // the last fifo packet read into the buffer.
        // Record its value so that it can be restored.
        uint8_t const restore_byte = *(--data_ptr);

        ex10_result = get_ex10_command_transactor()->receive_response(
            data_ptr, resp_len, NOMINAL_READY_N_TIMEOUT_MS);
        if (ex10_result.error)
        {
            return ex10_result;
        }

        enum ResponseCode device_response = (enum ResponseCode)(*data_ptr);
        if (device_response != Success)
        {
            return make_ex10_commands_w_resp_error(
                device_response, CommandReadFifo, HostResultSuccess);
        }

        *data_ptr++ = restore_byte;

        // The next data_ptr position will transfer the response code into
        // the last byte of the last response byte transferred.
        data_ptr += fifo_len;
        fifo_bytes_remaining -= fifo_len;
        bytes->length += fifo_len;
    }

    return ex10_result;
}

/**
 * @details
 * The WriteInfoPage command format:
 *
 *   Offset |  Size | Description
 *   -------|-------|------------
 *   0      | 1     | Command Code
 *   1      | 1     | PageId
 *   2      | N     | Image Data
 *   N + 2  | 2     | CRC-16
 *
 * The total command length = image_data->length + 4
 *
 * @note The WriteInfoPage command requires all data be written in host
 * interface transfer operation. The info page is 2048 bytes long,
 * which exceeds the EX10_SPI_BURST_SIZE limit.
 * In this case a local buffer EX10_BOOTLOADER_MAX_COMMAND_SIZE in size
 * is used to implement the host interface transfer.
 */
static struct Ex10Result command_write_info_page(
    uint8_t                     page_id,
    const struct ConstByteSpan* image_data,
    uint16_t                    crc16)
{
    if (image_data == NULL || image_data->data == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10SdkErrorNullPointer);
    }

    size_t const command_overhead_length =
        command_code_length + sizeof(struct Ex10WriteInfoPageFormat) -
        sizeof(uint8_t*);

    if (command_overhead_length + image_data->length >
        EX10_BOOTLOADER_MAX_COMMAND_SIZE)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands,
                                   Ex10SdkErrorBadParamLength);
    }

    uint8_t              bl_command_buffer[EX10_BOOTLOADER_MAX_COMMAND_SIZE];
    uint8_t*             command_ptr = bl_command_buffer;
    uint8_t const* const command_end =
        &bl_command_buffer[sizeof(bl_command_buffer)];
    *command_ptr++ = (uint8_t)CommandWriteInfoPage;
    *command_ptr++ = page_id;

    int const copy_result = ex10_memcpy(command_ptr,
                                        (size_t)(command_end - command_ptr),
                                        image_data->data,
                                        image_data->length);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10MemcpyFailed);
    }

    command_ptr += image_data->length;

    *command_ptr++ = (uint8_t)(crc16 >> 0u);
    *command_ptr++ = (uint8_t)(crc16 >> 8u);

    size_t const command_length = (size_t)(command_ptr - bl_command_buffer);

    struct Ex10Result ex10_result =
        get_ex10_command_transactor()->send_and_recv_bytes(
            bl_command_buffer,
            command_length,
            response_buffer,
            response_code_length,
            NOMINAL_READY_N_TIMEOUT_MS);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    enum ResponseCode device_response = (enum ResponseCode)response_buffer[0u];
    if (device_response != Success)
    {
        return make_ex10_commands_w_resp_error(
            device_response, CommandWriteInfoPage, HostResultSuccess);
    }

    return make_ex10_success();
}

static struct Ex10Result command_start_upload(
    uint8_t                     code,
    const struct ConstByteSpan* image_data)
{
    if (image_data == NULL || image_data->data == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10SdkErrorNullPointer);
    }

    // -1 is from the destination byte passed as part of the command
    if (image_data->length >= (EX10_MAX_IMAGE_CHUNK_SIZE - 1))
    {
        return make_ex10_sdk_error(Ex10ModuleCommands,
                                   Ex10SdkErrorBadParamLength);
    }

    uint8_t*             command_ptr = command_buffer;
    uint8_t const* const command_end = &command_buffer[sizeof(command_buffer)];
    *command_ptr++                   = (uint8_t)CommandStartUpload;
    *command_ptr++                   = code;
    int const copy_result            = ex10_memcpy(command_ptr,
                                        (size_t)(command_end - command_ptr),
                                        image_data->data,
                                        image_data->length);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10MemcpyFailed);
    }

    command_ptr += image_data->length;

    return get_ex10_command_transactor()->send_command(
        command_buffer,
        (size_t)(command_ptr - command_buffer),
        NOMINAL_READY_N_TIMEOUT_MS);
}

static struct Ex10Result command_continue_upload(
    const struct ConstByteSpan* image_data)
{
    if (image_data == NULL || image_data->data == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10SdkErrorNullPointer);
    }

    if (image_data->length >= EX10_MAX_IMAGE_CHUNK_SIZE)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands,
                                   Ex10SdkErrorBadParamLength);
    }

    uint8_t*             command_ptr = command_buffer;
    uint8_t const* const command_end = &command_buffer[sizeof(command_buffer)];
    *command_ptr++                   = (uint8_t)CommandContinueUpload;
    int const copy_result            = ex10_memcpy(command_ptr,
                                        (size_t)(command_end - command_ptr),
                                        image_data->data,
                                        image_data->length);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10MemcpyFailed);
    }
    command_ptr += image_data->length;

    return get_ex10_command_transactor()->send_command(
        command_buffer,
        (size_t)(command_ptr - command_buffer),
        NOMINAL_READY_N_TIMEOUT_MS);
}

static struct Ex10Result command_complete_upload(void)
{
    uint8_t* command_ptr = command_buffer;
    *command_ptr++       = (uint8_t)CommandCompleteUpload;
    return get_ex10_command_transactor()->send_command(
        command_buffer,
        (size_t)(command_ptr - command_buffer),
        NOMINAL_READY_N_TIMEOUT_MS);
}

static struct Ex10Result command_revalidate_main_image(void)
{
    uint8_t* command_ptr = command_buffer;
    *command_ptr++       = (uint8_t)CommandReValidateMainImage;
    return get_ex10_command_transactor()->send_command(
        command_buffer,
        (size_t)(command_ptr - command_buffer),
        NOMINAL_READY_N_TIMEOUT_MS);
}

static struct Ex10Result command_reset(enum Status destination)
{
    uint8_t* command_ptr = command_buffer;
    *command_ptr++       = (uint8_t)CommandReset;
    *command_ptr++       = (uint8_t)destination;
    return get_ex10_command_transactor()->send_command(
        command_buffer,
        (size_t)(command_ptr - command_buffer),
        NOMINAL_READY_N_TIMEOUT_MS);
}

static struct Ex10Result test_transfer(struct ConstByteSpan const* send,
                                       struct ByteSpan*            recv,
                                       bool                        verify)
{
    if (send == NULL || send->data == NULL || recv == NULL ||
        recv->data == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10SdkErrorNullPointer);
    }

    if (send->length >= EX10_SPI_BURST_SIZE)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands,
                                   Ex10SdkErrorBadParamLength);
    }

    size_t               transfer_length = command_code_length + send->length;
    uint8_t*             command_ptr     = command_buffer;
    uint8_t const* const command_end = &command_buffer[sizeof(command_buffer)];
    *command_ptr++                   = (uint8_t)CommandTestTransfer;

    int copy_result = ex10_memcpy(command_ptr,
                                  (size_t)(command_end - command_ptr),
                                  send->data,
                                  send->length);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10MemcpyFailed);
    }

    struct Ex10Result ex10_result =
        get_ex10_command_transactor()->send_and_recv_bytes(
            command_buffer,
            transfer_length,
            response_buffer,
            transfer_length,
            NOMINAL_READY_N_TIMEOUT_MS);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    enum ResponseCode device_response = (enum ResponseCode)response_buffer[0u];
    if (device_response != Success)
    {
        return make_ex10_commands_w_resp_error(
            device_response, CommandTestTransfer, HostResultSuccess);
    }

    recv->length = (size_t)(transfer_length - response_code_length);
    copy_result  = ex10_memcpy(recv->data,
                              transfer_length,
                              &response_buffer[response_code_length],
                              recv->length);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10MemcpyFailed);
    }

    if (verify)
    {
        for (size_t i = 0; i < send->length; i++)
        {
            uint8_t const expected = (uint8_t)(send->data[i] + i);
            if (expected != recv->data[i])
            {
                ex10_eprintf(
                    "Received '%0X' expected '%0X'\n", recv->data[i], expected);
                return make_ex10_commands_w_resp_error(
                    Success,
                    CommandTestTransfer,
                    HostResultTestTransferVerifyError);
            }
        }
    }
    return make_ex10_success();
}

static struct Ex10Result create_fifo_event(
    struct EventFifoPacket const* event_packet,
    uint8_t*                      command_ptr,
    size_t const                  padding_bytes,
    size_t const                  packet_bytes)
{
    if (event_packet == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10SdkErrorNullPointer);
    }

    uint8_t const* const end_ptr = command_ptr + packet_bytes;

    struct PacketHeader packet_header =
        get_ex10_event_parser()->make_packet_header(event_packet->packet_type);

    packet_header.packet_length = (uint8_t)(packet_bytes / sizeof(uint32_t));

    size_t const calculated_packet_bytes =
        sizeof(packet_header) + event_packet->static_data_length +
        event_packet->dynamic_data_length + padding_bytes;

    if (packet_bytes != calculated_packet_bytes)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands,
                                   Ex10SdkErrorBadParamValue);
    }

    int copy_result = ex10_memcpy(command_ptr,
                                  (size_t)(end_ptr - command_ptr),
                                  &packet_header,
                                  sizeof(packet_header));
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10MemcpyFailed);
    }
    command_ptr += sizeof(packet_header);

    copy_result = ex10_memcpy(command_ptr,
                              (size_t)(end_ptr - command_ptr),
                              event_packet->static_data,
                              event_packet->static_data_length);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10MemcpyFailed);
    }
    command_ptr += event_packet->static_data_length;

    copy_result = ex10_memcpy(command_ptr,
                              (size_t)(end_ptr - command_ptr),
                              event_packet->dynamic_data,
                              event_packet->dynamic_data_length);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10MemcpyFailed);
    }
    command_ptr += event_packet->dynamic_data_length;

    copy_result = ex10_memset(
        command_ptr, (size_t)(end_ptr - command_ptr), 0, padding_bytes);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleCommands, Ex10MemsetFailed);
    }
    command_ptr += padding_bytes;
    (void)command_ptr;

    return make_ex10_success();
}

static struct Ex10Result command_insert_fifo_event(
    const bool                    trigger_irq,
    struct EventFifoPacket const* event_packet)
{
    uint8_t* command_ptr              = command_buffer;
    *command_ptr++                    = (uint8_t)CommandInsertFifoEvent;
    *command_ptr++                    = (uint8_t)trigger_irq;
    size_t const command_prefix_bytes = (size_t)(command_ptr - command_buffer);

    if (event_packet == NULL)
    {
        return get_ex10_command_transactor()->send_command(
            command_buffer,
            (size_t)(command_ptr - command_buffer),
            NOMINAL_READY_N_TIMEOUT_MS);
    }

    size_t const event_bytes = sizeof(struct PacketHeader) +
                               event_packet->static_data_length +
                               event_packet->dynamic_data_length;

    size_t const padding_bytes =
        (sizeof(uint32_t) - event_bytes % sizeof(uint32_t)) % sizeof(uint32_t);

    size_t const packet_bytes = event_bytes + padding_bytes;

    struct Ex10Result const ex10_result = create_fifo_event(
        event_packet, command_ptr, padding_bytes, packet_bytes);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // The length of the command is
    // 1: Command Code
    // 1: Trigger Interrupt
    // N: packet_bytes
    size_t const command_bytes = command_prefix_bytes + packet_bytes;

    return get_ex10_command_transactor()->send_command(
        command_buffer, command_bytes, NOMINAL_READY_N_TIMEOUT_MS);
}

static const struct Ex10Commands ex10_commands = {
    .read                  = command_read,
    .test_read             = command_test_read,
    .write                 = command_write,
    .read_fifo             = command_read_fifo,
    .write_info_page       = command_write_info_page,
    .start_upload          = command_start_upload,
    .continue_upload       = command_continue_upload,
    .complete_upload       = command_complete_upload,
    .revalidate_main_image = command_revalidate_main_image,
    .reset                 = command_reset,
    .test_transfer         = test_transfer,
    .create_fifo_event     = create_fifo_event,
    .insert_fifo_event     = command_insert_fifo_event,
};

struct Ex10Commands const* get_ex10_commands(void)
{
    return &ex10_commands;
}
