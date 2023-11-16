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
set(CMAKE_PREFERRED_VERSION "3.16.0")

if(${CMAKE_VERSION} VERSION_LESS ${CMAKE_PREFERRED_VERSION})
    message("Please consider updating to ${CMAKE_PREFERRED_VERSION}")
endif()

# Read in the version.txt file, extract off the "version=" prefix to get
# just the version in "X.Y.Z" string format.
file(READ "version.txt" VERSION)
string(STRIP ${VERSION} VERSION)
# Remove the "version=" prefix and any whitespace around the '=' sign.
# VERSION_AND_STRING will be the text of X.Y.Z-VERSION_STRING
string(REGEX REPLACE "version[ \t\r\n]*=[ \t\r\n]*" "" VERSION_AND_STRING ${VERSION})
# Split the VERSION and VERSION_STRING by either the '-' or '+' character
string(REGEX REPLACE "[-+].*$" "" VERSION        "${VERSION_AND_STRING}")
string(REGEX REPLACE "^.*[-+]" "" VERSION_STRING "${VERSION_AND_STRING}")
# If VERSION and VERSION_STRING match then no '-' or '+' was found;
# i.e. there is no version string.
set(VERSION_DISPLAY ${VERSION})
if(NOT "${VERSION}" STREQUAL "${VERSION_STRING}")
    string(CONCAT VERSION_DISPLAY ${VERSION_DISPLAY} " '" ${VERSION_STRING} "'")
endif()

# Top level CMake file for the Impinj Reader Chip SDK
project(ex10_c_sdk
    VERSION ${VERSION}
    DESCRIPTION "Impinj Reader Chip SDK"
    LANGUAGES C
)

option(EX10_ENABLE_PRINT        "Enable/Disable library printfs to stdout" ON)
option(EX10_ENABLE_PRINT_ERR    "Enable/Disable library printfs to stderr" ON)
option(EX10_ENABLE_PRINT_EX     "Enable/Disable example printfs to stdout" ON)
option(EX10_ENABLE_PRINT_EX_ERR "Enable/Disable example printfs to stderr" ON)

if (EX10_ENABLE_PRINT)
    add_definitions(-DEX10_ENABLE_PRINT)
endif()
if (EX10_ENABLE_PRINT_ERR)
    add_definitions(-DEX10_ENABLE_PRINT_ERR)
endif()

if (EX10_ENABLE_PRINT_EX)
    add_definitions(-DEX10_ENABLE_PRINT_EX)
endif()
if (EX10_ENABLE_PRINT_EX_ERR)
    add_definitions(-DEX10_ENABLE_PRINT_EX_ERR)
endif()

if(NOT DEFINED CMAKE_CROSSCOMPILING OR NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR
            "A toolchain must be set when compiling the Impinj Reader Chip SDK. "
            "Use the -DCMAKE_TOOLCHAIN_FILE=board/e710_ref_design/Toolchain-gcc-linux.cmake "
            "command line parameter to build the C SDK on the reference design."
    )
endif()

if(NOT DEFINED BOARD_TARGET OR "${BOARD_TARGET}" STREQUAL "")
    set(BOARD_TARGET e710_ref_design)
    message(STATUS "Setting to default BOARD_TARGET     : ${BOARD_TARGET}")
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Setting to default CMAKE_BUILD_TYPE : ${CMAKE_BUILD_TYPE}")
endif()

include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}/board/${BOARD_TARGET}
                    ${CMAKE_SOURCE_DIR}/include
)

link_directories(${CMAKE_BINARY_DIR}/${BOARD_TARGET})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BOARD_TARGET})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BOARD_TARGET})

