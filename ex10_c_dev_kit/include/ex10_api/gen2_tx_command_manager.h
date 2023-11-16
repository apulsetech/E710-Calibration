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

#pragma once

#include "ex10_api/application_register_field_enums.h"
#include "ex10_api/gen2_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MaxTxCommandCount MaxNumGen2Commands

#define TxCommandDecodeBufferSize (40u)
#define TxCommandEncodeBufferSize (40u)

struct TxCommandInfo
{
    uint8_t                decoded_buffer[TxCommandDecodeBufferSize];
    uint8_t                encoded_buffer[TxCommandEncodeBufferSize];
    struct BitSpan         encoded_command;
    struct Gen2CommandSpec decoded_command;
    bool                   valid;
    uint8_t                transaction_id;
};

/**
 * @struct Ex10Gen2TxCommandManager
 * The Ex10 Gen2TxCommandManager interface.
 */
struct Ex10Gen2TxCommandManager
{
    /**
     * Clears validity and enable markers for every command
     */
    void (*clear_local_sequence)(void);

    /**
     * Clears the validity markers for the passed index.
     *
     * @param clear_idx The index to clear
     * @param cmd_index The cleared index
     * @return          Returns an instance of Ex10Result
     * which informs the user if any errors occurred while clearing the index.
     * If the user attempts to clear an index which is out of bounds, it
     * will return an error immediately.
     */
    struct Ex10Result (*clear_command_in_local_sequence)(uint8_t clear_idx,
                                                         size_t* cmd_index);

    /**
     * Clears the registers for command lengths and enables to ensure no
     * commands are valid on the device. Does not touch the command info.
     */
    void (*clear_sequence)(void);

    /**
     * Invalidate all commands and enables to start the buffer fresh.
     * Also sets up the encode and decode data to utilize the backing buffers so
     * local copies of commands can be used.
     */
    void (*init)(void);

    /**
     * Writes all valid commands into the device Gen2TxBuffer register and sets
     * up the required companion registers.
     *
     * @return Returns an instance of Ex10Result which informs
     * the user if any errors occurred while adding the command.
     */
    struct Ex10Result (*write_sequence)(void);

    /**
     * Set the Gen2SelectEnable register to use within the select op based on
     * the input boolean array. Any command enabled through this function will
     * be sent during the send select op, regardless of if it is a proper select
     * command. These enables are written directly to the device, regardless of
     * if you have updated the Gen2Buffer register.
     *
     * @param select_enables An array of booleans which signifies which indexes
     * within the command buffer to enable.
     * The sizing of this enables array is MaxTxCommandCount, signifying that
     * each index in your enables array matches the corresponding sequence
     * index.
     *
     * @param size The number of entries in the passed boolean array. This is
     * the number of entries to utilize in setting the enable register. The
     * size must be <= to the max command sizing of 10. If the size is less,
     * the latter enable indicies will be set to false.
     *
     * @param [out] cmd_index The index in the command array for the last
     * enabled bit.
     *
     * @return Returns any errors which may have occurred. If the user attempts
     * to enable an index which does not have a valid command, the function will
     * return immediately with an error.
     * If the user enables an index containing an access command, the error will
     * be set, but the function will finish normally.
     *
     * @details If the sequence contains 10 TxCommandInfo structures and the
     * first three are valid, you could pass in a boolean array with indices
     * 1 and 3 set to true to enable the selects at 1 and 3.
     *
     * @note If you attempt to enable a non-select command through this
     * function, the index will be ignored.
     */
    struct Ex10Result (*write_select_enables)(bool const* select_enables,
                                              uint8_t     size,
                                              size_t*     cmd_index);

    /**
     * Set the Gen2AccessEnable register to use within inventory based on the
     * input boolean array. Any command enabled  through this function can be
     * sent to a tag during the inventory halted state. These enables are
     * written directly to the device, regardless of if you have updated the
     * Gen2Buffer register.
     *
     * @param access_enables An array of booleans which signifies which indices
     * within the command buffer to enable.
     * The sizing of this enables array is MaxTxCommandCount, signifying that
     * each index in your enables array matches the corresponding sequence
     * index.
     *
     * @param size The number of entries in the passed boolean array. This is
     * the number of entries to utilize in setting the enable register. The
     * size must be <= to the max command sizing of 10. If the size is less,
     * the latter enable indices will be set to false.
     *
     * @param cmd_index The index in the command array for the last enabled bit.
     *
     * @return Returns any errors which may have occurred. If the user attempts
     * to enable an index which does not have a valid command, the function will
     * return immediately with an error.
     * If the user enables an index containing a select command, the error will
     * be set, but the function will finish normally.
     *
     * @details If the command buffer contains 10 TxCommandInfo structures, and
     * the first three are valid, you could pass in a boolean array with indices
     * 1 and 3 set to true to enable commands 1 and 3.
     *
     * @note If you attempt to enable a-select command through this function,
     * the index will be ignored.
     *
     * @note The enabled indices may or may not overlap with the other enables
     * (the same command may be used for auto halt as well).
     */
    struct Ex10Result (*write_halted_enables)(bool const* access_enables,
                                              uint8_t     size,
                                              size_t*     cmd_index);

