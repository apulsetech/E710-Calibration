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

#include <stdarg.h>
#include <stdio.h>

#include "ex10_api/ex10_print.h"

#ifdef EX10_PRINT_IMPL
int ex10_printf_impl(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vprintf(fmt, ap);
    va_end(ap);

    return n;
}
#endif

#ifdef EX10_PRINT_ERR_IMPL
/*
 * The 'puts' function is like 'eprintf', but it excludes
 * the leading error and function strings.  Can be used
 * like stdio puts() to output single chars or simple strings
 * to stderr.
 */
int ex10_eputs_impl(const char* fmt, ...)
{
    // Flush stdout before writing to stderr since stderr prints immediately.
    fflush(stdout);
    va_list ap;
    va_start(ap, fmt);
    int n = vfprintf(stderr, fmt, ap);
    va_end(ap);

    return n;
}
int ex10_eprintf_impl(const char* func, const char* fmt, ...)
{
    // Flush stdout before writing to stderr since stderr prints immediately.
    fflush(stdout);
    int     n = fprintf(stderr, "Error: %s(): ", func);
    va_list ap;
    va_start(ap, fmt);
    n += vfprintf(stderr, fmt, ap);
    va_end(ap);

    return n;
}
#endif

int ex10_empty_printf(const char* fmt, ...)
{
    (void)fmt;
    return 0;
}
