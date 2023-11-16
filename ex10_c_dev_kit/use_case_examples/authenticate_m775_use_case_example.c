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

/**
 * @file authenticate_m775_use_case_example.c
 * @details This example uses the ex10 tag access use case to halt on all the
 * m775 tags in the field of view and execute the authenticate command. On
 * each halt, the reader sends a unique challenge, then, the tag responds to
 * the reader with encrypted tag data. The authenticate command tag respond
 * includes the 'challenge', 'tag shortened TID', and 'Tag Response'.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ex10_use_cases/ex10_tag_access_use_case.h"

#include "calibration.h"

#include "board/ex10_osal.h"
#include "board/ex10_random.h"

#include "ex10_api/board_init_core.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_gen2_reply_string.h"
#include "ex10_api/ex10_inventory.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/gen2_commands.h"
#include "ex10_api/gen2_tx_command_manager.h"
#include "ex10_api/print_data.h"
#include "ex10_api/rf_mode_definitions.h"
#include "ex10_regulatory/ex10_default_region_names.h"

#include "utils/ex10_inventory_command_line.h"
#include "utils/ex10_use_case_example_errors.h"

/**
 * The expected number of bits in the response from an M775 tag to a properly
 * constructed Authenticate command. For details see set_authenticate_command()
 * below.
 */
static const uint16_t authenticate_rep_len_bits = 128;

struct AuthenticateChallengeAndReply
{
    uint8_t challenge[6];
    uint8_t shortened_tid[8];
    uint8_t tag_response[8];
};

/**
 * Decode M775 tag reply for the authenticate command
 *
 * @param [out] reply_data The authenticate command reply data
 * @param reply_length_bits The length of the reply data in bits
 * @param m775_auth_response The decoded authenticate command reply
 * @return true Decode was successful
 * @return false Decode failed
 */
static bool decode_m775_authenticate_reply(uint8_t const* reply_data,
                                           size_t         reply_length_bits,
                                           uint8_t*       m775_auth_response)
{
    /**
     * Tag will reply with In-process reply format with the length field
     * included, so reply will be at least 57 bits long:
     * - Barker code (7 bits)
     * - Done        (1 bit)
     * - Header      (1 bit)
     * - Length      (16 bits)
     * - Response    (variable)
     * - RN          (16 bits)
     * - CRC         (16 bits)
     */
    const size_t min_in_process_reply_len_bits = 57u;
    uint16_t     rx_length                     = reply_data[1] & 0x7Eu;
    rx_length <<= 8u;
    rx_length |= reply_data[2];

    if (reply_length_bits - min_in_process_reply_len_bits != rx_length)
    {
        ex10_ex_eprintf(
            "expected response length: %zu bits vs calculated length: %zu\n",
            reply_length_bits - min_in_process_reply_len_bits,
            rx_length);

        const uint8_t rx_barker_code = (reply_data[0] >> 1u) & 0x7Eu;
        const uint8_t rx_done        = (reply_data[0] & 0x1u);
        const uint8_t rx_header      = (reply_data[1] >> 7u) & 0x1u;
        const uint8_t rx_even_parity = (reply_data[3] >> 7u) & 0x1u;

        ex10_ex_eputs(
            "\nBarker code: 0x%x\nDone: 0x%x\nHeader: 0x%x\nParity: 0x%x\n",
            rx_barker_code,
            rx_done,
            rx_header,
            rx_even_parity);

        return false;
    }

    uint8_t rx_response_byte = 0u;
    for (size_t i = 0u; i < (rx_length / 8u); i++)
    {
        rx_response_byte = (uint8_t)(reply_data[3u + i] << 1u) |
                           ((reply_data[3u + i + 1u] >> 7u) & 0x1u);
        m775_auth_response[i] = rx_response_byte;
    }

    return true;
}

/**
 * This function verifies that the received Event FIFO packet type is
 * 'Gen2Transaction' and copies the packet information to the given parameter.
 *
 * @param [out]  authenticate_summary Authentication command reply
 * @return bool Indicating whether the authenticate reply was decoded
 *              successfully.
 * @retval true Received a 'Gen2Transaction' packet and successfully copied the
 *              packet data.
 * @retval false Expected Gen2 packet was not received or the received response
 *               was not as expected.
 */
