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

#include "ex10_api/application_register_definitions.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/ex10_continuous_inventory_common.h"
#include "ex10_api/ex10_ops.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/rf_mode_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @enum InventoryState Keep track of the continuous inventory state.
enum InventoryState
{
    /// Continuous inventory is not in progress.
    InvIdle,
    /// Continuous inventory is in progress.
    InvOngoing,
    /// The host requested continuous inventory stop via stop_transmitting().
    InvStopRequested,
};

/**
 * @struct ContinuousInventoryState
 * State variables that are set in update_inventory_state().
 */
struct ContinuousInventoryState
{
    /// Initialized in to InvOngoing inventory_continuous()
    /// Set in update_inventory_state().
    enum InventoryState state;

    /// The reason for inventory round completion.
    enum InventorySummaryReason done_reason;

    /// Inventory config at the beginning of continuous inventory
    struct InventoryRoundControlFields initial_inventory_config;

    /// The Q from the end of the previous inventory round.
    uint8_t previous_q;

    /// The min_q_count from the previous inventory round.
    uint8_t min_q_count;

    /// The queries_since_valid_epc_count from the previous round.
    uint8_t queries_since_valid_epc_count;

    /// set in check_stop_conditions,
    /// used to push the ContinuousInventorySummary EventFifo packet
    enum StopReason stop_reason;

    /// Initialized to zero in inventory_continuous(), updated by packets
    /// in update_inventory_state().
    size_t round_count;

    /// Counts the number of tags singulated when performing a
    /// continuous inventory sequence.
    size_t tag_count;

    /// Holds the current target state of the reader when performing a
    /// continuous inventory sequence.
    uint8_t target;

    /// Iterator through the inventory rounds sequence (if active).
    size_t inventory_round_iter;
};

/**
 * @struct Ex10Reader
 * The Ex10 reader interface.
 */
struct Ex10Reader
{
    /**
     * Initialize the Ex10Reader object.
     * Sets up the reader-like interface to an Ex10 device.
     *
     * @param region_id A valid region ID enum value. @see struct Ex10RegionId.
     */
    void (*init)(enum Ex10RegionId region_id);

    /**
     * Initialize the Impinj Reader Chip at the Ex10Reader layer.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*init_ex10)(void);

    /**
     * Read the calibration from the Impinj Reader Chip
     * and store it in the Ex10Reader layer.
     */
    void (*read_calibration)(void);

    /**
     * Release any resources used by the Ex10Reader object.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*deinit)(void);

    /**
     * Run a inventory continuously until the passed in stop conditions are met.
     *
     * @param stop_conditions A struct detailing any stop conditions. If 0,
     *                        the conditions is not used.
     * @param dual_target     Flip the target between rounds if true.
     * @param other           See inventory for all other parameters.
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*continuous_inventory)(
        uint8_t const                               antenna,
        enum RfModes                                rf_mode,
        int16_t const                               tx_power_cdbm,
        struct InventoryRoundControlFields const*   inventory_config,
        struct InventoryRoundControl_2Fields const* inventory_config_2,
        bool                                        send_selects,
        struct StopConditions const*                stop_conditions,
        bool                                        dual_target,
        bool                                        remain_on);

    /**
     * Run a single inventory round.
     *
     * @param antenna       The antenna to transmit on.
     * @param rf_mode       An Ex10 RF Mode to be used for this round.
     * @param tx_power_cdbm Target transmit power level in cdBm
     *                      (100th's of a dbm). Example: 2,950 = 29.5 dBm.
     * @note                Left out dual target on purpose - should be handled
     *                      by calling code based on inventory parameters
     * @param inventory_config The inventory settings to be used for this round.
     * @param inventory_config_2 The second inventory settings register
     * @param select_config   The parameters used for a select sent before
     *                        each inventory round.
     *                        By passing in a select configuration through
     *                        the inventory call, you are overriding any
     *                        previous command specifications made in the
     *                        TX buffer.
     * @param remain_on     If false, the normal region specific regulatory
     *                      timers will be used. If true, the timers will be
     *                      disabled and the Ex10 device will stay powered on
     *                      until told to ramp down.
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*inventory)(
        uint8_t                                     antenna,
        enum RfModes                                rf_mode,
        int16_t                                     tx_power_cdbm,
        struct InventoryRoundControlFields const*   inventory_config,
        struct InventoryRoundControl_2Fields const* inventory_config_2,
        bool                                        send_selects,
        bool                                        remain_on);

    /**
     * Called by the interrupt handler thread when there is a fifo related
     * interrupt.
     */
    void (*fifo_data_handler)(struct FifoBufferNode* fifo_buffer);

