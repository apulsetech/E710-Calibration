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

#include <stdint.h>

#include "board/ex10_osal.h"

#include "utils/ex10_select_commands.h"

#include "ex10_api/ex10_print.h"
#include "ex10_api/gen2_commands.h"
#include "ex10_api/gen2_tx_command_manager.h"

static void print_gen2_cmd_mgr_error(struct Ex10Result const* ex10_result,
                                     const size_t             cmd_index)
{
    if (ex10_result->error)
    {
        ex10_ex_eputs("Ex10Result: index: %zu, error: %u\n",
                      cmd_index,
                      ex10_result->error);
    }
}

static ssize_t set_select_command(enum SelectTarget     select_target,
                                  enum SelectAction     action,
                                  enum SelectMemoryBank memory_bank,
                                  uint32_t              bit_pointer,
                                  struct BitSpan        select_mask,
                                  bool                  truncate)
{
    struct SelectCommandArgs select_args = {
        .target      = select_target,
        .action      = action,
        .memory_bank = memory_bank,
        .bit_pointer = bit_pointer,
        .bit_count   = (uint8_t)select_mask.length,
        .mask        = &select_mask,
        .truncate    = truncate,
    };

    struct Gen2CommandSpec select_cmd = {
        .command = Gen2Select,
        .args    = &select_args,
    };

    struct Ex10Gen2TxCommandManager const* g2tcm =
        get_ex10_gen2_tx_command_manager();

    size_t                  cmd_index = 0;
    struct Ex10Result const gen2_encode_result =
        g2tcm->encode_and_append_command(&select_cmd, 0, &cmd_index);
    if (gen2_encode_result.error)
    {
        ex10_ex_eprintf("g2tcm->encode_and_append_command failed:\n");
        print_gen2_cmd_mgr_error(&gen2_encode_result, cmd_index);
        return -1;
    }

    // Write the buffer to the Ex10.
    struct Ex10Result const gen2_write_result = g2tcm->write_sequence();
    if (gen2_write_result.error)
    {
        ex10_ex_eprintf("g2tcm->write_sequence failed:\n");
        print_gen2_cmd_mgr_error(&gen2_write_result, cmd_index);
        return -1;
    }

    return (ssize_t)cmd_index;
}

static ssize_t set_select_session_command(uint8_t           target,
                                          enum SelectTarget session)
{
    // Action000: Inventoried matching tags -> group A
    // Action100: Inventoried matching tags -> group B
    enum SelectAction const action =
        (target == target_A) ? Action000 : Action100;

    // memory_bank must not be value SelectTID (2); anything else is ok.
    enum SelectMemoryBank const memory_bank = SelectEPC;

    // A valid zero length mask will match all.
    uint32_t const bit_pointer = 0u;

    // Local variables are OK. They are not used once the Ex10
    // gen2 command buffer and select enables are written.
    uint8_t        select_mask_buffer[1u] = {0u};
    struct BitSpan select_mask            = {select_mask_buffer, 0u};
    bool const     truncate               = false;

    return set_select_command(
        session, action, memory_bank, bit_pointer, select_mask, truncate);
}

static ssize_t enable_select_command(size_t select_command_index)
{
    if (select_command_index >= MaxTxCommandCount)
    {
        // out-of-bound error
        return -1;
    }

    struct Ex10Gen2TxCommandManager const* g2tcm =
        get_ex10_gen2_tx_command_manager();

    bool select_enables[MaxTxCommandCount];
    ex10_memzero(select_enables, sizeof(select_enables));
    select_enables[select_command_index]        = true;
    size_t                  cmd_index           = 0;
    struct Ex10Result const gen2_enables_result = g2tcm->write_select_enables(
        select_enables, MaxTxCommandCount, &cmd_index);
    if (gen2_enables_result.error)
    {
        return -1;
    }

    return (ssize_t)cmd_index;
}

static struct Ex10SelectCommands const ex10_select_commands = {
    .set_select_command         = set_select_command,
    .set_select_session_command = set_select_session_command,
    .enable_select_command      = enable_select_command,
};

struct Ex10SelectCommands const* get_ex10_select_commands(void)
{
    return &ex10_select_commands;
}
