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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "board/board_spec.h"
#include "board/ex10_osal.h"
#include "board/time_helpers.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/bootloader_registers.h"
#include "ex10_api/command_transactor.h"
#include "ex10_api/commands.h"
#include "ex10_api/crc16.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/fifo_buffer_list.h"
#include "ex10_api/gpio_interface.h"
#include "ex10_api/trace.h"


static struct InterruptMaskFields const irq_mask_clear = {
    .op_done                 = false,
    .halted                  = false,
    .event_fifo_above_thresh = false,
    .event_fifo_full         = false,
    .inventory_round_done    = false,
    .halted_sequence_done    = false,
    .command_error           = false,
    .aggregate_op_done       = false,
};

static void (*fifo_data_callback)(struct FifoBufferNode*)       = NULL;
static bool (*interrupt_callback)(struct InterruptStatusFields) = NULL;

static struct Ex10GpioInterface const*     _gpio_if                 = NULL;
static struct HostInterface const*         _host_if                 = NULL;
static struct Ex10Commands const*          _ex10_commands           = NULL;
static struct Ex10CommandTransactor const* _ex10_command_transactor = NULL;
static struct FifoBufferList const*        _fifo_buffer_list        = NULL;

/* Forward declarations as needed */
static struct Ex10Result proto_write(struct RegisterInfo const* const reg_info,
                                     void const*                      buffer);

static struct Ex10Result proto_read(struct RegisterInfo const* const reg_info,
                                    void*                            buffer);

static struct Ex10Result read_multiple(
    struct RegisterInfo const* const reg_list[],
    void*                            buffers[],
    size_t                           num_regs);

static enum Status                get_running_location(void);
static struct Ex10Result          wait_op_completion(void);
static struct Ex10Result          wait_op_completion_with_timeout(uint32_t);
static struct ImageValidityFields get_image_validity(void);

static size_t upload_remaining_length = 0;
static size_t upload_image_length     = 0;

static void upload_reset(void)
{
    upload_remaining_length = 0;
    upload_image_length     = 0;
}

static void unregister_fifo_data_callback(void)
{
    fifo_data_callback = NULL;
}

static struct Ex10Result unregister_interrupt_callback(void)
{
    _gpio_if->irq_enable(false);
    bool const ex10_is_powered = _gpio_if->get_board_power();
    _gpio_if->irq_enable(true);

    if (ex10_is_powered)
    {
        // Disable all interrupts
        struct Ex10Result ex10_result =
            proto_write(&interrupt_mask_reg, &irq_mask_clear);
        if (ex10_result.error)
        {
            return ex10_result;
        }
    }

    // Clear callback
    interrupt_callback = NULL;

    return make_ex10_success();
}

static struct Ex10Result register_fifo_data_callback(
    void (*fifo_cb)(struct FifoBufferNode*))
{
    if (fifo_data_callback != NULL)
    {
        ex10_eprintf("fifo_data_callback = %p, expected NULL\n",
                     fifo_data_callback);
        return make_ex10_sdk_error(Ex10ModuleProtocol,
                                   Ex10SdkErrorInvalidState);
    }

    fifo_data_callback = fifo_cb;

    return make_ex10_success();
}

static struct Ex10Result register_interrupt_callback(
    struct InterruptMaskFields enable_mask,
    bool (*interrupt_cb)(struct InterruptStatusFields))
{
    if (interrupt_callback != NULL)
    {
        ex10_eprintf("interrupt_callback = %p, expected NULL\n",
                     interrupt_callback);
        return make_ex10_sdk_error(Ex10ModuleProtocol,
                                   Ex10SdkErrorInvalidState);
    }

    interrupt_callback = interrupt_cb;

    // Overwrite the interrupt mask register
    return proto_write(&interrupt_mask_reg, &enable_mask);
}

/**
 * Read the EventFifo multiple times filling an EventFifoBuffer node.
 *
 * @param fifo_num_bytes The number of Event Fifo bytes to be read.
 * This value should have been read from the EventFifoNumBytes register.
 *
 * @return struct FifoBufferNode* A node pointing to captured EventFifo data
 * read from the Ex10 using the ReadFifo command.
 * @retval NULL If no EventFifo data was available then
 */
static struct FifoBufferNode* read_event_fifo(size_t fifo_num_bytes)
{
    struct FifoBufferNode* fifo_buffer = _fifo_buffer_list->free_list_get();

    if (!fifo_buffer)
    {
        ex10_eprintf("No free event fifo buffers\n");

        struct Ex10Result ex10_result = make_ex10_sdk_error(
            Ex10ModuleProtocol, Ex10SdkNoFreeEventFifoBuffers);

        const uint32_t us_counter = 0;
        return make_ex10_result_fifo_packet(ex10_result, us_counter);
    }

    // For the event fifo parsing to work we are relying on all fifo
    // packets to be read from the Ex10 into a contiguous buffer of data.
    // Otherwise packets will straddle buffers, making packet parsing much
    // more complicated.
    if (fifo_num_bytes > fifo_buffer->raw_buffer.length)
    {
        // Release the buffer back to the free list. It will not be used.
        ex10_release_buffer_node(fifo_buffer);

        struct Ex10Result ex10_result = make_ex10_sdk_error(
            Ex10ModuleProtocol, Ex10SdkFreeEventFifoBuffersLengthMismatch);

        const uint32_t us_counter = 0;
        return make_ex10_result_fifo_packet(ex10_result, us_counter);
    }

