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

// Data structure definition and initialization
// for all application registers. This contains the basic
// register info and a pointer to the register data.

#pragma once

#include <stddef.h>

#include "application_register_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
// IPJ_autogen | gen_c_app_ex10_api_reg_instances {

static struct RegisterInfo const command_result_reg = {
    .name = "CommandResult",
    .address = 0x0000,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define COMMAND_RESULT_REG_LENGTH  ((size_t)4u)
#define COMMAND_RESULT_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const reset_cause_reg = {
    .name = "ResetCause",
    .address = 0x0004,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadOnly,
};
#define RESET_CAUSE_REG_LENGTH  ((size_t)2u)
#define RESET_CAUSE_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const status_reg = {
    .name = "Status",
    .address = 0x0006,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadOnly,
};
#define STATUS_REG_LENGTH  ((size_t)2u)
#define STATUS_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const version_string_reg = {
    .name = "VersionString",
    .address = 0x0008,
    .length = 0x0020,
    .num_entries = 1,
    .access = ReadOnly,
};
#define VERSION_STRING_REG_LENGTH  ((size_t)32u)
#define VERSION_STRING_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const build_number_reg = {
    .name = "BuildNumber",
    .address = 0x0028,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define BUILD_NUMBER_REG_LENGTH  ((size_t)4u)
#define BUILD_NUMBER_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const git_hash_reg = {
    .name = "GitHash",
    .address = 0x002C,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define GIT_HASH_REG_LENGTH  ((size_t)4u)
#define GIT_HASH_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const timestamp_reg = {
    .name = "Timestamp",
    .address = 0x0030,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define TIMESTAMP_REG_LENGTH  ((size_t)4u)
#define TIMESTAMP_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const fref_freq_reg = {
    .name = "FrefFreq",
    .address = 0x0034,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define FREF_FREQ_REG_LENGTH  ((size_t)4u)
#define FREF_FREQ_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const product_sku_reg = {
    .name = "ProductSku",
    .address = 0x0068,
    .length = 0x0008,
    .num_entries = 1,
    .access = ReadOnly,
};
#define PRODUCT_SKU_REG_LENGTH  ((size_t)8u)
#define PRODUCT_SKU_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const serial_number_reg = {
    .name = "SerialNumber",
    .address = 0x0070,
    .length = 0x0020,
    .num_entries = 1,
    .access = ReadOnly,
};
#define SERIAL_NUMBER_REG_LENGTH  ((size_t)32u)
#define SERIAL_NUMBER_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const device_info_reg = {
    .name = "DeviceInfo",
    .address = 0x0090,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define DEVICE_INFO_REG_LENGTH  ((size_t)4u)
#define DEVICE_INFO_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const device_build_reg = {
    .name = "DeviceBuild",
    .address = 0x0094,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define DEVICE_BUILD_REG_LENGTH  ((size_t)4u)
#define DEVICE_BUILD_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const rtl_revision_reg = {
    .name = "RtlRevision",
    .address = 0x0098,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define RTL_REVISION_REG_LENGTH  ((size_t)4u)
#define RTL_REVISION_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const stack_depth_reg = {
    .name = "StackDepth",
    .address = 0x009C,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define STACK_DEPTH_REG_LENGTH  ((size_t)4u)
#define STACK_DEPTH_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const interrupt_mask_reg = {
    .name = "InterruptMask",
    .address = 0x00A0,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define INTERRUPT_MASK_REG_LENGTH  ((size_t)4u)
#define INTERRUPT_MASK_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const interrupt_mask_set_reg = {
    .name = "InterruptMaskSet",
    .address = 0x00A4,
    .length = 0x0004,
    .num_entries = 1,
    .access = WriteOnly,
};
#define INTERRUPT_MASK_SET_REG_LENGTH  ((size_t)4u)
#define INTERRUPT_MASK_SET_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const interrupt_mask_clear_reg = {
    .name = "InterruptMaskClear",
    .address = 0x00A8,
    .length = 0x0004,
    .num_entries = 1,
    .access = WriteOnly,
};
#define INTERRUPT_MASK_CLEAR_REG_LENGTH  ((size_t)4u)
#define INTERRUPT_MASK_CLEAR_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const interrupt_status_reg = {
    .name = "InterruptStatus",
    .address = 0x00AC,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define INTERRUPT_STATUS_REG_LENGTH  ((size_t)4u)
