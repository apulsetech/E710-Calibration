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

#include "ex10_api/event_fifo_packet_types.h"
#include "ex10_api/rf_mode_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum TagAccessResult the result code for access commands
 */
enum TagAccessResult
{
    // Tag access commands sent successfully
    TagAccessSuccess,
    // Tag lost before/during access commands.  This is normally caused
    // by regulatory ramp down while in the halted state
    TagAccessTagLost,
    // Writing halted sequence to the Ex10 device did not complete due to
    // an error
    TagAccessHaltSequenceWriteError,
};

/**
 * @struct These are the use case parameters used to run the inventory
 * and they are generally passed on to the inventory module to setup
 * the inventory rounds.
 */
struct Ex10TagAccessUseCaseParameters
{
    uint8_t      antenna;
    enum RfModes rf_mode;
    int16_t      tx_power_cdbm;
    uint8_t      initial_q;
    uint8_t      session;
    uint8_t      target;
    uint8_t      select;
    bool         send_selects;
};

enum HaltedCallbackResult
{
    // ACK the tag and continue inventory round
    AckTagAndContinue,
    // NAK the tag and continue inventory round
    NakTagAndContinue,
};

/**
 * @struct Ex10TagAccessUseCase
 */
struct Ex10TagAccessUseCase
{
    /**
     * Initialize the Ex10TagAccessUseCase object.
     * This must be called after the Ex10 core has been initialized
     */
    struct Ex10Result (*init)(void);

    /**
     * Release any resources used by the
     * Ex10TagAccessUseCase object.
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*deinit)(void);

    /**
     * Register a callback that will be called with the tag read reports
     * and will indicate if the LMAC has successfully halted on the tag
     *
     * @param tag_halted_callback
     *        Notify the client when a tag is singulated. The corresponding
     *        TagRead packet also indicates if theLMAC successfully halted
     *        on the tag.  When this function returns, the LMAC will continue
     *        the inventory round to the next tag.
     */
    void (*register_halted_callback)(
        void (*tag_halted_callback)(struct EventFifoPacket const*,
                                    enum HaltedCallbackResult*,
                                    struct Ex10Result*));

    /**
     * Run an inventory round and halt on all singulated tags
     *
     * @param params a pointer to the Ex10TagAccessUseCaseParameters
     *               structure.
     * @return Info about any encountered errors.
     */
    struct Ex10Result (*run_inventory)(
        struct Ex10TagAccessUseCaseParameters* params);

    /**
     * Execute Access commands that are enabled.  Should only
     * be called from the halted callback that was registered below.
     */
    enum TagAccessResult (*execute_access_commands)(void);

    /**
     *  A helper function to retrieve the access packet from the event
     *  fifo packet
     *
     * @return  This function returns a copy of the access packet received
     *          (it is a copy so the buffer can be released)
     */
    struct EventFifoPacket const* (*get_fifo_packet)(void);

    /**
     * A helper function to remove packets from the event fifo queue.
     */
    void (*remove_fifo_packet)(void);

    /**
     * This function verifies that the received Event FIFO packet type is
     * 'Halted' and if confirmed, removes the packet data.
     *
     * @return bool Indicates whether removed EventFifo packet is 'Halted' type.
     * @retval true Received and removed an 'Halted' packet.
     * @retval false Expected Gen2 packet was not received or the received
     *         response was not as expected.
     */
    bool (*remove_halted_packet)(void);
};

struct Ex10TagAccessUseCase const* get_ex10_tag_access_use_case(void);

#ifdef __cplusplus
}
#endif
