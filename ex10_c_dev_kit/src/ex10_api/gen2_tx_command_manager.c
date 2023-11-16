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

#include <string.h>

#include "board/ex10_osal.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/byte_span.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/gen2_tx_command_manager.h"


struct Gen2BufferBuilderVariables
{
    struct TxCommandInfo commands_list[10];
};
static struct Gen2BufferBuilderVariables builder;


static void clear_local_sequence(void)
{
    for (uint8_t idx = 0u; idx < MaxTxCommandCount; idx++)
    {
        builder.commands_list[idx].valid = false;
    }
}

static struct Ex10Result clear_command_in_local_sequence(uint8_t clear_idx,
                                                         size_t* cmd_index)
{
    if (!cmd_index)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorNullPointer);
    }
    *cmd_index = 0;

    if (clear_idx >= MaxTxCommandCount)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10ErrorGen2NumCommands);
    }
    builder.commands_list[clear_idx].valid = false;

    *cmd_index = clear_idx;
    return make_ex10_success();
}

static void clear_sequence(void)
{
    uint16_t zero_enables[MaxTxCommandCount] = {0u};
    uint16_t enable_bits                     = 0u;

    struct Ex10Protocol const* protocol = get_ex10_protocol();

    // By clearing the lengths, we are telling the device each command is
    // 0 bits long, thus invalidating them.
    protocol->write(&gen2_lengths_reg, zero_enables);

    // Clear the command enables
    struct Gen2AccessEnableFields const access_enable_field = {.access_enables =
                                                                   enable_bits};
    protocol->write(&gen2_access_enable_reg, &access_enable_field);

    struct Gen2SelectEnableFields const select_enable_field = {.select_enables =
                                                                   enable_bits};
    protocol->write(&gen2_select_enable_reg, &select_enable_field);

    struct Gen2AutoAccessEnableFields const auto_access_enable_field = {
        .auto_access_enables = enable_bits};
    get_ex10_protocol()->write(&gen2_auto_access_enable_reg,
                               &auto_access_enable_field);
}

static void buffer_builder_init(void)
{
    for (uint8_t idx = 0u; idx < MaxTxCommandCount; idx++)
    {
        builder.commands_list[idx].encoded_command.data =
            builder.commands_list[idx].encoded_buffer;
        builder.commands_list[idx].decoded_command.args =
            builder.commands_list[idx].decoded_buffer;
    }
    clear_local_sequence();
}

static struct Ex10Result write_sequence(void)
{
    uint8_t tx_buffer[GEN2_TX_BUFFER_REG_LENGTH];
    ex10_memzero(&tx_buffer, sizeof(tx_buffer));
    uint8_t ids_list[MaxTxCommandCount];
    ex10_memzero(&ids_list, sizeof(ids_list));
    uint8_t offset_reg_list[MaxTxCommandCount];
    ex10_memzero(&offset_reg_list, sizeof(offset_reg_list));
    uint16_t length_reg_list[MaxTxCommandCount];
    ex10_memzero(&length_reg_list, sizeof(length_reg_list));

    struct Gen2TxnControlsFields txn_control_list[MaxTxCommandCount];

    uint16_t buffer_offset = 0;
    for (uint8_t idx = 0u; idx < MaxTxCommandCount; idx++)
    {
        if (builder.commands_list[idx].valid)
        {
            // Update the register write for offset, length, and id.
            // We know that buffer_offset < sizeof(tx_buffer) - check below -
            // so the cast to uint8_t is valid.
            offset_reg_list[idx] = (uint8_t)buffer_offset;
            ids_list[idx]        = builder.commands_list[idx].transaction_id;
            // Note this is bit length
            length_reg_list[idx] =
                (uint16_t)builder.commands_list[idx].encoded_command.length;
            // find the byte length as well
            uint16_t byte_size =
                (length_reg_list[idx] - (length_reg_list[idx] % 8)) / 8;
            byte_size += (length_reg_list[idx] % 8) ? 1 : 0;

            // Check the command will fit in the buffer
            if (offset_reg_list[idx] + byte_size > sizeof(tx_buffer))
            {
                return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                           Ex10ErrorGen2BufferLength);
            }

            // Update the register write for the gen2 buffer
            int const copy_result =
                ex10_memcpy(&tx_buffer[buffer_offset],
                            sizeof(tx_buffer) - buffer_offset,
                            builder.commands_list[idx].encoded_command.data,
                            byte_size);
            if (copy_result != 0)
            {
                return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                           Ex10MemcpyFailed);
            }
            buffer_offset += byte_size;

