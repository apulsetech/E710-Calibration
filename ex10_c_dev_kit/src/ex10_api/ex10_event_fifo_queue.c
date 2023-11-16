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

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "board/ex10_osal.h"
#include "ex10_api/byte_span.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_event_fifo_queue.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/linked_list.h"

static struct ConstByteSpan event_packets_iterator = {.data   = NULL,
                                                      .length = 0u};

static struct EventFifoPacket        event_packet;
static struct Ex10LinkedList         event_fifo_list;
static struct Ex10EventParser const* event_parser = NULL;
/// Guards access to the fifo_buffer_list within this same structure.
static ex10_mutex_t list_mutex = EX10_MUTEX_INITIALIZER;
static ex10_cond_t  list_cond  = EX10_COND_INITIALIZER;


static struct EventFifoPacket const invalid_event_packet = {
    .packet_type         = InvalidPacket,
    .us_counter          = 0u,
    .static_data         = NULL,
    .static_data_length  = 0u,
    .dynamic_data        = NULL,
    .dynamic_data_length = 0u,
    .is_valid            = false,
};

static void init(void)
{
    list_init(&event_fifo_list);
    event_packets_iterator.data   = NULL;
    event_packets_iterator.length = 0u;
    event_packet                  = invalid_event_packet;
    event_parser                  = get_ex10_event_parser();
}

static void list_node_push_back(struct FifoBufferNode* fifo_buffer_node)
{
    ex10_mutex_lock(&list_mutex);
    list_push_back(&event_fifo_list, &fifo_buffer_node->list_node);
    ex10_mutex_unlock(&list_mutex);
    ex10_cond_signal(&list_cond);
}

/**
 * Pop a FifoBufferNode from the front of the event_fifo_list.
 * The list_mutex is used to maintain exclusive access to the
 * event_fifo_list. If the list is empty, then NULL is returned.
 *
 * @return struct FifoBufferNode* A pointer to the front FifoBufferNode on the
 *                                list.
 * @retval NULL If there are no free nodes in the list.
 */
static struct FifoBufferNode* list_node_pop_front(void)
{
    struct FifoBufferNode* fifo_buffer_node = NULL;

    ex10_mutex_lock(&list_mutex);
    struct Ex10ListNode* list_node = list_front(&event_fifo_list);

    // Note: if list_node->data ==  NULL, then the list is empty.
    // i.e. list_node is pointing to the sentinel node.
    if (list_node->data)
    {
        list_pop_front(&event_fifo_list);
        fifo_buffer_node = (struct FifoBufferNode*)list_node->data;
    }
    ex10_mutex_unlock(&list_mutex);

    return fifo_buffer_node;
}

static struct FifoBufferNode const* event_fifo_buffer_peek(void)
{
    // If the node is a sentinel node then list_node->data will be NULL
    // indicating that there are no buffers in the list.
    ex10_mutex_lock(&list_mutex);
    struct Ex10ListNode* list_node = list_front(&event_fifo_list);
    ex10_mutex_unlock(&list_mutex);
    return (struct FifoBufferNode const*)list_node->data;
}

static void event_fifo_buffer_pop(void)
{
    struct FifoBufferNode* fifo_buffer_node = list_node_pop_front();

    bool free_list_is_empty = false;
    if (fifo_buffer_node)
    {
        free_list_is_empty = ex10_release_buffer_node(fifo_buffer_node);
    }

    if (free_list_is_empty)
    {
        // The buffer queue for the interrupt to obtain packets from
        // was empty. Initiate an interrupt from Ex10 to get the ReadFifo
        // command restarted.
        struct Ex10Result ex10_result =
            get_ex10_protocol()->insert_fifo_event(true, NULL);
        if (ex10_result.error)
        {
            // There was an error triggering an interrupt
            // so we inject an error packet into the list
            // and we know that there is room because we just
            // put a node in the free list and we set the
            // us_counter to 0 as we have no idea what time it is.
            struct FifoBufferNode* result_buffer_node =
                make_ex10_result_fifo_packet(ex10_result, 0);

            if (result_buffer_node)
            {
                // The Ex10ResultPacket will be placed into the
                // list with full details on the encountered error.
                get_ex10_event_fifo_queue()->list_node_push_back(
                    result_buffer_node);
            }
            // else if the make_ex10_result_fifo_packet() returns
            // NULL that means that something else has already
            // consumed it so there is likley another error in
            // the system.  This failure is also a very basic
            // failure to communicate which should show up
            // in other places.  So this will fail silently to
            // avoid "piling on" to another error.
        }
    }
}

