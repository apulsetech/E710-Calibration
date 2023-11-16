/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2022 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#include "ex10_regulatory/ex10_default_region_names.h"

#include <stdlib.h>
#include <string.h>

/**
 * @struct Ex10RegionName
 */
struct Ex10RegionName
{
    char const* const       name;
    enum Ex10RegionId const region_id;
};

static struct Ex10RegionName const region_names[] = {
    {
        .name      = "FCC",
        .region_id = REGION_FCC,
    },
    {
        .name      = "HK",
        .region_id = REGION_HK,
    },
    {
        .name      = "TAIWAN",
        .region_id = REGION_TAIWAN,
    },
    {
        .name      = "ETSI_LOWER",
        .region_id = REGION_ETSI_LOWER,
    },
    {
        .name      = "ETSI_UPPER",
        .region_id = REGION_ETSI_UPPER,
    },
    {
        .name      = "KOREA",
        .region_id = REGION_KOREA,
    },
    {
        .name      = "MALAYSIA",
        .region_id = REGION_MALAYSIA,
    },
    {
        .name      = "CHINA",
        .region_id = REGION_CHINA,
    },
    {
        .name      = "SOUTH_AFRICA",
        .region_id = REGION_SOUTH_AFRICA,
    },
    {
        .name      = "BRAZIL",
        .region_id = REGION_BRAZIL,
    },
    {
        .name      = "THAILAND",
        .region_id = REGION_THAILAND,
    },
    {
        .name      = "SINGAPORE",
        .region_id = REGION_SINGAPORE,
    },
    {
        .name      = "AUSTRALIA",
        .region_id = REGION_AUSTRALIA,
    },
    {
        .name      = "INDIA",
        .region_id = REGION_INDIA,
    },
    {
        .name      = "URUGUAY",
        .region_id = REGION_URUGUAY,
    },
    {
        .name      = "VIETNAM",
        .region_id = REGION_VIETNAM,
    },
    {
        .name      = "ISRAEL",
        .region_id = REGION_ISRAEL,
    },
    {
        .name      = "PHILIPPINES",
        .region_id = REGION_PHILIPPINES,
    },
    {
        .name      = "INDONESIA",
        .region_id = REGION_INDONESIA,
    },
    {
        .name      = "NEW_ZEALAND",
        .region_id = REGION_NEW_ZEALAND,
    },
    {
        .name      = "JAPAN2",
        .region_id = REGION_JAPAN2,
    },
    {
        .name      = "PERU",
        .region_id = REGION_PERU,
    },
    {
        .name      = "RUSSIA",
        .region_id = REGION_RUSSIA,
    },
    {
        .name      = "CUSTOM",
        .region_id = REGION_CUSTOM,
    },
};

static struct Ex10RegionName const* const region_names_begin =
    &region_names[0u];
static struct Ex10RegionName const* const region_names_end =
    &region_names[0u] + sizeof(region_names) / sizeof(region_names[0u]);

static enum Ex10RegionId get_region_id(char const* region_name)
{
    for (struct Ex10RegionName const* iter = region_names_begin;
         iter < region_names_end;
         ++iter)
    {
        if (strcmp(iter->name, region_name) == 0)
        {
            return iter->region_id;
        }
    }
    return REGION_NOT_DEFINED;
}

static struct Ex10DefaultRegionNames const ex10_default_region_names = {
    .get_region_id = get_region_id,
};

struct Ex10DefaultRegionNames const* get_ex10_default_region_names(void)
{
    return &ex10_default_region_names;
}