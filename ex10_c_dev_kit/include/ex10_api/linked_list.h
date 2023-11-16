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

#include "ex10_api/list_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct Ex10LinkedList
 * Contains the nodes of a linked list structure.
 *
 * An empty list contains only a sentinel node which points bidirectionally
 * to itself.
 */
struct Ex10LinkedList
{
    struct Ex10ListNode sentinel;
};

/**
 * Initialize the struct Ex10LinkedList to its empty state.
 *
 * @param list The struct Ex10ListNode to initialize.
 */
static inline void list_init(struct Ex10LinkedList* list)
{
    get_ex10_list_node_helper()->init(&list->sentinel);
}

/**
 * @return bool true  if the list contains no non-sentinel nodes.
 *              false if there are active nodes within the list.
 */
static inline bool list_is_empty(struct Ex10LinkedList const* list)
{
    return list->sentinel.next == &list->sentinel;
}

/**
 * @return size_t The number of nodes contained in the list.
 * @note Unlike lists, this function is O(n), ! O(1).
 */
static inline size_t list_size(struct Ex10LinkedList const* list)
{
    return get_ex10_list_node_helper()->count(list->sentinel.next,
                                              &list->sentinel);
}

/** @{
 * Get the Ex10ListNode from the front of the list to be used as a forward
 * iterator.
 */
static inline struct Ex10ListNode* list_begin(struct Ex10LinkedList* list)
{
    return list->sentinel.next;
}

static inline struct Ex10ListNode const* list_begin_const(
    struct Ex10LinkedList const* list)
{
    return list->sentinel.next;
}
/** @} */

/** @{
 * Get the Ex10ListNode from the back of the list to be used as a reverse
 * iterator.
 */
static inline struct Ex10ListNode* list_rbegin(struct Ex10LinkedList* list)
{
    return list->sentinel.prev;
}

static inline struct Ex10ListNode const* list_rbegin_const(
    struct Ex10LinkedList const* list)
{
    return list->sentinel.prev;
}
/** @} */

/** @{
 * In both the forward and reverse iteration cases the end() iteration value
 * will be the sentinel node. This can be used to test whether the list
 * iteration has reached one past the end of the valid range.
 *
 * Example: forward iteration across a list:
 * for (struct Ex10ListNode* iter = list_begin(list); iter != list_end(list);
 *      iter = iter->next) { ... }
 * Example: reverse iteration across a list:
 * for (struct Ex10ListNode* iter = list_rbegin(list); iter != list_end(list);
 *      iter = iter->prev) { ... }
 */
static inline struct Ex10ListNode* list_end(struct Ex10LinkedList* list)
{
    return &list->sentinel;
}

static inline struct Ex10ListNode const* list_end_const(
    struct Ex10LinkedList const* list)
{
    return &list->sentinel;
}
/** @} */

/**
 * Insert a list node to the front of the list.
 * @param list The list into which the node will be inserted.
 * @param node The node to insert into the list.
 */
static inline void list_push_front(struct Ex10LinkedList* list,
                                   struct Ex10ListNode*   node)
{
    get_ex10_list_node_helper()->insert_next(&list->sentinel, node);
}

/**
 * Insert a list node to the back of the list.
 * @param list The list into which the node will be inserted.
 * @param node The node to insert into the list.
 */
static inline void list_push_back(struct Ex10LinkedList* list,
                                  struct Ex10ListNode*   node)
{
    get_ex10_list_node_helper()->insert_prev(&list->sentinel, node);
}

/**
 * Remove the node from the front of the list.
 * @note The node is not destroyed.
 */
static inline void list_pop_front(struct Ex10LinkedList* list)
{
    if (!list_is_empty(list))
    {
        get_ex10_list_node_helper()->remove(list->sentinel.next);
    }
}

/**
 * Remove the node from the back of the list.
 * @note The node is not destroyed.
 */
static inline void list_pop_back(struct Ex10LinkedList* list)
{
    if (!list_is_empty(list))
    {
        get_ex10_list_node_helper()->remove(list->sentinel.prev);
    }
}

/** @{ Access the front Ex10ListNode in the list. */
static inline struct Ex10ListNode* list_front(struct Ex10LinkedList* list)
{
    return list->sentinel.next;
}

static inline struct Ex10ListNode const* list_front_const(
    struct Ex10LinkedList const* list)
{
    return list->sentinel.next;
}
/** @} */

/** @{ Access the back Ex10ListNode in the list. */
static inline struct Ex10ListNode* list_back(struct Ex10LinkedList* list)
{
    return list->sentinel.prev;
}

static inline struct Ex10ListNode const* list_back_const(
    struct Ex10LinkedList const* list)
{
    return list->sentinel.prev;
}
    /** @} */

#ifdef __cplusplus
}
#endif