    // The response code will be placed in the byte prior to the
    // first packet which is 32-bit aligned.
    struct ByteSpan bytes = {
        .data   = fifo_buffer->raw_buffer.data,
        .length = fifo_num_bytes,
    };

    _gpio_if->irq_enable(false);
    const struct Ex10Result ex10_result =
        _ex10_commands->read_fifo(EventFifo, &bytes);
    _gpio_if->irq_enable(true);

    if (ex10_result.error == false)
    {
        fifo_buffer->fifo_data.length = bytes.length;
    }
    else
    {
        // If the ReadFifo command failed then the contents of the buffer
        // cannot be parsed. Release the fifo buffer to the free list.
        ex10_release_buffer_node(fifo_buffer);

        const uint32_t us_counter = 0;
        return make_ex10_result_fifo_packet(ex10_result, us_counter);
    }

    return fifo_buffer;
}

static void interrupt_handler(void)
{
    struct RegisterInfo const* const reg_list[] = {
        &status_reg, &interrupt_status_reg, &event_fifo_num_bytes_reg};

    struct StatusFields            status;
    struct InterruptStatusFields   irq_status;
    struct EventFifoNumBytesFields fifo_num_bytes;

    void*                   buffers[] = {&status, &irq_status, &fifo_num_bytes};
    struct Ex10Result const ex10_result =
        read_multiple(reg_list, buffers, ARRAY_SIZE(reg_list));
    if (ex10_result.error)
    {
        ex10_eprintf("read_multiple() failed:\n");
        print_ex10_result(ex10_result);

        if (fifo_data_callback != NULL)
        {
            const uint32_t         us_counter = 0;
            struct FifoBufferNode* fifo_buffer =
                make_ex10_result_fifo_packet(ex10_result, us_counter);
            if (fifo_buffer)
            {
                fifo_data_callback(fifo_buffer);
            }
        }

        return;
    }

    tracepoint(pi_ex10sdk, PROTOCOL_interrupt, irq_status);

    if (status.status != Application)
    {
        // Don't perform interrupt actions if we are not in the application.
        return;
    }

    // Determine if we want to read the fifo based on the interrupt_callback
    bool trigger_fifo_read = false;
    // If the interrupt fires, the interrupt registration set it, so we perform
    // the interrupt callback for any interrupt.
    if (interrupt_callback != NULL)
    {
        // The non fifo interrupt can trigger a fifo callback
        trigger_fifo_read = interrupt_callback(irq_status);
    }

    if (trigger_fifo_read)
    {
        // If the ReadFifo command fails the data should be discarded and not
        // reported up the stack. It cannot be parsed.
        if (fifo_num_bytes.num_bytes > 0)
        {
            // Note: The fifo_buffer may contain the Ex10ResultPacket indicating
            // an error occurred during processing. Do not assume it is a
            // full length EventFifo packet.
            struct FifoBufferNode* fifo_buffer =
                read_event_fifo(fifo_num_bytes.num_bytes);
            if (fifo_buffer != NULL)
            {
                if (fifo_data_callback != NULL)
                {
                    fifo_data_callback(fifo_buffer);
                }
                else
                {
                    // There are no consumers of the data; free the buffer.
                    ex10_release_buffer_node(fifo_buffer);
                }
            }
        }
    }
}

static void enable_interrupt_handlers(bool enable)
{
    // Note: The IRQ_N monitor thread remains running, even when we disable
    // its callback function int the Ex10GpioInterface.
    _gpio_if->irq_monitor_callback_enable(enable);
}

static void init(struct Ex10DriverList const* driver_list)
{
    interrupt_callback = NULL;
    fifo_data_callback = NULL;

    _gpio_if       = &driver_list->gpio_if;
    _host_if       = &driver_list->host_if;
    _ex10_commands = get_ex10_commands();

    _ex10_command_transactor = get_ex10_command_transactor();
    _ex10_command_transactor->init(&driver_list->gpio_if,
                                   &driver_list->host_if);

    _fifo_buffer_list = get_ex10_fifo_buffer_list();
}

static struct Ex10Result init_ex10(void)
{
    // Disable all interrupts
    struct Ex10Result ex10_result =
        proto_write(&interrupt_mask_reg, &irq_mask_clear);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Configure EventFifo interrupt threshold
    struct EventFifoIntLevelFields const level_data = {
        .threshold = DEFAULT_EVENT_FIFO_THRESHOLD};
    ex10_result = proto_write(&event_fifo_int_level_reg, &level_data);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Clear pending interrupts
    struct InterruptStatusFields irq_status;
    ex10_result = proto_read(&interrupt_status_reg, &irq_status);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Unregister interrupt based callbacks that may have already been
    // registered.
    unregister_fifo_data_callback();

    ex10_result = unregister_interrupt_callback();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Note: Ex10Protocol interrupt processing should not be enabled until
    // Ex10 is powered into the application.
    int result = _gpio_if->register_irq_callback(interrupt_handler);
    if (result != 0)
    {
        return make_ex10_sdk_error_with_status(
            Ex10ModuleProtocol, Ex10SdkErrorGpioInterface, (uint32_t)result);
    }

    return make_ex10_success();
}

static struct Ex10Result deinit(void)
{
    unregister_fifo_data_callback();
    struct Ex10Result ex10_result = unregister_interrupt_callback();
    if (ex10_result.error)
    {
        return ex10_result;
    }