            // Zero out the struct before setting it
            ex10_memzero(&txn_control_list[idx], sizeof(txn_control_list[idx]));
            // Update reg write for tx device controls
            struct Ex10Result ex10_result =
                get_ex10_gen2_commands()->get_gen2_tx_control_config(
                    &builder.commands_list[idx].decoded_command,
                    &txn_control_list[idx]);
            if (ex10_result.error)
            {
                return ex10_result;
            }
        }
    }

    // Write all the commands to the buffer
    struct RegisterInfo const* const regs[] = {
        &gen2_offsets_reg,
        &gen2_lengths_reg,
        &gen2_transaction_ids_reg,
        &gen2_txn_controls_reg,
    };

    void const* buffers[] = {
        offset_reg_list,
        length_reg_list,
        ids_list,
        txn_control_list,
    };

    get_ex10_protocol()->write_multiple(regs, buffers, ARRAY_SIZE(regs));
    get_ex10_protocol()->write(&gen2_tx_buffer_reg, tx_buffer);

    return make_ex10_success();
}

static bool get_is_select(enum Gen2Command current_command)
{
    return (current_command == Gen2Select);
}

static struct Ex10Result write_select_enables(bool const* select_enables,
                                              uint8_t     size,
                                              size_t*     cmd_index)
{
    if (!select_enables || !cmd_index)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorNullPointer);
    }

    struct Ex10Result ex10_result = make_ex10_success();
    *cmd_index                    = 0;

    if (size > MaxTxCommandCount)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10ErrorGen2NumCommands);
    }

    uint16_t select_enable_bits = 0u;
    for (uint8_t idx = 0u; idx < size; idx++)
    {
        // if there is an interest in enabling this index
        if (select_enables[idx])
        {
            if (builder.commands_list[idx].valid)
            {
                // check if the command being enabled matches this register
                // If not, we will still enable it, but warn the user
                if (!get_is_select(
                        builder.commands_list[idx].decoded_command.command))
                {
                    ex10_eprintf(
                        "NOTE: Enabling a non-select command at index %zd for "
                        "select op.\n",
                        idx);
                    *cmd_index = idx;
                    return make_ex10_sdk_error(
                        Ex10ModuleGen2Commands,
                        Ex10ErrorGen2CommandEnableMismatch);
                }
                select_enable_bits |= select_enables[idx] << idx;
            }
            else
            {
                *cmd_index = idx;
                return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                           Ex10ErrorGen2EmptyCommand);
            }
        }
    }

    struct Gen2SelectEnableFields const select_enable_field = {
        .select_enables = select_enable_bits};
    get_ex10_protocol()->write(&gen2_select_enable_reg, &select_enable_field);

    return ex10_result;
}

static struct Ex10Result write_halted_enables(bool const* access_enables,
                                              uint8_t     size,
                                              size_t*     cmd_index)
{
    if (access_enables == NULL || cmd_index == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorNullPointer);
    }

    struct Ex10Result ex10_result = make_ex10_success();
    *cmd_index                    = 0;

    if (size > MaxTxCommandCount)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10ErrorGen2NumCommands);
    }

    uint16_t access_bits = 0u;
    for (uint8_t idx = 0u; idx < size; idx++)
    {
        // if there is an interest in enabling this index
        if (access_enables[idx])
        {
            if (builder.commands_list[idx].valid)
            {
                // check if the command being enabled matches this register
                // If not, we will still enable it, but warn the user
                if (get_is_select(
                        builder.commands_list[idx].decoded_command.command))
                {
                    ex10_eprintf(
                        "NOTE: Enabling a select command at index %zd for "
                        "halted.\n",
                        idx);
                    *cmd_index = idx;
                    return make_ex10_sdk_error(
                        Ex10ModuleGen2Commands,
                        Ex10ErrorGen2CommandEnableMismatch);
                }
                access_bits |= access_enables[idx] << idx;
            }
            else
            {
                *cmd_index = idx;
                return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                           Ex10ErrorGen2EmptyCommand);
            }
        }
    }

    struct Gen2AccessEnableFields const access_enable_field = {.access_enables =
                                                                   access_bits};
    get_ex10_protocol()->write(&gen2_access_enable_reg, &access_enable_field);

    return ex10_result;
}