#define INTERRUPT_STATUS_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const event_fifo_num_bytes_reg = {
    .name = "EventFifoNumBytes",
    .address = 0x00B0,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadOnly,
};
#define EVENT_FIFO_NUM_BYTES_REG_LENGTH  ((size_t)2u)
#define EVENT_FIFO_NUM_BYTES_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const event_fifo_int_level_reg = {
    .name = "EventFifoIntLevel",
    .address = 0x00B2,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define EVENT_FIFO_INT_LEVEL_REG_LENGTH  ((size_t)2u)
#define EVENT_FIFO_INT_LEVEL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const gpio_output_enable_reg = {
    .name = "GpioOutputEnable",
    .address = 0x00B4,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define GPIO_OUTPUT_ENABLE_REG_LENGTH  ((size_t)4u)
#define GPIO_OUTPUT_ENABLE_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const gpio_output_level_reg = {
    .name = "GpioOutputLevel",
    .address = 0x00B8,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define GPIO_OUTPUT_LEVEL_REG_LENGTH  ((size_t)4u)
#define GPIO_OUTPUT_LEVEL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const power_control_loop_aux_adc_control_reg = {
    .name = "PowerControlLoopAuxAdcControl",
    .address = 0x00BC,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define POWER_CONTROL_LOOP_AUX_ADC_CONTROL_REG_LENGTH  ((size_t)4u)
#define POWER_CONTROL_LOOP_AUX_ADC_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const power_control_loop_gain_divisor_reg = {
    .name = "PowerControlLoopGainDivisor",
    .address = 0x00C0,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define POWER_CONTROL_LOOP_GAIN_DIVISOR_REG_LENGTH  ((size_t)4u)
#define POWER_CONTROL_LOOP_GAIN_DIVISOR_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const power_control_loop_max_iterations_reg = {
    .name = "PowerControlLoopMaxIterations",
    .address = 0x00C4,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define POWER_CONTROL_LOOP_MAX_ITERATIONS_REG_LENGTH  ((size_t)4u)
#define POWER_CONTROL_LOOP_MAX_ITERATIONS_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const power_control_loop_adc_target_reg = {
    .name = "PowerControlLoopAdcTarget",
    .address = 0x00CC,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define POWER_CONTROL_LOOP_ADC_TARGET_REG_LENGTH  ((size_t)4u)
#define POWER_CONTROL_LOOP_ADC_TARGET_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const power_control_loop_adc_thresholds_reg = {
    .name = "PowerControlLoopAdcThresholds",
    .address = 0x00D0,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define POWER_CONTROL_LOOP_ADC_THRESHOLDS_REG_LENGTH  ((size_t)4u)
#define POWER_CONTROL_LOOP_ADC_THRESHOLDS_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const delay_us_reg = {
    .name = "DelayUs",
    .address = 0x00D4,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define DELAY_US_REG_LENGTH  ((size_t)4u)
