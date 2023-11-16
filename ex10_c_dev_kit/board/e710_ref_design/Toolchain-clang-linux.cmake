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

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ARMv7)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED YES)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

if(DEFINED ENV{CC})
    set(CMAKE_C_COMPILER $ENV{CC})
else()
    set(CMAKE_C_COMPILER clang)
endif()

if(DEFINED ENV{CXX})
    set(CMAKE_CXX_COMPILER $ENV{CXX})
else()
    set(CMAKE_CXX_COMPILER clang++)
endif()

set(CMAKE_ASM_COMPILER clang)

set(CMAKE_OBJDUMP llvm-objdump)
set(CMAKE_OBJCOPY llvm-objcopy)
set(CMAKE_NM      llvm-nm)
set(CMAKE_SIZE    llvm-size)

set(CMAKE_C_COMPILER_ID   ARMCC)
set(CMAKE_CXX_COMPILER_ID ARMCC)
set(CMAKE_ASM_COMPILER_ID ARMCC)

set(CMAKE_C_COMPILER_VERSION,   12.0.0)
set(CMAKE_CXX_COMPILER_VERSION, 12.0.0)
set(CMAKE_ASM_COMPILER_VERSION, 12.0.0)

# The only directory that a toolchain files knows about is its current
# directory CMAKE_CURRENT_LIST_DIR.
message(DEBUG "TOOLCHAIN                   : ${CMAKE_TOOLCHAIN_FILE}")
message(DEBUG "CMAKE_CURRENT_LIST_DIR      : ${CMAKE_CURRENT_LIST_DIR}")

# The Impinj Reader Chip SDK supports these types of builds.
# The CMake BUILD_TYPES RelWithDebInfo MinSizeRel
# can be added here, if necessary.
set(CMAKE_CONFIGURATION_TYPES Release Debug Coverage Profile)
set(CMAKE_CONFIGURATION_TYPES
    "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
    "Impinj Reader Chip Build Types"
    FORCE
)

# Set compiler options strings to empty
set(COMPILER_COVERAGE_FLAGS)
set(LINKER_COVERAGE_FLAGS)
set(COMPILER_PROFILING_FLAGS)
set(COMPILER_LTTNG_FLAGS)
set(LINKER_LTTNG_FLAGS)

