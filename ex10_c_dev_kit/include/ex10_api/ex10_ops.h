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
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_regulatory.h"
#include "ex10_api/fifo_buffer_list.h"
#include "ex10_api/gen2_commands.h"
#include "ex10_api/rf_mode_definitions.h"
#include "ex10_api/sjc_accessor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct GpioControlFields
 * Used to control and read the GPIO outputs. Each Ex10 DIGITAL_IO pin is
 * assigned a numeric value represented as (1u << DIGITAL_IO[N]);
 */
struct GpioControlFields
{
    /// Set and get the GPIO pins output enable states.
    uint32_t output_enable;
    /// Set and get the GPIO pins output level states.
    uint32_t output_level;
};

struct PowerConfigs
{
    uint8_t                             tx_atten;
    int16_t                             tx_scalar;
    int32_t                             dc_offset;
    uint16_t                            adc_target;
    uint16_t                            loop_stop_threshold;
    uint16_t                            op_error_threshold;
    uint16_t                            loop_gain_divisor;
    uint32_t                            max_iterations;
    enum AuxAdcControlChannelEnableBits power_detector_adc;
};

static uint32_t const ATestMuxDisable = 0u;

/**
 * Allows for the routing of:
 * - AUX DAC[0] to ANA_TEST3
 * - AUX DAC[1] to ANA_TEST2
 */
static uint32_t const ATestMuxAuxDac = (1u << 19u);

/**
 * Allows for the routing of:
 * - AUX ADC [8] to ANA_TEST0
 * - AUX ADC [9] to ANA_TEST1
 * - AUX ADC[10] to ANA_TEST2
 * - AUX ADC[11] to ANA_TEST3
 */
static uint32_t const ATestMuxAuxAdc = (1u << 20u);

struct Ex10Ops
{
    /**
     * Initialize the Ex10 Ops instance
     */
    void (*init)(void);

    /// Cleans up dependencies between the SDK and Impinj Reader Chip.
    void (*release)(void);

    /**
     * Read the OpsStatus register.
     * @return struct OpsStatusFields The fields of the 'OpsStatus' register.
     */
    struct OpsStatusFields (*read_ops_status)(void);