    _gpio_if->deregister_irq_callback();
    _ex10_command_transactor->deinit();

    return make_ex10_success();
}

static struct Ex10Result read_partial(uint16_t address,
                                      uint16_t length,
                                      void*    buffer)
{
    struct RegisterInfo const reg_info = {
        .address     = address,
        .length      = length,
        .num_entries = 1,
        .access      = ReadWrite,
    };

    struct RegisterInfo const* reg_list[] = {
        &reg_info,
    };

    void* buffers[] = {
        buffer,
    };

    struct Ex10Result ex10_result = read_multiple(reg_list, buffers, 1);

    tracepoint(pi_ex10sdk,
               PROTOCOL_read,
               ex10_get_thread_id(),
               address,
               length,
               buffer);

    return ex10_result;
}

static struct Ex10Result read_index(struct RegisterInfo const* const reg_info,
                                    void*                            buffer,
                                    uint8_t                          index)
{
    if (reg_info == NULL || buffer == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorNullPointer);
    }

    if (index >= reg_info->num_entries)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol,
                                   Ex10SdkErrorBadParamValue);
    }

    return read_partial(reg_info->address + (reg_info->length * index),
                        reg_info->length,
                        buffer);
}

static struct Ex10Result proto_read(struct RegisterInfo const* const reg_info,
                                    void*                            buffer)
{
    struct RegisterInfo const* const reg_list[] = {
        reg_info,
    };
    void* buffers[] = {
        buffer,
    };

    return read_multiple(reg_list, buffers, 1);
}

static struct Ex10Result read_multiple(
    struct RegisterInfo const* const reg_list[],
    void*                            buffers[],
    size_t                           num_regs)
{
    _gpio_if->irq_enable(false);
    const struct Ex10Result ex10_result = _ex10_commands->read(
        reg_list, buffers, num_regs, NOMINAL_READY_N_TIMEOUT_MS);
    _gpio_if->irq_enable(true);

    return ex10_result;
}

static struct Ex10Result proto_test_read(uint32_t address,
                                         uint16_t length,
                                         void*    buffer)
{
    // Address reads needs to fall on 4 byte boundary
    if (address % sizeof(uint32_t) != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol,
                                   Ex10SdkErrorBadParamAlignment);
    }
    if (length % sizeof(uint32_t) != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol,
                                   Ex10SdkErrorBadParamAlignment);
    }

    uint16_t offset     = 0;
    uint8_t* buffer_ptr = (uint8_t*)buffer;

    // Determine max size while maintaining 4 byte increments
    uint16_t const burst_read_length   = EX10_SPI_BURST_SIZE - 1;
    uint16_t const max_u32_read_length = burst_read_length / sizeof(uint32_t);
    uint16_t       max_u8_read_length  = max_u32_read_length * 4;

    // Loops through multiple reads for long spans of memory
    while (length > 0)
    {
        // decide how much to read per transaction
        uint16_t const read_length_bytes =
            (length > max_u8_read_length) ? max_u8_read_length : length;

        // perform test read
        _gpio_if->irq_enable(false);
        struct Ex10Result ex10_result = _ex10_commands->test_read(
            address + offset, read_length_bytes, &buffer_ptr[offset]);
        _gpio_if->irq_enable(true);
        if (ex10_result.error)
        {
            return ex10_result;
        }

        offset += read_length_bytes;
        length -= read_length_bytes;
    }

    return make_ex10_success();
}

static struct Ex10Result read_info_page_buffer(uint32_t address,
                                               uint8_t* read_buffer)
{
    // Info page is 2048 bytes long, divided into chunks to fit in buffers.
    // The transfer size must be a multiple of 4, so mask out the lower 2 bits.
    uint16_t       bytes_left = (uint16_t)EX10_INFO_PAGE_SIZE;
    uint16_t const chunk_mask = (uint16_t)(~0x03u);
    uint16_t const chunk_size = (uint16_t)EX10_SPI_BURST_SIZE & chunk_mask;
    uint16_t const chunks =
        (uint16_t)EX10_INFO_PAGE_SIZE / (uint16_t)EX10_SPI_BURST_SIZE;
    uint16_t offset = 0u;

    struct Ex10Result ex10_result = make_ex10_success();

    for (uint32_t chunk = 0; chunk < chunks; chunk++)
    {
        ex10_result =
            proto_test_read(address + offset, chunk_size, &read_buffer[offset]);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        offset += chunk_size;
        bytes_left -= chunk_size;
    }
    if (bytes_left)
    {
        ex10_result =
            proto_test_read(address + offset, bytes_left, &read_buffer[offset]);
        if (ex10_result.error)
        {
            return ex10_result;
        }
    }

    return ex10_result;
}