#define DELAY_US_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const gpio_output_level_set_reg = {
    .name = "GpioOutputLevelSet",
    .address = 0x00E0,
    .length = 0x0004,
    .num_entries = 1,
    .access = WriteOnly,
};
#define GPIO_OUTPUT_LEVEL_SET_REG_LENGTH  ((size_t)4u)
#define GPIO_OUTPUT_LEVEL_SET_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const gpio_output_level_clear_reg = {
    .name = "GpioOutputLevelClear",
    .address = 0x00E4,
    .length = 0x0004,
    .num_entries = 1,
    .access = WriteOnly,
};
#define GPIO_OUTPUT_LEVEL_CLEAR_REG_LENGTH  ((size_t)4u)
#define GPIO_OUTPUT_LEVEL_CLEAR_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const gpio_output_enable_set_reg = {
    .name = "GpioOutputEnableSet",
    .address = 0x00E8,
    .length = 0x0004,
    .num_entries = 1,
    .access = WriteOnly,
};
#define GPIO_OUTPUT_ENABLE_SET_REG_LENGTH  ((size_t)4u)
#define GPIO_OUTPUT_ENABLE_SET_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const gpio_output_enable_clear_reg = {
    .name = "GpioOutputEnableClear",
    .address = 0x00EC,
    .length = 0x0004,
    .num_entries = 1,
    .access = WriteOnly,
};
#define GPIO_OUTPUT_ENABLE_CLEAR_REG_LENGTH  ((size_t)4u)
#define GPIO_OUTPUT_ENABLE_CLEAR_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const ops_control_reg = {
    .name = "OpsControl",
    .address = 0x0300,
    .length = 0x0001,
    .num_entries = 1,
    .access = ReadWrite,
};
#define OPS_CONTROL_REG_LENGTH  ((size_t)1u)
#define OPS_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const ops_status_reg = {
    .name = "OpsStatus",
    .address = 0x0304,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define OPS_STATUS_REG_LENGTH  ((size_t)4u)
#define OPS_STATUS_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const halted_control_reg = {
    .name = "HaltedControl",
    .address = 0x0308,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define HALTED_CONTROL_REG_LENGTH  ((size_t)4u)
#define HALTED_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const halted_status_reg = {
    .name = "HaltedStatus",
    .address = 0x030C,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define HALTED_STATUS_REG_LENGTH  ((size_t)4u)
#define HALTED_STATUS_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const log_test_period_reg = {
    .name = "LogTestPeriod",
    .address = 0x0320,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define LOG_TEST_PERIOD_REG_LENGTH  ((size_t)4u)
#define LOG_TEST_PERIOD_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const log_test_word_repeat_reg = {
    .name = "LogTestWordRepeat",
    .address = 0x0324,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define LOG_TEST_WORD_REPEAT_REG_LENGTH  ((size_t)4u)
#define LOG_TEST_WORD_REPEAT_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const event_fifo_test_period_reg = {
    .name = "EventFifoTestPeriod",
    .address = 0x0328,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define EVENT_FIFO_TEST_PERIOD_REG_LENGTH  ((size_t)4u)
#define EVENT_FIFO_TEST_PERIOD_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const event_fifo_test_payload_num_words_reg = {
    .name = "EventFifoTestPayloadNumWords",
    .address = 0x032C,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define EVENT_FIFO_TEST_PAYLOAD_NUM_WORDS_REG_LENGTH  ((size_t)4u)
#define EVENT_FIFO_TEST_PAYLOAD_NUM_WORDS_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const log_speed_reg = {
    .name = "LogSpeed",
    .address = 0x0330,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define LOG_SPEED_REG_LENGTH  ((size_t)2u)
#define LOG_SPEED_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const log_enables_reg = {
    .name = "LogEnables",
    .address = 0x0334,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define LOG_ENABLES_REG_LENGTH  ((size_t)4u)
#define LOG_ENABLES_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const ber_control_reg = {
    .name = "BerControl",
    .address = 0x0338,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define BER_CONTROL_REG_LENGTH  ((size_t)4u)
#define BER_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const ber_mode_reg = {
    .name = "BerMode",
    .address = 0x033C,
    .length = 0x0001,
    .num_entries = 1,
    .access = ReadWrite,
};
#define BER_MODE_REG_LENGTH  ((size_t)1u)
#define BER_MODE_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const hpf_override_settings_reg = {
    .name = "HpfOverrideSettings",
    .address = 0x0344,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define HPF_OVERRIDE_SETTINGS_REG_LENGTH  ((size_t)4u)
#define HPF_OVERRIDE_SETTINGS_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const aux_adc_control_reg = {
    .name = "AuxAdcControl",
    .address = 0x0400,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define AUX_ADC_CONTROL_REG_LENGTH  ((size_t)2u)
