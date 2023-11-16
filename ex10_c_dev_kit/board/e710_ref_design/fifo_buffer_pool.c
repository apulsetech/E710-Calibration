/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2021 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#include "board/fifo_buffer_pool.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/ex10_macros.h"

/**
 * Each buffer needs to be large enough to contain the full contents of
 * the ReadFifo command (4096 bytes), plus the 1-byte result code,
 * and maintain Fifo packet 4-byte alignment.
 */
#define EVENT_FIFO_BUFFER_SIZE (EX10_EVENT_FIFO_SIZE + FIFO_HEADER_SPACE)

/**
 * @note that the number of buffers should be changed based on the expected
 * event FIFO traffic and available memory on your host controller. For example,
 * if you have too few buffers and a large number of tags in a short window of
 * time, you may not have enough space to pull them from the device, and thus
 * the device-side event FIFO buffer could overfill.
 */
static uint8_t buffer_0[EVENT_FIFO_BUFFER_SIZE];
static uint8_t buffer_1[EVENT_FIFO_BUFFER_SIZE];
static uint8_t buffer_2[EVENT_FIFO_BUFFER_SIZE];
static uint8_t buffer_3[EVENT_FIFO_BUFFER_SIZE];
static uint8_t buffer_4[EVENT_FIFO_BUFFER_SIZE];
static uint8_t buffer_5[EVENT_FIFO_BUFFER_SIZE];
static uint8_t buffer_6[EVENT_FIFO_BUFFER_SIZE];
static uint8_t buffer_7[EVENT_FIFO_BUFFER_SIZE];

static struct ByteSpan const event_fifo_buffers[] = {
    {.data = buffer_0, .length = sizeof(buffer_0)},
    {.data = buffer_1, .length = sizeof(buffer_1)},
    {.data = buffer_2, .length = sizeof(buffer_2)},
    {.data = buffer_3, .length = sizeof(buffer_3)},
    {.data = buffer_4, .length = sizeof(buffer_4)},
    {.data = buffer_5, .length = sizeof(buffer_5)},
    {.data = buffer_6, .length = sizeof(buffer_6)},
    {.data = buffer_7, .length = sizeof(buffer_7)},
};

static struct FifoBufferNode
    event_fifo_buffer_nodes[ARRAY_SIZE(event_fifo_buffers)];

static struct FifoBufferPool const event_fifo_buffer_pool = {
    .fifo_buffer_nodes = event_fifo_buffer_nodes,
    .fifo_buffers      = event_fifo_buffers,
    .buffer_count      = ARRAY_SIZE(event_fifo_buffers)};

struct FifoBufferPool const* get_ex10_event_fifo_buffer_pool(void)
{
    return &event_fifo_buffer_pool;
}

/**
 * @note This is a pool of buffer to hold errors for error reporting in
 * interrupt context. When this pool becomes empty and an error occurred that
 * requires a free "result" buffer for reporting, the error will be silently
 * discarded.
 */
static uint32_t
    result_buffer_0[RESULT_FIFO_BUFFER_SIZE_BYTES / sizeof(uint32_t)];
static uint32_t
    result_buffer_1[RESULT_FIFO_BUFFER_SIZE_BYTES / sizeof(uint32_t)];
static uint32_t
    result_buffer_2[RESULT_FIFO_BUFFER_SIZE_BYTES / sizeof(uint32_t)];
static uint32_t
    result_buffer_3[RESULT_FIFO_BUFFER_SIZE_BYTES / sizeof(uint32_t)];

static struct ByteSpan const result_buffers[] = {
    {.data = (uint8_t*)result_buffer_0, .length = sizeof(result_buffer_0)},
    {.data = (uint8_t*)result_buffer_1, .length = sizeof(result_buffer_1)},
    {.data = (uint8_t*)result_buffer_2, .length = sizeof(result_buffer_2)},
    {.data = (uint8_t*)result_buffer_3, .length = sizeof(result_buffer_3)},
};

static struct FifoBufferNode result_buffer_nodes[ARRAY_SIZE(result_buffers)];

static struct FifoBufferPool const result_buffer_pool = {
    .fifo_buffer_nodes = result_buffer_nodes,
    .fifo_buffers      = result_buffers,
    .buffer_count      = ARRAY_SIZE(result_buffers)};

struct FifoBufferPool const* get_ex10_result_buffer_pool(void)
{
    return &result_buffer_pool;
}