static struct Ex10Result get_write_multiple_stored_settings(
    struct RegisterInfo const* const reg_list[],
    void const*                      buffers[],
    size_t                           num_regs,
    struct ByteSpan*                 span,
    size_t*                          regs_copied)
{
    if (reg_list == NULL || buffers == NULL || span == NULL ||
        span->data == NULL || regs_copied == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorNullPointer);
    }

    // Check the destination span length for header + all registers'
    // address + length + data.
    // The size needed for the destination span:
    // - 64-bits  remain_in_bl_after_crash
    // - 1-byte   write command
    // - 2-bytes  per register address
    // - 2-bytes  per register length
    // - register entry length for each register
    const size_t info_page_header_size  = sizeof(uint64_t) + sizeof(uint8_t);
    size_t       total_span_size_needed = info_page_header_size;
    size_t       iter                   = 0;
    for (; iter < num_regs; iter++)
    {
        total_span_size_needed += 4;  // addr/length pair
        uint16_t const reg_entry_length =
            reg_list[iter]->length * reg_list[iter]->num_entries;
        total_span_size_needed += reg_entry_length;
    }

    if (span->length < total_span_size_needed)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol,
                                   Ex10SdkErrorBadParamValue);
    }

    uint8_t*             command_ptr = span->data;
    uint8_t const* const end_ptr     = command_ptr + span->length;

    // Now begin setting the span for use with stored settings
    // Prepend 64 bits for REMAIN_IN_BL_AFTER_CRASH field.
    ex10_memset(command_ptr, sizeof(uint64_t), 0xff, sizeof(uint64_t));
    command_ptr += sizeof(uint64_t);

    // Set the WRITES_FORMAT value as CommandWrite (2). This tells the Ex10 to
    // parse stored settings data as a single Write command; where the
    // WRITES_FORMAT value of 2 is the Write command itself.
    *command_ptr++ = (uint8_t)CommandWrite;

    // Now we copy in the data to write for each register
    uint16_t const write_desc_length =
        sizeof(struct Ex10WriteFormat) - sizeof(uint8_t const*);

    for (iter = 0; iter < num_regs; iter++)
    {
        uint16_t const reg_entry_length =
            reg_list[iter]->length * reg_list[iter]->num_entries;
        uint16_t const reg_entry_address = reg_list[iter]->address;

        if (command_ptr + write_desc_length + reg_entry_length >= end_ptr)
        {
            // Attempting to write this segment and beyond will overflow
            // the buffer defined by the span.
            break;
        }

        // Each segment requires an address, length pair; sent LSByte first.
        *command_ptr++ = (uint8_t)(reg_entry_address >> 0u);
        *command_ptr++ = (uint8_t)(reg_entry_address >> 8u);
        *command_ptr++ = (uint8_t)(reg_entry_length >> 0u);
        *command_ptr++ = (uint8_t)(reg_entry_length >> 8u);

        ex10_memcpy(command_ptr,
                    (size_t)(end_ptr - command_ptr),
                    buffers[iter],
                    reg_entry_length);
        command_ptr += reg_entry_length;
    }

    span->length = (size_t)(command_ptr - span->data);
    *regs_copied = iter;
    return make_ex10_success();
}

static struct Ex10Result write_multiple(
    struct RegisterInfo const* const reg_list[],
    void const*                      buffers[],
    size_t                           num_regs)
{
    _gpio_if->irq_enable(false);
    const struct Ex10Result ex10_result = _ex10_commands->write(
        reg_list, buffers, num_regs, NOMINAL_READY_N_TIMEOUT_MS);
    _gpio_if->irq_enable(true);

    return ex10_result;
}

static struct Ex10Result proto_write(struct RegisterInfo const* const reg_info,
                                     void const*                      buffer)
{
    struct RegisterInfo const* reg_list[] = {
        reg_info,
    };
    void const* buffers[] = {
        buffer,
    };
    return write_multiple(reg_list, buffers, 1);
}

static struct Ex10Result write_partial(uint16_t    address,
                                       uint16_t    length,
                                       void const* buffer)
{
    struct RegisterInfo const reg_info = {
        .address     = address,
        .length      = length,
        .num_entries = 1,
        .access      = ReadWrite,
    };
    struct RegisterInfo const* reg_list[] = {
        &reg_info,
    };
    void const* buffers[] = {
        buffer,
    };

    return write_multiple(reg_list, buffers, 1);
}

static struct Ex10Result write_index(struct RegisterInfo const* const reg_info,
                                     void const*                      buffer,
                                     uint8_t                          index)
{
    if (reg_info == NULL || buffer == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorNullPointer);
    }

    if (index >= reg_info->num_entries)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol,
                                   Ex10SdkErrorBadParamValue);
    }

    return write_partial(reg_info->address + (reg_info->length * index),
                         reg_info->length,
                         buffer);
}

static int host_if_reopen(uint32_t clock_speed)
{
    _gpio_if->irq_enable(false);
    _host_if->close();
    int const error = _host_if->open(clock_speed);
    _gpio_if->irq_enable(true);
    return error;
}

/**
 * @details
 * In both Application mode and Bootloader mode, the host will be able to
 * communicate to the device using the BOOTLOADER_SPI_CLOCK_HZ (1 MHz) speed.
 *
 * In Bootloader mode, the device will not respond to SPI
 * transactions clocked faster than 1 MHz. Therefore, after a reset the
 * SPI clock speed must be set to the slower BOOTLOADER_SPI_CLOCK_HZ.
 * Once it is established that the Ex10 is running in Application mode,
 * then the clock speed can be increased to 4 MHz (DEFAULT_SPI_CLOCK_HZ).
 *
 * @note The HostInterface returns negative errno values on failure.
 */
static struct Ex10Result reset(enum Status destination)
{
    // Reopen the host interface to bootloader speed.
    int host_if_error = host_if_reopen(BOOTLOADER_SPI_CLOCK_HZ);
    if (host_if_error != 0)
    {
        return make_ex10_sdk_error_with_status(Ex10ModuleProtocol,
                                               Ex10SdkErrorHostInterface,
                                               (uint32_t)host_if_error);
    }

