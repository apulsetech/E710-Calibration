/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2022 Impinj, Inc. All rights reserved.                      *
 *                                                                           *
 *****************************************************************************/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ex10_api/ex10_regulatory.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @struct Ex10DefaultRegionNames
 * An interface to map string names of regions to an enum.
 */
struct Ex10DefaultRegionNames
{
    /**
     * Given a string return the Region enum.
     *
     * @param region_name A string that matches a name in the region_table[]
     *                    instantiation found in regions.c.
     *
     * @return enum Ex10RegionId An enum value of the region as defined in
     * ex10_regulatory.h
     */
    enum Ex10RegionId (*get_region_id)(char const* region_name);
};

struct Ex10DefaultRegionNames const* get_ex10_default_region_names(void);

#ifdef __cplusplus
}
#endif