static void parse_next_event_fifo_packet(void)
{
    if (event_packets_iterator.length == 0u)
    {
        // The iterator has reached the end of a buffer node or
        // it is not pointing to a node.
        if (event_packets_iterator.data != NULL)
        {
            // The event buffer ConstByteSpan data pointer was pointing
            // to the end of non-null.
            // Free the current FifoBufferNode in the list.
            event_fifo_buffer_pop();

            // Invalidate the ConstByteSpan data pointer to indicate that
            // there no current EventFifo data buffer being parsed.
            event_packets_iterator.data = NULL;
        }

        struct FifoBufferNode const* fifo_buffer = event_fifo_buffer_peek();
        if (fifo_buffer != NULL)
        {
            event_packets_iterator = fifo_buffer->fifo_data;
        }
        else
        {
            // There were no FifoBufferNode elements in the list.
            event_packets_iterator.data   = NULL;
            event_packets_iterator.length = 0u;
        }
    }

    if (event_packets_iterator.length > 0u)
    {
        event_packet =
            event_parser->parse_event_packet(&event_packets_iterator);
    }
    else
    {
        event_packet = invalid_event_packet;
    }
}

static struct EventFifoPacket const* packet_peek(void)
{
    // If the packet iterator is NULL, attempt to get a fifo buffer from the
    // free list and parse the first packet.
    if (event_packets_iterator.data == NULL)
    {
        parse_next_event_fifo_packet();
    }

    // If the packet buffer is not null, then the event_packet must have
    // been parsed (even if the packet was marked .is_value = false).
    if (event_packets_iterator.data != NULL)
    {
        return &event_packet;
    }

    // No packets have been parsed nor are any available for parsing.
    return NULL;
}

static void packet_remove(void)
{
    parse_next_event_fifo_packet();
}

static void packet_wait(void)
{
    ex10_mutex_lock(&list_mutex);
    while ((event_packets_iterator.data == NULL) &&
           list_is_empty(&event_fifo_list))
    {
        ex10_cond_wait(&list_cond, &list_mutex);
    }
    ex10_mutex_unlock(&list_mutex);
}

static bool packet_wait_with_timeout(uint32_t timeout_us)
{
    bool timeout_expired = false;
    ex10_mutex_lock(&list_mutex);
    while ((event_packets_iterator.data == NULL) &&
           list_is_empty(&event_fifo_list) && (timeout_expired == false))
    {
        int const result =
            ex10_cond_timed_wait_us(&list_cond, &list_mutex, timeout_us);
        timeout_expired = (result == ETIMEDOUT);
    }
    ex10_mutex_unlock(&list_mutex);
    return timeout_expired;
}

static void packet_unwait(void)
{
    ex10_cond_signal(&list_cond);
}

static const struct Ex10EventFifoQueue event_fifo_queue = {
    .init                     = init,
    .list_node_push_back      = list_node_push_back,
    .packet_peek              = packet_peek,
    .packet_remove            = packet_remove,
    .packet_wait              = packet_wait,
    .packet_wait_with_timeout = packet_wait_with_timeout,
    .packet_unwait            = packet_unwait,
};

const struct Ex10EventFifoQueue* get_ex10_event_fifo_queue(void)
{
    return &event_fifo_queue;
}