    /**
     * Start a log test op.
     * This op periodically writes a log entry to the debug port.
     * Each log entry consists of a timestamp, followed by a requested
     * number of repetitions of that timestamp and an end-of-line.
     * This op will run until stopped.
     *
     * @param period period (milliseconds) of log entries
     * @param repeat Number of timestamp repetitions in each log entry
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*start_log_test)(uint32_t period, uint16_t repeat);

    /**
     * Set the ATEST multiplexer channel routings by running the SetATestMuxOp.
     * See the documentation for a complete description.
     * @see ATestMuxAuxDac, @see ATestMuxAuxAdc
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_atest_mux)(uint32_t atest_mux_0,
                                       uint32_t atest_mux_1,
                                       uint32_t atest_mux_2,
                                       uint32_t atest_mux_3);

    /**
     * Route PGA3 to the ATEST bus: (1 << 4)
     * ATEST 0      ATEST 1     ATEST 2     ATEST 3
     * RxPga3Q_P    RxPga3Q_N   RxPga3I_P   RxPga3I_N
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*route_atest_pga3)(void);

    /**
     * This executes the MeasureAdcOp operation which performs AUX ADC for each
     * multiplexer input specified.
     *
     * @param adc_channel_start Determines the starting channel on which to
     *          begin ADC conversions.
     * @param num_channels The number of ADC conversions to perform starting
     *          with the adc_channel_start. Each conversion will be performed
     *          on the next multiplexer input. The number of channels converted
     *          will be limited to be within the valid AuxAdcResults range.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*measure_aux_adc)(
        enum AuxAdcResultsAdcResult adc_channel_start,
        uint8_t                     num_channels);

    /**
     * This executes the SetDacOp operation which writes values to the AUX DAC
     * for each channel specified. When the SetDacOp has completed the digital
     * to analog conversions will have completed.
     *
     * @note There are 2 AUX DAC channels which can be written.
     *       - If a channel is omitted from the DAC list then that channel
     *         is disabled and its output will be zero.
     *       - If more channels are set outside the range of the DAC list,
     *         then the out of bounds values are ignored.
     *
     * @param dac_channel_start Determines the starting DAC channel.
     * @param num_channels The number of DAC channel conversions to perform.
     * @param dac_values   An array of DAC channel data containing unsigned
     *                     10-bit values for conversion. The size of this array
     *                     must be uint16_t[num_channels].
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_aux_dac)(uint8_t         dac_channel_start,
                                     uint8_t         num_channels,
                                     uint16_t const* dac_values);

    /**
     * This function executes the SetRfModeOp which will set the Ex10 modem
     * registers for use with a specific RF mode.
     *
     * @param rf_mode The requested RF mode.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_rf_mode)(enum RfModes rf_mode);

    /**
     * Enable the transmitter and begin CW transmission. Sets a timer for any
     * non-zero regulatory dwell time.
     *
     * @param dc_offset DC offset of the transmitter in the EX10.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*tx_ramp_up)(int32_t dc_offset);

    /**
     * Disable the transmitter
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*tx_ramp_down)(void);

    /**
     * Set the TX coarse gain settings.
     * @param tx_atten The CT filter attenuation.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_tx_coarse_gain)(uint8_t tx_atten);

    /**
     * Tx fine gain value.
     * @param tx_scalar new fine gain value
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_tx_fine_gain)(int16_t tx_scalar);

    /**
     * Power up/down the LDOs and bias currents for the Ex10 radio
     * by running the RadioPowerControlOp.
     *
     * @param enable If true,  turns on  the Ex10 analog power supplies.
     *               If false, turns off the Ex10 analog power supplies.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*radio_power_control)(bool enable);

    /**
     * Set individual block gain of the receiver.
     * @param analog_rx_fields The gain we set the receiver to. This value is
     * stored in a state variable for later retrieval.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_analog_rx_config)(
        struct RxGainControlFields const* analog_rx_fields);

    /**
     * Setup and run the op to get the RSSI
     * @note Once run, the RSSI measurements must be retrieved through the
     * event FIFO
     * @param rssi_count The integration count to used for measurement.
     *                   The recommended value is 0x0Fu.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*measure_rssi)(uint8_t rssi_count);

    /**
     * Runs the listen before talk op.
     * The op goes through the following steps:
     * 1. Sets up the modem and internal low pass filter
     * 2. Set the RF synthesizer using the RF Synth control and offset frequency
     * 3. Run the RSSI measurement.
     * 4. Delay for the user specified time.
     * 5. Repeat 2-4 the number of user specified times using an index
     * incrementing from 0.
     *
     * @param lbt_settings     Controls for the LBT op including the number of
     * rounds to run and the delay between each round.
     * @param lbt_offsets      The offset frequency to use for each measurement.
     * Note that this is an array which can differ for each measurement. Ensure
     * that each index is set up to the number of measurements needed. If taking
     * 3 measurements, set index 0-2.
     * @param rf_synth_control The PLL controls to use for each measurement.
     * Note that this is an array which can differ for each measurement. Ensure
     * that each index is set up to the number of measurements needed. If taking
     * 3 measurements, set index 0-2.
     *
     * @param rssi_count       The integration count to used for the RSSI block
     *
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     *
     * @note: Before running LBT, the host must have run 2 ops: set_gpio,
     * set_analog_rx_config.
     * @note: CW must be ramped down before LBT
     * @note: The op leaves the synthesizer locked to the last frequency used
     * when done.
     */
    struct Ex10Result (*run_listen_before_talk)(
        struct LbtControlFields const*           lbt_settings,
        struct RxGainControlFields const*        used_rx_gains,
        struct LbtOffsetFields const*            lbt_offsets,
        struct RfSynthesizerControlFields const* rf_synth_control,
        uint8_t                                  rssi_count);

    /**
     * Run the us timer start op. This starts a timer which one can check
     * completion of via the UsTimerWaitOp.
     * @note If the op is run while a timer is already started, the current
     * timer will be cancelled and a new one will be started.
     *
     * @param delay_us The time in microseconds for the timer to run.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*start_timer_op)(uint32_t delay_us);

    /**
     * Run the us timer wait op. This waits for the timer to complete which was
     * started by the UsTimerStartOp.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     *
     * @note If the timer is done or no timer was started, the op will return
     * immediately.
     */
    struct Ex10Result (*wait_timer_op)(void);

    /**
     * Lock the RF synthesizer to the target frequency
     *
     * @param r_divider_index Index (0-7) of a divider value for
     *                       RfSynthesizer, divides down from FREF.
     * @param n_divider       N divider for the RfSynthesizer
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     *
     */
    struct Ex10Result (*lock_synthesizer)(uint8_t  r_divider_index,
                                          uint16_t n_divider);

