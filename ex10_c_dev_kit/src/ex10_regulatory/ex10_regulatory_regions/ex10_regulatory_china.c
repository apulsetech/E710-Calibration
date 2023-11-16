/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2020 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#include "ex10_regulatory/ex10_regulatory_region.h"

#include <stdlib.h>
#include <string.h>

static uint16_t const          china_usable_channels[] = {3, 7, 11, 15};
static const struct Ex10Region region                  = {
    .region_id = REGION_CHINA,
    .regulatory_timers =
        {
            .nominal_ms          = 1800,
            .extended_ms         = 1980,
            .regulatory_ms       = 2000,
            .off_same_channel_ms = 0,
        },
    .regulatory_channels =
        {
            .start_freq_khz = 920125,
            .spacing_khz    = 250,
            .count          = 4,
            .usable         = china_usable_channels,
            .usable_count   = sizeof(china_usable_channels) /
                            sizeof(china_usable_channels[0u]),
            .random_hop = false,
        },
    .pll_divider    = 48,
    .rf_filter      = UPPER_BAND,
    .max_power_cdbm = 3000,
};

static const struct Ex10Region* region_ptr = &region;

static void set_region(struct Ex10Region const* region_to_use)
{
    region_ptr = (region_to_use == NULL) ? &region : region_to_use;
}

static struct Ex10Region const* get_region(void)
{
    return region_ptr;
}

static void get_regulatory_timers(uint16_t                     channel,
                                  uint32_t                     time_ms,
                                  struct Ex10RegulatoryTimers* timers)
{
    (void)channel;
    (void)time_ms;
    *timers = region_ptr->regulatory_timers;
}

static void regulatory_timer_set_start(uint16_t channel, uint32_t time_ms)
{
    (void)channel;
    (void)time_ms;
}

static void regulatory_timer_set_end(uint16_t channel, uint32_t time_ms)
{
    (void)channel;
    (void)time_ms;
}

static void regulatory_timer_clear(void) {}

static struct Ex10RegionRegulatory const ex10_default_regulatory = {
    .set_region                 = set_region,
    .get_region                 = get_region,
    .get_regulatory_timers      = get_regulatory_timers,
    .regulatory_timer_set_start = regulatory_timer_set_start,
    .regulatory_timer_set_end   = regulatory_timer_set_end,
    .regulatory_timer_clear     = regulatory_timer_clear,
};

struct Ex10RegionRegulatory const* get_ex10_china_regulatory(void)
{
    return &ex10_default_regulatory;
}