static struct Ex10Result write_auto_access_enables(
    bool const* auto_access_enables,
    uint8_t     size,
    size_t*     cmd_index)
{
    if (auto_access_enables == NULL || cmd_index == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorNullPointer);
    }

    struct Ex10Result ex10_result = make_ex10_success();
    *cmd_index                    = 0;

    if (size > MaxTxCommandCount)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10ErrorGen2NumCommands);
    }

    uint16_t auto_access_bits = 0u;
    for (uint8_t idx = 0u; idx < size; idx++)
    {
        // if there is an interest in enabling this index
        if (auto_access_enables[idx])
        {
            if (builder.commands_list[idx].valid)
            {
                // check if the command being enabled matches this register
                // If not, we will still enable it, but warn the user
                if (get_is_select(
                        builder.commands_list[idx].decoded_command.command))
                {
                    ex10_eprintf(
                        "NOTE: Enabling a select command at index %zd for auto "
                        "access.\n",
                        idx);

                    *cmd_index = idx;
                    return make_ex10_sdk_error(
                        Ex10ModuleGen2Commands,
                        Ex10ErrorGen2CommandEnableMismatch);
                }
                auto_access_bits |= auto_access_enables[idx] << idx;
            }
            else
            {
                *cmd_index = idx;
                return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                           Ex10ErrorGen2EmptyCommand);
            }
        }
    }

    struct Gen2AutoAccessEnableFields const auto_access_enable_field = {
        .auto_access_enables = auto_access_bits};
    get_ex10_protocol()->write(&gen2_auto_access_enable_reg,
                               &auto_access_enable_field);

    return ex10_result;
}

static struct Ex10Result append_encoded_command(const struct BitSpan* tx_buffer,
                                                uint8_t transaction_id,
                                                size_t* cmd_index)
{
    if (tx_buffer == NULL || cmd_index == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorNullPointer);
    }

    *cmd_index = 0;

    // Find the next available slot
    uint8_t index = 0;
    while (builder.commands_list[index].valid)
    {
        index++;
        // No room, return an error
        if (index >= MaxTxCommandCount)
        {
            return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                       Ex10ErrorGen2NumCommands);
        }
    }

    // Also store the decoded command for debug and tx configuration registers
    struct Ex10Result ex10_result =
        get_ex10_gen2_commands()->decode_gen2_command(
            &builder.commands_list[index].decoded_command, tx_buffer);
    if (ex10_result.error)
    {
        ex10_eprintf("Command decode failed (transaction id = %d)\n",
                     transaction_id);
        return ex10_result;
    }

    // Store the encoded data
    builder.commands_list[index].encoded_command.length = tx_buffer->length;
    int const copy_result =
        ex10_memcpy(builder.commands_list[index].encoded_command.data,
                    sizeof(builder.commands_list[index].encoded_buffer),
                    tx_buffer->data,
                    tx_buffer->length);
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands, Ex10MemcpyFailed);
    }

    builder.commands_list[index].valid          = true;
    builder.commands_list[index].transaction_id = transaction_id;

    *cmd_index = index;
    return make_ex10_success();
}

