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

project(ex10_host_library
    VERSION ${VERSION}
    DESCRIPTION "Impinj Reader Chip host library"
    LANGUAGES C
)

set(LIBRARY_NAME host)

# For CMake 3.13 compatibility, create an intermediate object library first,
# and use its object list to create the static library. This allows the
# $<TARGET_OBJECTS:${LIBRARY_NAME}_objects> to be accessed.
# For CMake 3.15 and later, $<TARGET_OBJECTS:${LIBRARY_NAME}> can be accessed
# without the intermediate  $<TARGET_OBJECTS:${LIBRARY_NAME}_objects>.
add_library(${LIBRARY_NAME}_objects OBJECT)

target_sources(${LIBRARY_NAME}_objects
    PRIVATE
    ex10_api/aggregate_op_builder.c
    ex10_api/application_registers.c
    ex10_api/board_init_core.c
    ex10_api/board_init.c
    ex10_api/command_transactor.c
    ex10_api/commands.c
    ex10_api/crc16.c
    ex10_api/event_fifo_printer.c
    ex10_api/event_packet_parser.c
    ex10_api/ex10_active_region.c
    ex10_api/ex10_api_strings.c
    ex10_api/ex10_autoset_modes.c
    ex10_api/ex10_event_fifo_queue.c
    ex10_api/ex10_gen2_reply_string.c
    ex10_api/ex10_helpers.c
    ex10_api/ex10_inventory.c
    ex10_api/ex10_lbt_helpers.c
    ex10_api/ex10_ops.c
    ex10_api/ex10_power_modes.c
    ex10_api/ex10_protocol.c
    ex10_api/ex10_reader.c
    ex10_api/ex10_regulatory.c
    ex10_api/ex10_result.c
    ex10_api/ex10_result_strings.c
    ex10_api/ex10_rf_power.c
    ex10_api/ex10_utils.c
    ex10_api/ex10_device_time.c
    ex10_api/fifo_buffer_list.c
    ex10_api/gen2_commands.c
    ex10_api/gen2_tx_command_manager.c
    ex10_api/list_node.c
    ex10_api/power_transactor.c
    ex10_api/print_data.c
    ex10_api/rf_mode_definitions.c
    ex10_api/sjc_accessor.c
    ex10_api/ex10_test.c
    ex10_api/trace.c
    ex10_api/version_info.c
    ex10_modules/ex10_ramp_module_manager.c
    ex10_modules/ex10_antenna_disconnect.c
    ex10_modules/ex10_listen_before_talk.c
    ex10_modules/ex10_antenna_disconnect_and_listen_before_talk.c
    ex10_use_cases/ex10_continuous_inventory_use_case.c
    ex10_use_cases/ex10_inventory_sequence_use_case.c
    ex10_use_cases/ex10_tag_access_use_case.c

    ex10_regulatory/ex10_default_region_names.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_australia.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_brazil.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_china.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_error.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_etsi_lower.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_etsi_upper.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_fcc.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_hk.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_india.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_indonesia.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_israel.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_japan2.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_korea.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_malaysia.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_new_zealand.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_peru.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_philippines.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_russia.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_singapore.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_south_africa.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_taiwan.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_thailand.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_uruguay.c
    ex10_regulatory/ex10_regulatory_regions/ex10_regulatory_vietnam.c
 )

 add_library(${LIBRARY_NAME} STATIC)

 target_sources(${LIBRARY_NAME}
     PRIVATE
     $<TARGET_OBJECTS:${LIBRARY_NAME}_objects>
 )
