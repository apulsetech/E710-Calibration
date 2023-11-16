#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2022 Impinj, Inc. All rights reserved.                      #
#############################################################################

cmake_minimum_required(VERSION 3.13)

project(ex10_board_reference_design
    VERSION ${VERSION}
    DESCRIPTION "Impinj Reader Chip E710 reference design board library"
    LANGUAGES C
)

set(COMMON_BOARD_SPECIFIC_LIBRARIES gpiod pthread)
set(SHARED_LIB_SPECIFIC_LIBRARIES   ${COMMON_BOARD_SPECIFIC_LIBRARIES} PARENT_SCOPE)
set(EXECUTABLE_SPECIFIC_LIBRARIES   ${COMMON_BOARD_SPECIFIC_LIBRARIES} PARENT_SCOPE)

set(LIBRARY_NAME board)

# For CMake 3.13 compatibility, create an intermediate object library first,
# and use its object list to create the static library. This allows the
# $<TARGET_OBJECTS:${LIBRARY_NAME}_objects> to be accessed.
# For CMake 3.15 and later, $<TARGET_OBJECTS:${LIBRARY_NAME}> can be accessed
# without the intermediate  $<TARGET_OBJECTS:${LIBRARY_NAME}_objects>.
add_library(${LIBRARY_NAME}_objects OBJECT)
target_sources(${LIBRARY_NAME}_objects
    PRIVATE
    board_spec.c
    calibration.c
    calibration_v5.c
    driver_list.c
    ex10_gpio.c
    ex10_osal_posix.c
    ex10_print.c
    ex10_random.c
    ex10_rx_baseband_filter.c
    fifo_buffer_pool.c
    gpio_driver.c
    rssi_compensation_lut.c
    spi_driver.c
    time_helpers.c
    uart_driver.c
    uart_helpers.c
)

add_library(${LIBRARY_NAME} STATIC)

target_sources(${LIBRARY_NAME}
    PRIVATE
    $<TARGET_OBJECTS:${LIBRARY_NAME}_objects>
)
