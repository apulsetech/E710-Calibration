/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2021 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ex10_api/byte_span.h"
#include "ex10_api/commands.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/ex10_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
// IPJ_autogen | gen_c_app_aggregate_instructions {

// The codes which can be used in the aggregate op.
enum AggregateOpInstructionType
{
    InstructionTypeReserved = 0x00,
    InstructionTypeWrite = 0x02,
    InstructionTypeReset = 0x08,
    InstructionTypeInsertFifoEvent = 0x0E,
    InstructionTypeRunOp = 0x30,
    InstructionTypeGoToIndex = 0x31,
    InstructionTypeExitInstruction = 0x32,
    InstructionTypeIdentifier = 0x33,
};

#pragma pack(push, 1)
// The structures for the instructions which require data
struct AggregateRunOpFormat
{
    uint8_t op_to_run;
};

struct AggregateGoToIndexFormat
{
    uint16_t jump_index;
    uint8_t repeat_counter;
};

struct AggregateIdentifierFormat
{
    uint16_t identifier;
};


#pragma pack(pop)

/**
 * @union AggregateInstructionData
 * Union of specifiers which can be added to the aggregate op buffer.
 * All instructions are shown in AggregateOpInstructionType. If the
 * instruction does not have a field in the union, it does not require
 * additional data.
 */
union AggregateInstructionData
{
    struct Ex10WriteFormat                write_format;
    struct Ex10ResetFormat                reset_format;
    struct Ex10InsertFifoEventFormat      insert_fifo_event_format;
    struct AggregateRunOpFormat           run_op_format;
    struct AggregateGoToIndexFormat       go_to_index_format;
    struct AggregateIdentifierFormat      identifier_format;
};

// The structure to put into the aggregate instruction buffer
// This tells the op how to interpret the following data.
struct AggregateOpInstruction
{
    enum  AggregateOpInstructionType instruction_type;
    union AggregateInstructionData*  instruction_data;
};
// IPJ_autogen }
// clang-format on

/**
 * @struct Ex10AggregateOpBuilder
 * Interface to help with building the Ex10 aggregate op buffer.
 */
struct Ex10AggregateOpBuilder
{
    /**
     * Appends the passed instruction into the passed span. The function
     * looks at the current instruction and knows how to add it to the
     * passed span. The length and data fields of the span are updated
     * with the new data.
     * NOTE: If the span length does not change, nothing was added.
     *
     * @param op_instruction The current instruction being added
     * @param agg_op_span    The span which we are adding to. The length and
     *                       data field will be updated
     * @return               false if there isn't enough room to add
     *                       the new instruction, else it returns true.
     */
    bool (*append_instruction)(
        const struct AggregateOpInstruction op_instruction,
        struct ByteSpan*                    agg_op_span);

    /**
     * This clears the device side aggregate op buffer by writing all 0s
     * to the register which contains the buffer.
     */
    bool (*clear_buffer)(void);

    /**
     * Sets the passed byte span data into the device. There is a buffer
     * register used in an Ex10 device to read from in executing the aggregate
     * op. This register is set with the data in the byte span according to
     * the length dictated in the byte span.
     */
    bool (*set_buffer)(struct ByteSpan* agg_op_span);

    /**
     * Takes in the index of interest and finds the associated instruction
     * data if it exists.
     * @param index                 The index to check.
     * @param agg_op_span           The current buffer data to check through.
     * @param [out] instruction_at_index
     *              The instruction data at the given index.
     *              If the index is not found, then this will be garbage data.
     * @return The base 0 number of the instruction corresponding
     *         to the passed index. This is the instruction number, not
     *         the index. Will return a -1 if the index is not a valid
     *         instruction.
     */
    ssize_t (*get_instruction_from_index)(
        size_t                         index,
        struct ByteSpan*               agg_op_span,
        struct AggregateOpInstruction* instruction_at_index);

    /**
     * Scans through the passed span and prints the contained instructions.
     * This means it decodes the span as it reads through and prints what the
     * sequential order of instructions within.
     * @note This will not print logical flow AKA it will not follow jump logic.
     */
    void (*print_buffer)(struct ByteSpan* agg_op_span);

    /**
     *  Prints out useful debug around the aggregate op buffer
     */
    void (*print_aggregate_op_errors)(
        const struct AggregateOpSummary* agg_summary);