static bool get_authenticate_command_reply(
    struct AuthenticateChallengeAndReply* authenticate_summary)
{
    struct Ex10TagAccessUseCase const* tauc = get_ex10_tag_access_use_case();
    struct Ex10Gen2Commands const*     gen2_commands = get_ex10_gen2_commands();
    uint16_t                           reply_words[8] = {0u};
    struct Gen2Reply reply = {.error_code = NoError, .data = reply_words};

    // Get the packet for authenticate command
    struct EventFifoPacket const* packet = tauc->get_fifo_packet();
    get_ex10_event_fifo_printer()->print_packets(packet);
    if (packet->packet_type != Gen2Transaction)
    {
        ex10_ex_eprintf("Got unexpected packet, expected Gen2Transaction\n");
        return false;
    }

    // Decode the authenticate packet and check for error
    gen2_commands->decode_reply(Gen2Authenticate, packet, &reply);

    // When the reply does not authenticate properly, it likely that the
    // tag does support the Authenticate command. Return an Ex10Result that
    // is not an error, but indicates that the tag should be ignored.
    if (reply.error_code != NoError)
    {
        ex10_ex_eprintf("Authenticate command decode failed:\n");
        get_ex10_gen2_commands()->print_reply(reply);
        return false;
    }

    // Decode the authenticate message of an m775 tag
    uint8_t m775_auth_response[16];
    if (decode_m775_authenticate_reply(
            packet->dynamic_data,
            packet->static_data->gen2_transaction.num_bits,
            m775_auth_response) == false)
    {
        ex10_ex_eprintf("Decoding authenticate reply failed\n");
        return false;
    }

    int const copy_result_1 =
        ex10_memcpy(authenticate_summary->shortened_tid,
                    sizeof(authenticate_summary->shortened_tid),
                    m775_auth_response,
                    sizeof(authenticate_summary->shortened_tid));

    int const copy_result_2 = ex10_memcpy(
        authenticate_summary->tag_response,
        sizeof(authenticate_summary->tag_response),
        m775_auth_response + sizeof(authenticate_summary->shortened_tid),
        sizeof(authenticate_summary->tag_response));

    if (copy_result_1 != 0 || copy_result_2 != 0)
    {
        ex10_ex_eprintf("ex10_memcpy() failed\n");
        return false;
    }

    tauc->remove_fifo_packet();
    return true;
}

/**
 * Before starting inventory, setup Gen2 Authenticate command in Gen2 buffer.
 *
 * @param [out] msg_buffer Authenticate command message
 * @param msg_buffer_size Length of the authenticate command message
 * @return struct Ex10Result Indicates success or failure.
 */
static struct Ex10Result set_authenticate_command(uint8_t* msg_buffer,
                                                  size_t   msg_buffer_size)
{
    struct BitSpan auth_message = {
        .data   = msg_buffer,
        .length = msg_buffer_size * 8u,
    };

    // Create an authenticate command
    struct AuthenticateCommandArgs authenticate_args = {
        .send_rep     = true,
        .inc_rep_len  = true,
        .csi          = 1u,
        .length       = (uint16_t)auth_message.length,
        .message      = &auth_message,
        .rep_len_bits = authenticate_rep_len_bits,
    };

    struct Gen2CommandSpec authenticate_cmd = {
        .command = Gen2Authenticate,
        .args    = &authenticate_args,
    };

    struct Ex10Gen2TxCommandManager const* g2tcm =
        get_ex10_gen2_tx_command_manager();
    g2tcm->clear_local_sequence();
    size_t cmd_index = 0;

    // Encode the authenticate command and append it to the command buffer
    struct Ex10Result const gen2_encode_result =
        g2tcm->encode_and_append_command(&authenticate_cmd, 0, &cmd_index);
    if (gen2_encode_result.error || cmd_index != 0u)
    {
        ex10_ex_eprintf("g2tcm->encode_and_append_command failed:\n");
        print_ex10_result(gen2_encode_result);
        return gen2_encode_result;
    }

    // Write the command buffer to the Ex10 device
    struct Ex10Result const gen2_write_result = g2tcm->write_sequence();
    if (gen2_write_result.error)
    {
        ex10_ex_eprintf("g2tcm->write_sequence failed:\n");
        print_ex10_result(gen2_write_result);
        return gen2_write_result;
    }

    // Enable the authenticate command
    bool gen2_enables[MaxTxCommandCount];
    ex10_memzero(gen2_enables, sizeof(gen2_enables));
    gen2_enables[cmd_index] = true;

    struct Ex10Result const gen2_enables_result = g2tcm->write_halted_enables(
        gen2_enables, MaxTxCommandCount, &cmd_index);
    if (gen2_enables_result.error)
    {
        ex10_ex_eprintf("g2tcm->write_halted_enables() failed:\n");
        print_ex10_result(gen2_enables_result);
        return gen2_enables_result;
    }

    return make_ex10_success();
}