#define AUX_ADC_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const aux_adc_results_reg = {
    .name = "AuxAdcResults",
    .address = 0x0404,
    .length = 0x0002,
    .num_entries = 15,
    .access = ReadOnly,
};
#define AUX_ADC_RESULTS_REG_LENGTH  ((size_t)2u)
#define AUX_ADC_RESULTS_REG_ENTRIES ((size_t)15u)
static struct RegisterInfo const aux_dac_control_reg = {
    .name = "AuxDacControl",
    .address = 0x0430,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define AUX_DAC_CONTROL_REG_LENGTH  ((size_t)2u)
#define AUX_DAC_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const aux_dac_settings_reg = {
    .name = "AuxDacSettings",
    .address = 0x0432,
    .length = 0x0002,
    .num_entries = 2,
    .access = ReadWrite,
};
#define AUX_DAC_SETTINGS_REG_LENGTH  ((size_t)2u)
#define AUX_DAC_SETTINGS_REG_ENTRIES ((size_t)2u)
static struct RegisterInfo const a_test_mux_reg = {
    .name = "ATestMux",
    .address = 0x0440,
    .length = 0x0004,
    .num_entries = 4,
    .access = ReadWrite,
};
#define A_TEST_MUX_REG_LENGTH  ((size_t)4u)
#define A_TEST_MUX_REG_ENTRIES ((size_t)4u)
static struct RegisterInfo const tx_fine_gain_reg = {
    .name = "TxFineGain",
    .address = 0x0504,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define TX_FINE_GAIN_REG_LENGTH  ((size_t)4u)
#define TX_FINE_GAIN_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const rx_gain_control_reg = {
    .name = "RxGainControl",
    .address = 0x0508,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define RX_GAIN_CONTROL_REG_LENGTH  ((size_t)2u)
#define RX_GAIN_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const tx_coarse_gain_reg = {
    .name = "TxCoarseGain",
    .address = 0x050C,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define TX_COARSE_GAIN_REG_LENGTH  ((size_t)4u)
#define TX_COARSE_GAIN_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const rf_mode_reg = {
    .name = "RfMode",
    .address = 0x0514,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define RF_MODE_REG_LENGTH  ((size_t)4u)
#define RF_MODE_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const dc_offset_reg = {
    .name = "DcOffset",
    .address = 0x0518,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define DC_OFFSET_REG_LENGTH  ((size_t)4u)
#define DC_OFFSET_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const etsi_burst_off_time_reg = {
    .name = "EtsiBurstOffTime",
    .address = 0x051C,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define ETSI_BURST_OFF_TIME_REG_LENGTH  ((size_t)4u)
#define ETSI_BURST_OFF_TIME_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const cw_is_on_reg = {
    .name = "CwIsOn",
    .address = 0x0520,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define CW_IS_ON_REG_LENGTH  ((size_t)4u)
#define CW_IS_ON_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const measure_rssi_count_reg = {
    .name = "MeasureRssiCount",
    .address = 0x0528,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define MEASURE_RSSI_COUNT_REG_LENGTH  ((size_t)2u)
#define MEASURE_RSSI_COUNT_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const measured_rssi_linear_reg = {
    .name = "MeasuredRssiLinear",
    .address = 0x0540,
    .length = 0x0004,
    .num_entries = 5,
    .access = ReadOnly,
};
#define MEASURED_RSSI_LINEAR_REG_LENGTH  ((size_t)4u)
#define MEASURED_RSSI_LINEAR_REG_ENTRIES ((size_t)5u)
static struct RegisterInfo const measured_rssi_log2_reg = {
    .name = "MeasuredRssiLog2",
    .address = 0x0554,
    .length = 0x0002,
    .num_entries = 5,
    .access = ReadOnly,
};
#define MEASURED_RSSI_LOG2_REG_LENGTH  ((size_t)2u)
#define MEASURED_RSSI_LOG2_REG_ENTRIES ((size_t)5u)
static struct RegisterInfo const lbt_offset_reg = {
    .name = "LbtOffset",
    .address = 0x0560,
    .length = 0x0004,
    .num_entries = 5,
    .access = ReadWrite,
};
#define LBT_OFFSET_REG_LENGTH  ((size_t)4u)
#define LBT_OFFSET_REG_ENTRIES ((size_t)5u)
static struct RegisterInfo const lbt_control_reg = {
    .name = "LbtControl",
    .address = 0x0574,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define LBT_CONTROL_REG_LENGTH  ((size_t)4u)
#define LBT_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const rf_synthesizer_control_reg = {
    .name = "RfSynthesizerControl",
    .address = 0x0588,
    .length = 0x0004,
    .num_entries = 5,
    .access = ReadWrite,
};
#define RF_SYNTHESIZER_CONTROL_REG_LENGTH  ((size_t)4u)
#define RF_SYNTHESIZER_CONTROL_REG_ENTRIES ((size_t)5u)
static struct RegisterInfo const sjc_control_reg = {
    .name = "SjcControl",
    .address = 0x0600,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define SJC_CONTROL_REG_LENGTH  ((size_t)2u)
#define SJC_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const sjc_gain_control_reg = {
    .name = "SjcGainControl",
    .address = 0x0604,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define SJC_GAIN_CONTROL_REG_LENGTH  ((size_t)4u)
#define SJC_GAIN_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const sjc_initial_settling_time_reg = {
    .name = "SjcInitialSettlingTime",
    .address = 0x0608,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define SJC_INITIAL_SETTLING_TIME_REG_LENGTH  ((size_t)2u)
#define SJC_INITIAL_SETTLING_TIME_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const sjc_residue_settling_time_reg = {
    .name = "SjcResidueSettlingTime",
    .address = 0x060C,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define SJC_RESIDUE_SETTLING_TIME_REG_LENGTH  ((size_t)2u)
#define SJC_RESIDUE_SETTLING_TIME_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const sjc_cdac_i_reg = {
    .name = "SjcCdacI",
    .address = 0x0610,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define SJC_CDAC_I_REG_LENGTH  ((size_t)4u)
#define SJC_CDAC_I_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const sjc_cdac_q_reg = {
    .name = "SjcCdacQ",
    .address = 0x0614,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define SJC_CDAC_Q_REG_LENGTH  ((size_t)4u)
#define SJC_CDAC_Q_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const sjc_result_i_reg = {
    .name = "SjcResultI",
    .address = 0x0618,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define SJC_RESULT_I_REG_LENGTH  ((size_t)4u)
#define SJC_RESULT_I_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const sjc_result_q_reg = {
    .name = "SjcResultQ",
    .address = 0x061C,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadOnly,
};
#define SJC_RESULT_Q_REG_LENGTH  ((size_t)4u)
#define SJC_RESULT_Q_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const sjc_residue_threshold_reg = {
    .name = "SjcResidueThreshold",
    .address = 0x0620,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define SJC_RESIDUE_THRESHOLD_REG_LENGTH  ((size_t)2u)