static struct Ex10Result encode_and_append_command(
    struct Gen2CommandSpec* cmd_spec,
    uint8_t                 transaction_id,
    size_t*                 cmd_index)
{
    if (cmd_spec == NULL || cmd_index == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                   Ex10SdkErrorNullPointer);
    }

    *cmd_index = 0;

    // Find the next available slot
    uint8_t index = 0;
    while (builder.commands_list[index].valid)
    {
        index++;
        // No room, return an error
        if (index >= MaxTxCommandCount)
        {
            return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                       Ex10ErrorGen2NumCommands);
        }
    }

    // Attempt to store the encoded info
    struct Ex10Result const ex10_result =
        get_ex10_gen2_commands()->encode_gen2_command(
            cmd_spec, &builder.commands_list[index].encoded_command);
    if (ex10_result.error)
    {
        ex10_eprintf("Command encode failed (transaction id = %d)\n",
                     transaction_id);

        return ex10_result;
    }

    // Store the decoded info
    builder.commands_list[index].decoded_command.command = cmd_spec->command;
    int const copy_result =
        ex10_memcpy(builder.commands_list[index].decoded_command.args,
                    sizeof(builder.commands_list[index].decoded_buffer),
                    cmd_spec->args,
                    sizeof(builder.commands_list[index].decoded_buffer));
    if (copy_result != 0)
    {
        return make_ex10_sdk_error(Ex10ModuleGen2Commands, Ex10MemcpyFailed);
    }

    builder.commands_list[index].valid          = true;
    builder.commands_list[index].transaction_id = transaction_id;

    *cmd_index = index;
    return make_ex10_success();
}

static struct Ex10Result read_device_to_local_sequence(void)
{
    struct Ex10Protocol const* protocol = get_ex10_protocol();

    struct Gen2OffsetsFields gen2_offsets[MaxTxCommandCount];
    protocol->read(&gen2_offsets_reg, gen2_offsets);

    struct Gen2LengthsFields gen2_lengths[MaxTxCommandCount];
    protocol->read(&gen2_lengths_reg, gen2_lengths);

    uint8_t tx_buffer[GEN2_TX_BUFFER_REG_LENGTH];
    protocol->read(&gen2_tx_buffer_reg, tx_buffer);

    for (uint8_t idx = 0u; idx < MaxTxCommandCount; idx++)
    {
        // 0 length means the command is not valid
        if (gen2_lengths[idx].length != 0)
        {
            // Mark the command as valid
            builder.commands_list[idx].valid = true;
            // Copy the encoded command and length into the encoded storage
            builder.commands_list[idx].encoded_command.length =
                gen2_lengths[idx].length;
            // grab the byte size for copying over the data
            uint16_t byte_size =
                (gen2_lengths[idx].length - (gen2_lengths[idx].length % 8)) / 8;
            byte_size += (gen2_lengths[idx].length % 8) ? 1 : 0;

            int const copy_result =
                ex10_memcpy(builder.commands_list[idx].encoded_command.data,
                            sizeof(builder.commands_list[idx].encoded_buffer),
                            &tx_buffer[gen2_offsets[idx].offset],
                            byte_size);
            if (copy_result != 0)
            {
                return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                           Ex10MemcpyFailed);
            }

            // Decode the command from the buffer into the decoded storage
            struct Ex10Result const ex10_result =
                get_ex10_gen2_commands()->decode_gen2_command(
                    &builder.commands_list[idx].decoded_command,
                    &builder.commands_list[idx].encoded_command);

            if (ex10_result.error)
            {
                ex10_eprintf(
                    "Command number index=%zd, offset=%d, length=%d has an "
                    "invalid command type.\n",
                    idx,
                    gen2_offsets[idx].offset,
                    gen2_lengths[idx].length);
                return make_ex10_sdk_error(Ex10ModuleGen2Commands,
                                           Ex10ErrorGen2CommandDecode);
            }
        }
        else
        {
            builder.commands_list[idx].valid = false;
        }
    }

    return make_ex10_success();
}

static void print_local_sequence(void)
{
    for (uint8_t idx = 0u; idx < MaxTxCommandCount; idx++)
    {
        // 0 length means the command is not valid
        if (builder.commands_list[idx].valid)
        {
            ex10_printf("Command of length %zd\n",
                        builder.commands_list[idx].encoded_command.length);
            ex10_printf("Raw data: ");
            for (size_t buff_idx = 0;
                 buff_idx < builder.commands_list[idx].encoded_command.length;
                 buff_idx++)
            {
                ex10_printf(
                    "%d, ",
                    builder.commands_list[idx].encoded_command.data[buff_idx]);
            }
            ex10_printf("\n");
            // Add your own further debug based on need
            ex10_printf("Command type is: %d\n",
                        builder.commands_list[idx].decoded_command.command);
        }
    }
}

