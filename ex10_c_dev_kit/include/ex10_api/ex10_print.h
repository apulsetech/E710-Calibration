/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2023 Impinj, Inc. All rights reserved.                      *
 *                                                                           *
 *****************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int ex10_printf_impl(const char* fmt, ...);
int ex10_eprintf_impl(const char* func, const char* fmt, ...);
int ex10_eputs_impl(const char* fmt, ...);
int ex10_empty_printf(const char* fmt, ...);

// Use "-DEX10_ENABLE_PRINT=ON" when running cmake to enable printf()
// calls from the SDK.
#if defined(EX10_ENABLE_PRINT)
#define EX10_PRINT_IMPL
#define ex10_printf(...) ex10_printf_impl(__VA_ARGS__)
#else
#define ex10_printf(...) ex10_empty_printf(__VA_ARGS__)
#endif

// Use "-DEX10_ENABLE_PRINT_ERR=ON" when running cmake to enable error
// fprintf() calls from the SDK.
#if defined(EX10_ENABLE_PRINT_ERR)
#define EX10_PRINT_ERR_IMPL
#define ex10_eprintf(...) ex10_eprintf_impl(__func__, __VA_ARGS__)
#define ex10_eputs(...) ex10_eputs_impl(__VA_ARGS__)
#else
#define ex10_eprintf(...) ex10_empty_printf(__VA_ARGS__)
#define ex10_eputs(...) ex10_empty_printf(__VA_ARGS__)
#endif

// Use "-DEX10_ENABLE_PRINT_EX=ON when running cmake to enable printf()
// calls from the SDK examples.
#if defined(EX10_ENABLE_PRINT_EX)
#ifndef EX10_PRINT_IMPL
#define EX10_PRINT_IMPL
#endif
#define ex10_ex_printf(...) ex10_printf_impl(__VA_ARGS__)
#else
#define ex10_ex_printf(...) ex10_empty_printf(__VA_ARGS__)
#endif

// Use "-DEX10_ENABLE_PRINT_EX_ERR=ON" when running cmake to enable error
// fprintf() calls from the SDK examples.
#if defined(EX10_ENABLE_PRINT_EX_ERR)
#ifndef EX10_PRINT_ERR_IMPL
#define EX10_PRINT_ERR_IMPL
#endif
#define ex10_ex_eprintf(...) ex10_eprintf_impl(__func__, __VA_ARGS__)
#define ex10_ex_eputs(...) ex10_eputs_impl(__VA_ARGS__)
#else
#define ex10_ex_eprintf(...) ex10_empty_printf(__VA_ARGS__)
#define ex10_ex_eputs(...) ex10_empty_printf(__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif
