/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2021 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#include <stddef.h>

#include "board/ex10_osal.h"
#include "ex10_api/aggregate_op_builder.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/print_data.h"


static const uint8_t instruction_code_size = 1u;

static bool aggregate_buffer_overflow(struct ByteSpan* agg_op_span,
                                      size_t           size_to_add)
{
    return (agg_op_span->length + size_to_add >=
            aggregate_op_buffer_reg.length);
}

static ssize_t get_instruction_from_index(
    size_t                         index,
    struct ByteSpan*               agg_op_span,
    struct AggregateOpInstruction* instruction_at_index)
{
    if (agg_op_span == NULL || agg_op_span->data == NULL ||
        instruction_at_index == NULL ||
        instruction_at_index->instruction_data == NULL)
    {
        return -1;
    }

    size_t  idx                 = 0;
    ssize_t instruction_counter = 0;
    while ((idx < agg_op_span->length) && (idx != index))
    {
        // add 1 for the instruction
        size_t command_size = instruction_code_size;
        switch (agg_op_span->data[idx])
        {
            case InstructionTypeWrite:
            {
                struct Ex10WriteFormat const* write_inst =
                    (struct Ex10WriteFormat const*)&agg_op_span
                        ->data[idx + instruction_code_size];
                command_size += sizeof(write_inst->length) +
                                sizeof(write_inst->address) +
                                write_inst->length;
                break;
            }
            case InstructionTypeReset:
                command_size += sizeof(struct Ex10ResetFormat);
                break;
            case InstructionTypeInsertFifoEvent:
            {
                // instruction, trigger irq, then packet data
                uint8_t const irq_trigger_size = 1u;
                uint8_t const packet_offset =
                    instruction_code_size + irq_trigger_size;
                uint8_t const* packet_data =
                    &agg_op_span->data[idx + packet_offset];
                // The first byte of the fifo is the packet length
                // in 32 bit words
                uint8_t const packet_len = packet_data[0] * sizeof(uint32_t);
                // add in the size of the trigger and the fifo packet
                command_size += irq_trigger_size + packet_len;
                break;
            }
            case InstructionTypeRunOp:
                command_size += sizeof(struct AggregateRunOpFormat);
                break;
            case InstructionTypeGoToIndex:
                command_size += sizeof(struct AggregateGoToIndexFormat);
                break;
            case InstructionTypeIdentifier:
                command_size += sizeof(struct AggregateIdentifierFormat);
                break;
            case InstructionTypeExitInstruction:
                // No extra info needed for Exit command
                break;
            case InstructionTypeReserved:
            default:
                ex10_eprintf("Invalid instruction #%zu: 0x%02X @ 0x%04zX\n",
                             instruction_counter,
                             agg_op_span->data[idx],
                             idx);
                return -1;
        }
        idx += command_size;
        instruction_counter++;
    }
    // at correct index or end - check for past end
    if (idx > agg_op_span->length)
    {
        ex10_eprintf(
            "Buffer overflow past instruction #%zu: 0x%02X @ 0x%04zX\n",
            instruction_counter,
            agg_op_span->data[idx],
            idx);
        return -1;
    }
    // at correct index - check for instruction and fill struct
    instruction_at_index->instruction_type = agg_op_span->data[idx];
    union AggregateInstructionData const* agg_instruction_data =
        (union AggregateInstructionData const*)(&agg_op_span->data
                                                     [idx +
                                                      instruction_code_size]);

    switch (agg_op_span->data[idx])
    {
        case InstructionTypeWrite:
        {
            instruction_at_index->instruction_data->write_format.address =
                agg_instruction_data->write_format.address;
            instruction_at_index->instruction_data->write_format.length =
                agg_instruction_data->write_format.length;
            size_t const payload_offset =
                instruction_code_size +
                sizeof(agg_instruction_data->write_format.address) +
                sizeof(agg_instruction_data->write_format.length);
            instruction_at_index->instruction_data->write_format.data =
                &agg_op_span->data[idx + payload_offset];
            break;
        }
        case InstructionTypeInsertFifoEvent:
        {
            uint8_t const irq_trigger_size = 1u;
            size_t const  packet_offset =
                instruction_code_size + irq_trigger_size;

            instruction_at_index->instruction_data->insert_fifo_event_format
                .trigger_irq =
                agg_instruction_data->insert_fifo_event_format.trigger_irq;
            instruction_at_index->instruction_data->insert_fifo_event_format
                .packet = &agg_op_span->data[idx + packet_offset];
            break;
        }
        case InstructionTypeReset:
        {
            instruction_at_index->instruction_data->reset_format.destination =
                agg_instruction_data->reset_format.destination;
            break;
        }
        case InstructionTypeRunOp:
        {
            instruction_at_index->instruction_data->run_op_format.op_to_run =
                agg_instruction_data->run_op_format.op_to_run;
            break;
        }
        case InstructionTypeGoToIndex:
        {
            instruction_at_index->instruction_data->go_to_index_format
                .jump_index =
                agg_instruction_data->go_to_index_format.jump_index;
            instruction_at_index->instruction_data->go_to_index_format
                .repeat_counter =
                agg_instruction_data->go_to_index_format.repeat_counter;
            break;
        }
        case InstructionTypeIdentifier:
        {
            instruction_at_index->instruction_data->identifier_format
                .identifier =
                agg_instruction_data->identifier_format.identifier;
            break;
        }
        case InstructionTypeExitInstruction:
        {
            break;
        }
        case InstructionTypeReserved:
        default:
            return -1;
    }
    return instruction_counter;
}

static bool append_instruction(
    const struct AggregateOpInstruction op_instruction,
    struct ByteSpan*                    agg_op_span)
{
    if (agg_op_span == NULL || agg_op_span->data == NULL)
    {
        return false;
    }