    upload_reset();

    // Reset the Ex10, then read the Status register to get running location.
    _gpio_if->irq_enable(false);
    struct Ex10Result ex10_result = _ex10_commands->reset(destination);
    _gpio_if->irq_enable(true);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    enum Status const running_location = get_running_location();
    if (running_location != destination)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorRunLocation);
    }

    if (running_location == Application)
    {
        // Application execution confirmed. Set the SPI clock rate to 4 MHz.
        host_if_error = host_if_reopen(DEFAULT_SPI_CLOCK_HZ);
        if (host_if_error != 0)
        {
            return make_ex10_sdk_error_with_status(Ex10ModuleProtocol,
                                                   Ex10SdkErrorHostInterface,
                                                   (uint32_t)host_if_error);
        }
    }

    return make_ex10_success();
}

static struct Ex10Result set_event_fifo_threshold(size_t threshold)
{
    if (threshold > EX10_EVENT_FIFO_SIZE)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol,
                                   Ex10SdkErrorBadParamValue);
    }

    struct EventFifoIntLevelFields const event_fifo_thresh = {
        .threshold = (uint16_t)threshold, .rfu = 0u};
    return proto_write(&event_fifo_int_level_reg, &event_fifo_thresh);
}

static struct Ex10Result insert_fifo_event(
    const bool                    trigger_irq,
    struct EventFifoPacket const* event_packet)
{
    _gpio_if->irq_enable(false);
    struct Ex10Result ex10_result =
        _ex10_commands->insert_fifo_event(trigger_irq, event_packet);
    _gpio_if->irq_enable(true);

    return ex10_result;
}

static struct Ex10Result wait_for_event_fifo_empty(void)
{
    struct EventFifoNumBytesFields fifo_bytes = {0, 0};

    struct Ex10Result ex10_result =
        proto_read(&event_fifo_num_bytes_reg, &fifo_bytes);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    // Spin as long as there is data in the fifo still and as long
    // as there is a place to read it into the SDK.
    while (_fifo_buffer_list->free_list_size() > 0 && fifo_bytes.num_bytes > 0)
    {
        ex10_result = proto_read(&event_fifo_num_bytes_reg, &fifo_bytes);
        if (ex10_result.error)
        {
            return ex10_result;
        }

        get_ex10_time_helpers()->wait_ms(1);
    }

    if (fifo_bytes.num_bytes != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol,
                                   Ex10SdkNoFreeEventFifoBuffers);
    }

    return make_ex10_success();
}

static struct Ex10Result wait_op_completion(void)
{
    const uint32_t default_timeout_ms = 10000u;
    return wait_op_completion_with_timeout(default_timeout_ms);
}

static struct Ex10Result read_ops_status_reg(struct OpsStatusFields* ops_status)
{
    struct RegisterInfo const* reg_list[] = {&ops_status_reg};
    void*                      buffers[]  = {ops_status};

    struct Ex10Result ex10_result =
        read_multiple(reg_list, buffers, ARRAY_SIZE(reg_list));

    if (ex10_result.error == false && ops_status->error != ErrorNone)
    {
        ex10_result = make_ex10_ops_error(*ops_status);
    }

    return ex10_result;
}

static struct Ex10Result wait_op_completion_with_timeout(uint32_t timeout_ms)
{
    uint32_t const         start_time = get_ex10_time_helpers()->time_now();
    struct OpsStatusFields ops_status;
    struct Ex10Result      ex10_result = read_ops_status_reg(&ops_status);
    while (ops_status.busy && ex10_result.error == false)
    {
        if (get_ex10_time_helpers()->time_elapsed(start_time) >= timeout_ms)
        {
            ex10_result = make_ex10_ops_timeout_error(ops_status);
        }
        else
        {
            ex10_result = read_ops_status_reg(&ops_status);
        }
    }

    tracepoint(
        pi_ex10sdk, PROTOCOL_op_done, ops_status.op_id, ops_status.error);

    return ex10_result;
}

static struct Ex10Result start_op(enum OpId op_id)
{
    struct OpsControlFields const ops_control_data = {.op_id = (uint8_t)op_id};
    struct Ex10Result             ex10_result =
        proto_write(&ops_control_reg, &ops_control_data);
    tracepoint(pi_ex10sdk, PROTOCOL_start_op, op_id);
    return ex10_result;
}

static struct Ex10Result stop_op(void)
{
    return start_op(Idle);
}

static bool is_op_currently_running(void)
{
    struct OpsControlFields ops_control;
    proto_read(&ops_control_reg, &ops_control);
    return ops_control.op_id != Idle;
}

static enum Status get_running_location(void)
{
    struct StatusFields status;
    proto_read(&status_reg, &status);
    return status.status;
}

static struct RxGainControlFields get_analog_rx_config(void)
{
    struct RxGainControlFields rx_gain_control;
    proto_read(&rx_gain_control_reg, &rx_gain_control);
    return rx_gain_control;
}

