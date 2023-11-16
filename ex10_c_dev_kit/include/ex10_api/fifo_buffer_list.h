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
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "ex10_api/byte_span.h"
#include "ex10_api/list_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct FifoBufferNode
 * Used for reading EventFifo data using the ReadFifo command and for passing
 * the captured data across the protocol stack.
 */
struct FifoBufferNode
{
    /**
     * The fifo_data is read-only and is how the EventFifo clients access the
     * data.
     *
     * The fifo_data->data field is set during initialization as the first
     * 32-bit aligned byte within the allocated raw backing store.
     *
     * The fifo_data->length field is set to zero during init and set to the
     * the number of EventFifo bytes read using the ReadFifo command. It does
     * not include the ResponseCode byte.
     */
    struct ConstByteSpan fifo_data;

    /**
     * The raw_buffer fields are set during initialization and must not be
     * modified afterward.
     *
     * raw_buffer->data points to the same point in the backing store as
     * fifo_data->data but is non-cast and is to transfer data into the buffer
     * with the ReadFifo command.
     *
     * raw_buffer->length represents the number of bytes allocated by the
     * backing store for reading EventFifo data using the ReadFifo command.
     * This value must always be >= EX10_EVENT_FIFO_SIZE for the parsing
     * of EventFifo data to work properly.
     */
    struct ByteSpan raw_buffer;

    /// The list node which is used for list insertion operations.
    struct Ex10ListNode list_node;
};

/**
 * @struct FifoBufferList
 * The list of FifoBuffers statically allocated for reading the Ex10 EventFifo.
 *
 * The length of the list should be changed based on the expected event FIFO
 * traffic and available memory on your host controller. For example, if you
 * have too few buffers and a large number of tags in a short window of time,
 * you may not have enough space to pull them from the device, and thus the
 * device-side event FIFO buffer could overfill.
 */
struct FifoBufferList
{
    /**
     * Initialize the FifoBufferNode nodes, free list and queued list.
     * This function should be called at the board layer.
     *
     * After calling this function:
     * - All FifoBufferNode elements will be in the free list.
     * - The Queued buffer list will be empty.
     * - The FifoBufferNode structs will be properly initialized.
     *
     * @param fifo_buffer_nodes An array of FifoBufferNode elements to
     * initialize.
     * @param byte_arrays       An array of buffers allocated for ReadFifo
     * events.
     * @param buffer_count      The number FifoBufferNode and the ByteSpan
     * elements allocated for ReadFifo events.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*init)(struct FifoBufferNode* fifo_buffer_nodes,
                              struct ByteSpan const* byte_arrays,
                              size_t                 buffer_count);

    /**
     * Add a FifoBufferNode to the free list. Once all ReadFifo events have been
     * read from a the FifoBufferNode struct release the node to the free list
     * using this call.
     *
     * @param event_fifo_buffer_node The FifoBufferNode to release to the free
     * list.
     *
     * @return bool True if the free list was empty prior to performing the
     *              put operation. False if there were nodes in the free list
     *              prior to the put operation.
     */
    bool (*free_list_put)(struct FifoBufferNode* event_fifo_buffer_node);

    /**
     * Get a FifoBufferNode from the free list to fill with ReadFifo data.
     *
     * @return struct FifoBufferNode* The free node from the free list.
     * @retval NULL   If a list node is not available.
     */
    struct FifoBufferNode* (*free_list_get)(void);

    /**
     * @return size_t The number of FifoBufferNode elements contained in the
     * list.
     */
    size_t (*free_list_size)(void);
};

struct FifoBufferList const* get_ex10_fifo_buffer_list(void);

struct FifoBufferList const* get_ex10_result_buffer_list(void);

/**
 * A common function to release the struct FifoBufferNode to the correct list.
 * The list is determined by checking the size of the allocated buffer;
 * i.e. fifo_buffer_node->raw_buffer.length.
 *
 * @param fifo_buffer_node The FifoBufferNode to free to one of the free lists.
 *
 * @return bool Indicates whether the EventFifo free list was empty.
 * @retval true The FifoBufferNode was released to the event fifo free list and
 *              that list was empty prior to putting it on the free list.
 * @retval false Either the FifoBufferNode contained the Ex10ResultPacket packet
 *               type or the event fifo free list was not empty prior to being
 *               put back into the list.
 */
bool ex10_release_buffer_node(struct FifoBufferNode* fifo_buffer_node);

#ifdef __cplusplus
}
#endif
