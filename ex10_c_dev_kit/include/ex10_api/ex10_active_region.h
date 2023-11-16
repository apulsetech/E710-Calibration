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

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/ex10_result.h"

#ifdef __cplusplus
extern "C" {
#endif

// The maximum number of channels to use for the active region.
#define MAX_CHANNELS ((channel_size_t)50u)

// The return value when the R divider index calulation fails.
#define BAD_R_DIVIDER_INDEX (0xffu)

/**
 * @struct SynthesizerParams
 * The synthesizer parameters required to set the LO frequency.
 */
struct SynthesizerParams
{
    uint32_t freq_khz;
    uint8_t  r_divider_index;
    uint16_t n_divider;
};

/**
 * @struct Ex10ActiveRegion
 * The region programming interface.
 */
struct Ex10ActiveRegion
{
    /**
     * Sets the active region to the passed region.
     *
     * Load the region parameters from the regions table and builds the region's
     * frequency channels table.
     * @see ex10_api/src/regions.c
     * @param region_id Requested regulatory region enum
     * @param tcxo_freq_khz Crystal oscillator frequency used with the Ex10
     * device.
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_region)(enum Ex10RegionId region_id,
                                    uint32_t          tcxo_freq_khz);

    /**
     * Gets the active region enum.
     *
     * @return enum Ex10RegionId The enum of the region which has been set
     *                           through set_region.
     *
     * @retval REGION_NOT_DEFINED If current region has not been set.
     */
    enum Ex10RegionId (*get_region_id)(void);

    /**
     * Determine the next channel to use, from the active region hopping table,
     * and store it as the channel to use when next ramping up.
     * This channel will be used for the next ramp up event.
     */
    void (*update_active_channel)(void);

    /**
     * Get the number of channels within the region channel table.
     *
     * @note The initialize_region() function must have been called,
     *       otherwise this function will return zero.
     *
     * @return channel_size_t The number of channels in the region's channel
     *                        table.
     */
    channel_size_t (*get_channel_table_size)(void);

    /**
     * Gets the frequency used by the current CW ramp cycle
     * @return current channel frequency (kHz)
     */
    uint32_t (*get_active_channel_khz)(void);

    /**
     * Gets the frequency used by the next channel in the region table
     * This is used as a look ahead to calculate the parameters to pass
     * to the cw_on() function for ramping up on the next channel.
     * @return current channel frequency (kHz)
     */
    uint32_t (*get_next_channel_khz)(void);

    /**
     * This function looks at the region channel table and
     * determines the current channel and returns the appropriate
     * frequency of what the adjacent channel is.  If the channel
     * requested is off the end of the channel list or is a gap larger
     * than the specified distance, a frequency of 0 is returned.
     *
     * @param channel_index The index in the hop list to use as the channel
     *
     * @param channel_offset The number of channels to move up or down
     *
     * @param [out] uint32_t The frequency in kHz of the requested channel.
     *                       Zero if current region has not been set.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*get_adjacent_channel_khz)(
        channel_index_t  channel_index,
        channel_offset_t channel_offset,
        uint32_t*        adjacent_channel_khz);

    /**
     * This function gets the channel spacing, in kHz, for the region.
     *
     * @return uint32_t The channel spacing in Khz.
     * @retval          Zero if current region has not been set.
     */
    uint32_t (*get_channel_spacing)(void);

    /**
     * Gets the current channel index in the region table used
     * by the current CW ramp cycle
     */
    channel_index_t (*get_active_channel_index)(void);

    /**
     * Gets the next channel index in the region table that will
     * be used when the it ramps up next
     */
    channel_index_t (*get_next_channel_index)(void);

    /**
     * Look up what the channel index is for a given frequency.
     *
     * @param frequency_khz target channel frequency in khz
     *
     * @return The channel index of the specified frequency.
     * @retval -1 If the frequency is not contained in the channel table.
     */
    channel_index_t (*get_channel_index)(uint32_t frequency_khz);

    /**
     * Returns the regulatory dwell times for this region on the next
     * channel.
     * @param [out] timers A struct to hold returned timer information.
     */
    struct Ex10Result (*get_next_channel_regulatory_timers)(
        struct Ex10RegulatoryTimers* timers);

    /**
     * Returns the regulatory dwell times for this region on the current
     * channel.
     * @param [out] timers A struct to hold returned timer information.
     */
    struct Ex10Result (*get_regulatory_timers)(
        struct Ex10RegulatoryTimers* timers);

    /**
     * Generates the required synthesizer parameters for the next channel
     * in the active region.
     *
     * @param freq_khz           The frequency in kHz for calculating the PLL
     *                           synthesizer parameters.
     * @param [out] synth_params struct to hold returned synthesizer parameters.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*get_synthesizer_params)(
        uint32_t                  freq_khz,
        struct SynthesizerParams* synth_params);

    /**
     * Based on the board region settings and the synthesizer params,
     * return the LO synthesizer frequency in kHz.
     *
     * @param r_divider_index
     * These values must match the values enumerated in the documentation
     * RfSynthesizerControl: Parameters for the RF synthesizer
     * ``Bits 18:16 RDivider``.
     * @note r_divider_index references to a R divider value and is not the
     *       R divider itself.
     *
     * @param n_divider The synthesizer divisor where:
     *     ``F_LO = (FREF * NDivider) / (RDivider * 4)``
     *     Where FREF is the TCXO frequency of the board.
     * @param [out] frequency_khz The LO synthesizer frequency in kHz.
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*get_synthesizer_frequency_khz)(uint8_t  r_divider_index,
                                                       uint16_t n_divider,
                                                       uint32_t* frequency_khz);

    /**
     * Returns the filter that should be used for this region.
     * @return an enum value indicating which filter to use
     */
    enum RfFilter (*get_rf_filter)(void);

    /**
     * Get the PLL R divider from the current table
     * @return uint32_t The R divider value of this region.
     * @retval          Zero if current region has not been set.
     */
    uint32_t (*get_pll_r_divider)(void);

    /**
     * Calculate n_divider using:
     * - The target LO frequency in kHz.
     * - The TXCO frequency.
     *   This value  defined by the symbol ``TCXO_FREQ_KHZ`` in the file
     *   ``board_spec_constants.h``.
     * - The r_divider value.
     *
     * @param frequency The LO frequency in kHz.
     * @param r_divider The PLL R divider value.
     *
     * @return N divider value for this frequency.
     */
    uint16_t (*calculate_n_divider)(uint32_t frequency, uint32_t r_divider);

    /**
     * Translate from R divider values to index in the range ``[0,7]``.
     * @param r_divider PLL R divider value
     *
     * @return Index representing the provided R divider value
     */
    uint8_t (*calculate_r_divider_index)(uint32_t r_divider);

    /**
     * From a struct Ex10RegulatoryChannels build list of channel indices into
     * the channel table.
     *
     * @param regulatory_channels Info about the channels used in the region.
     * @param [out] channel_table Based on the channel requirements of the
     * Region this array of channel indices is created.
     *
     * @param [out] channel_size_t The number of channels in the table.
     * This value can be used to determine whether the expected channel size
     * matches the size used by call to this routine.
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*build_channel_table)(
        struct Ex10RegulatoryChannels const* regulatory_channels,
        uint16_t*                            channel_table,
        channel_size_t*                      channel_size);

    /**
     * Sets the region to a copy of the current region but changes the channel
     * list to a single channel of the passed frequency_khz.
     *
     * @param frequency_khz The frequency to change to a channel index and to
     * use for the current region.
     */
    void (*set_single_frequency)(uint32_t frequency_khz);

    /**
     * Disable the regulatory timers for testing purposes.
     *
     * Sets the region to use no regulatory timers, i.e. to remain on,  when cw
     * is ramped up to power. This is done by setting the regulatory region to
     * have all timer parameters set to zero.
     */
    void (*disable_regulatory_timers)(void);

    /**
     * Enable regulatory timers if they have been turned off.
     *
     * @note The default region timers are used. Therefore any other custom
     *       timers you may have set will be overriden.
     */
    void (*reenable_regulatory_timers)(void);

    /**
     * Sets the start time for the current channel to be logged and used for
     * regulatory requirements.
     *
     * @param time_ms   The current time.
     */
    void (*regulatory_timer_set_start)(uint32_t time_ms);

    /**
     * Sets the end time for the current channel to be logged and used for
     * regulatory requirements.
     *
     * @param time_ms   The current time.
     */
    void (*regulatory_timer_set_end)(uint32_t time_ms);

    /**
     * Updates the regulatory region with the proper start and stop time of
     * the previous channel. This is only useful when the active region utilizes
     * time tracking of each individual channel.
     * @note: This reads the device registers which show the latest frequency on
     * which the TxRampUp op and TxRampDown op were performed.
     */
    struct Ex10Result (*update_channel_time_tracking)(void);
};

struct Ex10ActiveRegion const* get_ex10_active_region(void);

#ifdef __cplusplus
}
#endif