static struct Ex10Result write_info_page(enum PageIds page_id,
                                         void const*  data_ptr,
                                         size_t       write_length,
                                         uint32_t     fref_khz)
{
    if (get_running_location() != Bootloader)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorRunLocation);
    }

    // A length of 0 will erase the calibration page
    // Set flash frequency to allow info page update
    struct FrefFreqBootloaderFields const fref_freq = {
        .fref_freq_khz = fref_khz,
    };
    struct Ex10Result ex10_result = proto_write(&fref_freq_reg, &fref_freq);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    uint16_t crc16 = 0;
    if (write_length)
    {
        crc16 = ex10_compute_crc16(data_ptr, write_length);
    }

    struct ConstByteSpan page_data = {
        .data   = data_ptr,
        .length = write_length,
    };

    // Send the data
    _gpio_if->irq_enable(false);
    ex10_result =
        _ex10_commands->write_info_page((uint8_t)page_id, &page_data, crc16);
    _gpio_if->irq_enable(true);

    return ex10_result;
}

static struct Ex10Result write_calibration_page(uint8_t const* data_ptr,
                                                size_t         write_length)
{
    return write_info_page(CalPageId, data_ptr, write_length, TCXO_FREQ_KHZ);
}

static struct Ex10Result write_stored_settings_page(uint8_t const* data_ptr,
                                                    size_t         write_length)
{
    return write_info_page(
        StoredSettingsId, data_ptr, write_length, TCXO_FREQ_KHZ);
}

static struct Ex10Result erase_info_page(enum PageIds page_id,
                                         uint32_t     fref_khz)
{
    uint8_t fake_data[1] = {0u};

    return write_info_page(page_id, fake_data, 0u, fref_khz);
}

static struct Ex10Result erase_calibration_page(void)
{
    return erase_info_page(CalPageId, TCXO_FREQ_KHZ);
}

static struct Ex10Result upload_image(uint8_t                    code,
                                      const struct ConstByteSpan upload_image)
{
    if (get_running_location() != Bootloader)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorRunLocation);
    }

    // Set flash frequency to allow flash programming
    struct FrefFreqBootloaderFields const fref_freq = {
        .fref_freq_khz = TCXO_FREQ_KHZ,
    };
    struct Ex10Result ex10_result = proto_write(&fref_freq_reg, &fref_freq);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    struct CommandResultFields cmd_result;
    size_t                     remaining_length = upload_image.length;

    // Use the maximum SPI burst size less 2 bytes (one byte for command code
    // and one byte for the destination).
    const size_t upload_chunk_size = EX10_SPI_BURST_SIZE - 2;

    // used to split image into uploadable chunks
    struct ConstByteSpan chunk = {
        .data   = upload_image.data,
        .length = upload_chunk_size,
    };

    // Upload the image
    while (remaining_length)
    {
        chunk.length = (remaining_length < upload_chunk_size)
                           ? remaining_length
                           : upload_chunk_size;
        if (remaining_length == upload_image.length)
        {
            _gpio_if->irq_enable(false);
            ex10_result = _ex10_commands->start_upload(code, &chunk);
            _gpio_if->irq_enable(true);

            if (ex10_result.error)
            {
                upload_reset();
                return ex10_result;
            }
        }
        else
        {
            _gpio_if->irq_enable(false);
            ex10_result = _ex10_commands->continue_upload(&chunk);
            _gpio_if->irq_enable(true);

            if (ex10_result.error)
            {
                upload_reset();
                return ex10_result;
            }
        }
        remaining_length -= chunk.length;
        chunk.data += chunk.length;

        // Check upload status
        ex10_result = proto_read(&command_result_reg, &cmd_result);
        if (ex10_result.error)
        {
            return ex10_result;
        }
        if (cmd_result.failed_result_code != Success)
        {
            return make_ex10_commands_no_resp_error(cmd_result);
        }
    }

    // Signify end of upload and check status
    _gpio_if->irq_enable(false);
    ex10_result = _ex10_commands->complete_upload();
    _gpio_if->irq_enable(true);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = proto_read(&command_result_reg, &cmd_result);
    if (ex10_result.error)
    {
        return ex10_result;
    }
    if (cmd_result.failed_result_code != Success)
    {
        return make_ex10_commands_no_resp_error(cmd_result);
    }

    return make_ex10_success();
}

static struct Ex10Result upload_start(uint8_t                    destination,
                                      size_t                     image_length,
                                      const struct ConstByteSpan image_chunk)
{
    if (get_running_location() != Bootloader)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorRunLocation);
    }

    // Set flash frequency to allow flash programming
    struct FrefFreqBootloaderFields const fref_freq = {
        .fref_freq_khz = TCXO_FREQ_KHZ,
    };
    struct Ex10Result ex10_result = proto_write(&fref_freq_reg, &fref_freq);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    upload_remaining_length = image_length;
    upload_image_length     = image_length;

    _gpio_if->irq_enable(false);
    ex10_result = _ex10_commands->start_upload(destination, &image_chunk);
    _gpio_if->irq_enable(true);

    return ex10_result;
}