    /**
     * Start event FIFO test op.
     * This op periodically pushes debug packets into the event FIFO.
     *
     * @param period period in milliseconds of event FIFO debug packets
     * @param num_words payload size in each event FIFO debug packet
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*start_event_fifo_test)(uint32_t period,
                                               uint8_t  num_words);

    /**
     * Enable the SDD log outputs and log speed
     *
     * @param enables the structure with the individual enables
     * @param speed_mhz  The speed of the sdd SPI clock (1-24 MHz)
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*enable_sdd_logs)(const struct LogEnablesFields enables,
                                         const uint8_t speed_mhz);

    /**
     * Send the enabled Gen2 Access command sequence for  halted which
     * are already stored in the Gen2TxBuffer.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     *
     * @note The modem MUST be in the halted state.
     */
    struct Ex10Result (*send_gen2_halted_sequence)(void);

    /**
     * Release the currently halted on tag and move on to the next.
     *
     * @param nak Indicates whether to nak the tag or not.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     *
     * @note The modem MUST be in the halted state.
     */
    struct Ex10Result (*continue_from_halted)(bool nak);

    /**
     * Run the RunRxSjcOp.
     * The settings by which the SJC will be configured are set up in
     * the ops init.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*run_sjc)(void);

    /**
     * Wait for previously-started op to complete.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*wait_op_completion)(void);

    /**
     * Wait for previously started op to complete.
     *
     * @param timeout_ms Function returns false if the op takes more
     *                   time than this number of milliseconds.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*wait_op_completion_with_timeout)(uint32_t timeout_ms);

    /**
     * Run the AggregateOp and waits for the Op to complete.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*run_aggregate_op)(void);

    /**
     *  Stop a previously-started op.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*stop_op)(void);

    /**
     *  Get the current GPIO output levels and enable.
     *
     * @return struct GpioControlFields
     *         Bitmasks that represent the current GPIO output levels and
     *         enable. Each Ex10 DIGITAL_IO pin is assigned a numeric value
     *         represented as (1u << DIGITAL_IO[N]);
     */
    struct GpioControlFields (*get_gpio)(void);

    /**
     * Set the GPIO levels and enables using the SetGpioOp.
     * There are 22 Ex10 GPIOs.
     * Each GPIO can be set as an output by setting the appropriate bit
     * in the gpio level and enable fields. This function uses the
     * SetGpioOp to set the GPIO levels and enables.
     *
     * @param gpio_levels  A bit-field which sets each GPIO pin output level.
     * @param gpio_enables A bit-field which enables each GPIO pin as an output.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_gpio)(uint32_t gpio_levels, uint32_t gpio_enables);

    /**
     * Set the GPIO levels and enables using the SetClearGpioPinsOp.
     * There are 22 Ex10 GPIOs. Each GPIO can be addressed within each bit-field
     * by setting or clearing the appropriate bit (1u << pin_number).
     *
     * Set the GPIO levels and enable. There are 22 Ex10 GPIOs.
     * The GPIOs are labelled DIGITAL_IO[N] on the Imping Reader Chip schematic.
     *
     * @param gpio_pins_set_clear The GPIO pin bit-fields to set and clear.
     * @see struct GpioPinsSetClear
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_clear_gpio_pins)(
        struct GpioPinsSetClear const* gpio_pins_set_clear);

    /**
     * Start a single inventory round by running the StartInventoryRoundOp.
     *
     * @param configs inventory round configuration
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*start_inventory_round)(
        struct InventoryRoundControlFields const*   configs,
        struct InventoryRoundControl_2Fields const* configs_2);

    /**
     * Enable PRBS modulation
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*start_prbs)(void);

    /**
     * Set the HPF override settings and run the HPF override test op.
     *
     * @param struct HpfOverrideSettingsFields
     *        Ex10 receiver HPF (high-pass filter) override setting.
     *        For details, see HpfOverrideSettings register documentation.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*start_hpf_override_test_op)(
        struct HpfOverrideSettingsFields const* hpf_settings);

    /**
     * Start the ETSI burst Op
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*run_etsi_burst)(void);

    /**
     * Enable the Ber Test
     *
     * @param num_bits       The number of bits to send
     * @param num_packets    The number of packets to transmit
     * @param delimiter_only Determine whether to use a delimiter only instead
     * of a full query
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*start_ber_test)(uint16_t num_bits,
                                        uint16_t num_packets,
                                        bool     delimiter_only);

    /**
     * Run the SendSelectOp. The op will look through the
     * Gen2SelectEnable register and begin send all enabled commands
     * from the gen2 tx buffer.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*send_select)(void);

    /**
     * Return the current 32 bit time (microseconds) since start from the
     * device.
     */
    uint32_t (*get_device_time)(void);

    /**
     * Run the power control loop to attempt to reach desired Tx power.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*run_power_control_loop)(
        struct PowerConfigs* power_config);
};

struct Ex10Ops const* get_ex10_ops(void);

#ifdef __cplusplus
}
#endif