    size_t new_command_size = 0;
    switch (op_instruction.instruction_type)
    {
        case InstructionTypeWrite:
        {
            struct Ex10WriteFormat* write_inst =
                &op_instruction.instruction_data->write_format;
            // Special copy needed for extra data
            size_t copy_size =
                sizeof(write_inst->length) + sizeof(write_inst->address);
            // Ensure the rest of the write will fit
            if (aggregate_buffer_overflow(
                    agg_op_span,
                    copy_size + write_inst->length + instruction_code_size))
            {
                return false;
            }
            // Add the command code
            agg_op_span->data[agg_op_span->length++] =
                (uint8_t)op_instruction.instruction_type;
            // Copy the command over
            int copy_result = ex10_memcpy(
                &agg_op_span->data[agg_op_span->length],
                aggregate_op_buffer_reg.length - agg_op_span->length,
                write_inst,
                copy_size);
            if (copy_result != 0)
            {
                return false;
            }
            agg_op_span->length += copy_size;

            copy_result = ex10_memcpy(
                &agg_op_span->data[agg_op_span->length],
                aggregate_op_buffer_reg.length - agg_op_span->length,
                write_inst->data,
                write_inst->length);
            if (copy_result != 0)
            {
                return false;
            }
            agg_op_span->length += write_inst->length;
            break;
        }
        case InstructionTypeInsertFifoEvent:
        {
            struct Ex10InsertFifoEventFormat* fifo_inst =
                &op_instruction.instruction_data->insert_fifo_event_format;

            // Ensure the the packet being appended is not NULL. Normally, the
            // SPI transaction length can be used to detect if there is a packet
            // or not. When used with the aggregate op, the length of the
            // command is not an indicator of packet validity (since the buffer
            // is always the same size). This means we must ensure there is a
            // valid packet header with no additional data (a header with no
            // additional data can trigger_irq but will not send anything)
            // This is handled in the helper function 'append_insert_fifo_event'
            // found in this file
            if (fifo_inst->packet == NULL)
            {
                return false;
            }

            // The first byte of the fifo is the length in 32 bit words
            const uint8_t packet_size = fifo_inst->packet[0] * 4;

            // Ensure the rest of the write will fit
            if (aggregate_buffer_overflow(agg_op_span,
                                          sizeof(fifo_inst->trigger_irq) +
                                              packet_size +
                                              instruction_code_size))
            {
                return false;
            }
            // Add the command code
            agg_op_span->data[agg_op_span->length++] =
                (uint8_t)op_instruction.instruction_type;
            // Copy the trigger irq byte over
            agg_op_span->data[agg_op_span->length++] = fifo_inst->trigger_irq;
            // Copy over the packet data
            int const copy_result = ex10_memcpy(
                &agg_op_span->data[agg_op_span->length],
                aggregate_op_buffer_reg.length - agg_op_span->length,
                fifo_inst->packet,
                packet_size);
            if (copy_result != 0)
            {
                return false;
            }
            // Advance by the payload length
            agg_op_span->length += packet_size;
            break;
        }
        case InstructionTypeReset:
            new_command_size +=
                instruction_code_size + sizeof(struct Ex10ResetFormat);
            break;
        case InstructionTypeRunOp:
            new_command_size +=
                instruction_code_size + sizeof(struct AggregateRunOpFormat);
            break;
        case InstructionTypeGoToIndex:
        {
            struct AggregateGoToIndexFormat* jump_data =
                &op_instruction.instruction_data->go_to_index_format;
            struct AggregateGoToIndexFormat goto_data;
            struct AggregateOpInstruction   jump_inst = {
                0, (union AggregateInstructionData*)&goto_data};
            // Allow user to jump to a future instruction at their own
            // discretion.
            bool future_jump = jump_data->jump_index >= agg_op_span->length;
            if (future_jump)
            {
                ex10_eprintf(
                    "Jumping to an instruction not yet in the buffer\n");
            }
            ssize_t const instruction_number =
                (future_jump)
                    ? 0
                    : get_instruction_from_index(
                          jump_data->jump_index, agg_op_span, &jump_inst);
            if (instruction_number == -1)
            {
                ex10_eprintf(
                    "The index attempted to jump to is not a valid "
                    "instruction\n");
                return false;
            }
            else
            {
                new_command_size += instruction_code_size +
                                    sizeof(struct AggregateGoToIndexFormat);
            }
            break;
        }
        case InstructionTypeIdentifier:
        {
            new_command_size += instruction_code_size +
                                sizeof(struct AggregateIdentifierFormat);
            break;
        }
        case InstructionTypeExitInstruction:
            new_command_size += instruction_code_size;
            break;
        case InstructionTypeReserved:
        default:
            // The command is not valid
            return false;
    }

    // if further data copy is needed
    if (new_command_size > 0)
    {
        if (aggregate_buffer_overflow(agg_op_span, new_command_size))
        {
            return false;
        }
        else
        {
            // Add the command code
            agg_op_span->data[agg_op_span->length++] =
                (uint8_t)op_instruction.instruction_type;
            new_command_size -= instruction_code_size;
            // Add the rest of the command. Note: If the instruction data
            // pointer is NULL, then there is no data to copy;
            // i.e. The exit instruction.
            if (op_instruction.instruction_data != NULL)
            {
                int const copy_result = ex10_memcpy(
                    &agg_op_span->data[agg_op_span->length],
                    aggregate_op_buffer_reg.length - agg_op_span->length,
                    op_instruction.instruction_data,
                    new_command_size);
                if (copy_result != 0)
                {
                    return false;
                }
                agg_op_span->length += new_command_size;
            }
        }
    }
    return true;
}

static bool clear_buffer(void)
{
    uint8_t clear_buffer[AGGREGATE_OP_BUFFER_REG_LENGTH];
    ex10_memzero(clear_buffer, sizeof(clear_buffer));

    // clear the device side buffer
    struct Ex10Result ex10_result =
        get_ex10_protocol()->write(&aggregate_op_buffer_reg, clear_buffer);

    return (ex10_result.error == false);
}

static bool set_buffer(struct ByteSpan* agg_op_span)
{
    if (agg_op_span == NULL || agg_op_span->data == NULL)
    {
        return false;
    }

    if (agg_op_span->length > aggregate_op_buffer_reg.length)
    {
        return false;
    }

    struct Ex10Result ex10_result =
        get_ex10_protocol()->write_partial(aggregate_op_buffer_reg.address,
                                           (uint16_t)agg_op_span->length,
                                           agg_op_span->data);

    return (ex10_result.error == false);
}