/**
 * Get a random challenge for the authenticate command
 *
 * @param [out] msg_buffer Generated random challenge value
 * @param msg_buffer_size The size of the random challenge to create
 * @return true Succeed to create a random challenge
 * @return false Failed to create a random challenge
 */
static bool get_random_challenge(uint8_t* msg_buffer, ssize_t msg_buffer_size)
{
    for (ssize_t idx = 0; idx < msg_buffer_size; idx++)
    {
        msg_buffer[idx] = (uint8_t)get_ex10_random()->get_random();
    }

    // Bits 0-5 on the first byte are 'AuthMethod', 'RFU' and 'Include TID'
    // Setting these to correct values.
    msg_buffer[0] &= 0x3u;
    msg_buffer[0] |= 0x4u;

    return true;
}

/**
 * This function is registered as the callback with the Ex10TagAccessUseCase
 * object. When a tag has been inventoried and transitioned into the
 * Acknowledged state, it is considered 'halted'. This callback is used to
 * notify the client with TagRead EventFifo packets. Performing Tag Access
 * commands should be performed within this function.
 *
 * @param packet The TagRead read packet.
 * @return enum HaltedCallbackResult
 *         Whether the Impinj Reader Chip should ACK the tag or NAK the tag.
 */
static void halted_on_tag_callback(struct EventFifoPacket const* packet,
                                   enum HaltedCallbackResult*    cb_result,
                                   struct Ex10Result*            ex10_result)
{
    *ex10_result                            = make_ex10_success();
    struct Ex10TagAccessUseCase const* tauc = get_ex10_tag_access_use_case();

    if (packet->packet_type != TagRead)
    {
        ex10_ex_eprintf("Got unexpected packet in the callback\n");
        tauc->remove_fifo_packet();
        // Error condition, we return and NAK the tag.
        *cb_result = NakTagAndContinue;
        return;
    }

    get_ex10_event_fifo_printer()->print_packets(packet);
    bool const halted_on_tag = packet->static_data->tag_read.halted_on_tag;
    // Discard the packet from the queue so we can look for some access packets.
    tauc->remove_fifo_packet();

    if (halted_on_tag == false)
    {
        // The LMAC read a tag, but did not successfully halt on it
        // (could have failed CRC in the ReqRN or hit a regulatory timer)
        // so we just return.  Note that the LMAC will have already continued
        // to the next slot on its own.
        ex10_ex_eprintf("LMAC failed to halt on tag\n");
        *cb_result = NakTagAndContinue;
        return;
    }

    // If this callback got called the LMAC halted on a tag and
    // should have sent a halted message back after the TagRead packet
    // so we verify that it is there, and discard it (note that the
    // packet pointer is no longer valid as it was removed above).
    if (tauc->remove_halted_packet() == false)
    {
        // Looks like there wasn't a halted packet, which suggests that
        // the LMAC didn't successfully halt on the tag, so we have the
        // same response as the halted_on_tag test above.
        // Actually, this path should not happen.
        *cb_result = NakTagAndContinue;
        return;
    }

    struct AuthenticateChallengeAndReply authenticate_summary;
    ex10_memzero(&authenticate_summary, sizeof(authenticate_summary));

    // Create a random authenticate message
    if (get_random_challenge(authenticate_summary.challenge,
                             sizeof(authenticate_summary.challenge)) == false)
    {
        ex10_ex_eprintf("Failed to obtain a random challenge\n");
        *cb_result = NakTagAndContinue;
        return;
    }

    // Setup and enable authenticate command.
    *ex10_result = set_authenticate_command(
        authenticate_summary.challenge, sizeof(authenticate_summary.challenge));
    if (ex10_result->error)
    {
        *cb_result = NakTagAndContinue;
        return;
    }

    // Execute the authenticate command
    enum TagAccessResult const result = tauc->execute_access_commands();
    if (result != TagAccessSuccess)
    {
        // Looks like we might have lost the tag so nothing more to do here
        // and NAK the tag just in case.
        *cb_result = NakTagAndContinue;
        return;
    }

