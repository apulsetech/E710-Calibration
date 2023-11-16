#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2022 - 2023 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################

cmake_minimum_required(VERSION 3.13)

project(ex10_examples
    VERSION ${VERSION}
    DESCRIPTION "Impinj Reader Chip examples"
    LANGUAGES C
)

add_example(access_read_write_example           access_read_write_example.c)
add_example(app_upload_example                  app_upload_example.c)
add_example(authenticate_m775_example           authenticate_m775_example.c)
add_example(auto_access_read_write_example      auto_access_read_write_example.c)
add_example(continuous_inventory_example        continuous_inventory_example.c)
add_example(continuous_inventory_stops_example  continuous_inventory_stops_example.c)
add_example(etsi_burst_example                  etsi_burst_example.c)
add_example(ex10_fct_wrapper                    ex10_fct_wrapper.c)
add_example(get_version_info                    get_version_info.c)
add_example(insert_event_fifo_example           insert_event_fifo_example.c)
add_example(inventory_example                   inventory_example.c)
add_example(inventory_fixed_q_example           inventory_fixed_q_example.c)
add_example(inventory_power_modes               inventory_power_modes.c)
add_example(lbt_continuous_inventory_example    lbt_continuous_inventory_example.c)
add_example(measure_chip_sensors                measure_chip_sensors.c)
add_example(prbs_example                        prbs_example.c)
add_example(print_calibration_v5                print_calibration_v5.c)
add_example(protected_mode_example              protected_mode_example.c)
add_example(regions_usage_example               regions_usage_example.c)
add_example(select_command_example              select_command_example.c)
add_example(simple_ramping_example              simple_ramping_example.c)
add_example(test_spi_transfer                   test_spi_transfer.c)