    /**
     * Set the Gen2AutoAccessEnable register to use within inventory based on
     * the input boolean array. Any command enabled  through this function can
     * be sent to a tag using the auto access functionality. These enables are
     * written directly to the device, regardless of if you have updated the
     * Gen2Buffer register.
     *
     * @param auto_access_enables An array of booleans which signifies which
     * indices within the command buffer to enable.
     * The sizing of this enables array is MaxTxCommandCount, signifying that
     * each index in your enables array matches the corresponding sequence
     * index.
     *
     * @param size The number of entries in the passed boolean array. This is
     * the number of entries to utilize in setting the enable register. The
     * size must be <= to the max command sizing of 10. If the size is less,
     * the latter enable indices will be set to false.
     *
     * @details If the command buffer contains 10 TxCommandInfo structures, and
     * the first three are valid, you could pass in a boolean array with indices
     * 1 and 3 set to true to enable commands 1 and 3.
     *
     * @param cmd_index The index in the command array for the last enabled bit.
     *
     * @return Returns any errors which may have occurred. If the user attempts
     * to enable an index which does not have a valid command, the function will
     * return immediately with an error.
     * If the user enables an index containing a select command, the error will
     * be set, but the function will finish normally.
     *
     * @note If you attempt to enable a-select command through this function,
     * the index will be ignored.
     *
     * @note The enabled indices may or may not overlap with the other enables
     * (the same command may be used with halted).
     */
    struct Ex10Result (*write_auto_access_enables)(
        bool const* auto_access_enables,
        uint8_t     size,
        size_t*     cmd_index);

    /**
     * Takes in an encoded command and appends it to the first free index in the
     * local buffer.
     *
     * @param tx_buffer A byte span with data of the encoded command
     *
     * @param transaction_id An Id to associate with the command. This ID is
     * sent back in the event fifo when the command is sent, but has no other
     * effect on the command sending itself.
     *
     * @param cmd_index The index in the command array where the encoded command
     * was added.
     *
     * @return Returns an instance of Ex10Result which informs
     * the user if any errors occurred while adding the command.
     *
     * @note There are no issues in ordering of id nor with repeating the same
     * transaction id.
     * @note The ID has no bearing on the order in which a command is sent.
     */
    struct Ex10Result (*append_encoded_command)(const struct BitSpan* tx_buffer,
                                                uint8_t transaction_id,
                                                size_t* cmd_index);

    /**
     * Takes in a Gen2CommandSpec, encodes it, and appends it to the first free
     * index in the local buffer.
     *
     * @param cmd_spec A specification for the gen2 command.
     *
     * @param transaction_id An Id to associate with the command. This ID is
     * sent back in the event fifo when the command is sent, but has no other
     * effect on the command sending itself.
     *
     * @param cmd_index The index in the command array where the encoded command
     * was added.
     *
     * @return Returns an instance of Ex10Result which informs
     * the user if any errors occurred while adding the command.
     *
     * @note There are no issues in ordering of id nor with repeating the same
     * transaction id.
     * @note The ID has no bearing on the order in which a command is sent.
     */
    struct Ex10Result (*encode_and_append_command)(
        struct Gen2CommandSpec* cmd_spec,
        uint8_t                 transaction_id,
        size_t*                 cmd_index);

    /**
     * Reads out the gen2 buffer from the device into the local storage.
     * @note This overwrites any previous commands in the local command
     * sequence.
     */
    struct Ex10Result (*read_device_to_local_sequence)(void);

    /**
     * Prints out the basics of the local command sequence for use in debugging.
     */
    void (*print_local_sequence)(void);

    /**
     * Reads all the registers associated with Gen2 commands from the device and
     * prints them out.
     */
    void (*dump_control_registers)(void);

    /**
     * Retrieves a pointer to the structure containing all commands. This allows
     * for debug and modifications by an expert user.
     * @note This will print off all commands in the buffer, even if not
     * enabled.
     *
     * @return Returns a pointer to the local command sequence.
     */
    struct TxCommandInfo* (*get_local_sequence)(void);
};

struct Ex10Gen2TxCommandManager const* get_ex10_gen2_tx_command_manager(void);

#ifdef __cplusplus
}
#endif