    // Process the received Gen2Transaction packet.
    if (get_authenticate_command_reply(&authenticate_summary) == false)
    {
        // The singulated tag does not support the authenticate command or
        // there was an error processing the tag reply.
        // Sending an Ack will release the tag from the inventory round.
        // If there were a application specific reason to retry the
        // authenticate command, then the tag should be Nak'd.
        *cb_result = NakTagAndContinue;
        return;
    }

    // Print the reply
    ex10_ex_printf("Challenge:\t\t0x");
    ex10_print_data(authenticate_summary.challenge,
                    sizeof(authenticate_summary.challenge),
                    DataPrefixNone);

    ex10_ex_printf("Tags Shortened TID:\t0x");
    ex10_print_data(authenticate_summary.shortened_tid,
                    sizeof(authenticate_summary.shortened_tid),
                    DataPrefixNone);

    ex10_ex_printf("Tag Response:\t\t0x");
    ex10_print_data(authenticate_summary.tag_response,
                    sizeof(authenticate_summary.tag_response),
                    DataPrefixNone);


    // The LMAC will return to the halted state when the Access commands
    // are done, so this consumes that packet
    if (tauc->remove_halted_packet() == false)
    {
        *ex10_result = make_ex10_app_error(Ex10ApplicationMissingHaltedPacket);
        *cb_result   = NakTagAndContinue;  // missing halted packet
        return;
    }

    // we completed the transactions we want; continue the inventory round
    *cb_result = AckTagAndContinue;
    return;
}

static struct Ex10Result authenticate_command_example(
    struct InventoryOptions const* inventory_options)
{
    ex10_ex_printf("Starting authenticate command use case example\n");

    struct Ex10TagAccessUseCase const* tauc = get_ex10_tag_access_use_case();
    tauc->init();
    tauc->register_halted_callback(halted_on_tag_callback);

    uint8_t const target =
        (inventory_options->target_spec == 'B') ? target_B : target_A;
    struct Ex10TagAccessUseCaseParameters params = {
        .antenna       = inventory_options->antenna,
        .rf_mode       = inventory_options->mode.rf_mode_id,
        .tx_power_cdbm = inventory_options->tx_power_cdbm,
        .initial_q     = inventory_options->initial_q,
        .session       = (uint8_t)inventory_options->session,
        .target        = target,
        .select        = (uint8_t)SelectAll,
        .send_selects  = false};

    if (inventory_options->frequency_khz != 0)
    {
        get_ex10_active_region()->set_single_frequency(
            inventory_options->frequency_khz);
    }

    if (inventory_options->remain_on)
    {
        get_ex10_active_region()->disable_regulatory_timers();
    }

    struct Ex10Result const ex10_result = tauc->run_inventory(&params);
    get_ex10_rf_power()->stop_op_and_ramp_down();
    return ex10_result;
}

int main(int argc, char const* const argv[])
{
    struct InventoryOptions inventory_options = {
        .region_name   = "FCC",
        .read_rate     = 0u,
        .antenna       = 1u,
        .frequency_khz = 0u,
        .remain_on     = false,
        .tx_power_cdbm = 3000,
        .mode          = {.rf_mode_id = mode_103},
        .target_spec   = 'D',
        .initial_q     = 8,
        .session       = SessionS2,
    };

    struct Ex10Result const ex10_result_command_line =
        ex10_inventory_parse_command_line(&inventory_options, argv, argc);
    ex10_print_inventory_command_line_settings(&inventory_options);
    if (ex10_result_command_line.error || ex10_command_line_help_requested())
    {
        return ex10_result_command_line.error ? EINVAL : 0;
    }

    enum Ex10RegionId const region_id =
        get_ex10_default_region_names()->get_region_id(
            inventory_options.region_name);

    struct Ex10Result ex10_result =
        ex10_core_board_setup(region_id, DEFAULT_SPI_CLOCK_HZ);
    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_core_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_core_board_teardown();
        return -1;
    }

    ex10_result = ex10_set_default_gpio_setup();
    get_ex10_calibration()->init(get_ex10_protocol());

    if (ex10_result.error == false)
    {
        ex10_result = authenticate_command_example(&inventory_options);
        if (ex10_result.error == true)
        {
            print_ex10_app_result(ex10_result);
        }
    }
    else
    {
        print_ex10_result(ex10_result);
    }

    ex10_core_board_teardown();
    return ex10_result.error ? -1 : 0;
}
