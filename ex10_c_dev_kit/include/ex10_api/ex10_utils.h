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

#include <sys/types.h>

#include "ex10_api/application_registers.h"
#include "ex10_api/ex10_protocol.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/gen2_tx_command_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/// The zero password is the default password of tags. It is also the value
/// to restore the tag to in examples.
#define ZERO_ACCESS_PWD ((uint32_t)0x00000000)

/// The non_zero password is used for all examples to align to one non-zero
/// password.
#define NON_ZERO_ACCESS_PWD ((uint32_t)0x44445555)

/**
 * Swaps the two bytes of a uint16_t. Useful for correcting endian-ness
 * of uint16_t values reported from the (big-endian) Gen2 tag.
 */
uint16_t ex10_swap_bytes(uint16_t value);

/**
 * Get the default board gpio setup from the board spec and send that
 * setup to the Ex10 device.
 *
 */
struct Ex10Result ex10_set_default_gpio_setup(void);

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
 * @retval         -1 Indicates ~~an error occurred when emptying the FIFO.
 */
ssize_t ex10_discard_packets(bool print_packets,
                             bool flush_packets,
                             bool debug_aggregate_op);

/**
 * Perform a deep copy of an EventFifo packet from one EventFifoPacket
 * struct to another.  the caller is responsible for allocating and
 * correctly setting up the backing memory in the destination packet.
 *
 * @param dst   pointer to the destination packet
 * @param src   pointer to the source packet
 */
struct Ex10Result ex10_deep_copy_packet(struct EventFifoPacket*       dst,
                                        struct EventFifoPacket const* src);

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
 * @retval struct Ex10Result fails if either the EPC or the TID length was
 *         larger than the struct TagReadData allocation. To determine which
 *         copy failed, the destination length fields should be compared
 *         with the source length fields.
 */
struct Ex10Result ex10_copy_tag_read_data(struct TagReadData*         dst,
                                          struct TagReadFields const* src);

/** Print the Command Result fields using ex10_eprintf(). */
void ex10_eprint_command_result_fields(struct CommandResultFields const* error);

/**
 * Read 4 bytes from a data stream, and return the little-endian uint32_t value.
 *
 * @param bytes     The byte stream to read from.
 * @return uint32_t The unsigned 32-bit value.
 */
uint32_t ex10_bytes_to_uint32(void const* bytes);

/**
 * Read 2 bytes from a data stream, and return the little-endian uint16_t value.
 *
 * @param bytes     The byte stream to read from.
 * @return uint16_t The unsigned 16-bit value.
 */
uint16_t ex10_bytes_to_uint16(void const* bytes);

/**
 * Width specific absolute value functions.
 *
 * @param signed_value The signed value of a width specific integer.
 * @return The unsigned absolute value of the associated unsigned value.
 */
uint16_t abs_int16(int16_t signed_value);
uint32_t abs_int32(int32_t signed_value);
/** @} */

void ex10_fill_u32(uint32_t* dest, uint32_t value, size_t count);

/**
 * A common function for read rate calculation.
 *
 * @param tag_count   The number of tags inventoried.
 * @param duration_us The duration in microseconds.
 * @return uint32_t   The tag read rate in tags/second.
 */
uint32_t ex10_calculate_read_rate(uint32_t tag_count, uint32_t duration_us);

#ifdef __cplusplus
}
#endif
