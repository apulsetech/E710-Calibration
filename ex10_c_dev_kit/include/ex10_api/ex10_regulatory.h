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

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ex10_api/channel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct RfFilter
 * Defines the RfFilter selection for a region.
 */
enum RfFilter
{
    UNDEFINED_BAND = 0,
    LOWER_BAND     = 1,  ///< 865 - 868_MHz
    UPPER_BAND     = 2,  ///< 900 - 930_MHz
};

/**
 * @struct Ex10RegulatoryTimers
 * Defines the channel typical and maximum transmitter dwell times and off
 * times for a region.
 *
 * @note Times are in milliseconds
 */
struct Ex10RegulatoryTimers
{
    /// The typical Tx dwell time during inventory.
    uint16_t nominal_ms;

    /// The maximum expected transmit time when accessing a tag.
    uint16_t extended_ms;

    /// The regulatory limit of transmit time.
    /// If this timer expires the transmitter will be squelched.
    uint16_t regulatory_ms;

    /// The minimum transmitter silent time between transitions when staying
    /// on the same frequency.
    uint16_t off_same_channel_ms;
};

/**
 * @struct Ex10RegulatoryChannels
 * Defines the channels which are usable within a specific region.
 *
 * If usable is NULL then the channels used are calculated by:
 *      start_freq + count * spacing
 *      from 0 to the count number.
 * If the usable field is non-NULL then the region uses channel indices
 * pointed to by the field.
 * The size of this channel array is given by count.
 */
struct Ex10RegulatoryChannels
{
    uint32_t       start_freq_khz;  ///< The starting channel frequency [kHz]
    uint32_t       spacing_khz;     ///< The spacing between channels [kHz]
    channel_size_t count;           ///< The number of channels in the plan.
    channel_index_t const* usable;  ///< Limits the channels within the plan.
    channel_size_t         usable_count;
    bool                   random_hop;
};

enum Ex10RegionId
{
    REGION_NOT_DEFINED = 0,
    REGION_FCC,
    REGION_HK,
    REGION_TAIWAN,
    REGION_ETSI_LOWER,
    REGION_ETSI_UPPER,
    REGION_KOREA,
    REGION_MALAYSIA,
    REGION_CHINA,
    REGION_SOUTH_AFRICA,
    REGION_BRAZIL,
    REGION_THAILAND,
    REGION_SINGAPORE,
    REGION_AUSTRALIA,
    REGION_INDIA,
    REGION_URUGUAY,
    REGION_VIETNAM,
    REGION_ISRAEL,
    REGION_PHILIPPINES,
    REGION_INDONESIA,
    REGION_NEW_ZEALAND,
    REGION_JAPAN2,
    REGION_PERU,
    REGION_RUSSIA,
    REGION_CUSTOM,
};

/**
 * @struct Ex10Region
 * The aggregate regulatory information for a specific region.
 */
struct Ex10Region
{
    enum Ex10RegionId             region_id;
    struct Ex10RegulatoryTimers   regulatory_timers;
    struct Ex10RegulatoryChannels regulatory_channels;
    uint32_t                      pll_divider;  ///< gets truncated to u8
    enum RfFilter                 rf_filter;
    int32_t                       max_power_cdbm;  ///< dBm x 100
};

/**
 * @struct Ex10Regulatory
 * Regions information interface.
 */
struct Ex10Regulatory
{
    /**
     * Sets the region_to_use pointer that get_region() will return.
     * This will substitute an alternate set of regulatory
     * parameters for a specific region.
     *
     * @note The original region parameters are immutable, are not over-written,
     *       and can be restored.
     *
     * @note The lifetime of the region_to_use pointer and its contents must
     *       be maintained for the duration of the region usage.
     *
     * @param region_id      The region_id of interest.
     * @param region_to_use  A pointer to the region information to assign to
     *                       the region_id. If this pointer value is NULL then
     *                       the original region parameters are restored.
     */
    void (*set_region)(enum Ex10RegionId        region_id,
                       struct Ex10Region const* region_to_use);

    /**
     * Returns the region information of the passed region enum.
     * Used to get information about a region regardless if it is the current
     * region.
     *
     * @note If set_region() has been previously called, then the region
     *       pointer previously passed in will be returned.
     *
     * @param region_id The region_id of interest.
     *
     * @return struct Ex10Region const* A pointer to the Region matching the
     * region requested.
     */
    struct Ex10Region const* (*get_region)(enum Ex10RegionId region_id);

    /**
     * Returns the regulatory timers for a given region. These timers will
     * be modified using run time information and time spent on other channels.
     * If further regulatory requirements are needed, they can be added to
     * ex10_regulatory.c or the specific region being used.
     *
     * @param region_id  The region_id of interest.
     * @param channel    The channel index to use for the regulatory timers.
     * @param time_ms    The current time, which can be used to adjust the
     *                   regulatory timers returned based on the previous start
     *                   and end calls.
     * @param [out] timers The timers pointer is set with the timers to use
     *                     for the passed region_id.
     */
    void (*get_regulatory_timers)(enum Ex10RegionId            region_id,
                                  channel_index_t              channel,
                                  uint32_t                     time_ms,
                                  struct Ex10RegulatoryTimers* timers);

    /**
     * Sets the start time for the current channel to be logged and used for
     * regulatory requirements.
     *
     * @param region_id The region_id of interest.
     * @param channel   The channel which we are spending time on.
     * @param time_ms   The current time.
     */
    void (*regulatory_timer_set_start)(enum Ex10RegionId region_id,
                                       channel_index_t   channel,
                                       uint32_t          time_ms);

    /**
     * Sets the end time for the current channel to be logged and used for
     * regulatory requirements.
     *
     * @param region_id The region_id of interest.
     * @param channel   The channel which we are spending time on.
     * @param time_ms   The current time.
     */
    void (*regulatory_timer_set_end)(enum Ex10RegionId region_id,
                                     channel_index_t   channel,
                                     uint32_t          time_ms);

    /**
     * Calculate the frequency in khz from the given region and channel index
     *
     * @param region_id The region to user for channel calculations.
     * @param channel     The channel index to use with the regulatory info of
     *                    the passed channel in calculating the frequency.
     *
     * @return uint16_t The channel index of the passed frequency for the
     * currently set region.
     */
    uint32_t (*calculate_channel_khz)(enum Ex10RegionId region_id,
                                      channel_index_t   channel);

    /**
     * Calculate the channel index in the passed region for a passed frequency.
     *
     * @param region_id     The region to user for channel calculations.
     * @param frequency_khz The frequency in khz to calculate the channel
     *                      index from.
     *
     * @return channel_index_t The channel index of the passed frequency
     *                         for the currently set region.
     */
    channel_index_t (*calculate_channel_index)(enum Ex10RegionId region_id,
                                               uint32_t          frequency_khz);
};

struct Ex10Regulatory const* get_ex10_regulatory(void);

#ifdef __cplusplus
}
#endif