message(STATUS "TOOLCHAIN                   : ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "VERSION                     : ${VERSION_DISPLAY}")
message(STATUS "BOARD_TARGET                : ${BOARD_TARGET}")
message(STATUS "CMAKE_BUILD_TYPE            : ${CMAKE_BUILD_TYPE}")
message(STATUS "LTTNG                       : ${LTTNG}")
message(STATUS "EX10_ENABLE_PRINT           : ${EX10_ENABLE_PRINT}")
message(STATUS "EX10_ENABLE_PRINT_ERR       : ${EX10_ENABLE_PRINT_ERR}")
message(STATUS "EX10_ENABLE_PRINT_EX        : ${EX10_ENABLE_PRINT_EX}")
message(STATUS "EX10_ENABLE_PRINT_EX_ERR    : ${EX10_ENABLE_PRINT_EX_ERR}")

# block() # -- Not available until CMake version 3.26
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
message(DEBUG  "CMAKE_CONFIGURATION_TYPES   : ${CMAKE_CONFIGURATION_TYPES}")
message(DEBUG  "C_FLAGS                     : ${CMAKE_C_FLAGS_${BUILD_TYPE_UPPER}}")
message(DEBUG  "CXX_FLAGS                   : ${CMAKE_CXX_FLAGS_${BUILD_TYPE_UPPER}}")
message(DEBUG  "EXE_LINKER_FLAGS            : ${CMAKE_EXE_LINKER_FLAGS_${BUILD_TYPE_UPPER}}")
message(DEBUG  "SHARED_LINKER_FLAGS         : ${CMAKE_SHARED_LINKER_FLAGS_${BUILD_TYPE_UPPER}}")
# endblock()

# The examples target dependencies get added in the examples subdirectory
add_custom_target(examples)

function(add_example example_name)

    add_executable(${example_name} ${ARGN})

    add_dependencies(examples ${example_name})

    set_target_properties(${example_name}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${BOARD_TARGET}/examples"
        SUFFIX ".bin"
    )

    target_link_libraries(${example_name}
        "-Wl,--start-group"
        ${EXECUTABLE_SPECIFIC_LIBRARIES}
        host
        board
        m
        "-Wl,--end-group"
    )

    get_target_property(OUTPUT_PATH ${example_name} RUNTIME_OUTPUT_DIRECTORY)
    get_target_property(DOT_EXT ${example_name} SUFFIX)
    add_custom_command(TARGET ${example_name} POST_BUILD
        COMMAND ${CMAKE_OBJDUMP}             --source ${OUTPUT_PATH}/${example_name}${DOT_EXT} > ${OUTPUT_PATH}/${example_name}.lst
        COMMAND ${CMAKE_NM} --demangle --numeric-sort ${OUTPUT_PATH}/${example_name}${DOT_EXT} > ${OUTPUT_PATH}/${example_name}.symbols.txt
    )

    # Note: CMAKE_OBJDUMP, CMAKE_NM are standard CMake defined variables.
    # CMAKE_SIZE is not, and may not be set in the toolchain file.
    if(DEFINED CMAKE_SIZE)
        add_custom_command(TARGET ${example_name} POST_BUILD
            COMMAND ${CMAKE_SIZE} ${OUTPUT_PATH}/${example_name}${DOT_EXT} > ${OUTPUT_PATH}/${example_name}.size.txt
        )
    endif()

endfunction()

add_subdirectory(board/${BOARD_TARGET})
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(use_case_examples)

message(DEBUG  "SHARED_LIB_SPECIFIC_LIBRARIES : ${SHARED_LIB_SPECIFIC_LIBRARIES}")
message(DEBUG  "EXECUTABLE_SPECIFIC_LIBRARIES : ${EXECUTABLE_SPECIFIC_LIBRARIES}")
message(DEBUG  "CMAKE_OBJCOPY                 : ${CMAKE_OBJCOPY}")
message(DEBUG  "CMAKE_OBJDUMP                 : ${CMAKE_OBJDUMP}")
message(DEBUG  "CMAKE_NM                      : ${CMAKE_NM}")
message(DEBUG  "CMAKE_SIZE                    : ${CMAKE_SIZE}")

# Building the lib_py2c.so:
# Note: for CMake version 3.15 and later, this could be:
# $<TARGET_OBJECTS:board> $<TARGET_OBJECTS:host>
add_library(_py2c SHARED
    $<TARGET_OBJECTS:board_objects>
    $<TARGET_OBJECTS:host_objects>
)

include(impinj_internal_testing.cmake OPTIONAL)

target_link_options(_py2c PRIVATE
    -fPIC -shared
)

target_link_libraries(_py2c PRIVATE
    "-Wl,--start-group"
    ${SHARED_LIB_SPECIFIC_LIBRARIES}
    "-Wl,--end-group"
)

# Add 'libs' target for building just the libraries.
add_custom_target(libs DEPENDS _py2c host board)
