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

//
// Field definitions for all the bootloader registers in an EX10 device.
//
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ex10_api/application_register_field_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off

// IPJ_autogen | gen_c_app_ex10_api_reg {
// Shows the access given to the host for each register
enum RegisterAccessType {
    ReadOnly,
    WriteOnly,
    ReadWrite,
    Restricted
};

// The basic info needed by all registers
struct RegisterInfo {
    char const * const name;
    uint16_t address;
    uint16_t length;
    uint8_t  num_entries;
    enum RegisterAccessType access;
};

// Structs which break down the fields and sizing within each register
#pragma pack(push, 1)

struct CommandResultFields {
    enum ResponseCode failed_result_code : 8;
    enum CommandCode failed_command_code : 8;
    uint16_t commands_since_first_error : 16;
};

struct ResetCauseFields {
    bool software_reset : 1;
    bool watchdog_timeout : 1;
    bool lockup : 1;
    bool external_reset : 1;
    int16_t rfu : 12;
};

struct StatusFields {
    enum Status status : 2;
    int16_t rfu : 14;
};

struct VersionStringFields {
    uint8_t* data;
};

struct BuildNumberFields {
    uint8_t* data;
};

struct GitHashFields {
    uint8_t* data;
};

struct TimestampFields {
    uint32_t current_timestamp_us : 32;
};

struct FrefFreqFields {
    uint32_t fref_freq_khz : 32;
};

struct ProductSkuFields {
    uint8_t* data;
};

struct SerialNumberFields {
    uint8_t* data;
};

struct DeviceInfoFields {
    uint8_t eco_revision : 8;
    uint8_t device_revision_lo : 8;
    uint8_t device_revision_hi : 8;
    uint8_t device_identifier : 8;
};

struct DeviceBuildFields {
    uint8_t spar_revision : 8;
    uint8_t rtl_build_number_lo : 8;
    uint8_t rtl_build_number_hi : 8;
    int8_t rfu : 8;
};

struct RtlRevisionFields {
    uint32_t rtl_revision : 32;
};

struct StackDepthFields {
    uint32_t depth : 32;
};

struct InterruptMaskFields {
    bool op_done : 1;
    bool halted : 1;
    bool event_fifo_above_thresh : 1;
    bool event_fifo_full : 1;
    bool inventory_round_done : 1;
    bool halted_sequence_done : 1;
    bool command_error : 1;
    bool aggregate_op_done : 1;
    int32_t rfu : 24;
};

struct InterruptMaskSetFields {
    bool op_done : 1;
    bool halted : 1;
    bool event_fifo_above_thresh : 1;
    bool event_fifo_full : 1;
    bool inventory_round_done : 1;
    bool halted_sequence_done : 1;
    bool command_error : 1;
    bool aggregate_op_done : 1;
    int32_t rfu : 24;
};

struct InterruptMaskClearFields {
    bool op_done : 1;
    bool halted : 1;
    bool event_fifo_above_thresh : 1;
    bool event_fifo_full : 1;
    bool inventory_round_done : 1;
    bool halted_sequence_done : 1;
    bool command_error : 1;
    bool aggregate_op_done : 1;
    int32_t rfu : 24;
};

struct InterruptStatusFields {
    bool op_done : 1;
    bool halted : 1;
    bool event_fifo_above_thresh : 1;
    bool event_fifo_full : 1;
    bool inventory_round_done : 1;
    bool halted_sequence_done : 1;
    bool command_error : 1;
    bool aggregate_op_done : 1;
    int32_t rfu : 24;
};

struct EventFifoNumBytesFields {
    uint16_t num_bytes : 12;
    int8_t rfu : 4;
};

struct EventFifoIntLevelFields {
    uint16_t threshold : 12;
    int8_t rfu : 4;
};

struct GpioOutputEnableFields {
    uint32_t enable_bits : 32;
};

struct GpioOutputLevelFields {
    uint32_t level_bits : 32;
};

struct PowerControlLoopAuxAdcControlFields {
    uint16_t channel_enable_bits : 15;
    int32_t rfu : 17;
};

struct PowerControlLoopGainDivisorFields {
    uint16_t gain_divisor : 16;
    int16_t rfu : 16;
};

struct PowerControlLoopMaxIterationsFields {
    uint32_t max_iterations : 32;
};

struct PowerControlLoopAdcTargetFields {
    uint16_t adc_target_value : 16;
    int16_t rfu : 16;
};

struct PowerControlLoopAdcThresholdsFields {
    uint16_t loop_stop_threshold : 16;
    uint16_t op_error_threshold : 16;
};

struct DelayUsFields {
    uint32_t delay : 32;
};

struct GpioOutputLevelSetFields {
    uint32_t level_bits_set : 32;
};

struct GpioOutputLevelClearFields {
    uint32_t level_bits_clear : 32;
};

struct GpioOutputEnableSetFields {
    uint32_t enable_bits_set : 32;
};

struct GpioOutputEnableClearFields {
    uint32_t enable_bits_clear : 32;
};

struct OpsControlFields {
    enum OpId op_id : 8;
};