static void print_buffer(struct ByteSpan* agg_op_span)
{
    if (agg_op_span == NULL || agg_op_span->data == NULL)
    {
        return;
    }

    size_t  idx                 = 0;
    ssize_t instruction_counter = 0;
    while (idx < agg_op_span->length)
    {
        ex10_printf("Instruction #%zu @ 0x%04zX: ", instruction_counter, idx);
        // add 1 for the instruction
        size_t command_size = 1;
        switch (agg_op_span->data[idx])
        {
            case InstructionTypeWrite:
            {
                struct Ex10WriteFormat const* write_inst =
                    ((struct Ex10WriteFormat const*)&agg_op_span
                         ->data[idx + 1]);
                struct RegisterInfo const* reg =
                    ex10_register_lookup_by_addr(write_inst->address);
                const char* reg_name = "";
                if (reg)
                {
                    reg_name = reg->name;
                }

                uint8_t const* data_pointer = (uint8_t const*)write_inst +
                                              sizeof(write_inst->address) +
                                              sizeof(write_inst->length);
                ex10_printf(
                    "Write command - address: 0x%04X %s, length: %d\n"
                    "write data: ",
                    write_inst->address,
                    reg_name,
                    write_inst->length);
                ex10_print_data_line(data_pointer, write_inst->length);
                ex10_printf("\n");

                // add in the size of the struct and the total data length, but
                // remove the size of the data pointer
                command_size += sizeof(write_inst->length) +
                                sizeof(write_inst->address) +
                                write_inst->length;
                break;
            }
            case InstructionTypeReset:
                ex10_printf(
                    "Reset command - location: %d\n",
                    ((struct Ex10ResetFormat const*)&agg_op_span->data[idx + 1])
                        ->destination);
                command_size += sizeof(struct Ex10ResetFormat);
                break;
            case InstructionTypeInsertFifoEvent:
            {
                // If the user passed NULL for the packet, the aggregate op
                // buffer should have a packet header with no additional data
                uint8_t const  trigger_irq = agg_op_span->data[idx + 1];
                uint8_t const* packet_data = &agg_op_span->data[idx + 2];
                // The first byte of the fifo is the length in 32 bit words
                uint8_t packet_len = packet_data[0] * 4;
                ex10_printf(
                    "Insert Fifo command - trigger_irq: %d, packet length: "
                    "%d\n",
                    trigger_irq,
                    packet_len);

                // add in the size of the trigger and the fifo packet
                command_size += 1 + packet_len;
                break;
            }
            case InstructionTypeRunOp:
                ex10_printf("Run op id 0x%02X\n",
                            ((struct AggregateRunOpFormat const*)&agg_op_span
                                 ->data[idx + 1])
                                ->op_to_run);
                command_size += sizeof(struct AggregateRunOpFormat);
                break;
            case InstructionTypeGoToIndex:
                ex10_printf(
                    "Goto index: %d\n",
                    ((struct AggregateGoToIndexFormat const*)&agg_op_span
                         ->data[idx + 1])
                        ->jump_index);
                command_size += sizeof(struct AggregateGoToIndexFormat);
                break;
            case InstructionTypeIdentifier:
                ex10_printf(
                    "Update identifier: 0x%04X\n",
                    ((struct AggregateIdentifierFormat const*)&agg_op_span
                         ->data[idx + 1])
                        ->identifier);
                command_size += sizeof(struct AggregateIdentifierFormat);
                break;
            case InstructionTypeExitInstruction:
                ex10_printf("Exit command\n");
                // No extra info needed for Exit command
                return;
            default:
                ex10_eprintf("No matching command found\n");
                return;
        }
        // add the size of the current instruction data
        idx += command_size;
        instruction_counter++;
    }
}

static bool append_reg_write(struct RegisterInfo const* const reg_info,
                             struct ConstByteSpan const*      data_to_write,
                             struct ByteSpan*                 agg_op_span)
{
    if (reg_info == NULL || data_to_write == NULL ||
        data_to_write->data == NULL)
    {
        return false;
    }

    union AggregateInstructionData write_format = {
        .write_format = {
            .address = reg_info->address,
            .length  = (uint16_t)data_to_write->length,
            .data    = data_to_write->data,
        }};

    // Create the write instruction
    const struct AggregateOpInstruction op_instruction = {
        .instruction_type = InstructionTypeWrite,
        .instruction_data = &write_format,
    };
    // Append to the current buffer
    return append_instruction(op_instruction, agg_op_span);
}

static bool append_reset(uint8_t destination, struct ByteSpan* agg_op_span)
{
    union AggregateInstructionData reset_format = {
        .reset_format = {.destination = destination}};

    const struct AggregateOpInstruction op_instruction = {
        .instruction_type = InstructionTypeReset,
        .instruction_data = &reset_format,
    };

    return append_instruction(op_instruction, agg_op_span);
}

static bool append_op_run(enum OpId op_id, struct ByteSpan* agg_op_span)
{
    union AggregateInstructionData run_op = {
        .run_op_format = {.op_to_run = (uint8_t)op_id}};

    const struct AggregateOpInstruction op_instruction = {
        .instruction_type = InstructionTypeRunOp,
        .instruction_data = &run_op,
    };

    return append_instruction(op_instruction, agg_op_span);
}

static bool append_go_to_instruction(uint16_t         jump_index,
                                     uint8_t          repeat_counter,
                                     struct ByteSpan* agg_op_span)
{
    union AggregateInstructionData go_to_index = {
        .go_to_index_format = {
            .jump_index     = jump_index,
            .repeat_counter = repeat_counter,
        }};

    struct AggregateOpInstruction op_instruction = {
        .instruction_type = InstructionTypeGoToIndex,
        .instruction_data = &go_to_index,
    };

    return append_instruction(op_instruction, agg_op_span);
}

static bool append_identifier(uint16_t new_id, struct ByteSpan* agg_op_span)
{
    union AggregateInstructionData identifier = {
        .identifier_format = {.identifier = new_id}};

    const struct AggregateOpInstruction op_instruction = {
        .instruction_type = InstructionTypeIdentifier,
        .instruction_data = &identifier,
    };

    return append_instruction(op_instruction, agg_op_span);
}

static bool append_exit_instruction(struct ByteSpan* agg_op_span)
{
    const struct AggregateOpInstruction op_instruction = {
        .instruction_type = InstructionTypeExitInstruction,
        .instruction_data = NULL};

    return append_instruction(op_instruction, agg_op_span);
}