    /**
     * Called by the interrupt handler thread when there is a non-fifo related
     * interrupt.
     */
    bool (*interrupt_handler)(struct InterruptStatusFields irq_status);

    /**
     * Return the packet at the front of the packet queue.
     *
     * @return An EventFifoPacket or NULL if none available.
     */
    struct EventFifoPacket const* (*packet_peek)(void);

    /**
     * Delete the packet at the front of the packet queue.
     *
     * @note There is a limited amount of room for packets in the packet queue,
     *       so packets should be discarded regularly using remove_packet() to
     *       ensure room for new packets.
     *
     * @note The packet must only be removed once it is not longer in use.
     *       packet_remove() is similar to calling free() on allocated memory.
     *       Once the packet has been removed its packet pointer obtained
     *       through packet_peek() is invalid.
     *
     * @note Calling packet_free() allows for new packets to be read from the
     *       Ex10. If packet_remove() is not called then reading will stall and
     *       the available buffer space will have been used up.
     */
    void (*packet_remove)(void);

    /**
     * Query whether packets are available for reading.
     *
     * @note This is merely a proxy for calling packet_peek() and returns
     *       true if packet_peek() returns non-NULL.
     *
     * @return bool true if packets are available for reading.
     *              false if there are no packets available for reading.
     */
    bool (*packets_available)(void);

    /**
     * Release the current tag and move on to the next.
     *
     * @param nak Indicates whether to nak the tag or not.
     *
     * @note The modem MUST be in the halted state.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*continue_from_halted)(bool nak);

    /**
     * Transmit a continuous wave until regulatory timers expire.
     *
     * @see inventory() for parameter descriptions.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*cw_test)(uint8_t      antenna,
                                 enum RfModes rf_mode,
                                 int16_t      tx_power_cdbm,
                                 uint32_t     frequency_khz,
                                 bool         remain_on);

    /**
     * Transmit a pseudo-random bit stream until regulatory timers expire.
     * User must call stop_transmitting() to end this operation.
     *
     * @see inventory() for parameter descriptions.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*prbs_test)(uint8_t      antenna,
                                   enum RfModes rf_mode,
                                   int16_t      tx_power_cdbm,
                                   uint32_t     frequency_khz,
                                   bool         remain_on);

    /**
     * Run a packetized BER test.
     *
     * @param num_bits       Number of bits per packet
     * @param num_packets    Number of packets to receive
     * @param delimiter_only Use a delimiter instead of a query to trigger a
     * packet.
     * @see inventory() for other parameter descriptions.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*ber_test)(uint8_t      antenna,
                                  enum RfModes rf_mode,
                                  int16_t      tx_power_cdbm,
                                  uint32_t     frequency_khz,
                                  uint16_t     num_bits,
                                  uint16_t     num_packets,
                                  bool         delimiter_only);

    /**
     * Start up a continuous cycle of ramp up, inventory, ramp down.
     * User must call stop_transmitting() to end this operation.
     *
     * @see inventory() for parameter descriptions.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*etsi_burst_test)(
        struct InventoryRoundControlFields const*   inventory_config,
        struct InventoryRoundControl_2Fields const* inventory_config_2,
        uint8_t                                     antenna,
        enum RfModes                                rf_mode,
        int16_t                                     tx_power_cdbm,
        uint16_t                                    on_time_ms,
        uint16_t                                    off_time_ms,
        uint32_t                                    frequency_khz);

    /**
     * Insert a custom event into the Event Fifo
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*insert_fifo_event)(
        const bool                    trigger_irq,
        struct EventFifoPacket const* event_packet);

    /**
     * Enable the SDD log outputs and log speed
     * @param enables the structure with the individual enables
     * @param speed_mhz  The speed of the sdd SPI clock (1-24 MHz)
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*enable_sdd_logs)(const struct LogEnablesFields enables,
                                         const uint8_t speed_mhz);

    /**
     * Stop any ongoing inventory round, stop transmitting, ramp down power.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*stop_transmitting)(void);

    struct Ex10Result (*build_cw_configs)(uint8_t          antenna,
                                          enum RfModes     rf_mode,
                                          int16_t          tx_power_cdbm,
                                          uint32_t         frequency_khz,
                                          bool             remain_on,
                                          struct CwConfig* cw_config);

    /**
     * Compensate an RSSI value from a TagRead EventFifoPacket. This
     * compensation is calibration version dependent.
     * @param rssi_raw The non-compensated log2 RSSI value.
     * @return The compensated RSSI value in cdBm
     */
    int16_t (*get_current_compensated_rssi)(uint16_t rssi_raw);

