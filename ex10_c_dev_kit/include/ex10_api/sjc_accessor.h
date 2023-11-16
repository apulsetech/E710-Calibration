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

#include "ex10_api/ex10_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct CdacRange
 * The CDAC range can be set to limit the SJC search range.
 * By default the SJC search range is initialized by the Ex10SjcAccessor module
 * to perform a full search of the CDAC range.
 *
 * Limiting the CDAC range is useful when the target solution has been
 * previously determined or for testing purposes.
 */
struct CdacRange
{
    int8_t  center;     ///< The center of the CDAC range. Nominally zero.
    uint8_t limit;      ///< The symmetric start, stop span for searching.
    uint8_t step_size;  ///< The CDAC step size when searching.
};

/**
 * @struct SjcResult
 * Once the SJC solution has been found the resulting residue measurement
 * and CDAC location may be read.
 */
struct SjcResult
{
    int32_t residue;       ///< The relative LO power detected by the receiver.
    int8_t  cdac;          ///< The CDAC location of the SJC solution.
    bool    cdac_limited;  ///< The SJC solution was outside the SKU range.
};

/**
 * @struct SjcResultPair
 * The SJC I, Q result pair.
 */
struct SjcResultPair
{
    struct SjcResult i;
    struct SjcResult q;
};

/**
 * @struct Ex10SjcAccessor
 * The SJC interface.
 */
struct Ex10SjcAccessor
{
    /** Write the default SJC settings to find a solution. */
    void (*init)(struct Ex10Protocol const*);

    /**
     * Write the SjcControl Register
     *
     * @param sample_average_coarse
     *     The number of samples to take for each SJC measurement when the step
     *     size >= 8.
     * @param sample_average_fine
     *     The number of samples to take for each SJC measurement when the step
     *     size < 8.
     * @param events_enable
     *     Enable SJC measurement data to be output on the Events stream.
     * @param fixed_rx_atten
     *    When set to true the SJC algorithm will not manipulate the
     *    RxGainControl.RxAtten value when searching for a solution.
     *    When set to false the SJC algorithm will change the RxAtten value
     *    to decrease solution search time and optimize the solution.
     * @param sample_decimator
     *     Reduces the Rx sample rate by 2^sample_decimator using the decimator
     *     lowpass filter. This value is applied to all samples.
     */
    void (*set_sjc_control)(uint8_t sample_average_coarse,
                            uint8_t sample_average_fine,
                            bool    events_enable,
                            bool    fixed_rx_atten,
                            uint8_t sample_decimator);

    /// Write the typical Analog Rx Gain values for SJC operation.
    void (*set_analog_rx_config)(void);

    /**
     * Write the DC offset and residue measurement settling times.
     *
     * @param initial_measure_settling_usec
     * The number of microseconds to wait after the receiver has been configured
     * for SJC measurements to the time of the first sample taken within a group
     * of measurements.
     * @param residue_measure_settling_usec
     * The number of microseconds to wait after the CDAC I,Q attenuator has
     * changed, prior to taking SJC residue measurements.
     * This allows the transient related to the CDAC switching to settle out.
     */
    void (*set_settling_time)(uint16_t initial_measure_settling_usec,
                              uint16_t residue_measure_settling_usec);

    /// Prior to the running the RxRunSjcOp set the SJC CDAC ranges.
    /// @see struct CdacRange
    void (*set_cdac_range)(struct CdacRange cdac_i, struct CdacRange cdac_q);

    /**
     * Sets the SJC residue threshold, which defines the pass/fail criterion
     * when running the RxRunSjcOp. If the magnitude of the resulting SJC
     * residue voltage magnitude exceeds this value, then the SJC solution
     * is not considered adequate for LO carrier cancellation into the
     * receiver. If the value zero is set, then this threshold is ignored
     * by the RxRunSjcOp.
     *
     * @param residue_threshold The SJC residue voltage magnitude threshold as
     * measured after the PGA3 receiver stage.
     */
    void (*set_residue_threshold)(uint16_t residue_threshold);

    /**
     * Initialize the CDAC I,Q values to null out the reflected Tx LO
     * within the Rx chain.
     */
    void (*set_cdac_to_find_solution)(void);

    /// After the RxRunSjcOp has completed read the SJC results.
    struct SjcResultPair (*get_sjc_results)(void);
};

/// Return a reference to the sjc instance
const struct Ex10SjcAccessor* get_ex10_sjc(void);

#ifdef __cplusplus
}
#endif