struct OpsStatusFields {
    enum OpId op_id : 8;
    bool busy : 1;
    uint8_t Reserved0 : 7;
    enum OpsStatus error : 8;
    int8_t rfu : 8;
};

struct HaltedControlFields {
    bool go : 1;
    bool resume : 1;
    bool nak_tag : 1;
    int32_t rfu : 29;
};

struct HaltedStatusFields {
    bool halted : 1;
    bool busy : 1;
    uint8_t rfu_1 : 6;
    uint16_t rfu_2 : 16;
    enum HaltedStatusError error : 8;
};

struct LogTestPeriodFields {
    uint32_t period : 32;
};

struct LogTestWordRepeatFields {
    uint16_t repeat : 16;
    bool test_type : 1;
    int16_t rfu : 15;
};

struct EventFifoTestPeriodFields {
    uint32_t period : 32;
};

struct EventFifoTestPayloadNumWordsFields {
    uint8_t num_words : 8;
    int32_t rfu : 24;
};

struct LogSpeedFields {
    uint8_t speed_mhz : 8;
    int8_t rfu : 8;
};

struct LogEnablesFields {
    bool op_logs : 1;
    bool ramping_logs : 1;
    bool config_logs : 1;
    bool lmac_logs : 1;
    bool sjc_solution_logs : 1;
    bool rf_synth_logs : 1;
    bool power_control_solution_logs : 1;
    bool aux_logs : 1;
    bool regulatory_logs : 1;
    bool command_response_logs : 1;
    bool insert_fifo_event_logs : 1;
    bool host_irq_logs : 1;
    bool timer_start_logs : 1;
    bool timer_wait_logs : 1;
    bool aggregate_op_logs : 1;
    bool read_fifo_logs : 1;
    bool lbt_op_logs : 1;
    uint8_t rfu_1 : 7;
    bool rfu_2 : 1;
    bool wfe_trace : 1;
    bool rssi_trace_logs : 1;
    bool modem_data_logs : 1;
    bool code_coverage : 1;
    bool lmac_debug_logs : 1;
    bool sjc_trace_logs : 1;
    bool power_control_trace_logs : 1;
};

struct BerControlFields {
    uint16_t num_bits : 16;
    uint16_t num_packets : 16;
};

struct BerModeFields {
    bool del_only_mode : 1;
    int8_t rfu : 7;
};

struct HpfOverrideSettingsFields {
    uint8_t hpf_mode : 8;
    int32_t rfu : 24;
};

struct AuxAdcControlFields {
    uint16_t channel_enable_bits : 15;
    bool rfu : 1;
};

struct AuxAdcResultsFields {
    uint16_t adc_result : 10;
    int8_t rfu : 6;
};

struct AuxDacControlFields {
    uint8_t channel_enable_bits : 2;
    int16_t rfu : 14;
};

struct AuxDacSettingsFields {
    uint16_t value : 10;
    int8_t rfu : 6;
};

struct ATestMuxFields {
    uint32_t a_test_mux : 32;
};

struct TxFineGainFields {
    int16_t tx_scalar : 16;
    int16_t rfu : 16;
};

struct RxGainControlFields {
    enum RxGainControlRxAtten rx_atten : 2;
    enum RxGainControlPga1Gain pga1_gain : 2;
    enum RxGainControlPga2Gain pga2_gain : 2;
    enum RxGainControlPga3Gain pga3_gain : 2;
    uint8_t Reserved0 : 2;
    enum RxGainControlMixerGain mixer_gain : 2;
    bool pga1_rin_select : 1;
    bool Reserved1 : 1;
    bool mixer_bandwidth : 1;
    bool rfu : 1;
};

struct TxCoarseGainFields {
    uint8_t tx_atten : 5;
    int32_t rfu : 27;
};

struct RfModeFields {
    uint16_t id : 16;
    int16_t rfu : 16;
};

struct DcOffsetFields {
    int32_t offset : 20;
    int16_t rfu : 12;
};

struct EtsiBurstOffTimeFields {
    uint16_t off_time : 16;
    int16_t rfu : 16;
};

struct CwIsOnFields {
    bool is_on : 1;
    int32_t rfu : 31;
};

struct MeasureRssiCountFields {
    uint8_t samples : 4;
    int16_t rfu : 12;
};

struct MeasuredRssiLinearFields {
    uint32_t value : 32;
};

struct MeasuredRssiLog2Fields {
    uint16_t value : 16;
};

struct LbtOffsetFields {
    int32_t khz : 32;
};

struct LbtControlFields {
    bool override : 1;
    bool narrow_bandwidth_mode : 1;
    uint8_t Reserved0 : 6;
    uint8_t num_rssi_measurements : 8;
    uint16_t measurement_delay_us : 16;
};

struct RfSynthesizerControlFields {
    uint16_t n_divider : 16;
    uint8_t r_divider : 3;
    uint8_t Reserved0 : 5;
    bool lf_type : 1;
    int8_t rfu : 7;
};

struct SjcControlFields {
    uint8_t sample_average_coarse : 4;
    uint8_t sample_average_fine : 4;
    bool events_enable : 1;
    bool fixed_rx_atten : 1;
    uint8_t decimator : 3;
    int8_t rfu : 3;
};