static bool append_set_rf_mode(uint16_t rf_mode, struct ByteSpan* agg_op_span)
{
    struct ConstByteSpan rf_mode_data = {.data   = ((uint8_t const*)&rf_mode),
                                         .length = sizeof(rf_mode)};
    if (!append_reg_write(&rf_mode_reg, &rf_mode_data, agg_op_span) ||
        !append_op_run(SetRfModeOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static uint16_t channel_enable_bits(uint8_t channel_start, uint8_t num_channels)
{
    uint8_t const max_channels =
        aux_adc_results_reg.num_entries - (uint8_t)channel_start;
    num_channels = (num_channels <= max_channels) ? num_channels : max_channels;
    uint16_t enable_bits = (uint16_t)(1u << num_channels);
    enable_bits -= 1u;
    enable_bits <<= channel_start;
    return enable_bits;
}

static bool append_measure_aux_adc(
    enum AuxAdcResultsAdcResult adc_channel_start,
    uint8_t                     num_channels,
    struct ByteSpan*            agg_op_span)
{
    // Limit the number of ADC conversion channels to the possible range.
    if (adc_channel_start >= aux_adc_results_reg.num_entries)
    {
        return false;
    }

    struct AuxAdcControlFields const adc_control = {
        .channel_enable_bits =
            channel_enable_bits((uint8_t)adc_channel_start, num_channels),
        .rfu = 0u};
    struct ConstByteSpan adc_control_span = {
        .data = ((uint8_t const*)&adc_control), .length = sizeof(adc_control)};

    if (!append_reg_write(
            &aux_adc_control_reg, &adc_control_span, agg_op_span) ||
        !append_op_run(MeasureAdcOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_set_gpio(uint32_t         gpio_levels,
                            uint32_t         gpio_enables,
                            struct ByteSpan* agg_op_span)
{
    struct ConstByteSpan levels  = {.data   = ((uint8_t const*)&gpio_levels),
                                   .length = sizeof(gpio_levels)};
    struct ConstByteSpan enables = {.data   = ((uint8_t const*)&gpio_enables),
                                    .length = sizeof(gpio_enables)};

    if (!append_reg_write(&gpio_output_level_reg, &levels, agg_op_span) ||
        !append_reg_write(&gpio_output_enable_reg, &enables, agg_op_span) ||
        !append_op_run(SetGpioOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_set_clear_gpio_pins(
    struct GpioPinsSetClear const* gpio_pins_set_clear,
    struct ByteSpan*               agg_op_span)
{
    struct ConstByteSpan const span_gpio_set_clear_pins_data = {
        .data   = (uint8_t const*)gpio_pins_set_clear,
        .length = sizeof(*gpio_pins_set_clear),
    };

    if (!append_reg_write(&gpio_output_level_set_reg,
                          &span_gpio_set_clear_pins_data,
                          agg_op_span) ||
        !append_op_run(SetClearGpioPinsOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_lock_synthesizer(uint8_t          r_divider,
                                    uint16_t         n_divider,
                                    struct ByteSpan* agg_op_span)
{
    struct RfSynthesizerControlFields const synth_control = {
        .n_divider = n_divider, .r_divider = r_divider, .lf_type = 1u};
    struct RfSynthesizerControlFields
        synth_control_array[RF_SYNTHESIZER_CONTROL_REG_ENTRIES];
    synth_control_array[0] = synth_control;
    synth_control_array[1] = synth_control;
    synth_control_array[2] = synth_control;
    synth_control_array[3] = synth_control;
    synth_control_array[4] = synth_control;

    struct ConstByteSpan synth_span = {
        .data = ((uint8_t const*)synth_control_array),
        .length =
            sizeof(synth_control) * rf_synthesizer_control_reg.num_entries};

    if (!append_reg_write(
            &rf_synthesizer_control_reg, &synth_span, agg_op_span) ||
        !append_op_run(LockSynthesizerOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_sjc_settings(
    struct SjcControlFields const*             sjc_control,
    struct SjcGainControlFields const*         sjc_rx_gain,
    struct SjcInitialSettlingTimeFields const* initial_settling_time,
    struct SjcResidueSettlingTimeFields const* residue_settling_time,
    struct SjcCdacIFields const*               cdac,
    struct SjcResidueThresholdFields const*    sjc_residue_threshold,
    struct ByteSpan*                           agg_op_span)
{
    if (sjc_control == NULL || sjc_rx_gain == NULL ||
        initial_settling_time == NULL || residue_settling_time == NULL ||
        cdac == NULL || sjc_residue_threshold == NULL)
    {
        return false;
    }

    struct ConstByteSpan sjc_control_span = {
        .data   = ((uint8_t const*)sjc_control),
        .length = sizeof(*sjc_control),
    };

    bool append_ok = true;
    append_ok =
        append_reg_write(&sjc_control_reg, &sjc_control_span, agg_op_span);
    if (!append_ok)
    {
        return false;
    }

    struct ConstByteSpan sjc_rx_gain_span = {
        .data   = ((uint8_t const*)sjc_rx_gain),
        .length = sizeof(*sjc_rx_gain),
    };

    append_ok =
        append_reg_write(&sjc_gain_control_reg, &sjc_rx_gain_span, agg_op_span);
    if (!append_ok)
    {
        return false;
    }

    struct ConstByteSpan initial_settling_time_span = {
        .data   = ((uint8_t const*)initial_settling_time),
        .length = sizeof(*initial_settling_time),
    };

    append_ok = append_reg_write(&sjc_initial_settling_time_reg,
                                 &initial_settling_time_span,
                                 agg_op_span);
    if (!append_ok)
    {
        return false;
    }

    struct ConstByteSpan residue_settling_time_span = {
        .data   = ((uint8_t const*)residue_settling_time),
        .length = sizeof(*residue_settling_time),
    };

    append_ok = append_reg_write(&sjc_residue_settling_time_reg,
                                 &residue_settling_time_span,
                                 agg_op_span);
    if (!append_ok)
    {
        return false;
    }

    struct ConstByteSpan sjc_cdac_span = {
        .data   = ((uint8_t const*)cdac),
        .length = sizeof(*cdac),
    };

    append_ok =
        (append_reg_write(&sjc_cdac_i_reg, &sjc_cdac_span, agg_op_span) &&
         append_reg_write(&sjc_cdac_q_reg, &sjc_cdac_span, agg_op_span));
    if (!append_ok)
    {
        return false;
    }

    struct ConstByteSpan sjc_residue_threshold_span = {
        .data   = ((uint8_t const*)sjc_residue_threshold),
        .length = sizeof(*sjc_residue_threshold),
    };

    append_ok = append_reg_write(
        &sjc_residue_threshold_reg, &sjc_residue_threshold_span, agg_op_span);
    return append_ok;
}

static bool append_run_sjc(struct ByteSpan* agg_op_span)
{
    return append_op_run(RxRunSjcOp, agg_op_span);
}

static bool append_set_tx_coarse_gain(uint8_t          tx_atten,
                                      struct ByteSpan* agg_op_span)
{
    struct TxCoarseGainFields const coarse_gain = {.tx_atten = tx_atten};
    struct ConstByteSpan coarse_span = {.data = ((uint8_t const*)&coarse_gain),
                                        .length = sizeof(coarse_gain)};
    if (!append_reg_write(&tx_coarse_gain_reg, &coarse_span, agg_op_span) ||
        !append_op_run(SetTxCoarseGainOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_set_tx_fine_gain(int16_t          tx_scalar,
                                    struct ByteSpan* agg_op_span)
{
    struct TxFineGainFields const fine_gain = {.tx_scalar = tx_scalar};
    struct ConstByteSpan fine_span = {.data   = ((uint8_t const*)&fine_gain),
                                      .length = sizeof(fine_gain)};

    if (!append_reg_write(&tx_fine_gain_reg, &fine_span, agg_op_span) ||
        !append_op_run(SetTxFineGainOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_set_regulatory_timers(
    struct Ex10RegulatoryTimers const* timer_config,
    struct ByteSpan*                   agg_op_span)
{
    if (timer_config == NULL)
    {
        return false;
    }

    struct NominalStopTimeFields const nominal_timer = {
        .dwell_time = timer_config->nominal_ms};
    struct ExtendedStopTimeFields const extended_timer = {
        .dwell_time = timer_config->extended_ms};
    struct RegulatoryStopTimeFields const regulatory_timer = {
        .dwell_time = timer_config->regulatory_ms};
    struct EtsiBurstOffTimeFields const off_timer = {
        .off_time = timer_config->off_same_channel_ms};

    struct ConstByteSpan nom_span = {.data   = ((uint8_t const*)&nominal_timer),
                                     .length = sizeof(nominal_timer)};
    struct ConstByteSpan ext_span = {.data = ((uint8_t const*)&extended_timer),
                                     .length = sizeof(extended_timer)};
    struct ConstByteSpan reg_span = {
        .data   = ((uint8_t const*)&regulatory_timer),
        .length = sizeof(regulatory_timer)};
    struct ConstByteSpan burst_span = {.data   = ((uint8_t const*)&off_timer),
                                       .length = sizeof(off_timer)};

    if (!append_reg_write(&nominal_stop_time_reg, &nom_span, agg_op_span) ||
        !append_reg_write(&extended_stop_time_reg, &ext_span, agg_op_span) ||
        !append_reg_write(&regulatory_stop_time_reg, &reg_span, agg_op_span) ||
        !append_reg_write(&etsi_burst_off_time_reg, &burst_span, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_tx_ramp_up(int32_t dc_offset, struct ByteSpan* agg_op_span)
{
    struct DcOffsetFields const offset_fields = {.offset = dc_offset};
    struct ConstByteSpan        offset_span   = {
        .data   = ((uint8_t const*)&offset_fields),
        .length = sizeof(offset_fields)};

    if (!append_reg_write(&dc_offset_reg, &offset_span, agg_op_span) ||
        !append_op_run(TxRampUpOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_power_control(struct PowerConfigs const* power_config,
                                 struct ByteSpan*           agg_op_span)
{
    if (power_config == NULL)
    {
        return false;
    }

    // Adc target 0 means we don't intend to run power control
    if (power_config->adc_target == 0)
    {
        return true;
    }

    // Registers used to configure the power control loop
    struct PowerControlLoopAuxAdcControlFields const adc_control = {
        .channel_enable_bits = (uint16_t)power_config->power_detector_adc};
    struct PowerControlLoopGainDivisorFields const gain_divisor = {
        .gain_divisor = power_config->loop_gain_divisor};
    struct PowerControlLoopMaxIterationsFields const max_iterations = {
        .max_iterations = power_config->max_iterations};
    struct PowerControlLoopAdcTargetFields const adc_target = {
        .adc_target_value = power_config->adc_target};
    struct PowerControlLoopAdcThresholdsFields const adc_thresholds = {
        .loop_stop_threshold = power_config->loop_stop_threshold,
        .op_error_threshold  = power_config->op_error_threshold};

    struct ConstByteSpan adc_control_span = {
        .data = ((uint8_t const*)&adc_control), .length = sizeof(adc_control)};
    struct ConstByteSpan gain_divisor_span = {
        .data   = ((uint8_t const*)&gain_divisor),
        .length = sizeof(gain_divisor)};
    struct ConstByteSpan max_iterations_span = {
        .data   = ((uint8_t const*)&max_iterations),
        .length = sizeof(max_iterations)};
    struct ConstByteSpan adc_target_span = {
        .data = ((uint8_t const*)&adc_target), .length = sizeof(adc_target)};
    struct ConstByteSpan adc_thresholds_span = {
        .data   = ((uint8_t const*)&adc_thresholds),
        .length = sizeof(adc_thresholds)};

    if (!append_reg_write(&power_control_loop_aux_adc_control_reg,
                          &adc_control_span,
                          agg_op_span) ||
        !append_reg_write(&power_control_loop_gain_divisor_reg,
                          &gain_divisor_span,
                          agg_op_span) ||
        !append_reg_write(&power_control_loop_max_iterations_reg,
                          &max_iterations_span,
                          agg_op_span) ||
        !append_reg_write(&power_control_loop_adc_target_reg,
                          &adc_target_span,
                          agg_op_span) ||
        !append_reg_write(&power_control_loop_adc_thresholds_reg,
                          &adc_thresholds_span,
                          agg_op_span) ||
        !append_op_run(PowerControlLoopOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_start_log_test(uint32_t         period,
                                  uint16_t         repeat,
                                  struct ByteSpan* agg_op_span)
{
    struct LogTestPeriodFields const     log_period = {.period = period};
    struct LogTestWordRepeatFields const log_repeat = {.repeat = repeat};

    struct ConstByteSpan period_span = {.data   = ((uint8_t const*)&log_period),
                                        .length = sizeof(log_period)};
    struct ConstByteSpan word_repeat_span = {
        .data = ((uint8_t const*)&log_repeat), .length = sizeof(log_repeat)};

    if (!append_reg_write(&log_test_period_reg, &period_span, agg_op_span) ||
        !append_reg_write(
            &log_test_word_repeat_reg, &word_repeat_span, agg_op_span) ||
        !append_op_run(LogTestOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_set_atest_mux(uint32_t         atest_mux_0,
                                 uint32_t         atest_mux_1,
                                 uint32_t         atest_mux_2,
                                 uint32_t         atest_mux_3,
                                 struct ByteSpan* agg_op_span)
{
    uint32_t const atest_mux[] = {
        atest_mux_0, atest_mux_1, atest_mux_2, atest_mux_3};
    struct ConstByteSpan atest_span = {.data   = ((uint8_t const*)atest_mux),
                                       .length = sizeof(atest_mux)};

    if (!append_reg_write(&a_test_mux_reg, &atest_span, agg_op_span) ||
        !append_op_run(SetATestMuxOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_set_aux_dac(uint8_t          dac_channel_start,
                               uint8_t          num_channels,
                               uint16_t const*  dac_values,
                               struct ByteSpan* agg_op_span)
{
    if (dac_values == NULL)
    {
        return false;
    }

    // Limit the number of ADC conversion channels to the possible range.
    if (dac_channel_start + num_channels > aux_dac_settings_reg.num_entries)
    {
        return false;
    }

    struct AuxDacControlFields const dac_control = {
        .channel_enable_bits =
            (uint8_t)channel_enable_bits(dac_channel_start, num_channels),
        .rfu = 0u};

    // Create new reg info based off the subset of entries to use
    uint16_t const offset = dac_channel_start * aux_dac_settings_reg.length;
    struct RegisterInfo const dac_settings_reg = {
        .address     = aux_dac_settings_reg.address + offset,
        .length      = aux_dac_settings_reg.length,
        .num_entries = num_channels,
        .access      = ReadOnly,
    };

    struct ConstByteSpan dac_control_span = {
        .data = ((uint8_t const*)&dac_control), .length = sizeof(dac_control)};
    struct ConstByteSpan dac_settings_span = {
        .data   = ((uint8_t const*)dac_values),
        .length = num_channels * sizeof(uint16_t)};

    if (!append_reg_write(
            &aux_dac_control_reg, &dac_control_span, agg_op_span) ||
        !append_reg_write(&dac_settings_reg, &dac_settings_span, agg_op_span) ||
        !append_op_run(SetDacOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_tx_ramp_down(struct ByteSpan* agg_op_span)
{
    return append_op_run(TxRampDownOp, agg_op_span);
}

static bool append_radio_power_control(bool             enable,
                                       struct ByteSpan* agg_op_span)
{
    struct AnalogEnableFields const analog_enable = {.all = enable};

    struct ConstByteSpan analog_span = {
        .data   = ((uint8_t const*)&analog_enable),
        .length = sizeof(analog_enable)};

    if (!append_reg_write(&analog_enable_reg, &analog_span, agg_op_span) ||
        !append_op_run(RadioPowerControlOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_set_analog_rx_config(
    struct RxGainControlFields const* analog_rx_fields,
    struct ByteSpan*                  agg_op_span)
{
    if (analog_rx_fields == NULL)
    {
        return false;
    }

    struct ConstByteSpan analog_rx_span = {
        .data   = ((uint8_t const*)analog_rx_fields),
        .length = sizeof(struct RxGainControlFields)};

    // NOTE: This does not update the op_variables.stored_analog_rx_fields,
    // therefore the ops layer rx gain will be incorrect.
    // The reader layer uses this value when reading tags.
    if (!append_reg_write(&rx_gain_control_reg, &analog_rx_span, agg_op_span) ||
        !append_op_run(SetRxGainOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_measure_rssi(struct ByteSpan* agg_op_span,
                                uint8_t          rssi_count)
{
    struct MeasureRssiCountFields rssi_count_fields = {.samples = rssi_count};
    struct ConstByteSpan          rssi_count_span   = {
        .data   = ((uint8_t const*)&rssi_count_fields),
        .length = sizeof(rssi_count_fields)};

    if (!append_reg_write(
            &measure_rssi_count_reg, &rssi_count_span, agg_op_span) ||
        !append_op_run(MeasureRssiOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_hpf_override_test(struct ByteSpan* agg_op_span,
                                     uint8_t          hpf_mode)
{
    struct HpfOverrideSettingsFields hpf_fields = {.hpf_mode = hpf_mode};
    struct ConstByteSpan hpf_span = {.data   = ((uint8_t const*)&hpf_fields),
                                     .length = sizeof(hpf_fields)};

    if (!append_reg_write(&hpf_override_settings_reg, &hpf_span, agg_op_span) ||
        !append_op_run(HpfOverrideTestOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_listen_before_talk(struct ByteSpan* agg_op_span,
                                      uint8_t          r_divider_index,
                                      uint16_t         n_divider,
                                      int32_t          offset_frequency_khz,
                                      uint8_t          rssi_count)
{
    struct RfSynthesizerControlFields const synth_control = {
        .n_divider = n_divider, .r_divider = r_divider_index, .lf_type = 1u};
    struct ConstByteSpan synth_span = {.data = ((uint8_t const*)&synth_control),
                                       .length = sizeof(synth_control)};

    struct MeasureRssiCountFields rssi_count_fields = {.samples = rssi_count};
    struct ConstByteSpan          rssi_count_span   = {
        .data   = ((uint8_t const*)&rssi_count_fields),
        .length = sizeof(rssi_count_fields)};

    struct LbtOffsetFields const offset_fields = {.khz = offset_frequency_khz};
    struct ConstByteSpan         offset_span   = {
        .data   = ((uint8_t const*)&offset_fields),
        .length = sizeof(offset_fields)};

    if (!append_reg_write(
            &rf_synthesizer_control_reg, &synth_span, agg_op_span) ||
        !append_reg_write(
            &measure_rssi_count_reg, &rssi_count_span, agg_op_span) ||
        !append_reg_write(&lbt_offset_reg, &offset_span, agg_op_span) ||
        !append_op_run(ListenBeforeTalkOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_start_timer_op(uint32_t         delay_us,
                                  struct ByteSpan* agg_op_span)
{
    struct DelayUsFields delay_fields = {.delay = delay_us};

    struct ConstByteSpan delay_us_span = {
        .data   = ((uint8_t const*)&delay_fields),
        .length = sizeof(delay_fields)};

    if (!append_reg_write(&delay_us_reg, &delay_us_span, agg_op_span) ||
        !append_op_run(UsTimerStartOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_wait_timer_op(struct ByteSpan* agg_op_span)
{
    return append_op_run(UsTimerWaitOp, agg_op_span);
}

static bool append_start_event_fifo_test(uint32_t         period,
                                         uint8_t          num_words,
                                         struct ByteSpan* agg_op_span)
{
    struct EventFifoTestPeriodFields const test_period = {.period = period};
    struct EventFifoTestPayloadNumWordsFields const payload_words = {
        .num_words = num_words};

    struct ConstByteSpan period_span = {.data = ((uint8_t const*)&test_period),
                                        .length = sizeof(test_period)};
    struct ConstByteSpan num_words_span = {
        .data   = ((uint8_t const*)&payload_words),
        .length = sizeof(payload_words)};

    if (!append_reg_write(
            &event_fifo_test_period_reg, &period_span, agg_op_span) ||
        !append_reg_write(&event_fifo_test_payload_num_words_reg,
                          &num_words_span,
                          agg_op_span) ||
        !append_op_run(EventFifoTestOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_insert_fifo_event(const bool                    trigger_irq,
                                     const struct EventFifoPacket* event_packet,
                                     struct ByteSpan*              agg_op_span)
{
    // Create fifo buffer
    // The max sized fifo to insert with the aggregate op is 64 bytes. This
    // is larger than any existing fifo events and still allows plenty of
    // room for custom events.
    uint8_t fifo_buffer[64];
    ex10_memzero(&fifo_buffer, sizeof(fifo_buffer));

    // Create a basic packet for if the passed event is NULL
    struct EventFifoPacket const empty_event_packet = {
        .packet_type         = (enum EventPacketType)0,
        .us_counter          = 0u,
        .static_data         = NULL,
        .static_data_length  = 0,
        .dynamic_data        = NULL,
        .dynamic_data_length = 0u,
        .is_valid            = true};
    if (event_packet == NULL)
    {
        event_packet = &empty_event_packet;
    }

    // Find size necessary to append the fifo event
    size_t event_bytes = sizeof(struct PacketHeader) +
                         event_packet->static_data_length +
                         event_packet->dynamic_data_length;
    size_t padding_bytes = (4 - event_bytes % 4) % 4;
    size_t packet_bytes  = event_bytes + padding_bytes;

    // Packet is too large to append
    if (packet_bytes > sizeof(fifo_buffer))
    {
        return false;
    }

    // Translate the packet into a buffer format
    get_ex10_commands()->create_fifo_event(
        event_packet, fifo_buffer, padding_bytes, packet_bytes);

    // Create fifo event using created buffer
    union AggregateInstructionData insert_fifo_event_format = {
        .insert_fifo_event_format = {.trigger_irq = trigger_irq,
                                     .packet      = fifo_buffer}};

    // Create aggregate op instruction
    const struct AggregateOpInstruction op_instruction = {
        .instruction_type = InstructionTypeInsertFifoEvent,
        .instruction_data = &insert_fifo_event_format};

    // Add the instruction to the aggregate buffer
    return append_instruction(op_instruction, agg_op_span);
}

static bool append_enable_sdd_logs(const struct LogEnablesFields enables,
                                   const uint8_t                 speed_mhz,
                                   struct ByteSpan*              agg_op_span)
{
    struct LogSpeedFields log_speed = {.speed_mhz = speed_mhz, .rfu = 0u};

    struct ConstByteSpan enables_span = {.data   = ((uint8_t const*)&enables),
                                         .length = sizeof(enables)};
    struct ConstByteSpan speed_span   = {.data   = ((uint8_t const*)&log_speed),
                                       .length = sizeof(log_speed)};

    if (!append_reg_write(&log_enables_reg, &enables_span, agg_op_span) ||
        !append_reg_write(&log_speed_reg, &speed_span, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_start_inventory_round(
    struct InventoryRoundControlFields const*   configs,
    struct InventoryRoundControl_2Fields const* configs_2,
    struct ByteSpan*                            agg_op_span)
{
    if (configs == NULL || configs_2 == NULL)
    {
        return false;
    }

    struct ConstByteSpan control_1_span = {.data   = ((uint8_t const*)configs),
                                           .length = sizeof(*configs)};
    struct ConstByteSpan control_2_span = {.data = ((uint8_t const*)configs_2),
                                           .length = sizeof(*configs_2)};

    if (!append_reg_write(
            &inventory_round_control_reg, &control_1_span, agg_op_span) ||
        !append_reg_write(
            &inventory_round_control_2_reg, &control_2_span, agg_op_span) ||
        !append_op_run(StartInventoryRoundOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_start_prbs(struct ByteSpan* agg_op_span)
{
    return append_op_run(RunPrbsDataOp, agg_op_span);
}

static bool append_start_ber_test(uint16_t         num_bits,
                                  uint16_t         num_packets,
                                  bool             delimiter_only,
                                  struct ByteSpan* agg_op_span)
{
    // Determine whether to use a delimiter only instead of a full query
    struct BerModeFields    ber_mode    = {.del_only_mode = delimiter_only};
    struct BerControlFields ber_control = {.num_bits    = num_bits,
                                           .num_packets = num_packets};

    struct ConstByteSpan mode_span    = {.data   = ((uint8_t const*)&ber_mode),
                                      .length = sizeof(ber_mode)};
    struct ConstByteSpan control_span = {.data = ((uint8_t const*)&ber_control),
                                         .length = sizeof(ber_control)};

    if (!append_reg_write(&ber_mode_reg, &mode_span, agg_op_span) ||
        !append_reg_write(&ber_control_reg, &control_span, agg_op_span) ||
        !append_op_run(BerTestOp, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_ramp_transmit_power(
    struct PowerConfigs*               power_config,
    struct Ex10RegulatoryTimers const* timer_config,
    struct ByteSpan*                   agg_op_span)
{
    if (power_config == NULL || timer_config == NULL)
    {
        return false;
    }

    // If the coarse gain has changes, store the new val and run the op
    if (!append_set_tx_coarse_gain(power_config->tx_atten, agg_op_span) ||
        !append_set_tx_fine_gain(power_config->tx_scalar, agg_op_span) ||
        !append_set_regulatory_timers(timer_config, agg_op_span) ||
        !append_tx_ramp_up(power_config->dc_offset, agg_op_span) ||
        !append_power_control(power_config, agg_op_span))
    {
        return false;
    }
    return true;
}

static bool append_droop_compensation(
    struct PowerDroopCompensationFields const* compensation,
    struct ByteSpan*                           agg_op_span)
{
    if (compensation == NULL)
    {
        return false;
    }

    struct ConstByteSpan comp_span = {.data   = ((uint8_t const*)compensation),
                                      .length = sizeof(*compensation)};

    if (!append_reg_write(
            &power_droop_compensation_reg, &comp_span, agg_op_span))
    {
        return false;
    }
    return true;
}

static void print_aggregate_op_errors(
    const struct AggregateOpSummary* agg_summary)
{
    // Print out any  errors related to ops for clarity
    if (agg_summary->last_inner_op_error != ErrorNone)
    {
        ex10_eputs(
            "The last op run in the aggregate op was 0x%X, "
            "which ended with an error code of %d\n",
            agg_summary->last_inner_op_run,
            agg_summary->last_inner_op_error);
        ex10_eputs("For meaning of the error code, reference enum OpsStatus\n");
        ex10_eputs(
            "The number of ops that ran including the one that failed is %d\n",
            agg_summary->op_run_count);
    }

    // Grab the aggregate op buffer from the device
    uint8_t         aggregate_buffer[AGGREGATE_OP_BUFFER_REG_LENGTH];
    struct ByteSpan agg_op_span = {.data   = aggregate_buffer,
                                   .length = aggregate_op_buffer_reg.length};
    get_ex10_protocol()->read(&aggregate_op_buffer_reg, aggregate_buffer);

    // Parse the buffer
    struct AggregateOpInstruction  instruction_at_index;
    union AggregateInstructionData instruction_data;
    instruction_at_index.instruction_data = &instruction_data;

    ssize_t err_inst =
        get_instruction_from_index(agg_summary->final_buffer_byte_index,
                                   &agg_op_span,
                                   &instruction_at_index);
    if (err_inst == -1)
    {
        ex10_eputs(
            "There is no valid instruction at the index which was "
            "reported to cause the error\n");
        ex10_eputs("agg op buffer:\n");
        ex10_print_data(agg_op_span.data, agg_op_span.length, DataPrefixIndex);
    }
    else if (instruction_at_index.instruction_type ==
             InstructionTypeExitInstruction)
    {
        ex10_eputs(
            "The aggregate op successfully reached an exit instruction.\n");
    }
    else
    {
        ex10_eputs("The error occurred at instruction %zd in the buffer\n",
                   err_inst);
    }

    ex10_eputs("Dumping the contents of the buffer for debug\n");
    print_buffer(&agg_op_span);
}


struct Ex10AggregateOpBuilder const* get_ex10_aggregate_op_builder(void)
{
    static struct Ex10AggregateOpBuilder gen2_aggregate_op_builder = {
        .append_instruction           = append_instruction,
        .clear_buffer                 = clear_buffer,
        .set_buffer                   = set_buffer,
        .get_instruction_from_index   = get_instruction_from_index,
        .print_buffer                 = print_buffer,
        .print_aggregate_op_errors    = print_aggregate_op_errors,
        .append_reg_write             = append_reg_write,
        .append_reset                 = append_reset,
        .append_insert_fifo_event     = append_insert_fifo_event,
        .append_op_run                = append_op_run,
        .append_go_to_instruction     = append_go_to_instruction,
        .append_identifier            = append_identifier,
        .append_exit_instruction      = append_exit_instruction,
        .append_set_rf_mode           = append_set_rf_mode,
        .append_measure_aux_adc       = append_measure_aux_adc,
        .append_set_gpio              = append_set_gpio,
        .append_set_clear_gpio_pins   = append_set_clear_gpio_pins,
        .append_lock_synthesizer      = append_lock_synthesizer,
        .append_sjc_settings          = append_sjc_settings,
        .append_run_sjc               = append_run_sjc,
        .append_set_tx_coarse_gain    = append_set_tx_coarse_gain,
        .append_set_tx_fine_gain      = append_set_tx_fine_gain,
        .append_set_regulatory_timers = append_set_regulatory_timers,
        .append_tx_ramp_up            = append_tx_ramp_up,
        .append_power_control         = append_power_control,
        .append_start_log_test        = append_start_log_test,
        .append_set_atest_mux         = append_set_atest_mux,
        .append_set_aux_dac           = append_set_aux_dac,
        .append_tx_ramp_down          = append_tx_ramp_down,
        .append_radio_power_control   = append_radio_power_control,
        .append_set_analog_rx_config  = append_set_analog_rx_config,
        .append_measure_rssi          = append_measure_rssi,
        .append_hpf_override_test     = append_hpf_override_test,
        .append_listen_before_talk    = append_listen_before_talk,
        .append_start_timer_op        = append_start_timer_op,
        .append_wait_timer_op         = append_wait_timer_op,
        .append_start_event_fifo_test = append_start_event_fifo_test,
        .append_enable_sdd_logs       = append_enable_sdd_logs,
        .append_start_inventory_round = append_start_inventory_round,
        .append_start_prbs            = append_start_prbs,
        .append_start_ber_test        = append_start_ber_test,
        .append_ramp_transmit_power   = append_ramp_transmit_power,
        .append_droop_compensation    = append_droop_compensation,
    };

    return &gen2_aggregate_op_builder;
}
