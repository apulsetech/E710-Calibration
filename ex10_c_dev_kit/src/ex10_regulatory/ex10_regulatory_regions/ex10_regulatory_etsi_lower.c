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

#include "board/ex10_osal.h"

#include "ex10_api/ex10_device_time.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_regulatory/ex10_regulatory_region.h"

// NOTE: These channel time arrays are currently in the c file to be region
// specific. If desired, these can be moved to the regulatory_region.h to use
// across regions and save space.
#define ETSI_UPPER_CHANNELS 4
static uint32_t channel_last_start[ETSI_UPPER_CHANNELS];
static uint32_t channel_last_end[ETSI_UPPER_CHANNELS];
static uint32_t channel_total_time[ETSI_UPPER_CHANNELS];

static channel_index_t const etsi_lower_usable_channels[] = {4, 7, 10, 13};

static const struct Ex10Region region = {
    .region_id = REGION_ETSI_LOWER,
    .regulatory_timers =
        {
            .nominal_ms          = 3800,
            .extended_ms         = 3980,
            .regulatory_ms       = 4000,
            .off_same_channel_ms = 100,
        },
    .regulatory_channels =
        {
            .start_freq_khz = 865100,
            .spacing_khz    = 200,
            .count          = 4,
            .usable         = etsi_lower_usable_channels,
            .usable_count =
                (channel_size_t)ARRAY_SIZE(etsi_lower_usable_channels),
            .random_hop = false,
        },
    .pll_divider    = 60,
    .rf_filter      = LOWER_BAND,
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

static void get_regulatory_timers(channel_index_t              channel,
                                  uint32_t                     time_ms,
                                  struct Ex10RegulatoryTimers* timers)
{
    (void)channel;
    (void)time_ms;
    *timers = region_ptr->regulatory_timers;
}

static void regulatory_timer_set_start(channel_index_t channel,
                                       uint32_t        time_ms)
{
    channel_last_start[channel] = time_ms;

    // If we were not off for the specified off time, then the off time counts
    // towards on time since the device did not give enough time to relinquish
    // control of the channel.
    const uint32_t time_since_off = time_ms - channel_last_end[channel];
    if (time_since_off < region_ptr->regulatory_timers.off_same_channel_ms)
    {
        channel_total_time[channel] += time_since_off;
    }
    else
    {
        channel_total_time[channel] = 0;
    }
}

static void regulatory_timer_set_end(channel_index_t channel, uint32_t time_ms)
{
    channel_last_end[channel]   = time_ms;
    const uint32_t time_elapsed = time_ms - channel_last_start[channel];
    channel_total_time[channel] += time_elapsed;
}

static void regulatory_timer_clear(void)
{
    ex10_memzero(channel_last_start, sizeof(channel_last_start));
    ex10_memzero(channel_last_end, sizeof(channel_last_end));
    ex10_memzero(channel_total_time, sizeof(channel_total_time));
}

static struct Ex10RegionRegulatory const ex10_default_regulatory = {
    .set_region                 = set_region,
    .get_region                 = get_region,
    .get_regulatory_timers      = get_regulatory_timers,
    .regulatory_timer_set_start = regulatory_timer_set_start,
    .regulatory_timer_set_end   = regulatory_timer_set_end,
    .regulatory_timer_clear     = regulatory_timer_clear,
};

struct Ex10RegionRegulatory const* get_ex10_etsi_lower_regulatory(void)
{
    return &ex10_default_regulatory;
}