    /**
     * Appends a register write to the passed in aggregate buffer
     * @param data_to_write The data to add.
     * @param agg_op_span   The local copy of the aggregate buffer.
     * @return              false if a buffer overflow occurs in adding
     *                      to the buffer
     * @note When used as an ex10 command, writes only require a single
     * instruction code and can be followed by multiple write address/length
     * operations. In the aggregate op, we need to do write commands one at a
     * time. This means we can only write to one register per write instruction.
     */
    bool (*append_reg_write)(struct RegisterInfo const* const reg_info,
                             struct ConstByteSpan const*      data_to_write,
                             struct ByteSpan*                 agg_op_span);
    /**
     * Append an Ex10 Reset command to the passed in aggregate buffer.
     * @param destination   Reset destination
     * @return              false if a buffer overflow occurs in adding
     *                      to the buffer
     */
    bool (*append_reset)(uint8_t destination, struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer append_insert_fifo_event() call
     * @note Works the same as the normal Ex10-api command with one exception.
     * Normally, the insert fifo command can be used with a NULL packet by
     * sending nothing more than the command id and the trigger irq. The
     * aggregate op can not find the command length if the packet is  NULL. If
     * you do not wish to send out a packet, use a packet header which dictates
     * static data length and 0 dynamic data length. This empty packet will not
     * be sent out, but an irq can still be generated.
     */
    bool (*append_insert_fifo_event)(const bool                    trigger_irq,
                                     const struct EventFifoPacket* event_packet,
                                     struct ByteSpan*              agg_op_span);

    /**
     * Appends an op run to the passed in aggregate buffer
     * @param op_id       The op to run.
     * @param agg_op_span The local copy of the aggregate buffer.
     * @return            false if a buffer overflow occurs in adding to
     *                    the buffer
     */
    bool (*append_op_run)(enum OpId op_id, struct ByteSpan* agg_op_span);

    /**
     * Appends a Go-To instruction to the passed aggregate buffer.
     * @param jump_index     Index of the aggregate buffer (in bytes) to jump to
     * @param repeat_counter Number of times the jump will be taken before
     * proceeding to the next instruction. If 0, the jump will be bypassed.
     * @return            false if a buffer overflow occurs in adding to
     *                    the buffer
     */
    bool (*append_go_to_instruction)(uint16_t         jump_index,
                                     uint8_t          repeat_counter,
                                     struct ByteSpan* agg_op_span);

    /**
     * Appends an instruction to update the aggregate op identifier.
     * @param new_id        The new identifier which will be sent back as part
     * of the aggregate op FIFO summary.
     * @param agg_op_span   The local copy of the aggregate buffer.
     * @return false if a buffer overflow occurs in adding to the buffer
     */
    bool (*append_identifier)(uint16_t new_id, struct ByteSpan* agg_op_span);

    /**
     * Appends an exit instruction to the passed in aggregate buffer
     * @param agg_op_span   The local copy of the aggregate buffer.
     * @return false if a buffer overflow occurs in adding to the buffer
     */
    bool (*append_exit_instruction)(struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer set_rf_mode() call
     */
    bool (*append_set_rf_mode)(uint16_t rf_mode, struct ByteSpan* agg_op_span);

    /**
     * Appends a measurement of the aux adc. Note that this append appends the
     * register to control the read as well as the op start. This means that the
     * op to read the ADC occurs and the result is left in a register for the
     * user to read out of the device at a later time.
     *
     * @param agg_op_span   The local copy of the aggregate buffer.
     * @return false if a buffer overflow occurs in adding to the buffer,
     *          or if the starting channel specified was out of bounds
     *          for the device.
     */
    bool (*append_measure_aux_adc)(
        enum AuxAdcResultsAdcResult adc_channel_start,
        uint8_t                     num_channels,
        struct ByteSpan*            agg_op_span);

    /**
     * @see  ops layer set_gpio() call
     */
    bool (*append_set_gpio)(uint32_t         gpio_levels,
                            uint32_t         gpio_enables,
                            struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer set_clear_gpio_pins() call
     */
    bool (*append_set_clear_gpio_pins)(
        struct GpioPinsSetClear const* gpio_pins_set_clear,
        struct ByteSpan*               agg_op_span);

    /**
     * @see  ops layer lock_synthesizer() call
     */
    bool (*append_lock_synthesizer)(uint8_t          r_divider_index,
                                    uint16_t         n_divider,
                                    struct ByteSpan* agg_op_span);

    /**
     * Append SJC settings register writes to the passed in aggregate buffer
     *
     * @param sjc_control               SJC algorithm settings.
     * @param sjc_rx_gain               Rx gain.
     * @param initial_settling_time     Initial settling time.
     * @param residue_settling_time     Residue settling time.
     * @param cdac                      CDAC I search.
     * @param sjc_residue_threshold     Residue pass/fail threshold.
     * @param agg_op_span               The aggregate buffer to append.
     *
     */
    bool (*append_sjc_settings)(
        struct SjcControlFields const*             sjc_control,
        struct SjcGainControlFields const*         sjc_rx_gain,
        struct SjcInitialSettlingTimeFields const* initial_settling_time,
        struct SjcResidueSettlingTimeFields const* residue_settling_time,
        struct SjcCdacIFields const*               cdac,
        struct SjcResidueThresholdFields const*    sjc_residue_threshold,
        struct ByteSpan*                           agg_op_span);

    /**
     * @see  ops layer run_sjc() call
     */
    bool (*append_run_sjc)(struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer set_tx_coarse_gain() call
     */
    bool (*append_set_tx_coarse_gain)(uint8_t          tx_atten,
                                      struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer set_tx_fine_gain() call
     */
    bool (*append_set_tx_fine_gain)(int16_t          tx_scalar,
                                    struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer append_set_regulatory_timers() call
     */
    bool (*append_set_regulatory_timers)(
        struct Ex10RegulatoryTimers const* timer_config,
        struct ByteSpan*                   agg_op_span);

    /**
     * @see  ops layer tx_ramp_up() call
     */
    bool (*append_tx_ramp_up)(int32_t dc_offset, struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer append_power_control() call
     */
    bool (*append_power_control)(struct PowerConfigs const* power_config,
                                 struct ByteSpan*           agg_op_span);

    /**
     * @see  ops layer start_log_test() call
     */
    bool (*append_start_log_test)(uint32_t         period,
                                  uint16_t         repeat,
                                  struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer set_atest_mux() call
     */
    bool (*append_set_atest_mux)(uint32_t         atest_mux_0,
                                 uint32_t         atest_mux_1,
                                 uint32_t         atest_mux_2,
                                 uint32_t         atest_mux_3,
                                 struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer set_aux_dac() call
     */
    bool (*append_set_aux_dac)(uint8_t          dac_channel_start,
                               uint8_t          num_channels,
                               uint16_t const*  dac_values,
                               struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer tx_ramp_down() call
     */
    bool (*append_tx_ramp_down)(struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer radio_power_control() call
     */
    bool (*append_radio_power_control)(bool             enable,
                                       struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer set_analog_rx_config() call
     */
    bool (*append_set_analog_rx_config)(
        struct RxGainControlFields const* analog_rx_fields,
        struct ByteSpan*                  agg_op_span);

    /**
     * @see  ops layer measure_rssi() call
     */
    bool (*append_measure_rssi)(struct ByteSpan* agg_op_span,
                                uint8_t          rssi_count);

    /**
     * @see  ops layer start_hpf_override_test_op() call
     */
    bool (*append_hpf_override_test)(struct ByteSpan* agg_op_span,
                                     uint8_t          hpf_mode);

    /**
     * @see  ops layer run_listen_before_talk() call
     */
    bool (*append_listen_before_talk)(struct ByteSpan* agg_op_span,
                                      uint8_t          r_divider_index,
                                      uint16_t         n_divider,
                                      int32_t          offset_frequency_khz,
                                      uint8_t          rssi_count);

    /**
     * @see  ops layer start_timer_op() call
     */
    bool (*append_start_timer_op)(uint32_t         delay_us,
                                  struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer wait_timer_op() call
     */
    bool (*append_wait_timer_op)(struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer start_event_fifo_test() call
     */
    bool (*append_start_event_fifo_test)(uint32_t         period,
                                         uint8_t          num_words,
                                         struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer enable_sdd_logs() call
     */
    bool (*append_enable_sdd_logs)(const struct LogEnablesFields enables,
                                   const uint8_t                 speed_mhz,
                                   struct ByteSpan*              agg_op_span);

    /**
     * @see  ops layer start_inventory_round() call
     */
    bool (*append_start_inventory_round)(
        struct InventoryRoundControlFields const*   configs,
        struct InventoryRoundControl_2Fields const* configs_2,
        struct ByteSpan*                            agg_op_span);

    /**
     * @see  ops layer start_prbs() call
     */
    bool (*append_start_prbs)(struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer start_ber_test() call
     */
    bool (*append_start_ber_test)(uint16_t         num_bits,
                                  uint16_t         num_packets,
                                  bool             delimiter_only,
                                  struct ByteSpan* agg_op_span);

    /**
     * @see  ops layer ramp_transmit_power() call
     */
    bool (*append_ramp_transmit_power)(
        struct PowerConfigs*               power_config,
        struct Ex10RegulatoryTimers const* timer_config,
        struct ByteSpan*                   agg_op_span);

    /**
     * Append droop compensation register writes to the aggregate buffer
     *
     * @param compensation      Droop compensation settings
     * @param agg_op_span       The aggregate buffer to append
     */
    bool (*append_droop_compensation)(
        struct PowerDroopCompensationFields const* compensation,
        struct ByteSpan*                           agg_op_span);
};

struct Ex10AggregateOpBuilder const* get_ex10_aggregate_op_builder(void);

#ifdef __cplusplus
}
#endif