struct SjcGainControlFields {
    enum RxGainControlRxAtten rx_atten : 2;
    enum RxGainControlPga1Gain pga1_gain : 2;
    enum RxGainControlPga2Gain pga2_gain : 2;
    enum RxGainControlPga3Gain pga3_gain : 2;
    uint8_t Reserved0 : 2;
    enum RxGainControlMixerGain mixer_gain : 2;
    bool pga1_rin_select : 1;
    bool Reserved1 : 1;
    bool mixer_bandwidth : 1;
    int32_t rfu : 17;
};

struct SjcInitialSettlingTimeFields {
    uint16_t settling_time : 16;
};

struct SjcResidueSettlingTimeFields {
    uint16_t settling_time : 16;
};

struct SjcCdacIFields {
    int8_t center : 8;
    uint8_t limit : 8;
    uint8_t step_size : 8;
    int8_t rfu : 8;
};

struct SjcCdacQFields {
    int8_t center : 8;
    uint8_t limit : 8;
    uint8_t step_size : 8;
    int8_t rfu : 8;
};

struct SjcResultFields {
    int8_t cdac : 8;
    bool cdac_sku_limited : 1;
    uint8_t Reserved0 : 3;
    int32_t residue : 20;
};

struct SjcResidueThresholdFields {
    uint16_t magnitude : 16;
};

struct AnalogEnableFields {
    bool all : 1;
    int32_t rfu : 31;
};

struct AggregateOpBufferFields {
    uint8_t* command_buffer;
};

struct PowerDroopCompensationFields {
    bool enable : 1;
    uint16_t rfu : 15;
    uint8_t compensation_interval_ms : 8;
    uint8_t fine_gain_step_cd_b : 8;
};

struct LastTxRampUpTimeMsFields {
    uint32_t time_ms : 32;
};

struct LastTxRampUpLoFreqKhzFields {
    uint32_t lo_freq_khz : 32;
};

struct LastTxRampDownTimeMsFields {
    uint32_t time_ms : 32;
};

struct LastTxRampDownLoFreqKhzFields {
    uint32_t lo_freq_khz : 32;
};

struct LastTxRampDownReasonFields {
    uint8_t reason : 8;
};

struct RssiThresholdRn16Fields {
    uint16_t threshold : 16;
};

struct RssiThresholdEpcFields {
    uint16_t threshold : 16;
};

struct InventoryRoundControlFields {
    uint8_t initial_q : 4;
    uint8_t max_q : 4;
    uint8_t min_q : 4;
    uint8_t num_min_q_cycles : 4;
    bool fixed_q_mode : 1;
    bool q_increase_use_query : 1;
    bool q_decrease_use_query : 1;
    enum InventoryRoundControlSession session : 2;
    uint8_t select : 2;
    bool target : 1;
    bool halt_on_all_tags : 1;
    bool fast_id_enable : 1;
    bool tag_focus_enable : 1;
    bool auto_access : 1;
    bool abort_on_fail : 1;
    bool halt_on_fail : 1;
    bool always_ack : 1;
    bool rfu : 1;
};

struct InventoryRoundControl_2Fields {
    uint8_t max_queries_since_valid_epc : 8;
    uint8_t Reserved0 : 8;
    uint8_t starting_min_q_count : 8;
    uint8_t starting_max_queries_since_valid_epc_count : 8;
};

struct NominalStopTimeFields {
    uint16_t dwell_time : 16;
};

struct ExtendedStopTimeFields {
    uint16_t dwell_time : 16;
};

struct RegulatoryStopTimeFields {
    uint16_t dwell_time : 16;
};

struct Gen2SelectEnableFields {
    uint16_t select_enables : 10;
    int8_t rfu : 6;
};

struct Gen2AccessEnableFields {
    uint16_t access_enables : 10;
    int8_t rfu : 6;
};

struct Gen2AutoAccessEnableFields {
    uint16_t auto_access_enables : 10;
    int8_t rfu : 6;
};

struct Gen2OffsetsFields {
    uint8_t offset : 8;
};

struct Gen2LengthsFields {
    uint16_t length : 16;
};

struct Gen2TransactionIdsFields {
    uint8_t transaction_id : 8;
};

struct Gen2TxnControlsFields {
    uint8_t response_type : 3;
    bool has_header_bit : 1;
    bool use_cover_code : 1;
    bool append_handle : 1;
    bool append_crc16 : 1;
    bool is_kill_command : 1;
    uint8_t Reserved0 : 8;
    uint16_t rx_length : 16;
};

struct DropQueryControlFields {
    uint8_t drop_power : 8;
    uint8_t Reserved0 : 8;
    uint16_t drop_dwell : 16;
};

struct TagFeaturesControlFields {
    bool memory_diagnostic_mode : 1;
    int8_t rfu : 7;
};

struct Gen2TxBufferFields {
    uint8_t* tx_buffer;
};

struct CalibrationInfoFields {
    uint8_t* data;
};


#pragma pack(pop)
// IPJ_autogen }
// clang-format on

#ifdef __cplusplus
}
#endif
