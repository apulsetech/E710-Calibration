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

#include <sys/types.h>

#include "board/ex10_gpio.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/fifo_buffer_list.h"
#include "ex10_api/gen2_commands.h"
#include "ex10_api/rf_mode_definitions.h"
#include "ex10_api/sjc_accessor.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CwConfig
{
    struct GpioPinsSetClear           gpio;
    enum RfModes                      rf_mode;
    struct PowerConfigs               power;
    struct RfSynthesizerControlFields synth;
    struct Ex10RegulatoryTimers       timer;
};

struct Ex10RfPower
{
    /// Initialize the Impinj Reader Chip RF power.
    struct Ex10Result (*init_ex10)(void);

    /**
     * Return whether or not CW is currently on.
     * @param cw_is_on A bool of whether CW is currently on.
     */
    bool (*get_cw_is_on)(void);

    /**
     * Blindly stop any running op (such as inventory) and then
     * ramp down the transmitter
     */
    struct Ex10Result (*stop_op_and_ramp_down)(void);

    /**
     * Ramp down and turn off the radio transmitter.
     * @note The radio remains powered on but the transmitter is stopped.
     */
    struct Ex10Result (*cw_off)(void);

    /**
     * This executes the MeasureAdcOp operation which performs AUX ADC for each
     * multiplexer input specified. Once the conversions are completed the
     * ADC results are placed in the adc_results output parameter.
     *
     * @param adc_channel_start Determines the starting channel on which to
     *          begin ADC conversions.
     * @param num_channels The number of ADC conversions to perform starting
     *                     with the adc_channel_start. Each conversion will
     *                     be performed on the next multiplexer input. The
     *                     number of channels converted will be limited to be
     *                     within the valid AuxAdcResults range.
     *
     * @param adc_results [out] The array into which of ADC conversion results
     *                          will be contained. The adc_results must be able
     *                          to fit num_channels conversion results.
     * @return                  Info about any encountered errors.
     */
    struct Ex10Result (*measure_and_read_aux_adc)(
        enum AuxAdcResultsAdcResult adc_channel_start,
        uint8_t                     num_channels,
        uint16_t*                   adc_results);


    /**
     * This executes the MeasureAdcOp specifying the read of a single channel
     * starting at the AdcTemperature channel.
     *
     * @param temp_adc [out] The uint16_t pointer where the temperature ADC
     *                       is placed.
     *
     * @return Info about any encountered errors.
     */
    struct Ex10Result (*measure_and_read_adc_temperature)(uint16_t* temp_adc);

    /**
     * This sets the RF Mode in the Ex10 device modem and sets the approprate
     * gpio for the DRM mode
     */
    struct Ex10Result (*set_rf_mode)(enum RfModes rf_mode);

    /**
     * Builds the configuration to use for cw_on.
     * @param antenna       The antenna to transmit on.
     * @param rf_mode       An Ex10 RF Mode to be used for this round.
     * @param tx_power_cdbm Target transmit power level in cdBm
                            (100th's of a dbm). Example: 2,950 = 29.5 dBm.
     * @note                Left out dual target on purpose - should be handled
     *                      by calling code based on inventory parameters
     * @param cw_config     Configuration for cw on is placed in this structure
     *                      passed by reference.
     */
    struct Ex10Result (*build_cw_configs)(uint8_t          antenna,
                                          enum RfModes     rf_mode,
                                          int16_t          tx_power_cdbm,
                                          uint16_t         temperature_adc,
                                          bool             temp_comp_enabled,
                                          struct CwConfig* cw_config);

    /**
     * Ramp power while transmitting CW and prepare the receiver
     * @param gpio_pins_set_clear The gpio settings to use for the ramp up.
     * @param power_config        Parameters to use for the open loop and closed
     *                            loop power control during the ramp process.
     * @param synth_control       Parameters used for the PLL lock op.
     * @param timer_config        Time configurations to use for automatic CW
     * off.
     * @param droop_comp          Configuration for the droop compensation run
     *                            during a ramp period.
     * @return Info about any encountered errors.
     */
    struct Ex10Result (*cw_on)(
        struct GpioPinsSetClear const*             gpio_pins_set_clear,
        struct PowerConfigs*                       power_config,
        struct RfSynthesizerControlFields const*   synth_control,
        struct Ex10RegulatoryTimers const*         timer_config,
        struct PowerDroopCompensationFields const* droop_comp);

    /**
     * Ramps to the target transmit power.
     * @param power_config        Parameters to use for the open loop and closed
     *                            loop power control during the ramp process.
     * @param timer_config        Time configurations to use for automatic CW
     * off.
     * @return Info about any encountered errors.
     */
    struct Ex10Result (*ramp_transmit_power)(
        struct PowerConfigs*               power_config,
        struct Ex10RegulatoryTimers const* timer_config);

    /**
     * Grab the default values for power droop compensation.
     * @return The power droop default structure.
     */
    struct PowerDroopCompensationFields (*get_droop_compensation_defaults)(
        void);

    /**
     * Sets the regulatory timers into the device.
     *
     * - nominal_stop_time: Ex10 will complete the ongoing Gen2 operation until
     *   it reaches a Query, QueryAdj or QueryRep and ramp down Tx.
     * - extended_stop_time: Ex10 will terminate any ongoing Gen2 operation
     *   and immediately ramp down Tx.
     * - regulatory_stop_time: Ex10 will terminate any ongoing Gen2 operation,
     *   immediately ramp down Tx and set the TX scalar to 0.
     */
    void (*set_regulatory_timers)(
        struct Ex10RegulatoryTimers const* timer_config);

    /**
     * Sets the analog rx configs and stores a local copy for faster
     * retrieval instead of reading it off the device each time.
     *
     * @param analog_rx_fields The settings by which the SJC will be
     *                         configured.
     */
    struct Ex10Result (*set_analog_rx_config)(
        struct RxGainControlFields const* analog_rx_fields);

    /**
     * Writes the user passed compensation into the power droop compensation
     * register on the device. This allows the user a simple way to enable the
     * droop compensation.
     *
     * @param compensation The compensation settings for the
     *                     power droop compensation register.
     */
    struct Ex10Result (*enable_droop_compensation)(
        struct PowerDroopCompensationFields const* compensation);

    /**
     * Creates a set of compensation settings with the enable set to false and
     * writes it to the device power droop compensation register. This allowes
     * the user a simple way to disable the droop compensation.
     */
    void (*disable_droop_compensation)(void);
};

struct Ex10RfPower const* get_ex10_rf_power(void);

#ifdef __cplusplus
}
#endif
