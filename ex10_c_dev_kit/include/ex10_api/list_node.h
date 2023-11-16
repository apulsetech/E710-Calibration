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

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct Ex10ListNode
 * A linked list node.
 */
struct Ex10ListNode
{
    void*                data;
    struct Ex10ListNode* next;
    struct Ex10ListNode* prev;
};

struct Ex10ListNodeHelper
{
    /**
     * Initialize a Ex10ListNode by setting the prev and next pointers to
     * itself.
     *
     * @param node The Ex10ListNode struct to initialize.
     */
    void (*init)(struct Ex10ListNode* node);

    /**
     * Insert a Ex10ListNode after another Ex10ListNode.
     *
     * @param head_node The list node onto which node will be appended.
     * @param node      The node to insert after the head_node.
     */
    void (*insert_next)(struct Ex10ListNode* head_node,
                        struct Ex10ListNode* node);

    /**
     * Insert a Ex10ListNode before another Ex10ListNode.
     *
     * @param head_node The list node onto which node will be prepended.
     * @param node      The node to insert before the head_node.
     */
    void (*insert_prev)(struct Ex10ListNode* head_node,
                        struct Ex10ListNode* node);

    /**
     * Determine the number of nodes whcih are linked between [first, last).
     *
     * @param first The node to begin counting from.  Counting includes this
     * node.
     * @param last  The last node to terminate count. Counting excludes this
     * node.
     *
     * @return size_t The number of linked nodes [first, last).
     */
    size_t (*count)(struct Ex10ListNode const* first,
                    struct Ex10ListNode const* last);

    /**
     * Insert the range of nodes [first, last) before a specified list node.
     * This disconnects the range of nodes from the previous list.
     *
     * @param node  The node to prepend the list range in front of.
     * @param first The first node to insert.
     * @param last  The terminating forward node in the list to insert.
     *              This node is not removed from the original list and is not
     *              inserted into the new list.
     */
    void (*insert_before)(struct Ex10ListNode* node,
                          struct Ex10ListNode* first,
                          struct Ex10ListNode* last);

    /**
     * Remove this node from list containment and safely point its next, prev
     * pointers at itself.
     */
    void (*remove)(struct Ex10ListNode* node);

    /**
     * Remove the node from the list in the manner a forward iterator
     * remove opperation would perform.
     *
     * @return struct Ex10ListNode* The list node in back of the node removed.
     */
    struct Ex10ListNode* (*remove_forward)(struct Ex10ListNode* node);

    /**
     * Remove the node from the list in the manner a reverse iterator
     * remove opperation would perform.
     *
     * @return struct Ex10ListNode* The list node in front of the node removed.
     */
    struct Ex10ListNode* (*remove_reverse)(struct Ex10ListNode* node);

    /**
     * Do the list node prev, next pointers point to something other than
     * itself?
     *
     * @param node The node being queried.
     *
     * @return bool true if the list is not self-referencing.
     * @note Uninitialized next, prev pointers will appear linked.
     */
    bool (*is_linked)(struct Ex10ListNode const* node);
};

struct Ex10ListNodeHelper const* get_ex10_list_node_helper(void);

#ifdef __cplusplus
}
#endif
