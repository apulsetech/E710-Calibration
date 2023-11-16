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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "board/ex10_osal.h"
#include "board/ex10_random.h"

#include "ex10_api/application_registers.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_result.h"
#include "ex10_regulatory/ex10_regulatory_region.h"

static struct Ex10Region const* region = NULL;
// The custom active region is used for test features such as setting a single
// frequency and remain on.
static struct Ex10Region custom_active_region;
static channel_index_t   single_freq_channel[1] = {0};
static uint32_t          tcxo_frequency_khz     = 0;
static channel_index_t   channel_hop_table[MAX_CHANNELS];
static uint32_t          channel_table_khz[MAX_CHANNELS];
static channel_index_t   active_channel_index  = 0;
static channel_size_t    len_channel_hop_table = 0;

/**
 * @note These values must match the values enumerated in the documentation
 * Section 6.5.1 RfSynthesizerControl: Parameters for the RF synthesizer
 * Bits 18:16 RDivider.
 */
static const uint8_t divider_value_to_index[] =
    {24u, 48u, 96u, 192u, 30u, 60u, 120u, 240u};

static struct Ex10RegulatoryTimers const regulatory_timers_disabled = {
    .nominal_ms          = 0u,
    .extended_ms         = 0u,
    .regulatory_ms       = 0u,
    .off_same_channel_ms = 0u};

/**
 * Shuffle a 'deck' of channel_index_t 'cards'.
 *
 * @note This function uses the ex10_random implementation
 * to shuffle the deck.  It is left up to the application
 * to create a random sequence through setup_random().
 *
 * @param deck  An array of channel_index_t nodes.
 * @param count The number of channel_index_t nodes in the 'deck'.
 */
static void shuffle_u16(channel_index_t* deck, channel_size_t count)
{
    for (channel_index_t idx = 0u; idx < count; ++idx)
    {
        int const             random_number = get_ex10_random()->get_random();
        channel_index_t const pick =
            (channel_index_t)(random_number % (count - idx));

        // swap deck[idx] <-> deck[pick]
        channel_index_t const temp = deck[idx];
        deck[idx]                  = deck[pick];
        deck[pick]                 = temp;
    }
}

static struct Ex10Result build_channel_table(
    struct Ex10RegulatoryChannels const* regulatory_channels,
    channel_index_t*                     channel_table,
    channel_size_t*                      channel_size)
{
    if (regulatory_channels == NULL || channel_table == NULL ||
        channel_size == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleRegion, Ex10SdkErrorNullPointer);
    }

    if (sizeof(*channel_table) != sizeof(*regulatory_channels->usable))
    {
        ex10_eprintf("Mismatched usable channels table size\n");
        return make_ex10_sdk_error(Ex10ModuleRegion, Ex10SdkErrorBadParamValue);
    }

    channel_size_t channel_count = 0u;

    if (regulatory_channels->usable_count > 0u)
    {
        if (!regulatory_channels->usable)
        {
            ex10_eprintf("No usable regulatory channels\n");
            return make_ex10_sdk_error(Ex10ModuleRegion,
                                       Ex10SdkErrorBadParamValue);
        }
        int const copy_result = ex10_memcpy(
            channel_table,
            regulatory_channels->usable_count * sizeof(*channel_table),
            regulatory_channels->usable,
            regulatory_channels->usable_count * sizeof(*channel_table));
        if (copy_result != 0)
        {
            *channel_size = 0u;
            return make_ex10_success();
        }

        channel_count = regulatory_channels->usable_count;
    }
    else
    {
        channel_count = regulatory_channels->count;

        for (channel_index_t chn_idx = 0u; chn_idx < channel_count; ++chn_idx)
        {
            channel_table[chn_idx] = chn_idx + 1;
        }
    }

    if (regulatory_channels->random_hop)
    {
        shuffle_u16(channel_table, channel_count);
    }

    *channel_size = channel_count;
    return make_ex10_success();
}