static struct Ex10Result upload_continue(const struct ConstByteSpan image_chunk)
{
    if (get_running_location() != Bootloader)
    {
        upload_reset();
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorRunLocation);
    }

    // Use the maximum SPI burst size less 2 bytes (one byte for command code
    // and one byte for the destination).
    const size_t upload_chunk_size = EX10_MAX_IMAGE_CHUNK_SIZE - 2;

    // The chunk must be within the max chunk and the remaining size.
    if (image_chunk.length > upload_chunk_size)
    {
        upload_reset();
        return make_ex10_sdk_error(Ex10ModuleProtocol,
                                   Ex10SdkErrorBadParamValue);
    }

    if (image_chunk.length > upload_remaining_length)
    {
        upload_reset();
        return make_ex10_sdk_error(Ex10ModuleProtocol,
                                   Ex10SdkErrorBadParamValue);
    }

    _gpio_if->irq_enable(false);
    struct Ex10Result ex10_result =
        _ex10_commands->continue_upload(&image_chunk);
    _gpio_if->irq_enable(true);

    if (ex10_result.error)
    {
        upload_reset();
        return ex10_result;
    }

    upload_remaining_length -= image_chunk.length;

    // Check upload status
    struct CommandResultFields cmd_result;
    ex10_result = proto_read(&command_result_reg, &cmd_result);
    if (ex10_result.error)
    {
        upload_reset();
        return ex10_result;
    }
    if (cmd_result.failed_result_code != Success)
    {
        upload_reset();
        return make_ex10_commands_no_resp_error(cmd_result);
    }

    return make_ex10_success();
}

static struct Ex10Result upload_complete(void)
{
    upload_reset();

    if (get_running_location() != Bootloader)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorRunLocation);
    }

    // Signify end of upload and check status
    _gpio_if->irq_enable(false);
    struct Ex10Result ex10_result = _ex10_commands->complete_upload();
    _gpio_if->irq_enable(true);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    struct CommandResultFields cmd_result;
    ex10_result = proto_read(&command_result_reg, &cmd_result);
    if (ex10_result.error)
    {
        return ex10_result;
    }
    if (cmd_result.failed_result_code != Success)
    {
        return make_ex10_commands_no_resp_error(cmd_result);
    }

    return make_ex10_success();
}

static struct ImageValidityFields revalidate_image(void)
{
    // Read the command result register to insure an prior state is cleared.
    struct CommandResultFields command_result_fields;
    struct ImageValidityFields image_validity;
    ex10_memzero(&image_validity, sizeof(image_validity));

    struct Ex10Result ex10_result =
        proto_read(&command_result_reg, &command_result_fields);
    if (ex10_result.error)
    {
        // Return nullified image validity markers due to the occurred error
        return image_validity;
    }

    // Set flash frequency to allow flash programming
    struct FrefFreqBootloaderFields const fref_freq = {
        .fref_freq_khz = TCXO_FREQ_KHZ,
    };
    ex10_result = proto_write(&fref_freq_reg, &fref_freq);
    if (ex10_result.error)
    {
        // Return nullified image validity markers due to the occurred error
        return image_validity;
    }

    _gpio_if->irq_enable(false);
    ex10_result = _ex10_commands->revalidate_main_image();
    _gpio_if->irq_enable(true);
    if (ex10_result.error)
    {
        // Return nullified image validity markers due to the occurred error
        return image_validity;
    }

    return get_image_validity();
}

static struct Ex10Result proto_test_transfer(struct ConstByteSpan const* send,
                                             struct ByteSpan*            recv,
                                             bool                        verify)
{
    _gpio_if->irq_enable(false);
    const struct Ex10Result ex10_result =
        _ex10_commands->test_transfer(send, recv, verify);
    _gpio_if->irq_enable(true);

    return ex10_result;
}

static struct Ex10Result get_device_info(struct DeviceInfoFields* dev_info)
{
    if (dev_info == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorNullPointer);
    }

    if (get_running_location() == Application)
    {
        struct Ex10Result const ex10_result =
            proto_read(&device_info_reg, dev_info);
        if (ex10_result.error)
        {
            return ex10_result;
        }
    }
    else
    {
        // In bootloader:
        uint8_t git_hash_buffer[GIT_HASH_REG_LENGTH];

        struct Ex10Result const ex10_result =
            proto_read(&bootloader_git_hash_reg, git_hash_buffer);
        if (ex10_result.error)
        {
            return ex10_result;
        }

        uint32_t bootloader_git_hash = 0u;
        bootloader_git_hash |= git_hash_buffer[0];
        bootloader_git_hash <<= 8u;
        bootloader_git_hash |= git_hash_buffer[1];
        bootloader_git_hash <<= 8u;
        bootloader_git_hash |= git_hash_buffer[2];
        bootloader_git_hash <<= 8u;
        bootloader_git_hash |= git_hash_buffer[3];

        switch (bootloader_git_hash)
        {
            case 0xb3a01818:
                dev_info->eco_revision       = 0;
                dev_info->device_revision_lo = 4;
                dev_info->device_revision_hi = 0;
                dev_info->device_identifier  = 1;
                break;

            case 0x804499bc:
                dev_info->eco_revision       = 3;
                dev_info->device_revision_lo = 3;
                dev_info->device_revision_hi = 0;
                dev_info->device_identifier  = 1;
                break;

            default:
                dev_info->eco_revision       = 0;
                dev_info->device_revision_lo = 0;
                dev_info->device_revision_hi = 0;
                dev_info->device_identifier  = 0;
                break;
        }
    }

    return make_ex10_success();
}

static struct Ex10Result get_application_version(
    struct Ex10FirmwareVersion* firmware_version)
{
    if (firmware_version == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorNullPointer);
    }

    ex10_memzero(firmware_version, sizeof(*firmware_version));
    struct Ex10Result ex10_result =
        proto_read(&version_string_reg, firmware_version->version_string);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = proto_read(&git_hash_reg, firmware_version->git_hash_bytes);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result =
        proto_read(&build_number_reg, &firmware_version->build_number);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    return make_ex10_success();
}

