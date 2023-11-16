/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2022 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ex10_api/fifo_buffer_list.h"

#ifdef __cplusplus
extern "C" {
#endif


struct Ex10EventFifoQueue
{
    /**
     * Initialize the Event Fifo Queue structures.
     */
    void (*init)(void);

    /**
     * Push a FifoBufferNode onto the back of the reader.event_fifo_list.
     * The reader list mutex is used to maintain exclusive access to the
     * reader list.
     *
     * @param fifo_buffer_node A pointer to the FifoBufferNode.
     */
    void (*list_node_push_back)(struct FifoBufferNode* fifo_buffer_node);

    /**
     * Return the packet at the front of the packet queue.
     *
     * @return An EventFifoPacket or NULL if none available.
     */
    struct EventFifoPacket const* (*packet_peek)(void);

    /**
     * Delete the packet at the front of the packet queue.
     *
     * @note There is a limited amount of room for packets in the packet queue,
     *       so packets should be discarded regularly using remove_packet() to
     *       ensure room for new packets.
     *
     * @note The packet must only be removed once it is not longer in use.
     *       packet_remove() is similar to calling free() on allocated memory.
     *       Once the packet has been removed its packet pointer obtained
     *       through packet_peek() is invalid.
     *
     * @note Calling packet_free() allows for new packets to be read from the
     *       Ex10. If packet_remove() is not called then reading will stall and
     *       the available buffer space will have been used up.
     */
    void (*packet_remove)(void);

    /**
     * Wait, blocking until packets are ready for reading.
     * When packets are available to read, this function will unblock.
     */
    void (*packet_wait)(void);

    /**
     * Wait, blocking until packets are ready for reading, with a expiration
     * timeout.
     *
     * @return bool Indicates the reason the function has returned.
     * @retval true The function timeout occurred without packets being
     *              available.
     * @retval false Packets are available for reading.
     */
    bool (*packet_wait_with_timeout)(uint32_t timeout_us);

    /**
     * Unblock the packet_wait() call, even when no packets are pending.
     * @note This function should be called from a thread other than the thread
     *       that is waiting for packets.
     */
    void (*packet_unwait)(void);
};

const struct Ex10EventFifoQueue* get_ex10_event_fifo_queue(void);

#ifdef __cplusplus
}
#endif