static struct Ex10Result set_region(enum Ex10RegionId region_id,
                                    uint32_t          tcxo_freq_khz)
{
    tcxo_frequency_khz   = tcxo_freq_khz;
    active_channel_index = 0;

    /* Find region */
    region = get_ex10_regulatory()->get_region(region_id);
    if (region->region_id == REGION_NOT_DEFINED)
    {
        ex10_eprintf("No region defined for ID %d\n", region_id);
        return make_ex10_sdk_error(Ex10ModuleRegion, Ex10SdkErrorBadParamValue);
    }

    /* Build channel hop table (kHz) */
    struct Ex10Result ex10_result =
        build_channel_table(&region->regulatory_channels,
                            channel_hop_table,
                            &len_channel_hop_table);
    if (ex10_result.error)
    {
        return ex10_result;
    }
    if (len_channel_hop_table > MAX_CHANNELS)
    {
        ex10_eprintf("Hop table length %d exceeds max %d\n",
                     len_channel_hop_table,
                     MAX_CHANNELS);
        return make_ex10_sdk_error(Ex10ModuleRegion, Ex10SdkErrorBadParamValue);
    }

    for (channel_index_t iter = 0; iter < len_channel_hop_table; iter++)
    {
        channel_table_khz[iter] = get_ex10_regulatory()->calculate_channel_khz(
            region->region_id, channel_hop_table[iter]);
    }

    active_channel_index = 0;

    return make_ex10_success();
}

static enum Ex10RegionId get_region_id(void)
{
    if (region == NULL)
    {
        region = get_ex10_regulatory()->get_region(REGION_NOT_DEFINED);
    }
    return region->region_id;
}

static channel_index_t get_next_index(void)
{
    channel_index_t next_index = active_channel_index + 1;
    if (next_index == len_channel_hop_table)
    {
        next_index = 0;
    }
    return next_index;
}

static void update_active_channel(void)
{
    active_channel_index = get_next_index();
}

static channel_size_t get_channel_table_size(void)
{
    return len_channel_hop_table;
}

static uint32_t get_active_channel_khz(void)
{
    return channel_table_khz[active_channel_index];
}

static uint32_t get_next_channel_khz(void)
{
    channel_index_t const next_channel_index = get_next_index();
    return channel_table_khz[next_channel_index];
}

static channel_index_t get_active_channel_index(void)
{
    return active_channel_index;
}

static struct Ex10Result get_adjacent_channel_khz(
    channel_index_t  channel_index,
    channel_offset_t channel_offset,
    uint32_t*        adjacent_channel_khz)
{
    if (adjacent_channel_khz == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleRegion, Ex10SdkErrorNullPointer);
    }

    // 0 is the default result if the calculated channel out of range
    uint32_t              result_khz      = 0;
    channel_index_t const current_channel = channel_hop_table[channel_index];

    if (region == NULL)
    {
        region = get_ex10_regulatory()->get_region(REGION_NOT_DEFINED);
    }
    if (region->regulatory_channels.usable == NULL)
    {
        // The region's channels are linearly spaced from
        // the start frequency though the number of channels
        channel_offset_t const adjacent_channel =
            (channel_offset_t)(current_channel + channel_offset);

        channel_size_t const count = region->regulatory_channels.count;
        if (adjacent_channel <= (channel_offset_t)count && adjacent_channel > 0)
        {
            result_khz = get_ex10_regulatory()->calculate_channel_khz(
                region->region_id, (channel_index_t)adjacent_channel);
        }
    }
    else
    {
        // The region uses a limited number of channels in the space

        // find the current channel in the usable channels
        uint16_t const* channel_array = region->regulatory_channels.usable;
        int usable_count = (int)region->regulatory_channels.usable_count;
        int iter;
        for (iter = 0; iter < usable_count; iter++)
        {
            if (channel_array[iter] == current_channel)
            {
                break;
            }
        }

        // Check to make sure we found the channel
        if (iter >= usable_count)
        {
            return make_ex10_sdk_error(Ex10ModuleRegion,
                                       Ex10SdkErrorBadParamValue);
        }
        // move to the next channel
        iter += channel_offset;
        if (iter >= 0 && iter < usable_count)
        {
            result_khz = get_ex10_regulatory()->calculate_channel_khz(
                region->region_id, channel_array[iter]);
        }
    }

    *adjacent_channel_khz = result_khz;
    return make_ex10_success();
}

