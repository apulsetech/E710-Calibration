/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2020 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ex10_api/application_register_definitions.h"
#include "ex10_api/board_init_core.h"
#include "ex10_api/bootloader_registers.h"
#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/gen2_commands.h"
#include "ex10_api/gen2_tx_command_manager.h"
#include "ex10_api/rf_mode_definitions.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum InventoryHelperReturns
 * This enum details the different reasons for return when using
 * 'simple_inventory' or 'continuous_inventory'.
 */
enum InventoryHelperReturns
{
    InvHelperSuccess        = 0,
    InvHelperOpStatusError  = 1,
    InvHelperStopConditions = 2,
    InvHelperTimeout        = 3,
};

// Stores information based on fifo data returns.
struct InfoFromPackets
{
    size_t             gen2_transactions;
    size_t             total_singulations;
    size_t             total_tid_count;
    size_t             times_halted;
    struct TagReadData access_tag;
};

struct InventoryHelperParams
{
    uint8_t const      antenna;
    enum RfModes const rf_mode;
    int16_t const      tx_power_cdbm;
    // Config parameters for the inventory control register
    struct InventoryRoundControlFields* inventory_config;
    // Config parameters for the inventory control 2 register
    struct InventoryRoundControl_2Fields const* inventory_config_2;
    // Whether to execute the send select op
    bool const send_selects;
    // If true, the radio does not turn off due to regulatory timing
    bool const remain_on;
    // Automatically flips the target between inventory rounds
    bool const              dual_target;
    uint32_t const          inventory_duration_ms;
    struct InfoFromPackets* packet_info;
    // Prints all incoming fifo packets
    bool const verbose;
};

struct ContInventoryHelperParams
{
    struct InventoryHelperParams*      inventory_params;
    struct StopConditions const*       stop_conditions;
    struct ContinuousInventorySummary* summary_packet;
};

struct Ex10Helpers
{
    /// Print the contents of the struct CommandResultFields.
    void (*print_command_result_fields)(
        struct CommandResultFields const* error);

    /// Checks the Gen2Reply for error and prints an error message.
    bool (*check_gen2_error)(struct Gen2Reply const* reply);

    /// Prints out useful debug around the aggregate op buffer
    void (*print_aggregate_op_errors)(
        const struct AggregateOpSummary agg_summary);

    /**
     * Purge FIFO of remaining packets. Print summary of each packet
     * @param print_packets Print each packet purged from the EventFifo queue.
     * @param flush_packets Uses the InsertFifoEvent command, with the
     *                      Trigger Interrupt flag set, to trigger an ReadFifo
     *                      command, allowing the host to receive all pending
     *                      EventFifo data held by the Reader Chip.
     * @param debug_aggregate_op Provides detailed information about an
     *                      AggregateOp failure, if an AggregateOpSummary
     *                      packet is encountered in the dumped packet stream.
     *
     * @return ssize_t On success, the number of EventFifo packets discarded.
     * @retval         -1 Indicates an error occurred when emptying the FIFO.
     */
    ssize_t (*discard_packets)(bool print_packets,
                               bool flush_packets,
                               bool debug_aggregate_op);

    /// Check if inventory round is halted on a tag
    bool (*inventory_halted)(void);

    /**
     * Clears the state of the passed struct which stores state collected from
     * event fifo packets.
     * @param return_info The accumulated inventory state.
     */
    void (*clear_info_from_packets)(struct InfoFromPackets* return_info);

    /**
     * When iterating through EventFifo packets, this function can be used to
     * accumulate the inventory state stored in the InfoFromPackets struct.
     * @param packet      The EventFifo packet to examine.
     * @param return_info The accumulated inventory state.
     * @note  If the EventFifo packet is of type TagRead then the
     *        return_info->access_tag values are filled in.
     */
    void (*examine_packets)(struct EventFifoPacket const* packet,
                            struct InfoFromPackets*       return_info);

    /**
     * Perform a deep copy of an EventFifo packet from one EventFifoPacket
     * struct to another.  The caller is responsible for the backing memory
     * of the destination's pointers and lengths
     *
     * @param dst The pointer to the destination packet
     * @param src The pointer to the source packet
     */
    bool (*deep_copy_packet)(struct EventFifoPacket*       dst,
                             struct EventFifoPacket const* src);

    /// Run inventory rounds until a timeout or halted
    enum InventoryHelperReturns (*simple_inventory)(
        struct InventoryHelperParams* ihp);

    /// Run continuous inventory until a stop condition is met or halted
    enum InventoryHelperReturns (*continuous_inventory)(
        struct ContInventoryHelperParams* cihp);

    /**
     * Copy the EventFifo TagRead extracted data into the destination type
     * struct TagReadData.
     *
     * @details
     * The struct TagReadFields contains data pointers into the relevant fields
     * of the EventFifo TagRead packet, allowing for easy data extraction.
     * The EventFifo packet will be deleted when the function
     * Ex10Reader.packet_remove() function is called, which releases the
     * EventFifo memory back to the pool for reuse.
     * The struct TagRead provides a destination for the TagRead data, allowing
     * the client to use the tag data beyond the lifetime of the EventFifo
     * TagRead packet.
     *
     * @return bool Indicates whether the copy operation was successful.
     * @retval true indicates that the tag was copied without error.
     * @retval false indicates that either the EPC or the TID length was
     *         larger than the struct TagReadData allocation. To determine which
     *         copy failed, the destination length fields should be compared
     *         with the source length fields.
     */
    bool (*copy_tag_read_data)(struct TagReadData*         dst,
                               struct TagReadFields const* src);

    /// Translate from RemainReason enum to a human-readable string
    const char* (*get_remain_reason_string)(enum RemainReason remain_reason);

    /// Swaps the two bytes of a uint16_t. Useful for correcting endian-ness
    /// of uint16_t values reported from the (big-endian) Gen2 tag.
    uint16_t (*swap_bytes)(uint16_t value);

    /// Runs the MeasureRssiOp, waits for the event fifo corresponding to the op
    /// finishing, and returns the log2 Rssi reported back.
    uint16_t (*read_rssi_value_from_op)(uint8_t rssi_count);

    /// Clears the gen2 buffer, adds a new command, and immediately sends it.
    struct Ex10Result (*send_single_halted_command)(
        struct Gen2CommandSpec* cmd_spec);

    /// Starting a a memory address, begins filling in the same u32 word given
    /// in value count number of times.
    void (*fill_u32)(uint32_t* dest, uint32_t value, size_t count);
};

const struct Ex10Helpers* get_ex10_helpers(void);

#ifdef __cplusplus
}
#endif
