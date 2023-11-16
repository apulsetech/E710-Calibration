/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2021 Impinj, Inc. All rights reserved.                      *
 *                                                                           *
 *****************************************************************************/

#include "ex10_api/list_node.h"

static void list_node_init(struct Ex10ListNode* node)
{
    node->next = node;
    node->prev = node;
}

/**
 * @private
 * Unlink a list node from its neighbors, thereby removing it from the list.
 * @warning node->next, node->prev still point to the original list.
 * @see list_node_remove().
 */
static void list_node_unlink(struct Ex10ListNode* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

static void list_node_insert_next(struct Ex10ListNode* head_node,
                                  struct Ex10ListNode* node)
{
    list_node_unlink(node);

    node->next = head_node->next;
    node->prev = head_node;

    head_node->next->prev = node;
    head_node->next       = node;
}

static void list_node_insert_prev(struct Ex10ListNode* head_node,
                                  struct Ex10ListNode* node)
{
    list_node_unlink(node);

    node->next = head_node;
    node->prev = head_node->prev;

    head_node->prev->next = node;
    head_node->prev       = node;
}

static size_t list_node_count(struct Ex10ListNode const* first,
                              struct Ex10ListNode const* last)
{
    size_t node_count = 0u;
    for (struct Ex10ListNode const* iter = first; iter != last;
         iter                            = iter->next)
    {
        node_count += 1u;
    }
    return node_count;
}

static void list_node_insert_before(struct Ex10ListNode* node,
                                    struct Ex10ListNode* first,
                                    struct Ex10ListNode* last)
{
    // last is the node beyond the nodes being inserted.
    // last_prev will be the last node inserted.
    struct Ex10ListNode* last_prev = last->prev;

    // Point the neighbors of first, last_prev around them so that
    // the neighobrs are now connected.
    first->prev->next     = last_prev->next;
    last_prev->next->prev = first->prev;

    node->prev->next = first;
    first->prev      = node->prev;

    last_prev->next = node;
    node->prev      = last_prev;
}

static void list_node_remove(struct Ex10ListNode* node)
{
    list_node_unlink(node);
    list_node_init(node);
}

static struct Ex10ListNode* list_node_remove_forward(struct Ex10ListNode* node)
{
    struct Ex10ListNode* next = node->next;
    list_node_remove(node);
    return next;
}

static struct Ex10ListNode* list_node_remove_reverse(struct Ex10ListNode* node)
{
    struct Ex10ListNode* prev = node->prev;
    list_node_remove(node);
    return prev;
}

static bool list_node_is_linked(struct Ex10ListNode const* node)
{
    return (node->next != node) && (node->prev != node);
}

static struct Ex10ListNodeHelper const ex10_list_node = {
    .init           = list_node_init,
    .insert_next    = list_node_insert_next,
    .insert_prev    = list_node_insert_prev,
    .count          = list_node_count,
    .insert_before  = list_node_insert_before,
    .remove         = list_node_remove,
    .remove_forward = list_node_remove_forward,
    .remove_reverse = list_node_remove_reverse,
    .is_linked      = list_node_is_linked,
};

struct Ex10ListNodeHelper const* get_ex10_list_node_helper(void)
{
    return &ex10_list_node;
}