if(NOT DEFINED CMAKE_BUILD_TYPE OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Setting to default CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    # No special flags need to be added for build type 'Release'
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    # No special flags need to be added for build type 'Debug'
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Coverage")
    set(COMPILER_COVERAGE_FLAGS --coverage -fprofile-abs-path)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Profile")
    set(COMPILER_PROFILING_FLAGS -pg -fno-reorder-functions)
else()
    message(FATAL_ERROR "Invalid CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")
endif()

if(DEFINED LTTNG)
    set(COMPILER_LTTNG_FLAGS -DLTTNG_ENABLE=${LTTNG} -Wno-redundant-decls)
    set(LINKER_LTTNG_FLAGS   -llttng-ust -ldl)
endif()

set(COMPILER_COMMON_FLAGS
    -D EX10_OSAL_TYPE=EX10_OS_TYPE_POSIX
    -fno-common
    -Wall
    -Weverything
    -Wextra
    -pedantic
    -Wformat=2
    -Wswitch-default
    -Wpointer-arith
    -Winline
    -Wundef
    -Wcast-qual
    -Wshadow
    -Wunreachable-code
    -Wfloat-equal
    -Wredundant-decls
    -fstrict-aliasing
    -fstrict-overflow
    -D_FORTIFY_SOURCE=2
    -fstack-protector-all
    -Wmissing-field-initializers
    -fPIC
    -Wno-documentation
    -Wno-padded
    -Wno-reserved-identifier
    -Wno-disabled-macro-expansion
    -Wno-declaration-after-statement
    -Wno-switch-enum
    -Wno-unknown-pragmas
    -Wno-covered-switch-default
    -Wno-double-promotion
    -Wno-psabi
    -Wno-newline-eof
    -Wno-format-nonliteral
    -Wno-implicit-float-conversion
    -Wno-tautological-value-range-compare
    -Wno-unreachable-code-break
    -Werror
    -Wno-cast-align
)

# gcc options that cannot be used with g++
set(COMPILER_C_SPECIFIC_FLAGS
    -Werror=vla
    -Wbad-function-cast
    -Wstrict-prototypes
    -Wnested-externs
    -Wold-style-definition
)

# C++ is used for functional tests.
set(COMPILER_CXX_SPECIFIC_FLAGS
    -Wno-strict-overflow    # googletest
    -Wno-inline             # googletest, not important for tests
    -Wno-undef              # googletest
    -Wno-switch-default     # googletest
    -Wno-switch-enum        # googletest
    -Wno-pedantic           # Allow C++ designated initializers
    -Wno-shadow
)

set(COMPILER_OPTIMIZE_RELEASE    -g -O3)
set(COMPILER_OPTIMIZE_DEBUG      -g -O0)
set(COMPILER_OPTIMIZE_MINSIZEREL -g -Os)

# All of the C compiler flags except optimization
set(COMMON_C_COMPILER_FLAGS
    ${COMPILER_COMMON_FLAGS}
    ${COMPILER_C_SPECIFIC_FLAGS}
    ${COMPILER_COVERAGE_FLAGS}
    ${COMPILER_PROFILING_FLAGS}
    ${COMPILER_LTTNG_FLAGS}
)

set(CMAKE_C_FLAGS_RELEASE_INIT
    ${COMMON_C_COMPILER_FLAGS}
    ${COMPILER_OPTIMIZE_RELEASE}
)

set(CMAKE_C_FLAGS_DEBUG_INIT
    ${COMMON_C_COMPILER_FLAGS}
    ${COMPILER_OPTIMIZE_DEBUG}
)

set(CMAKE_C_FLAGS_COVERAGE_INIT
    ${COMMON_C_COMPILER_FLAGS}
    ${COMPILER_OPTIMIZE_DEBUG}  # Use debug level opimization for coverage
)

set(CMAKE_C_FLAGS_PROFILE_INIT
    ${COMMON_C_COMPILER_FLAGS}
    ${COMPILER_OPTIMIZE_DEBUG}  # Use debug level opimization for profiling
)

# All of the C++ compiler flags except optimization
set(COMMON_CXX_COMPILER_FLAGS
    ${COMPILER_COMMON_FLAGS}
    ${COMPILER_CXX_SPECIFIC_FLAGS}
    ${COMPILER_COVERAGE_FLAGS}
    ${COMPILER_PROFILING_FLAGS}
    ${COMPILER_LTTNG_FLAGS}
)

set(CMAKE_CXX_FLAGS_RELEASE_INIT
    ${COMMON_CXX_COMPILER_FLAGS}
    ${COMPILER_OPTIMIZE_RELEASE}
)

set(CMAKE_CXX_FLAGS_DEBUG_INIT
    ${COMMON_CXX_COMPILER_FLAGS}
    ${COMPILER_OPTIMIZE_DEBUG}
)

set(CMAKE_CXX_FLAGS_COVERAGE_INIT
    ${COMMON_CXX_COMPILER_FLAGS}
    ${COMPILER_OPTIMIZE_DEBUG}
)

set(CMAKE_CXX_FLAGS_PROFILE_INIT
    ${COMMON_CXX_COMPILER_FLAGS}
    ${COMPILER_OPTIMIZE_DEBUG}  # Use debug level opimization for profiling
)

set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT     ${LINKER_LTTNG_FLAGS})
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT  ${LINKER_LTTNG_FLAGS})

set(CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT       ${LINKER_LTTNG_FLAGS})
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT    ${LINKER_LTTNG_FLAGS})

set(CMAKE_EXE_LINKER_FLAGS_COVERAGE_INIT    ${LINKER_LTTNG_FLAGS} --coverage)
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE_INIT ${LINKER_LTTNG_FLAGS} --coverage)

set(CMAKE_EXE_LINKER_FLAGS_PROFILE_INIT     ${LINKER_LTTNG_FLAGS}
    -lprofiler -Wl,--no-as-needed -lprofiler -Wl,--as-needed
)

set(CMAKE_SHARED_LINKER_FLAGS_PROFILE_INIT  ${LINKER_LTTNG_FLAGS}
    -lprofiler -Wl,--no-as-needed -lprofiler -Wl,--as-needed
)

# Convenience function for printing a variable
macro(print_variable string_var)
    message(STATUS "${string_var} : ${${string_var}}")
endmacro()

# Older versions of CMake (i.e. 3.16) do not properly convert semicolon
# separated strings into a whitespace separated argument list when used by
# target compiler and linker options. Therefore do this here and it will
# work with both old and new CMake versions.
macro(replace_semicolons_w_spaces string_var)
    string(REPLACE ";" " " ${string_var} "${${string_var}}")
endmacro()

# CMake uses the format: CMAKE_<LANG>_FLAGS_<CONFIG>_INIT
# https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_FLAGS_CONFIG_INIT.html
# in the toolchain file to set tool flags per build configuration type.
# lang_list contains the <LANG> part of the tool configuration variable (above).
set(lang_list C CXX EXE_LINKER SHARED_LINKER)

foreach(config_type ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config_type} config_type)
    foreach(lang ${lang_list})
        set(tool_str CMAKE_${lang}_FLAGS_${config_type}_INIT)
        replace_semicolons_w_spaces(${tool_str})
    endforeach()
endforeach()
