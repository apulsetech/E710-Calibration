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

#include "ex10_api/ex10_inventory_sequence.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct Ex10InventorySequenceUseCase
 * The inventory sequence use case interface.
 */
struct Ex10InventorySequenceUseCase
{
    /**
     * Initialize the Ex10InventorySequenceUseCase object.
     * This must be called after the Ex10 core has been initialized.
     */
    struct Ex10Result (*init)(void);

    /**
     * Release any resources used by the Ex10InventorySequenceUseCase object.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*deinit)(void);

    /**
     * Register a Callback to subscribe to TagRead and InventoryRoundSummary
     * packets during the set of inventory sequences.
     *
     * @note This function must be called before calling
     *       run_inventory_sequence() or it will have no effect.
     *
     * @param packet_subscriber_callback
     * A pointer to a function that will be called back during the inventory
     * sequence with pointers to packet data. If the callback Ex10Result
     * indicates an error, then the inventory sequence will terminate.
     */
    void (*register_packet_subscriber_callback)(
        void (*packet_subscriber_callback)(struct EventFifoPacket const*,
                                           struct Ex10Result*));

    /**
     * By default only the TagRead and InventoryRoundSummary packet types are
     * sent to the packet subscriber.
     *
     * @param enable_filter If set to false, all packets will be sent to the
     * packet subscriber. If true, the normal behavior of only sending the
     * TagRead and InventoryRoundSummary packets to the subscriber is enforced.
     */
    void (*enable_packet_filter)(bool enable_filter);

    /**
     * Get the inventory sequence that is actively in use.
     *
     * @return struct InventoryRoundSequence const*
     * A pointer to the inventory sequence that was passed in to the
     * run_inventory_sequence() function.
     */
    struct InventoryRoundSequence const* (*get_inventory_sequence)(void);

    /**
     * Get the inventory round that is active within the published packets
     * context.
     *
     * @return struct InventoryRoundConfigBasic const*
     * A pointer to the inventory round that is active within the
     * published packets context.
     */
    struct InventoryRoundConfigBasic const* (*get_inventory_round)(void);

    /**
     * Run a sequence of inventory rounds specified by a container of
     * struct InventoryRoundConfig nodes.
     *
     * If a packet subscriber is registered with the
     * Ex10InventorySequenceUseCase using the
     * register_packet_subscriber_callback() function, then this function will
     * block until the inventory sequence completes. If no packet subscriber
     * is registered, then this function returns once the inventory has started.
     *
     * @param inventory_sequence
     *  A pointer to a filled in struct InventoryRoundSequence, containing a
     *  valid array of struct InventoryRoundConfigBasic nodes.
     *
     * @return struct Ex10Result An indication of whether a run-time
     *         error has occurred during the initial sequencing of inventories.
     */
    struct Ex10Result (*run_inventory_sequence)(
        struct InventoryRoundSequence const* inventory_sequence);
};

struct Ex10InventorySequenceUseCase const* get_ex10_inventory_sequence_use_case(
    void);

#ifdef __cplusplus
}
#endif