#define SJC_RESIDUE_THRESHOLD_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const analog_enable_reg = {
    .name = "AnalogEnable",
    .address = 0x0700,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define ANALOG_ENABLE_REG_LENGTH  ((size_t)4u)
#define ANALOG_ENABLE_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const aggregate_op_buffer_reg = {
    .name = "AggregateOpBuffer",
    .address = 0x0704,
    .length = 0x0200,
    .num_entries = 1,
    .access = ReadWrite,
};
#define AGGREGATE_OP_BUFFER_REG_LENGTH  ((size_t)512u)
#define AGGREGATE_OP_BUFFER_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const power_droop_compensation_reg = {
    .name = "PowerDroopCompensation",
    .address = 0x0904,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define POWER_DROOP_COMPENSATION_REG_LENGTH  ((size_t)4u)
#define POWER_DROOP_COMPENSATION_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const last_tx_ramp_up_time_ms_reg = {
    .name = "LastTxRampUpTimeMs",
    .address = 0x0908,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define LAST_TX_RAMP_UP_TIME_MS_REG_LENGTH  ((size_t)4u)
#define LAST_TX_RAMP_UP_TIME_MS_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const last_tx_ramp_up_lo_freq_khz_reg = {
    .name = "LastTxRampUpLoFreqKhz",
    .address = 0x090C,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define LAST_TX_RAMP_UP_LO_FREQ_KHZ_REG_LENGTH  ((size_t)4u)