static uint32_t get_channel_spacing(void)
{
    if (region == NULL)
    {
        region = get_ex10_regulatory()->get_region(REGION_NOT_DEFINED);
    }
    return region->regulatory_channels.spacing_khz;
}

static channel_index_t get_next_channel_index(void)
{
    return get_next_index();
}

static channel_index_t get_channel_index(uint32_t frequency_khz)
{
    for (channel_index_t iter = 0; iter < len_channel_hop_table; iter++)
    {
        if (channel_table_khz[iter] == frequency_khz)
        {
            return iter;
        }
    }

    return channel_index_invalid;
}

static struct Ex10Result get_next_channel_regulatory_timers(
    struct Ex10RegulatoryTimers* timers)
{
    if (timers == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleRegion, Ex10SdkErrorNullPointer);
    }

    if (region == NULL)
    {
        region = get_ex10_regulatory()->get_region(REGION_NOT_DEFINED);
    }
    struct TimestampFields time_us;
    get_ex10_protocol()->read(&timestamp_reg, &time_us);

    uint32_t const time_ms = time_us.current_timestamp_us / 1000;
    get_ex10_regulatory()->get_regulatory_timers(
        region->region_id, get_next_index(), time_ms, timers);
    return make_ex10_success();
}

static struct Ex10Result get_regulatory_timers(
    struct Ex10RegulatoryTimers* timers)
{
    if (timers == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleRegion, Ex10SdkErrorNullPointer);
    }

    if (region == NULL)
    {
        region = get_ex10_regulatory()->get_region(REGION_NOT_DEFINED);
    }
    struct TimestampFields time_us;
    get_ex10_protocol()->read(&timestamp_reg, &time_us);

    uint32_t const time_ms = time_us.current_timestamp_us / 1000;
    get_ex10_regulatory()->get_regulatory_timers(
        region->region_id, active_channel_index, time_ms, timers);
    return make_ex10_success();
}

/**
 * Get the PLL R divider from the current table
 * @return uint32_t The R divider value of this region.
 * @retval          Zero if current region has not been set.
 */
static uint32_t get_pll_r_divider(void)
{
    if (region == NULL)
    {
        region = get_ex10_regulatory()->get_region(REGION_NOT_DEFINED);
    }
    return region->pll_divider;
}

/**
 * Calculate n_divider using known TXCO frequency, frequency, and r_divider.
 * @param frequency The LO frequency in kHz.
 * @param r_divider PLL R divider value
 *
 * @return N divider value for this frequency
 */
static uint16_t calculate_n_divider(uint32_t freq_khz, uint32_t r_divider)
{
    return (uint16_t)((4 * freq_khz * r_divider + tcxo_frequency_khz / 2) /
                      tcxo_frequency_khz);
}

/**
 * Translate from R divider values to index in the range [0,7]
 * @param r_divider PLL R divider value
 *
 * @return Index representing the provided R divider value
 */
static uint8_t calculate_r_divider_index(uint32_t r_divider)
{
    for (uint8_t iter = 0; iter < sizeof(divider_value_to_index); iter++)
    {
        if (r_divider == divider_value_to_index[iter])
        {
            return iter;
        }
    }

    ex10_eprintf("Unknown R divider value %u\n", r_divider);

    return BAD_R_DIVIDER_INDEX;
}

