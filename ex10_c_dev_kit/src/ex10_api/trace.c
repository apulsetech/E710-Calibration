/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2021 - 2022 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#ifdef LTTNG_ENABLE

#define TRACEPOINT_CREATE_PROBES
#define TRACEPOINT_DEFINE

#include "ex10_api/trace.h"

#endif  // LTTNG_ENABLE

// ISO compliance dictates that one shall not have a file which is empty outside
// of all conditional macros. This is known as an empty translation unit. The
// typedef ensures this is not empty.
typedef int make_iso_happy;
