/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2020 - 2022 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ex10_api/gpio_interface.h"
#include "ex10_api/host_interface.h"
#include "ex10_api/uart_interface.h"

struct Ex10DriverList
{
    struct Ex10GpioInterface gpio_if;
    struct HostInterface     host_if;
    struct UartInterface     uart_if;
};

struct Ex10DriverList* get_ex10_board_driver_list(void);

#ifdef __cplusplus
}
#endif