static void dump_control_registers(void)
{
    struct Ex10Protocol const* protocol = get_ex10_protocol();

    uint8_t gen2_tx_buffer[GEN2_TX_BUFFER_REG_LENGTH];
    protocol->read(&gen2_tx_buffer_reg, gen2_tx_buffer);
    struct Gen2AccessEnableFields access_enable;
    protocol->read(&gen2_access_enable_reg, &access_enable);
    ex10_eputs("gen2_tx_buffer: ");
    for (size_t i = 0; i < GEN2_TX_BUFFER_REG_LENGTH; i++)
    {
        ex10_eputs("%02x ", gen2_tx_buffer[i]);
    }
    ex10_eputs("\n");

    struct Gen2OffsetsFields gen2_offsets[GEN2_OFFSETS_REG_ENTRIES];
    protocol->read(&gen2_offsets_reg, gen2_offsets);

    struct Gen2LengthsFields gen2_lengths[GEN2_LENGTHS_REG_ENTRIES];
    protocol->read(&gen2_lengths_reg, gen2_lengths);

    struct Gen2TxnControlsFields
        gen2_txn_controls[GEN2_TXN_CONTROLS_REG_ENTRIES];
    protocol->read(&gen2_txn_controls_reg, gen2_txn_controls);

    for (size_t i = 0; i < GEN2_OFFSETS_REG_ENTRIES; i++)
    {
        ex10_eputs("gen2_offsets[%zu]: %d\n", i, gen2_offsets[i].offset);
    }

    for (size_t i = 0; i < GEN2_LENGTHS_REG_ENTRIES; i++)
    {
        ex10_eputs("gen2_lengths[%zu]: %d\n", i, gen2_lengths[i].length);
    }

    for (size_t i = 0; i < GEN2_TXN_CONTROLS_REG_ENTRIES; i++)
    {
        ex10_eputs("gen2_txn_controls[%zu]:  ", i);
        ex10_eputs("response_type=%d, ", gen2_txn_controls[i].response_type);
        ex10_eputs("has_header_bit=%d ,", gen2_txn_controls[i].has_header_bit);
        ex10_eputs("use_cover_code=%d, ", gen2_txn_controls[i].use_cover_code);
        ex10_eputs("append_handle=%d, ", gen2_txn_controls[i].append_handle);
        ex10_eputs("append_crc16=%d, ", gen2_txn_controls[i].append_crc16);
        ex10_eputs("is_kill_command=%d, ",
                   gen2_txn_controls[i].is_kill_command);
        ex10_eputs("rx_length=%d\n", gen2_txn_controls[i].rx_length);
    }
}

static struct TxCommandInfo* get_local_sequence(void)
{
    return builder.commands_list;
}

struct Ex10Gen2TxCommandManager const* get_ex10_gen2_tx_command_manager(void)
{
    static struct Ex10Gen2TxCommandManager g2tcm_instance = {
        .clear_local_sequence            = clear_local_sequence,
        .clear_command_in_local_sequence = clear_command_in_local_sequence,
        .clear_sequence                  = clear_sequence,
        .init                            = buffer_builder_init,
        .write_sequence                  = write_sequence,
        .write_select_enables            = write_select_enables,
        .write_halted_enables            = write_halted_enables,
        .write_auto_access_enables       = write_auto_access_enables,
        .append_encoded_command          = append_encoded_command,
        .encode_and_append_command       = encode_and_append_command,
        .read_device_to_local_sequence   = read_device_to_local_sequence,
        .print_local_sequence            = print_local_sequence,
        .dump_control_registers          = dump_control_registers,
        .get_local_sequence              = get_local_sequence,
    };

    return &g2tcm_instance;
}