static struct Ex10Result get_bootloader_version(
    struct Ex10FirmwareVersion* firmware_version)
{
    if (firmware_version == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorNullPointer);
    }

    ex10_memzero(firmware_version, sizeof(*firmware_version));
    enum Status const initial_status = get_running_location();

    if (initial_status == Application)
    {
        reset(Bootloader);
        if (get_running_location() != Bootloader)
        {
            return make_ex10_sdk_error(Ex10ModuleProtocol,
                                       Ex10SdkErrorRunLocation);
        }
    }

    struct Ex10Result ex10_result = proto_read(
        &bootloader_version_string_reg, firmware_version->version_string);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result =
        proto_read(&bootloader_git_hash_reg, firmware_version->git_hash_bytes);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    ex10_result = proto_read(&bootloader_build_number_reg,
                             &firmware_version->build_number);
    if (ex10_result.error)
    {
        return ex10_result;
    }

    if (initial_status == Bootloader)
    {
        return make_ex10_success();
    }

    reset(Application);
    if (get_running_location() != Application)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorRunLocation);
    }

    return make_ex10_success();
}

static struct ImageValidityFields get_image_validity(void)
{
    if (get_running_location() == Application)
    {
        struct ImageValidityFields const image_validity = {
            .image_valid_marker     = true,
            .image_non_valid_marker = false,
            .rfu                    = 0u,
        };
        return image_validity;
    }
    else
    {
        struct ImageValidityFields image_validity;
        struct Ex10Result          ex10_result =
            proto_read(&image_validity_reg, &image_validity);
        if (ex10_result.error)
        {
            ex10_memzero(&image_validity, sizeof(image_validity));
        }
        return image_validity;
    }
}

static struct Ex10Result get_remain_reason(
    struct RemainReasonFields* remain_reason)
{
    if (remain_reason == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleProtocol, Ex10SdkErrorNullPointer);
    }

    struct Ex10Result ex10_result = make_ex10_success();

    if (get_running_location() == Application)
    {
        remain_reason->remain_reason = RemainReasonNoReason;
    }
    else
    {
        ex10_result = proto_read(&remain_reason_reg, remain_reason);
    }
    return ex10_result;
}

static enum ProductSku get_sku(void)
{
    if (get_running_location() != Application)
    {
        return SkuUnknown;
    }

    uint8_t           buffer[PRODUCT_SKU_REG_LENGTH];
    struct Ex10Result ex10_result = proto_read(&product_sku_reg, buffer);
    if (ex10_result.error)
    {
        return SkuUnknown;
    }
    uint16_t sku_value = 0u;
    sku_value |= buffer[1];
    sku_value <<= 8u;
    sku_value |= buffer[0];

    return (enum ProductSku)sku_value;
}

static const struct Ex10Protocol ex10_protocol = {
    .init                               = init,
    .init_ex10                          = init_ex10,
    .deinit                             = deinit,
    .register_fifo_data_callback        = register_fifo_data_callback,
    .register_interrupt_callback        = register_interrupt_callback,
    .unregister_fifo_data_callback      = unregister_fifo_data_callback,
    .unregister_interrupt_callback      = unregister_interrupt_callback,
    .enable_interrupt_handlers          = enable_interrupt_handlers,
    .read                               = proto_read,
    .test_read                          = proto_test_read,
    .read_index                         = read_index,
    .write                              = proto_write,
    .write_index                        = write_index,
    .read_partial                       = read_partial,
    .write_partial                      = write_partial,
    .write_multiple                     = write_multiple,
    .get_write_multiple_stored_settings = get_write_multiple_stored_settings,
    .read_info_page_buffer              = read_info_page_buffer,
    .read_multiple                      = read_multiple,
    .stop_op                            = stop_op,
    .start_op                           = start_op,
    .is_op_currently_running            = is_op_currently_running,
    .wait_op_completion                 = wait_op_completion,
    .wait_op_completion_with_timeout    = wait_op_completion_with_timeout,
    .read_ops_status_reg                = read_ops_status_reg,
    .reset                              = reset,
    .set_event_fifo_threshold           = set_event_fifo_threshold,
    .insert_fifo_event                  = insert_fifo_event,
    .get_running_location               = get_running_location,
    .get_analog_rx_config               = get_analog_rx_config,
    .write_info_page                    = write_info_page,
    .erase_info_page                    = erase_info_page,
    .write_calibration_page             = write_calibration_page,
    .erase_calibration_page             = erase_calibration_page,
    .write_stored_settings_page         = write_stored_settings_page,
    .upload_image                       = upload_image,
    .upload_start                       = upload_start,
    .upload_continue                    = upload_continue,
    .upload_complete                    = upload_complete,
    .revalidate_image                   = revalidate_image,
    .test_transfer                      = proto_test_transfer,
    .wait_for_event_fifo_empty          = wait_for_event_fifo_empty,
    .get_device_info                    = get_device_info,
    .get_application_version            = get_application_version,
    .get_bootloader_version             = get_bootloader_version,
    .get_image_validity                 = get_image_validity,
    .get_remain_reason                  = get_remain_reason,
    .get_sku                            = get_sku,
};

struct Ex10Protocol const* get_ex10_protocol(void)
{
    return &ex10_protocol;
}