static struct Ex10Result get_synthesizer_params(
    uint32_t                  freq_khz,
    struct SynthesizerParams* params)
{
    if (params == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleRegion, Ex10SdkErrorNullPointer);
    }
    uint32_t r_divider = get_pll_r_divider();

    params->freq_khz        = freq_khz;
    params->r_divider_index = calculate_r_divider_index(r_divider);
    if (params->r_divider_index == BAD_R_DIVIDER_INDEX)
    {
        return make_ex10_sdk_error(Ex10ModuleRegion, Ex10SdkErrorBadParamValue);
    }

    params->n_divider = calculate_n_divider(freq_khz, r_divider);
    return make_ex10_success();
}

static struct Ex10Result get_synthesizer_frequency_khz(uint8_t  r_divider_index,
                                                       uint16_t n_divider,
                                                       uint32_t* frequency_khz)
{
    if (frequency_khz == NULL)
    {
        return make_ex10_sdk_error(Ex10ModuleRegion, Ex10SdkErrorNullPointer);
    }

    if (r_divider_index < ARRAY_SIZE(divider_value_to_index))
    {
        // For F tcxo = 24,000 kHz, F lo = 930,000 kHz, Rdiv = 240
        // The expected max N div < 40E3, F lo 930E3 > UIN32_MAX,
        // Use uint64_t for the numerator.
        uint32_t const r_divider   = divider_value_to_index[r_divider_index];
        uint64_t const numerator   = ((uint64_t)tcxo_frequency_khz) * n_divider;
        uint32_t const denominator = 4u * r_divider;
        uint64_t const freq_khz    = numerator / denominator;

        *frequency_khz = (uint32_t)freq_khz;

        return make_ex10_success();
    }

    ex10_eprintf("r_divider_index out of range\n");
    *frequency_khz = 0u;
    return make_ex10_sdk_error(Ex10ModuleRegion, Ex10SdkErrorBadParamValue);
}

static enum RfFilter get_rf_filter(void)
{
    if (region == NULL)
    {
        region = get_ex10_regulatory()->get_region(REGION_NOT_DEFINED);
    }
    return region->rf_filter;
}

static void set_single_frequency(uint32_t frequency_khz)
{
    if (region == NULL)
    {
        region = get_ex10_regulatory()->get_region(REGION_NOT_DEFINED);
    }

    // Grab the channel index from the single frequency
    single_freq_channel[0] = get_ex10_regulatory()->calculate_channel_index(
        region->region_id, frequency_khz);

    // Update the region for the setter
    custom_active_region                            = *region;
    custom_active_region.regulatory_channels.usable = single_freq_channel;
    custom_active_region.regulatory_channels.usable_count =
        sizeof(single_freq_channel) / sizeof(single_freq_channel[0u]);
    custom_active_region.regulatory_channels.count = 1;

    // Set the now single frequency region
    get_ex10_regulatory()->set_region(region->region_id, &custom_active_region);
    // Update the local region to reflect
    set_region(region->region_id, tcxo_frequency_khz);
}

static void disable_regulatory_timers(void)
{
    if (region == NULL)
    {
        region = get_ex10_regulatory()->get_region(REGION_NOT_DEFINED);
    }

    // Update the region for the setter
    custom_active_region                   = *region;
    custom_active_region.regulatory_timers = regulatory_timers_disabled;

    // Set the now no regulatory timer region
    get_ex10_regulatory()->set_region(region->region_id, &custom_active_region);
    // Update the local region to reflect
    set_region(region->region_id, tcxo_frequency_khz);
}

static void reenable_regulatory_timers(void)
{
    if (region == NULL)
    {
        region = get_ex10_regulatory()->get_region(REGION_NOT_DEFINED);
    }

    custom_active_region = *region;

    // Set the region to NULL to get the default timers
    get_ex10_regulatory()->set_region(region->region_id, NULL);
    custom_active_region.regulatory_timers =
        get_ex10_regulatory()->get_region(region->region_id)->regulatory_timers;

    // Set the now no regulatory timer region
    get_ex10_regulatory()->set_region(region->region_id, &custom_active_region);
    // Update the local region to reflect
    set_region(region->region_id, tcxo_frequency_khz);
}

