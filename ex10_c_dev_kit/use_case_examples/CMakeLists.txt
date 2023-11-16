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

project(ex10_use_case_examples
    VERSION ${VERSION}
    DESCRIPTION "Impinj Reader Chip use case examples"
    LANGUAGES C
)

add_example(access_read_write_use_case_example
    access_read_write_use_case_example.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_select_commands.c
    ./utils/ex10_use_case_example_errors.c
)

add_example(access_read_write_use_case_antenna_disconnect_example
    access_read_write_use_case_antenna_disconnect_example.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_select_commands.c
    ./utils/ex10_use_case_example_errors.c
)

add_example(authenticate_m775_use_case_example
    authenticate_m775_use_case_example.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_use_case_example_errors.c
)

add_example(auto_access_read_write_use_case_example
auto_access_read_write_use_case_example.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_select_commands.c
    ./utils/ex10_use_case_example_errors.c
)


add_example(autoset_use_case_example
    autoset_use_case_example.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_select_commands.c
    ./utils/ex10_use_case_example_errors.c
)

add_example(continuous_inventory_use_case_antenna_disconnect_example
    continuous_inventory_use_case_antenna_disconnect_example.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_use_case_example_errors.c
)

add_example(continuous_inventory_use_case_example
    continuous_inventory_use_case_example.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_use_case_example_errors.c
)

add_example(continuous_inventory_use_case_lbt_example
    continuous_inventory_use_case_lbt_example.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_use_case_example_errors.c
)

add_example(continuous_inventory_use_case_lbt_antenna_disconnect_example
    continuous_inventory_use_case_lbt_antenna_disconnect_example.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_use_case_example_errors.c
)

add_example(lock_kill_use_case_example
    lock_kill_use_case_example.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_use_case_example_errors.c
)

add_example(margin_read_use_case_example
    margin_read_use_case_example.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_use_case_example_errors.c
)

add_example(select_command_use_case_example
    select_command_use_case_example.c
    ./utils/ex10_select_commands.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_use_case_example_errors.c
)

add_example(select_inventory_session_flag
    select_inventory_session_flag.c
    ./utils/ex10_select_commands.c
    ./utils/ex10_command_line.c
    ./utils/ex10_inventory_command_line.c
    ./utils/ex10_use_case_example_errors.c
)

include_directories(.)
