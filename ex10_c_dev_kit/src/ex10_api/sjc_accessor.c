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

#include "ex10_api/sjc_accessor.h"
#include "board/board_spec.h"
#include "board/ex10_osal.h"
#include "ex10_api/application_registers.h"


// Local variables to be used within the static functions outlined by the
// Ex10SjcAccessor struct.
static struct SjcVariables
{
    const struct Ex10Protocol* _ex10_protocol;
} sjc_variables;

static void set_sjc_control(uint8_t sample_average_coarse,
                            uint8_t sample_average_fine,
                            bool    events_enable,
                            bool    fixed_rx_atten,
                            uint8_t sample_decimator)
{
    struct SjcControlFields control_fields;
    ex10_memzero(&control_fields, sizeof(control_fields));
    control_fields.sample_average_coarse = sample_average_coarse;
    control_fields.sample_average_fine   = sample_average_fine;
    control_fields.events_enable         = events_enable;
    control_fields.fixed_rx_atten        = fixed_rx_atten;
    control_fields.decimator             = sample_decimator;
    sjc_variables._ex10_protocol->write(&sjc_control_reg, &control_fields);
}

static void set_analog_rx_config(void)
{
    struct SjcGainControlFields const sjc_rx_gain = {
        .rx_atten        = RxAttenAtten_0_dB,
        .pga1_gain       = Pga1GainGain_n6_dB,
        .pga2_gain       = Pga2GainGain_0_dB,
        .pga3_gain       = Pga3GainGain_0_dB,
        .mixer_gain      = MixerGainGain_1p6_dB,
        .mixer_bandwidth = true,
        .pga1_rin_select = true,
    };
    sjc_variables._ex10_protocol->write(&sjc_gain_control_reg, &sjc_rx_gain);
}

static void set_settling_time(uint16_t initial_measure_usec,
                              uint16_t residue_measure_usec)
{
    struct SjcInitialSettlingTimeFields const initial_settling_time = {
        .settling_time = initial_measure_usec};
    struct SjcResidueSettlingTimeFields const residue_settling_time = {
        .settling_time = residue_measure_usec};

    struct RegisterInfo const* const regs[] = {&sjc_initial_settling_time_reg,
                                               &sjc_residue_settling_time_reg};

    void const* buffers[] = {
        &initial_settling_time,
        &residue_settling_time,
    };

    sjc_variables._ex10_protocol->write_multiple(
        regs, buffers, sizeof(regs) / sizeof(void const*));
}

static void set_cdac_range(struct CdacRange cdac_i, struct CdacRange cdac_q)
{
    struct SjcCdacIFields const cdac_i_fields = {.center    = cdac_i.center,
                                                 .limit     = cdac_i.limit,
                                                 .step_size = cdac_i.step_size,
                                                 .rfu       = 0};

    struct SjcCdacIFields const cdac_q_fields = {.center    = cdac_q.center,
                                                 .limit     = cdac_q.limit,
                                                 .step_size = cdac_q.step_size,
                                                 .rfu       = 0};

    sjc_variables._ex10_protocol->write(&sjc_cdac_i_reg, &cdac_i_fields);
    sjc_variables._ex10_protocol->write(&sjc_cdac_q_reg, &cdac_q_fields);
}

static void set_residue_threshold(uint16_t residue_threshold)
{
    struct SjcResidueThresholdFields const residue_threshold_fields = {
        .magnitude = residue_threshold,
    };

    sjc_variables._ex10_protocol->write(&sjc_residue_threshold_reg,
                                        &residue_threshold_fields);
}

static void set_cdac_to_find_solution(void)
{
    struct CdacRange cdac_nominal = {.center = 0, .limit = 60, .step_size = 8};
    set_cdac_range(cdac_nominal, cdac_nominal);
}

static struct SjcResultPair get_sjc_results(void)
{
    struct SjcResultFields sjc_result_i;
    struct SjcResultFields sjc_result_q;

    sjc_variables._ex10_protocol->read(&sjc_result_i_reg, &sjc_result_i);
    sjc_variables._ex10_protocol->read(&sjc_result_q_reg, &sjc_result_q);

    struct SjcResultPair const sjc_results = {
        .i = {.residue      = sjc_result_i.residue,
              .cdac         = sjc_result_i.cdac,
              .cdac_limited = sjc_result_i.cdac_sku_limited},
        .q = {.residue      = sjc_result_q.residue,
              .cdac         = sjc_result_q.cdac,
              .cdac_limited = sjc_result_i.cdac_sku_limited},
    };

    return sjc_results;
}

static void init(const struct Ex10Protocol* protocol)
{
    sjc_variables._ex10_protocol = protocol;

    // The number of ADC samples to take per SJC measurement: 2^samp_avg_exp.
    uint8_t sample_average_coarse = 1u;  // Coarse initial scan
    uint8_t sample_average_fine   = 5u;  // Fine recursive zoom into solution.

    // Insert SJC debug events into the Event FIFO.
    bool const sjc_events_enable = false;
    bool const fixed_rx_atten    = false;
    // The number of ADC sample decimation filter: 2^sample_decimation_exp.
    uint8_t const sample_decimation_exp = 0u;
    set_sjc_control(sample_average_coarse,
                    sample_average_fine,
                    sjc_events_enable,
                    fixed_rx_atten,
                    sample_decimation_exp);

    set_analog_rx_config();

    uint16_t const initial_delay_usec = 5u;
    uint16_t const residue_delay_usec = 1u;
    set_settling_time(initial_delay_usec, residue_delay_usec);

    set_residue_threshold(get_ex10_board_spec()->get_sjc_residue_threshold());

    set_cdac_to_find_solution();
}

static const struct Ex10SjcAccessor ex10_sjc = {
    .init                      = init,
    .set_sjc_control           = set_sjc_control,
    .set_analog_rx_config      = set_analog_rx_config,
    .set_settling_time         = set_settling_time,
    .set_cdac_range            = set_cdac_range,
    .set_residue_threshold     = set_residue_threshold,
    .set_cdac_to_find_solution = set_cdac_to_find_solution,
    .get_sjc_results           = get_sjc_results,
};

const struct Ex10SjcAccessor* get_ex10_sjc(void)
{
    return &ex10_sjc;
}
