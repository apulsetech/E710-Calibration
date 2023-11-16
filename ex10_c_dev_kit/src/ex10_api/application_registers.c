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

#include <string.h>

#include "ex10_api/application_registers.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/print_data.h"

// clang-format off
// IPJ_autogen | gen_c_app_ex10_api_reg_mapping {
static struct RegisterInfo const* const all_regs[] = {
    &command_result_reg,
    &reset_cause_reg,
    &status_reg,
    &version_string_reg,
    &build_number_reg,
    &git_hash_reg,
    &timestamp_reg,
    &fref_freq_reg,
    &product_sku_reg,
    &serial_number_reg,
    &device_info_reg,
    &device_build_reg,
    &rtl_revision_reg,
    &stack_depth_reg,
    &interrupt_mask_reg,
    &interrupt_mask_set_reg,
    &interrupt_mask_clear_reg,
    &interrupt_status_reg,
    &event_fifo_num_bytes_reg,
    &event_fifo_int_level_reg,
    &gpio_output_enable_reg,
    &gpio_output_level_reg,
    &power_control_loop_aux_adc_control_reg,
    &power_control_loop_gain_divisor_reg,
    &power_control_loop_max_iterations_reg,
    &power_control_loop_adc_target_reg,
    &power_control_loop_adc_thresholds_reg,
    &delay_us_reg,
    &gpio_output_level_set_reg,
    &gpio_output_level_clear_reg,
    &gpio_output_enable_set_reg,
    &gpio_output_enable_clear_reg,
    &ops_control_reg,
    &ops_status_reg,
    &halted_control_reg,
    &halted_status_reg,
    &log_test_period_reg,
    &log_test_word_repeat_reg,
    &event_fifo_test_period_reg,
    &event_fifo_test_payload_num_words_reg,
    &log_speed_reg,
    &log_enables_reg,
    &ber_control_reg,
    &ber_mode_reg,
    &hpf_override_settings_reg,
    &aux_adc_control_reg,
    &aux_adc_results_reg,
    &aux_dac_control_reg,
    &aux_dac_settings_reg,
    &a_test_mux_reg,
    &tx_fine_gain_reg,
    &rx_gain_control_reg,
    &tx_coarse_gain_reg,
    &rf_mode_reg,
    &dc_offset_reg,
    &etsi_burst_off_time_reg,
    &cw_is_on_reg,
    &measure_rssi_count_reg,
    &measured_rssi_linear_reg,
    &measured_rssi_log2_reg,
    &lbt_offset_reg,
    &lbt_control_reg,
    &rf_synthesizer_control_reg,
    &sjc_control_reg,
    &sjc_gain_control_reg,
    &sjc_initial_settling_time_reg,
    &sjc_residue_settling_time_reg,
    &sjc_cdac_i_reg,
    &sjc_cdac_q_reg,
    &sjc_result_i_reg,
    &sjc_result_q_reg,
    &sjc_residue_threshold_reg,
    &analog_enable_reg,
    &aggregate_op_buffer_reg,
    &power_droop_compensation_reg,
    &last_tx_ramp_up_time_ms_reg,
    &last_tx_ramp_up_lo_freq_khz_reg,
    &last_tx_ramp_down_time_ms_reg,
    &last_tx_ramp_down_lo_freq_khz_reg,
    &last_tx_ramp_down_reason_reg,
    &rssi_threshold_rn16_reg,
    &rssi_threshold_epc_reg,
    &inventory_round_control_reg,
    &inventory_round_control_2_reg,
    &nominal_stop_time_reg,
    &extended_stop_time_reg,
    &regulatory_stop_time_reg,
    &gen2_select_enable_reg,
    &gen2_access_enable_reg,
    &gen2_auto_access_enable_reg,
    &gen2_offsets_reg,
    &gen2_lengths_reg,
    &gen2_transaction_ids_reg,
    &gen2_txn_controls_reg,
    &drop_query_control_reg,
    &tag_features_control_reg,
    &gen2_tx_buffer_reg,
    &calibration_info_reg,
};
// IPJ_autogen }
// clang-format on

// The largest Ex10 register is CalibrationInfo at address 0xE800
#define MAX_REGISTER_SIZE ((size_t)0x0800)

void ex10_dump_all_registers(void)
{
    ex10_printf("Registers:\n");
    uint8_t reg_data[MAX_REGISTER_SIZE];

    for (size_t idx = 0; idx < ARRAY_SIZE(all_regs); idx++)
    {
        ex10_printf(
            "%s, len: %u\n", all_regs[idx]->name, all_regs[idx]->length);
        for (size_t entry = 0; entry < all_regs[idx]->num_entries; entry++)
        {
            // Check that reg data will fit in read buffer.
            if (all_regs[idx]->length > MAX_REGISTER_SIZE)
            {
                ex10_printf(" Skipping since size %d exceeds %zu\n",
                            all_regs[idx]->length,
                            MAX_REGISTER_SIZE);
                continue;
            }

            // Read register and print
            struct Ex10Result ex10_result =
                get_ex10_protocol()->read(all_regs[idx], reg_data);
            if (ex10_result.error)
            {
                ex10_eprintf("Register at address 0x%x read failed\n",
                             all_regs[idx]->address);
                print_ex10_result(ex10_result);
            }
            ex10_print_data_indexed(reg_data, all_regs[idx]->length, entry);
        }
    }
}

struct RegisterInfo const* ex10_register_lookup_by_addr(uint16_t const addr)
{
    for (size_t idx = 0; idx < ARRAY_SIZE(all_regs); idx++)
    {
        if (addr == all_regs[idx]->address)
            return all_regs[idx];
    }

    return NULL;
}

struct RegisterInfo const* ex10_register_lookup_by_name(char const* const name)
{
    for (size_t idx = 0; idx < ARRAY_SIZE(all_regs); idx++)
    {
        if (strcmp(name, all_regs[idx]->name) == 0)
            return all_regs[idx];
    }

    return NULL;
}