    /**
     * Calculate the RSSI count equivalent for a RSSI cdBm input.
     * This compensation is calibration version dependent.
     * @param rssi_cdbm The RSSI cdBm value.
     * @return The log2 RSSI value
     */
    uint16_t (*get_current_rssi_log2)(int16_t rssi_cdbm);

    /**
     * Runs the LBT op and retrieves a single RSSI measurement through the
     * measured_rssi_log2_reg register. This value is then thrown through the
     * calibration layer compensation before being returned to the user.
     * @param antenna       The antenna to listen on.
     * @param frequency_khz If a frequency is specified here, this forces the
     *                      stack to always ramp up to the same frequency. If 0,
     *                      the region specific jump table will be used.
     * @param lbt_offset    The offset frequency for measuring the RSSI.
     * @param rssi_count    The integration count for measuring RSSI.
     * @param override_used Determines whether or not to use the Rx gain
     * override setting in the LBT control register. This is normally false,
     * which means the default LBT-specific settings are used in RSSI
     * measurement and compensation. These default settings override the
     * RxGainControlFields in the RxGainControl register. If the override is
     * set, the user-specified settings in the RxGainControl register will be
     * used during measurement and compensation. Note that these user settings
     * must be set before this function call.
     * @return The compensated RSSI value in cdBm. In case of any error which
     * causes an issue in reading the RSSI, EX10_RSSI_INVALID is returned.
     */
    int16_t (*get_listen_before_talk_rssi)(uint8_t  antenna,
                                           uint32_t frequency_khz,
                                           int32_t  lbt_offset,
                                           uint8_t  rssi_count,
                                           bool     override_used);

    /**
     * Runs the LBT op with a variable number of measurements. The user
     * configures the number of measurements up to 5 along with the delay
     * between each measurement. This delay will be the same between all
     * measurements. The user may also set the LBT offset and base frequency to
     * a different value for each measurement.
     * @param antenna       The antenna to listen on.
     * @param frequency_khz If a frequency is specified here, this forces the
     *                      stack to always ramp up to the same frequency. If 0,
     *                      the frequency used will iterate through the region
     * specific jump table.
     * @param rssi_count    The integration count for measuring RSSI.
     * @param lbt_settings  The lbt controls for the entire op. This tells the
     *                      op whether to use a delay between measurements, how
     *                      many measurements to make, whether or not to
     *                      override the RX gain settings, and if a narrower
     *                      bandwidth should be used.
     * @note: The override field determines whether or not to use the Rx gain
     * override setting in the LBT control register. This is normally false,
     * which means the default LBT-specific settings are used in RSSI
     * measurement and compensation. These default settings override the
     * RxGainControlFields in the RxGainControl register. If the override is
     * set, the user-specified settings in the RxGainControl register will be
     * used during measurement and compensation. Note that these user settings
     * must be set before this function call.
     *
     * @param frequencies_khz       The frequencies array for all LBT
     *                              measurements. Note that these can all
     * differ.
     * @param lbt_offset            The offset frequency for measuring the RSSI
     *                              in each measurement. Note that these can all
     *                              differ.
     * @param rssi_measurements     The rssi results are placed in this array.
     *                              Ensure that the array contains enough space
     *                              for the number of measurements that were
     *                              specified in lbt_settings.
     * @return                      The status showing any errors which
     * occurred.
     */
    struct Ex10Result (*listen_before_talk_multi)(
        uint8_t                 antenna,
        uint8_t                 rssi_count,
        struct LbtControlFields lbt_settings,
        uint32_t*               frequencies_khz,
        int32_t*                lbt_offsets,
        int16_t*                rssi_measurements);

    /**
     * Used to return the last analog RX configuration into the device from
     * reader.
     * @return A pointer to the stored rx configuration.
     */
    struct RxGainControlFields const* (*get_current_analog_rx_fields)(void);

    /**
     * Used to return the state of continuous inventory. Useful in the case of
     * checking overall progress in long runs.
     * @return The continuous inventory state.
     */
    struct ContinuousInventoryState volatile const* (
        *get_continuous_inventory_state)(void);
};

struct Ex10Reader const* get_ex10_reader(void);

#ifdef __cplusplus
}
#endif
