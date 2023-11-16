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

#include "ex10_api/application_register_field_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

char const* ex10_get_command_string(enum CommandCode);
char const* ex10_get_response_string(enum ResponseCode);
char const* ex10_get_op_id_string(enum OpId op_id);
char const* ex10_get_ops_status_string(enum OpsStatus ops_status);

#ifdef __cplusplus
}
#endif
