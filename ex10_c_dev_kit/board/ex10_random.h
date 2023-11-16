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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Ex10Random
{
    /**
     * Initializes the system random number generator
     */
    void (*setup_random)(void);

    /**
     * Retrieve a random number
     *
     * @return int  a random number.
     */
    int (*get_random)(void);
};

struct Ex10Random* get_ex10_random(void);

#ifdef __cplusplus
}
#endif
