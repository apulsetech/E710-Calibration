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

#include "ex10_api/ex10_regulatory.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct Ex10RegionRegulatory
 * Interface to use regulatory information from all regions.
 */
struct Ex10RegionRegulatory
{
    /**
     * Sets the current region information. This allows the user to override the
     * region with their own custom values.
     *
     * @param region_to_use The region info to assign to the regulatory region.
     */
    void (*set_region)(struct Ex10Region const* region_to_use);

    /**
     * Returns the currently set region info for the given region. This is const
     * such that it can only be changed through the set_region function.
     *
     * @return Info for the given region.
     */
    struct Ex10Region const* (*get_region)(void);

    /**
     * Returns regulatory info for the given channel and region. This differs
     * from the basic regulatory info of the region in that the chosen
     * regulatory file can contain logic to alter the regulatory times used for
     * the chosen channel. This logic can be changed in any of the given regions
     * files under the board directory.
     * @note: for info on customizing this logic, look at regulatory_default.c
     *
     * @param channel The channel index to use for the regulatory timers.
     * @param time_ms The current time, which can be used to adjust the
     *                regulatory timers returned based on the previous start and
     *                end calls.
     * @param [out] channel The timers pointer where the timers will be placed.
     */
    void (*get_regulatory_timers)(channel_index_t              channel,
                                  uint32_t                     time_ms,
                                  struct Ex10RegulatoryTimers* timers);

    /**
     * Sets the start timer for the current channel. Further logic depends on
     * the region specifics.
     *
     * @param channel The channel index to use for the time in.
     * @param time_ms The time to set as the start time for regulatory.
     */
    void (*regulatory_timer_set_start)(channel_index_t channel,
                                       uint32_t        time_ms);

    /**
     * Sets the stop timer for the current channel. Further logic depends on the
     * region specifics.
     *
     * @param channel The channel index to use for the time in.
     * @param time_ms The time to set as the end time for regulatory.
     */
    void (*regulatory_timer_set_end)(channel_index_t channel, uint32_t time_ms);

    /**
     * Clears any set timers used in the regulatory region.
     */
    void (*regulatory_timer_clear)(void);
};

struct Ex10RegionRegulatory const* get_ex10_fcc_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_australia_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_brazil_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_china_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_etsi_lower_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_etsi_upper_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_hk_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_india_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_indonesia_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_israel_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_japan2_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_korea_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_malaysia_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_new_zealand_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_peru_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_philippines_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_russia_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_singapore_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_south_africa_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_taiwan_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_thailand_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_uruguay_regulatory(void);
struct Ex10RegionRegulatory const* get_ex10_vietnam_regulatory(void);

struct Ex10RegionRegulatory const* get_ex10_error_regulatory(void);

#ifdef __cplusplus
}
#endif