#define LAST_TX_RAMP_UP_LO_FREQ_KHZ_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const last_tx_ramp_down_time_ms_reg = {
    .name = "LastTxRampDownTimeMs",
    .address = 0x0910,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define LAST_TX_RAMP_DOWN_TIME_MS_REG_LENGTH  ((size_t)4u)
#define LAST_TX_RAMP_DOWN_TIME_MS_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const last_tx_ramp_down_lo_freq_khz_reg = {
    .name = "LastTxRampDownLoFreqKhz",
    .address = 0x0914,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define LAST_TX_RAMP_DOWN_LO_FREQ_KHZ_REG_LENGTH  ((size_t)4u)
#define LAST_TX_RAMP_DOWN_LO_FREQ_KHZ_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const last_tx_ramp_down_reason_reg = {
    .name = "LastTxRampDownReason",
    .address = 0x0918,
    .length = 0x0001,
    .num_entries = 1,
    .access = ReadWrite,
};
#define LAST_TX_RAMP_DOWN_REASON_REG_LENGTH  ((size_t)1u)
#define LAST_TX_RAMP_DOWN_REASON_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const rssi_threshold_rn16_reg = {
    .name = "RssiThresholdRn16",
    .address = 0x0FFC,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define RSSI_THRESHOLD_RN16_REG_LENGTH  ((size_t)2u)
#define RSSI_THRESHOLD_RN16_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const rssi_threshold_epc_reg = {
    .name = "RssiThresholdEpc",
    .address = 0x0FFE,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define RSSI_THRESHOLD_EPC_REG_LENGTH  ((size_t)2u)
#define RSSI_THRESHOLD_EPC_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const inventory_round_control_reg = {
    .name = "InventoryRoundControl",
    .address = 0x1000,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define INVENTORY_ROUND_CONTROL_REG_LENGTH  ((size_t)4u)
#define INVENTORY_ROUND_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const inventory_round_control_2_reg = {
    .name = "InventoryRoundControl_2",
    .address = 0x1004,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define INVENTORY_ROUND_CONTROL_2_REG_LENGTH  ((size_t)4u)
#define INVENTORY_ROUND_CONTROL_2_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const nominal_stop_time_reg = {
    .name = "NominalStopTime",
    .address = 0x1008,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define NOMINAL_STOP_TIME_REG_LENGTH  ((size_t)2u)
#define NOMINAL_STOP_TIME_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const extended_stop_time_reg = {
    .name = "ExtendedStopTime",
    .address = 0x100C,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define EXTENDED_STOP_TIME_REG_LENGTH  ((size_t)2u)
#define EXTENDED_STOP_TIME_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const regulatory_stop_time_reg = {
    .name = "RegulatoryStopTime",
    .address = 0x1010,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define REGULATORY_STOP_TIME_REG_LENGTH  ((size_t)2u)
#define REGULATORY_STOP_TIME_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const gen2_select_enable_reg = {
    .name = "Gen2SelectEnable",
    .address = 0x1014,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define GEN2_SELECT_ENABLE_REG_LENGTH  ((size_t)2u)
#define GEN2_SELECT_ENABLE_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const gen2_access_enable_reg = {
    .name = "Gen2AccessEnable",
    .address = 0x1018,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define GEN2_ACCESS_ENABLE_REG_LENGTH  ((size_t)2u)