static void regulatory_timer_set_start(uint32_t time_ms)
{
    get_ex10_regulatory()->regulatory_timer_set_start(
        region->region_id, active_channel_index, time_ms);
}

static void regulatory_timer_set_end(uint32_t time_ms)
{
    if (region == NULL)
    {
        region = get_ex10_regulatory()->get_region(REGION_NOT_DEFINED);
    }
    get_ex10_regulatory()->regulatory_timer_set_end(
        region->region_id, active_channel_index, time_ms);
}

static struct Ex10Result update_channel_time_tracking(void)
{
    // To avoid guessing at SDK state, we pull the last ramp up/down times
    // and channels to set here.
    // Note: Both the start and end time are set only if CW is off.
    // If already on, there is no need to update the regulatory timers.
    struct LastTxRampUpTimeMsFields      last_up_ms;
    struct LastTxRampUpLoFreqKhzFields   last_up_khz;
    struct LastTxRampDownTimeMsFields    last_down_ms;
    struct LastTxRampDownLoFreqKhzFields last_down_khz;

    struct RegisterInfo const* const regs[] = {
        &last_tx_ramp_up_time_ms_reg,
        &last_tx_ramp_up_lo_freq_khz_reg,
        &last_tx_ramp_down_time_ms_reg,
        &last_tx_ramp_down_lo_freq_khz_reg,
    };
    void* buffers[] = {
        &last_up_ms,
        &last_up_khz,
        &last_down_ms,
        &last_down_khz,
    };

    struct Ex10Result ex10_result =
        get_ex10_protocol()->read_multiple(regs, buffers, ARRAY_SIZE(regs));
    if (ex10_result.error)
    {
        return ex10_result;
    }

    const channel_index_t up_channel =
        get_channel_index(last_up_khz.lo_freq_khz);
    const channel_index_t down_channel =
        get_channel_index(last_down_khz.lo_freq_khz);

    if (up_channel != channel_index_invalid &&
        down_channel != channel_index_invalid)
    {
        get_ex10_regulatory()->regulatory_timer_set_start(
            region->region_id, up_channel, last_up_ms.time_ms);
        get_ex10_regulatory()->regulatory_timer_set_end(
            region->region_id, down_channel, last_down_ms.time_ms);
    }
    return make_ex10_success();
}

static struct Ex10ActiveRegion const ex10_active_region = {
    .set_region                         = set_region,
    .get_region_id                      = get_region_id,
    .update_active_channel              = update_active_channel,
    .get_channel_table_size             = get_channel_table_size,
    .get_active_channel_khz             = get_active_channel_khz,
    .get_next_channel_khz               = get_next_channel_khz,
    .get_active_channel_index           = get_active_channel_index,
    .get_next_channel_index             = get_next_channel_index,
    .get_channel_index                  = get_channel_index,
    .get_adjacent_channel_khz           = get_adjacent_channel_khz,
    .get_channel_spacing                = get_channel_spacing,
    .get_next_channel_regulatory_timers = get_next_channel_regulatory_timers,
    .get_regulatory_timers              = get_regulatory_timers,
    .get_synthesizer_params             = get_synthesizer_params,
    .get_synthesizer_frequency_khz      = get_synthesizer_frequency_khz,
    .get_rf_filter                      = get_rf_filter,
    .get_pll_r_divider                  = get_pll_r_divider,
    .calculate_n_divider                = calculate_n_divider,
    .calculate_r_divider_index          = calculate_r_divider_index,
    .build_channel_table                = build_channel_table,
    .set_single_frequency               = set_single_frequency,
    .disable_regulatory_timers          = disable_regulatory_timers,
    .reenable_regulatory_timers         = reenable_regulatory_timers,
    .regulatory_timer_set_start         = regulatory_timer_set_start,
    .regulatory_timer_set_end           = regulatory_timer_set_end,
    .update_channel_time_tracking       = update_channel_time_tracking,
};

struct Ex10ActiveRegion const* get_ex10_active_region(void)
{
    return &ex10_active_region;
}
