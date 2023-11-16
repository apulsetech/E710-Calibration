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

#include "ex10_api/gen2_commands.h"

/**
 * Get the ASCII null terminated string associated with Gen2Reply error.
 *
 * @param error_code   The Gen2Reply error_code value.
 * @return char const* The associated string for the TagErrorCode.
 */
char const* get_ex10_gen2_error_string(enum TagErrorCode error_code);

/**
 * Get the ASCII null terminated string associated with Gen2Transaction status.
 *
 * @param transaction_status The Gen2Reply transaction_status value.
 * @return char const*  The associated string for the Gen2TransactionStatus.
 */
char const* get_ex10_gen2_transaction_status_string(
    enum Gen2TransactionStatus transaction_status);

/**
 * Get the ASCII null terminated string associated with Gen2Command type.
 *
 * @param gen2_command The Gen2Reply reply value.
 * @return char const* The associated string for the Gen2Command.
 */
char const* get_ex10_gen2_command_string(enum Gen2Command gen2_command);
