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
    set(CMAKE_C_COMPILER /usr/bin/gcc)
endif()

if(DEFINED ENV{CXX})
    set(CMAKE_CXX_COMPILER $ENV{CXX})
else()
    set(CMAKE_CXX_COMPILER /usr/bin/g++)
endif()

set(CMAKE_SIZE /usr/bin/size)

set(CMAKE_C_COMPILER_ID   ARMCC)
set(CMAKE_CXX_COMPILER_ID ARMCC)
set(CMAKE_ASM_COMPILER_ID ARMCC)

set(CMAKE_C_COMPILER_VERSION,   8.3.0)
set(CMAKE_CXX_COMPILER_VERSION, 8.3.0)
set(CMAKE_ASM_COMPILER_VERSION, 8.3.0)

set(COMPILER_C_EXTRA_FLAGS
    -D EX10_OSAL_TYPE=EX10_OS_TYPE_POSIX
)
set(COMPILER_CXX_EXTRA_FLAGS
    -D EX10_OSAL_TYPE=EX10_OS_TYPE_POSIX
)


# The only directory that a toolchain files knows about is its current
# directory CMAKE_CURRENT_LIST_DIR.
message(DEBUG "TOOLCHAIN                   : ${CMAKE_TOOLCHAIN_FILE}")
message(DEBUG "TOOLCHAIN_PREFIX            : ${TOOLCHAIN_PREFIX}")
message(DEBUG "CMAKE_CURRENT_LIST_DIR      : ${CMAKE_CURRENT_LIST_DIR}")

include("${CMAKE_CURRENT_LIST_DIR}/../gnu_gcc_options.cmake")
