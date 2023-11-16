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

#pragma once

#include <stdlib.h>

#include "ex10_api/ex10_inventory.h"
#include "ex10_api/gen2_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Ex10SelectCommands
{
    /**
     * Set a "select command action" into the Tx Command Buffer.
     * For all tags in the field of view, sets their SL and behavior: assert,
     * deassert, or do nothing.
     *
     * @param select_target Set the target of the select command as defined in
     *                      the Gen2 specification Table 6-29.
     * @note                This is not the inventory flag/session "target"
     *                      A or B as is often referred to in the source
     *                      and documentation.
     * @param action        The behavior of the tag in which matching or
     *                      non-matching tags should assert/deassert/do nothing.
     *                      Action000: match:SL=1           non-match:SL=0
     *                      Action001: match:SL=1           non-match:do nothing
     *                      Action010: match:do nothing     non-match:SL=0
     *                      Action011: match:SL=!           non-match:do nothing
     *                      Action100: match:SL=0           non-match:SL=1
     *                      Action101: match:SL=0           non-match:do nothing
     *                      Action110: match:do nothing     non-match:SL=1
     *                      Action111: match:do nothing     non-match:SL=!
     * @param memory_bank   The type of the memory bank for the select_mask.
     *                      EPC, TID, User
     * @param bit_pointer   The starting bit address form the memory bank for
     *                      the select mask.
     * @param select_mask   The mask of the select command.
     * @param truncate      Specify whether the tag backscatter should be
     *                      truncated.
     *                      0: disable truncation, 1: enable truncation
     *
     * @return ssize_t The Gen2 command buffer index into which the last select
     *                 operation was written. In this case 2 select commands are
     *                 written, so the expected return value is 1; indicating
     *                 that select commands were written into slots 0, 1.
     * @retval -1      Indicates that the operation failed.
     */
    ssize_t (*set_select_command)(enum SelectTarget     select_target,
                                  enum SelectAction     action,
                                  enum SelectMemoryBank memory_bank,
                                  uint32_t              bit_pointer,
                                  struct BitSpan        select_mask,
                                  bool                  truncate);
    /**
     * Set a "session select command" into the Tx Command Buffer.
     * For all tags in the field of view, sets their inventoried flag for a
     * specific session to either A or B state.
     *
     * @details
     * From Gen2 spec, release 2.1, page 75:
     * For MemBank != 2 select match is determined by pointer, length match.
     * If Length is zero then the Tag is matching, unless Pointer references a
     * memory location that does not exist, or Truncate=1 and Pointer is outside
     * the EPC specified in the length field in the StoredPC.
     *
     * @param target   Inventory tags with their inventory flag matching this
     *                 value;
     *                 0: Target A flag set, 1: Target B inventory flag set
     * @param session  The session to use when inventorying tags. [0 ... 3].
     *
     * @return ssize_t The Gen2 command buffer index into which the last select
     *                 operation was written. In this case 2 select commands are
     *                 written, so the expected return value is 1; indicating
     *                 that select commands were written into slots 0, 1.
     * @retval -1      Indicates that the operation failed.
     */
    ssize_t (*set_select_session_command)(uint8_t           target,
                                          enum SelectTarget session);

    /**
     * Enable one, and only one, of the select commands previously written to
     * the Tx Command Buffer.
     *
     * @param select_command_index The Tx Command Buffer index to enable.
     *
     * @return int If positive, returns the Tx command index that was
     *                 successfully enabled. If negative, there was a problem
     *                 and no commands were enabled.
     */
    ssize_t (*enable_select_command)(size_t select_command_index);
};

struct Ex10SelectCommands const* get_ex10_select_commands(void);

#ifdef __cplusplus
}
#endif