#define GEN2_ACCESS_ENABLE_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const gen2_auto_access_enable_reg = {
    .name = "Gen2AutoAccessEnable",
    .address = 0x101C,
    .length = 0x0002,
    .num_entries = 1,
    .access = ReadWrite,
};
#define GEN2_AUTO_ACCESS_ENABLE_REG_LENGTH  ((size_t)2u)
#define GEN2_AUTO_ACCESS_ENABLE_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const gen2_offsets_reg = {
    .name = "Gen2Offsets",
    .address = 0x1020,
    .length = 0x0001,
    .num_entries = 10,
    .access = ReadWrite,
};
#define GEN2_OFFSETS_REG_LENGTH  ((size_t)1u)
#define GEN2_OFFSETS_REG_ENTRIES ((size_t)10u)
static struct RegisterInfo const gen2_lengths_reg = {
    .name = "Gen2Lengths",
    .address = 0x1030,
    .length = 0x0002,
    .num_entries = 10,
    .access = ReadWrite,
};
#define GEN2_LENGTHS_REG_LENGTH  ((size_t)2u)
#define GEN2_LENGTHS_REG_ENTRIES ((size_t)10u)
static struct RegisterInfo const gen2_transaction_ids_reg = {
    .name = "Gen2TransactionIds",
    .address = 0x1050,
    .length = 0x0001,
    .num_entries = 10,
    .access = ReadWrite,
};
#define GEN2_TRANSACTION_IDS_REG_LENGTH  ((size_t)1u)
#define GEN2_TRANSACTION_IDS_REG_ENTRIES ((size_t)10u)
static struct RegisterInfo const gen2_txn_controls_reg = {
    .name = "Gen2TxnControls",
    .address = 0x1060,
    .length = 0x0004,
    .num_entries = 10,
    .access = ReadWrite,
};
#define GEN2_TXN_CONTROLS_REG_LENGTH  ((size_t)4u)
#define GEN2_TXN_CONTROLS_REG_ENTRIES ((size_t)10u)
static struct RegisterInfo const drop_query_control_reg = {
    .name = "DropQueryControl",
    .address = 0x1090,
    .length = 0x0004,
    .num_entries = 1,
    .access = ReadWrite,
};
#define DROP_QUERY_CONTROL_REG_LENGTH  ((size_t)4u)
#define DROP_QUERY_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const tag_features_control_reg = {
    .name = "TagFeaturesControl",
    .address = 0x1094,
    .length = 0x0001,
    .num_entries = 1,
    .access = ReadWrite,
};
#define TAG_FEATURES_CONTROL_REG_LENGTH  ((size_t)1u)
#define TAG_FEATURES_CONTROL_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const gen2_tx_buffer_reg = {
    .name = "Gen2TxBuffer",
    .address = 0x1100,
    .length = 0x0080,
    .num_entries = 1,
    .access = ReadWrite,
};
#define GEN2_TX_BUFFER_REG_LENGTH  ((size_t)128u)
#define GEN2_TX_BUFFER_REG_ENTRIES ((size_t)1u)
static struct RegisterInfo const calibration_info_reg = {
    .name = "CalibrationInfo",
    .address = 0xE800,
    .length = 0x0800,
    .num_entries = 1,
    .access = ReadOnly,
};
#define CALIBRATION_INFO_REG_LENGTH  ((size_t)2048u)
#define CALIBRATION_INFO_REG_ENTRIES ((size_t)1u)
// IPJ_autogen }
// clang-format on

/** Print the contents of all Ex10 registers.
 *
 */
void ex10_dump_all_registers(void);

/** Search for an Ex10 application register by address.
 *
 * @return NULL if no register found by the provided address, otherwise returns
 * a pointer to a RegisterInfo struct.
 */
struct RegisterInfo const* ex10_register_lookup_by_addr(uint16_t const addr);

/** Search for Ex10 application register by name.
 *
 * Finds the offset and length for a register based on a provided
 * register name. The search is case sensitive, so name="RfMode" will work but
 * name="rfmode" will not. This is an O(n) lookup.
 *
 *
 * @return NULL if no register found by the provided name, otherwise returns a
 * pointer to a RegisterInfo struct.
 */
struct RegisterInfo const* ex10_register_lookup_by_name(char const* const name);

#ifdef __cplusplus
}
#endif
