#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2020 - 2021 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################

from ctypes import *
from enum import IntEnum

# There is a bug in ctypes based on machine
# in which this is run where a packed bit field
# of c_bool will not be set properly. Instead,
# we use c_uint8 with a bit limit of 1

# IPJ_autogen | gen_c2python_regs {
class RegisterAccessType(IntEnum):
    ReadOnly = 0
    WriteOnly = 1
    ReadWrite = 2
    Restricted = 3

class RegisterInfo(Structure):
    _fields_ = [('name', c_wchar_p),
                ('address', c_uint16),
                ('length', c_uint16),
                ('num_entries', c_uint8),
                ('access', c_uint8)] # RegisterAccessType

# Structs which break down the fields and sizing within each register
class CommandResultFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('failed_result_code', c_uint8, 8), # enum ResponseCode
        ('failed_command_code', c_uint8, 8), # enum CommandCode
        ('commands_since_first_error', c_uint16, 16),
    ]

class ResetCauseFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('software_reset', c_uint8, 1),
        ('watchdog_timeout', c_uint8, 1),
        ('lockup', c_uint8, 1),
        ('external_reset', c_uint8, 1),
        ('rfu', c_int16, 12),
    ]

class StatusFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('status', c_uint8, 2), # enum Status
        ('rfu', c_int16, 14),
    ]

class VersionStringFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('data', POINTER(c_uint8)),
    ]

class BuildNumberFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('data', POINTER(c_uint8)),
    ]

class GitHashFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('data', POINTER(c_uint8)),
    ]

class TimestampFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('current_timestamp_us', c_uint32, 32),
    ]

class FrefFreqFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('fref_freq_khz', c_uint32, 32),
    ]

class ProductSkuFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('data', POINTER(c_uint8)),
    ]

class SerialNumberFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('data', POINTER(c_uint8)),
    ]

class DeviceInfoFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('eco_revision', c_uint8, 8),
        ('device_revision_lo', c_uint8, 8),
        ('device_revision_hi', c_uint8, 8),
        ('device_identifier', c_uint8, 8),
    ]

class DeviceBuildFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('spar_revision', c_uint8, 8),
        ('rtl_build_number_lo', c_uint8, 8),
        ('rtl_build_number_hi', c_uint8, 8),
        ('rfu', c_int8, 8),
    ]

class RtlRevisionFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('rtl_revision', c_uint32, 32),
    ]

class StackDepthFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('depth', c_uint32, 32),
    ]

class InterruptMaskFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('op_done', c_uint8, 1),
        ('halted', c_uint8, 1),
        ('event_fifo_above_thresh', c_uint8, 1),
        ('event_fifo_full', c_uint8, 1),
        ('inventory_round_done', c_uint8, 1),
        ('halted_sequence_done', c_uint8, 1),
        ('command_error', c_uint8, 1),
        ('aggregate_op_done', c_uint8, 1),
        ('rfu', c_int32, 24),
    ]

class InterruptMaskSetFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('op_done', c_uint8, 1),
        ('halted', c_uint8, 1),
        ('event_fifo_above_thresh', c_uint8, 1),
        ('event_fifo_full', c_uint8, 1),
        ('inventory_round_done', c_uint8, 1),
        ('halted_sequence_done', c_uint8, 1),
        ('command_error', c_uint8, 1),
        ('aggregate_op_done', c_uint8, 1),
        ('rfu', c_int32, 24),
    ]

class InterruptMaskClearFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('op_done', c_uint8, 1),
        ('halted', c_uint8, 1),
        ('event_fifo_above_thresh', c_uint8, 1),
        ('event_fifo_full', c_uint8, 1),
        ('inventory_round_done', c_uint8, 1),
        ('halted_sequence_done', c_uint8, 1),
        ('command_error', c_uint8, 1),
        ('aggregate_op_done', c_uint8, 1),
        ('rfu', c_int32, 24),
    ]

class InterruptStatusFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('op_done', c_uint8, 1),
        ('halted', c_uint8, 1),
        ('event_fifo_above_thresh', c_uint8, 1),
        ('event_fifo_full', c_uint8, 1),
        ('inventory_round_done', c_uint8, 1),
        ('halted_sequence_done', c_uint8, 1),
        ('command_error', c_uint8, 1),
        ('aggregate_op_done', c_uint8, 1),
        ('rfu', c_int32, 24),
    ]

class EventFifoNumBytesFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('num_bytes', c_uint16, 12),
        ('rfu', c_int8, 4),
    ]

class EventFifoIntLevelFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('threshold', c_uint16, 12),
        ('rfu', c_int8, 4),
    ]

class GpioOutputEnableFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('enable_bits', c_uint32, 32),
    ]

class GpioOutputLevelFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('level_bits', c_uint32, 32),
    ]

class PowerControlLoopAuxAdcControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('channel_enable_bits', c_uint16, 15),
        ('rfu', c_int32, 17),
    ]

class PowerControlLoopGainDivisorFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('gain_divisor', c_uint16, 16),
        ('rfu', c_int16, 16),
    ]

class PowerControlLoopMaxIterationsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('max_iterations', c_uint32, 32),
    ]

class PowerControlLoopAdcTargetFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('adc_target_value', c_uint16, 16),
        ('rfu', c_int16, 16),
    ]

class PowerControlLoopAdcThresholdsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('loop_stop_threshold', c_uint16, 16),
        ('op_error_threshold', c_uint16, 16),
    ]

class DelayUsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('delay', c_uint32, 32),
    ]

class GpioOutputLevelSetFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('level_bits_set', c_uint32, 32),
    ]

class GpioOutputLevelClearFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('level_bits_clear', c_uint32, 32),
    ]

class GpioOutputEnableSetFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('enable_bits_set', c_uint32, 32),
    ]

class GpioOutputEnableClearFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('enable_bits_clear', c_uint32, 32),
    ]

class OpsControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('op_id', c_uint8, 8), # enum OpId
    ]

class OpsStatusFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('op_id', c_uint8, 8), # enum OpId
        ('busy', c_uint8, 1),
        ('Reserved0', c_uint8, 7),
        ('error', c_uint8, 8), # enum OpsStatus
        ('rfu', c_int8, 8),
    ]

class HaltedControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('go', c_uint8, 1),
        ('resume', c_uint8, 1),
        ('nak_tag', c_uint8, 1),
        ('rfu', c_int32, 29),
    ]

class HaltedStatusFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('halted', c_uint8, 1),
        ('busy', c_uint8, 1),
        ('rfu_1', c_uint8, 6),
        ('rfu_2', c_uint16, 16),
        ('error', c_uint8, 8), # enum HaltedStatusError
    ]

class LogTestPeriodFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('period', c_uint32, 32),
    ]

class LogTestWordRepeatFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('repeat', c_uint16, 16),
        ('test_type', c_uint8, 1),
        ('rfu', c_int16, 15),
    ]

class EventFifoTestPeriodFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('period', c_uint32, 32),
    ]

class EventFifoTestPayloadNumWordsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('num_words', c_uint8, 8),
        ('rfu', c_int32, 24),
    ]

class LogSpeedFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('speed_mhz', c_uint8, 8),
        ('rfu', c_int8, 8),
    ]

class LogEnablesFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('op_logs', c_uint8, 1),
        ('ramping_logs', c_uint8, 1),
        ('config_logs', c_uint8, 1),
        ('lmac_logs', c_uint8, 1),
        ('sjc_solution_logs', c_uint8, 1),
        ('rf_synth_logs', c_uint8, 1),
        ('power_control_solution_logs', c_uint8, 1),
        ('aux_logs', c_uint8, 1),
        ('regulatory_logs', c_uint8, 1),
        ('command_response_logs', c_uint8, 1),
        ('insert_fifo_event_logs', c_uint8, 1),
        ('host_irq_logs', c_uint8, 1),
        ('timer_start_logs', c_uint8, 1),
        ('timer_wait_logs', c_uint8, 1),
        ('aggregate_op_logs', c_uint8, 1),
        ('read_fifo_logs', c_uint8, 1),
        ('lbt_op_logs', c_uint8, 1),
        ('rfu_1', c_uint8, 7),
        ('rfu_2', c_uint8, 1),
        ('wfe_trace', c_uint8, 1),
        ('rssi_trace_logs', c_uint8, 1),
        ('modem_data_logs', c_uint8, 1),
        ('code_coverage', c_uint8, 1),
        ('lmac_debug_logs', c_uint8, 1),
        ('sjc_trace_logs', c_uint8, 1),
        ('power_control_trace_logs', c_uint8, 1),
    ]

class BerControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('num_bits', c_uint16, 16),
        ('num_packets', c_uint16, 16),
    ]

class BerModeFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('del_only_mode', c_uint8, 1),
        ('rfu', c_int8, 7),
    ]

class HpfOverrideSettingsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('hpf_mode', c_uint8, 8),
        ('rfu', c_int32, 24),
    ]

class AuxAdcControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('channel_enable_bits', c_uint16, 15),
        ('rfu', c_uint8, 1),
    ]

class AuxAdcResultsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('adc_result', c_uint16, 10),
        ('rfu', c_int8, 6),
    ]

class AuxDacControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('channel_enable_bits', c_uint8, 2),
        ('rfu', c_int16, 14),
    ]

class AuxDacSettingsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('value', c_uint16, 10),
        ('rfu', c_int8, 6),
    ]

class ATestMuxFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('a_test_mux', c_uint32, 32),
    ]

class TxFineGainFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('tx_scalar', c_int16, 16),
        ('rfu', c_int16, 16),
    ]

class RxGainControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('rx_atten', c_uint8, 2), # enum RxGainControlRxAtten
        ('pga1_gain', c_uint8, 2), # enum RxGainControlPga1Gain
        ('pga2_gain', c_uint8, 2), # enum RxGainControlPga2Gain
        ('pga3_gain', c_uint8, 2), # enum RxGainControlPga3Gain
        ('Reserved0', c_uint8, 2),
        ('mixer_gain', c_uint8, 2), # enum RxGainControlMixerGain
        ('pga1_rin_select', c_uint8, 1),
        ('Reserved1', c_uint8, 1),
        ('mixer_bandwidth', c_uint8, 1),
        ('rfu', c_uint8, 1),
    ]

class TxCoarseGainFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('tx_atten', c_uint8, 5),
        ('rfu', c_int32, 27),
    ]

class RfModeFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('id', c_uint16, 16),
        ('rfu', c_int16, 16),
    ]

class DcOffsetFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('offset', c_int32, 20),
        ('rfu', c_int16, 12),
    ]

class EtsiBurstOffTimeFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('off_time', c_uint16, 16),
        ('rfu', c_int16, 16),
    ]

class CwIsOnFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('is_on', c_uint8, 1),
        ('rfu', c_int32, 31),
    ]

class MeasureRssiCountFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('samples', c_uint8, 4),
        ('rfu', c_int16, 12),
    ]

class MeasuredRssiLinearFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('value', c_uint32, 32),
    ]

class MeasuredRssiLog2Fields(Structure):
    _pack_ = 1
    _fields_ = [
        ('value', c_uint16, 16),
    ]

class LbtOffsetFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('khz', c_int32, 32),
    ]

class LbtControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('override', c_uint8, 1),
        ('narrow_bandwidth_mode', c_uint8, 1),
        ('Reserved0', c_uint8, 6),
        ('num_rssi_measurements', c_uint8, 8),
        ('measurement_delay_us', c_uint16, 16),
    ]

class RfSynthesizerControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('n_divider', c_uint16, 16),
        ('r_divider', c_uint8, 3),
        ('Reserved0', c_uint8, 5),
        ('lf_type', c_uint8, 1),
        ('rfu', c_int8, 7),
    ]

class SjcControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('sample_average_coarse', c_uint8, 4),
        ('sample_average_fine', c_uint8, 4),
        ('events_enable', c_uint8, 1),
        ('fixed_rx_atten', c_uint8, 1),
        ('decimator', c_uint8, 3),
        ('rfu', c_int8, 3),
    ]

class SjcGainControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('rx_atten', c_uint8, 2), # enum RxGainControlRxAtten
        ('pga1_gain', c_uint8, 2), # enum RxGainControlPga1Gain
        ('pga2_gain', c_uint8, 2), # enum RxGainControlPga2Gain
        ('pga3_gain', c_uint8, 2), # enum RxGainControlPga3Gain
        ('Reserved0', c_uint8, 2),
        ('mixer_gain', c_uint8, 2), # enum RxGainControlMixerGain
        ('pga1_rin_select', c_uint8, 1),
        ('Reserved1', c_uint8, 1),
        ('mixer_bandwidth', c_uint8, 1),
        ('rfu', c_int32, 17),
    ]

class SjcInitialSettlingTimeFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('settling_time', c_uint16, 16),
    ]

class SjcResidueSettlingTimeFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('settling_time', c_uint16, 16),
    ]

class SjcCdacIFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('center', c_int8, 8),
        ('limit', c_uint8, 8),
        ('step_size', c_uint8, 8),
        ('rfu', c_int8, 8),
    ]

class SjcCdacQFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('center', c_int8, 8),
        ('limit', c_uint8, 8),
        ('step_size', c_uint8, 8),
        ('rfu', c_int8, 8),
    ]

class SjcResultFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('cdac', c_int8, 8),
        ('cdac_sku_limited', c_uint8, 1),
        ('Reserved0', c_uint8, 3),
        ('residue', c_int32, 20),
    ]

class SjcResidueThresholdFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('magnitude', c_uint16, 16),
    ]

class AnalogEnableFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('all', c_uint8, 1),
        ('rfu', c_int32, 31),
    ]

class AggregateOpBufferFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('command_buffer', POINTER(c_uint8)),
    ]

class PowerDroopCompensationFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('enable', c_uint8, 1),
        ('rfu', c_uint16, 15),
        ('compensation_interval_ms', c_uint8, 8),
        ('fine_gain_step_cd_b', c_uint8, 8),
    ]

class LastTxRampUpTimeMsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('time_ms', c_uint32, 32),
    ]

class LastTxRampUpLoFreqKhzFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('lo_freq_khz', c_uint32, 32),
    ]

class LastTxRampDownTimeMsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('time_ms', c_uint32, 32),
    ]

class LastTxRampDownLoFreqKhzFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('lo_freq_khz', c_uint32, 32),
    ]

class LastTxRampDownReasonFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('reason', c_uint8, 8),
    ]

class RssiThresholdRn16Fields(Structure):
    _pack_ = 1
    _fields_ = [
        ('threshold', c_uint16, 16),
    ]

class RssiThresholdEpcFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('threshold', c_uint16, 16),
    ]

class InventoryRoundControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('initial_q', c_uint8, 4),
        ('max_q', c_uint8, 4),
        ('min_q', c_uint8, 4),
        ('num_min_q_cycles', c_uint8, 4),
        ('fixed_q_mode', c_uint8, 1),
        ('q_increase_use_query', c_uint8, 1),
        ('q_decrease_use_query', c_uint8, 1),
        ('session', c_uint8, 2), # enum InventoryRoundControlSession
        ('select', c_uint8, 2),
        ('target', c_uint8, 1),
        ('halt_on_all_tags', c_uint8, 1),
        ('fast_id_enable', c_uint8, 1),
        ('tag_focus_enable', c_uint8, 1),
        ('auto_access', c_uint8, 1),
        ('abort_on_fail', c_uint8, 1),
        ('halt_on_fail', c_uint8, 1),
        ('always_ack', c_uint8, 1),
        ('rfu', c_uint8, 1),
    ]

class InventoryRoundControl_2Fields(Structure):
    _pack_ = 1
    _fields_ = [
        ('max_queries_since_valid_epc', c_uint8, 8),
        ('Reserved0', c_uint8, 8),
        ('starting_min_q_count', c_uint8, 8),
        ('starting_max_queries_since_valid_epc_count', c_uint8, 8),
    ]

class NominalStopTimeFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('dwell_time', c_uint16, 16),
    ]

class ExtendedStopTimeFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('dwell_time', c_uint16, 16),
    ]

class RegulatoryStopTimeFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('dwell_time', c_uint16, 16),
    ]

class Gen2SelectEnableFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('select_enables', c_uint16, 10),
        ('rfu', c_int8, 6),
    ]

class Gen2AccessEnableFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('access_enables', c_uint16, 10),
        ('rfu', c_int8, 6),
    ]

class Gen2AutoAccessEnableFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('auto_access_enables', c_uint16, 10),
        ('rfu', c_int8, 6),
    ]

class Gen2OffsetsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('offset', c_uint8, 8),
    ]

class Gen2LengthsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('length', c_uint16, 16),
    ]

class Gen2TransactionIdsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('transaction_id', c_uint8, 8),
    ]

class Gen2TxnControlsFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('response_type', c_uint8, 3),
        ('has_header_bit', c_uint8, 1),
        ('use_cover_code', c_uint8, 1),
        ('append_handle', c_uint8, 1),
        ('append_crc16', c_uint8, 1),
        ('is_kill_command', c_uint8, 1),
        ('Reserved0', c_uint8, 8),
        ('rx_length', c_uint16, 16),
    ]

class DropQueryControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('drop_power', c_uint8, 8),
        ('Reserved0', c_uint8, 8),
        ('drop_dwell', c_uint16, 16),
    ]

class TagFeaturesControlFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('memory_diagnostic_mode', c_uint8, 1),
        ('rfu', c_int8, 7),
    ]

class Gen2TxBufferFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('tx_buffer', POINTER(c_uint8)),
    ]

class CalibrationInfoFields(Structure):
    _pack_ = 1
    _fields_ = [
        ('data', POINTER(c_uint8)),
    ]
# IPJ_autogen }
