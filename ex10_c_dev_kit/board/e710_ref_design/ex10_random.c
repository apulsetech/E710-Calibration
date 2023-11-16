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

#include "board/ex10_random.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>


static void setup_random(void)
{
    // Use the time of day as the seed value
    srand((unsigned int)time(NULL));
}

static int get_random(void)
{
    // this assumes the setup_rand() has been called to
    // initialize the pseudo random number generator
    return rand();
}

static struct Ex10Random ex10_random = {.setup_random = setup_random,
                                        .get_random   = get_random};

struct Ex10Random* get_ex10_random(void)
{
    return &ex10_random;
}
