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

/**
 * @file protected_mode_example.c
 * @details  This example shows how to use the protected mode on an Impinj
 *  tag that supports protected mode.
 *
 *  - This example assumes a single tag on antenna port one.
 *  - This example assumes a starting password of all 0s
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "board/ex10_osal.h"
#include "board/time_helpers.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/gen2_tx_command_manager.h"
#include "ex10_api/rf_mode_definitions.h"


/* Settings used when running this example */
static const uint32_t inventory_duration_ms = 5000;  // Duration in milliseconds
static const uint8_t  antenna               = 1;
static const uint16_t rf_mode               = mode_148;
static const uint16_t transmit_power_cdbm   = 2500;
static const uint8_t  initial_q             = 4;
static const uint8_t  session               = 0;

static struct InfoFromPackets packet_info = {0u, 0u, 0u, 0u, {0u}};

static uint32_t zero_access_pwd     = ZERO_ACCESS_PWD;
static uint32_t non_zero_access_pwd = NON_ZERO_ACCESS_PWD;

// Allows for testing the short range functionality of Impinj tags.
// This is not enabled for general testing, but if enabled, the power
// should be increased.
static bool test_sr_bit = false;

enum PROTECTED_MODE_ERROR
{
    PROTECTED_NO_ERROR = 0,
    PROTECTED_HALT_ERROR,
    PROTECTED_ENTER_ACCESS_ERROR,
    PROTECTED_CHANGE_PWD_ERROR,
    PROTECTED_SET_MODE_ERROR,
    PROTECTED_CREATE_SELECT_ERROR,
    NO_ARGUMENT_ERROR,
};

// Used to return information from a tag read specific to this example
struct ProtectedExampleInfo
{
    bool     protected_mode_enabled;
    bool     short_range_enabled;
    uint16_t page_info;
};

struct TagFoundInfo
{
    uint8_t epc_array[20];
    size_t  epc_length;
    bool    first_tag_found;
};

static struct TagFoundInfo tag_info = {{0}, 0, false};

/**
 * Send Gen2 command, wait for reply, and decode
 */
static int send_gen2_command_wait(struct Gen2CommandSpec* cmd_spec,
                                  struct Gen2Reply*       reply)
{
    const uint32_t                timeout = 1000;
    struct EventFifoPacket const* packet  = NULL;

    // Ensure we are not still in the previous transaction
    struct HaltedStatusFields halt_fields = {.halted = true, .busy = true};

    // Ensure any previous command is done first
    uint32_t start_time = get_ex10_time_helpers()->time_now();
    while (halt_fields.busy == true &&
           get_ex10_time_helpers()->time_elapsed(start_time) < timeout)
    {
        get_ex10_protocol()->read(&halted_status_reg, &halt_fields);
    }
    // Ensure we are still halted and not busy any more
    if (halt_fields.halted == false || halt_fields.busy == true)
    {
        return -1;
    }

    packet_info.gen2_transactions = 0;

    // Overwrite the buffer and send a single command
    struct Ex10Result ex10_result =
        get_ex10_helpers()->send_single_halted_command(cmd_spec);
    if (ex10_result.error)
    {
        ex10_ex_eprintf("Sending single halted command failed\n");
        return -1;
    }

    struct Ex10Reader const* reader = get_ex10_reader();
    start_time                      = get_ex10_time_helpers()->time_now();
    while (get_ex10_time_helpers()->time_elapsed(start_time) < timeout &&
           !packet_info.gen2_transactions)
    {
        packet = reader->packet_peek();
        while (packet != NULL)
        {
            get_ex10_helpers()->examine_packets(packet, &packet_info);
            if (packet->packet_type == Gen2Transaction)
            {
                get_ex10_gen2_commands()->decode_reply(
                    cmd_spec->command, packet, reply);
            }
            if (packet->packet_type == InventoryRoundSummary)
            {
                ex10_ex_eprintf(
                    "Inventory ended while waiting for Gen2 transaction\n");
                return -1;
            }
            reader->packet_remove();
            packet = reader->packet_peek();
        }
    }

    if (reply->error_code != NoError || !packet_info.gen2_transactions)
    {
        return -1;
    }
    return 0;
}

static int inventory_and_halt(struct Gen2CommandSpec* select_config,
                              bool                    expecting_tag)
{
    struct InventoryRoundControlFields inventory_config = {
        .initial_q        = initial_q,
        .session          = session,
        .select           = SelectAll,
        .target           = 0,
        .halt_on_all_tags = true,
        .tag_focus_enable = false,
        .fast_id_enable   = false,
    };

    struct InventoryRoundControl_2Fields inventory_config_2 = {
        .max_queries_since_valid_epc = 0};

    bool     round_done = true;
    uint32_t start_time = get_ex10_time_helpers()->time_now();

    // Clear the number of tags found so that if we halt, we can return
    packet_info.total_singulations = 0u;
    packet_info.gen2_transactions  = 0u;
    packet_info.total_tid_count    = 0u;

    struct Ex10Helpers const* helpers = get_ex10_helpers();
    helpers->discard_packets(false, true, false);

    // Add the select command of interest
    struct Ex10Gen2TxCommandManager const* g2tcm =
        get_ex10_gen2_tx_command_manager();
    bool select_enables[MaxTxCommandCount] = {0u};

    if (select_config)
    {
        size_t            cmd_index = 0;
        struct Ex10Result ex10_result =
            g2tcm->encode_and_append_command(select_config, 0, &cmd_index);
        if (ex10_result.error)
        {
            ex10_ex_eprintf(
                "Encoding and appending the select "
                "command failed\n");
            print_ex10_result(ex10_result);
            return -1;
        }
        select_enables[cmd_index] = true;
        ex10_result               = g2tcm->write_sequence();
        if (ex10_result.error)
        {
            ex10_ex_eprintf("Writing the command sequence failed\n");
            print_ex10_result(ex10_result);
            return -1;
        }

        ex10_result = g2tcm->write_select_enables(
            select_enables, MaxTxCommandCount, &cmd_index);
        if (ex10_result.error)
        {
            ex10_ex_eprintf("Gen2 command write enables failed.\n");
            print_ex10_result(ex10_result);
            return -1;
        }
    }

    struct Ex10Reader const* reader = get_ex10_reader();
    while (packet_info.total_singulations == 0)
    {
        if (get_ex10_time_helpers()->time_elapsed(start_time) >
            inventory_duration_ms)
        {
            break;
        }
        if (round_done)
        {
            round_done = false;
            struct Ex10Result ex10_result =
                reader->inventory(antenna,
                                  rf_mode,
                                  transmit_power_cdbm,
                                  &inventory_config,
                                  &inventory_config_2,
                                  (select_config) ? true : false,
                                  true);
            if (ex10_result.error)
            {
                ex10_discard_packets(true, true, true);
                return -1;
            }
        }

        struct EventFifoPacket const* packet = reader->packet_peek();
        while (packet)
        {
            helpers->examine_packets(packet, &packet_info);
            if (packet->packet_type == InventoryRoundSummary)
            {
                round_done = true;
            }
            else if (packet->packet_type == TagRead)
            {
                ex10_ex_printf("Tag found with epc: ");
                for (size_t i = 0; i < packet_info.access_tag.epc_length; i++)
                {
                    ex10_ex_printf("%d, ", packet_info.access_tag.epc[i]);
                }
                ex10_ex_printf("\n");
                if (tag_info.first_tag_found == 0)
                {
                    // Save the epc of the first tag to ensure we only use one
                    // tag for this test
                    ex10_memcpy(tag_info.epc_array,
                                sizeof(tag_info.epc_array),
                                packet_info.access_tag.epc,
                                packet_info.access_tag.epc_length);
                    tag_info.first_tag_found = true;
                }
                else
                {
                    // Check if the found tag matches the one previously found
                    if (0 != memcmp(tag_info.epc_array,
                                    packet_info.access_tag.epc,
                                    packet_info.access_tag.epc_length))
                    {
                        // If this is a new tag, it does not count for the test
                        packet_info.total_singulations--;
                    }
                }
                helpers->examine_packets(packet, &packet_info);
            }
            reader->packet_remove();
            packet = reader->packet_peek();
        }
    }

    if (expecting_tag)
    {
        // expecting tag - return -1 if no tag found
        if (!packet_info.total_singulations)
        {
            ex10_ex_eprintf("No tag found when expected\n");
            return -1;
        }
        return 0;
    }
    else
    {
        // not expecting tag - return -1 if tag found
        if (packet_info.total_singulations)
        {
            ex10_ex_eprintf("Tag found when not expected\n");
            return -1;
        }
        return 0;
    }
}

static int write_to_reserved(uint16_t page_to_write, uint16_t page_data)
{
    uint16_t         reply_words[10u] = {0};
    struct Gen2Reply reply = {.error_code = NoError, .data = reply_words};

    struct WriteCommandArgs write_args = {
        .memory_bank  = Reserved,
        .word_pointer = page_to_write,
        .data         = page_data,
    };
    struct Gen2CommandSpec write_cmd = {
        .command = Gen2Write,
        .args    = &write_args,
    };

    if (send_gen2_command_wait(&write_cmd, &reply) ||
        reply.error_code != NoError)
    {
        return -1;
    }
    return 0;
}

static void read_reserved_memory(uint16_t          word_pointer,
                                 uint8_t           word_count,
                                 struct Gen2Reply* reply)
{
    struct ReadCommandArgs read_args = {
        .memory_bank  = Reserved,
        .word_pointer = word_pointer,
        .word_count   = word_count,
    };

    struct Gen2CommandSpec read_cmd = {
        .command = Gen2Read,
        .args    = &read_args,
    };

    send_gen2_command_wait(&read_cmd, reply);
    if (reply->error_code != NoError ||
        reply->transaction_status != Gen2TransactionStatusOk ||
        reply->reply != Gen2Read)
    {
        ex10_ex_printf(
            "Reserved memory read returned - reply: %d, error_code: %d, "
            "transaction_status: %d",
            reply->reply,
            reply->error_code,
            reply->transaction_status);
    }
}

static struct ProtectedExampleInfo read_settings(void)
{
    uint16_t         reply_words[10u] = {0};
    struct Gen2Reply reply = {.error_code = NoError, .data = reply_words};

    // Read the settings from word 4 of reserved memory
    read_reserved_memory(4, 1, &reply);
    uint16_t settings_data = reply.data[0];

    ex10_ex_printf("Read Back: %d\n", settings_data);
    // base 0: bit 1 is P and bit 4 is SR
    bool protected   = (settings_data >> 1) & 1;
    bool short_range = (settings_data >> 4) & 1;

    struct ProtectedExampleInfo read_info = {
        .protected_mode_enabled = protected,
        .short_range_enabled    = short_range,
        .page_info              = settings_data,
    };
    return read_info;
}

static struct Ex10Result create_select_args(
    struct BitSpan*           protected_mode_pin,
    struct SelectCommandArgs* select_args)
{
    // We pass in the protected mode pin to use as
    // the mask. This allows the tag to respond
    // when in protected mode.
    select_args->target      = Session0;
    select_args->action      = Action001;
    select_args->memory_bank = SelectFile0;
    select_args->bit_pointer = 0;
    select_args->bit_count   = 32;
    select_args->mask        = protected_mode_pin;
    select_args->truncate    = false;

    // Ensure the passed mask was bit length 32 to match our expectation for the
    // protected mode password.
    if (protected_mode_pin->length != 32)
    {
        return make_ex10_sdk_error(Ex10ModuleUndefined,
                                   Ex10SdkErrorBadParamValue);
    }
    return make_ex10_success();
}

static int enter_access_mode(uint32_t access_password)
{
    /* Should be halted on a tag already */
    uint16_t         reply_words[10u] = {0};
    struct Gen2Reply reply = {.error_code = NoError, .data = reply_words};

    // Create structs to write the assess pwd in two steps
    uint16_t msb_pwd_value = access_password & 0xFFFF;
    uint16_t lsb_pwd_value = access_password >> 16;

    struct AccessCommandArgs msb_pwd_args = {
        .password = msb_pwd_value,
    };
    struct Gen2CommandSpec msb_pwd_cmd = {
        .command = Gen2Access,
        .args    = &msb_pwd_args,
    };
    struct AccessCommandArgs lsb_pwd_args = {
        .password = lsb_pwd_value,
    };
    struct Gen2CommandSpec lsb_pwd_cmd = {
        .command = Gen2Access,
        .args    = &lsb_pwd_args,
    };

    // We can not know if the response back will contain an error
    // or not, thus we will trust CRC and the tag handle in this test.
    ex10_ex_printf("Access command 1 sent\n");
    if (send_gen2_command_wait(&msb_pwd_cmd, &reply) ||
        reply.error_code != NoError)
    {
        return -1;
    }
    // We get a handle back from this first access command.
    // We will use this to ensure that the next commands are proper responses
    // rather than noise. This is important as an incorrect password means
    // the tag will not respond at all for a given security timeout. An error
    // in the received handle thus likely means a bad password was given here.
    struct AccessCommandReply* access_resp =
        (struct AccessCommandReply*)reply.data;
    uint16_t proper_tag_handle = access_resp->tag_handle;
    ex10_memzero(reply_words, sizeof(reply_words));

    ex10_ex_printf("Access command 2 sent\n");
    if (send_gen2_command_wait(&lsb_pwd_cmd, &reply) ||
        reply.error_code != NoError)
    {
        return -1;
    }
    // Check response from tag for proper handle
    access_resp = (struct AccessCommandReply*)reply.data;
    if (proper_tag_handle != access_resp->tag_handle)
    {
        return -1;
    }

    return 0;
}

static int change_access_pwd(uint32_t ending_pwd)
{
    ex10_ex_printf("Changing access pwd\n");
    // The memory on the tag is interpreted MSB thus on the tag...
    // u16 pieces[] = {0x1111, 0x2222}
    // u32 password = 0x22221111
    uint16_t pwd_word_0 = ending_pwd & 0xFFFF;
    uint16_t pwd_word_1 = ending_pwd >> 16;

    // Write the new password to memory
    if (write_to_reserved(2, pwd_word_0) || write_to_reserved(3, pwd_word_1))
    {
        return -1;
    }
    ex10_ex_printf("Password changed\n");
    return 0;
}

static int set_mode_state(bool enable)
{
    ex10_ex_printf("%s protected mode\n", enable ? "Entering" : "Leaving");

    // Read reserved memory for modify write
    struct ProtectedExampleInfo read_info = read_settings();

    // Modify the memory page to write in
    read_info.page_info &= ~(1u << 1u);
    read_info.page_info &= ~(1u << 4u);
    // To enable protected mode
    read_info.page_info |= ((uint16_t)enable << 1u);
    // To enable short range mode - this should be done
    // when the output power and tag distance is known
    if (test_sr_bit)
    {
        read_info.page_info |= ((uint16_t)enable << 4u);
    }

    // Write memory controlling protected mode and short range control
    // We change both together here though not necessary
    ex10_ex_printf("Writing to change protected and sr bit\n");
    if (write_to_reserved(4, read_info.page_info))
    {
        return -1;
    }

    // Read back memory to ensure the data was written
    ex10_ex_printf("Read back memory to ensure it was set properly\n");
    read_info = read_settings();
    if (read_info.protected_mode_enabled != enable ||
        read_info.short_range_enabled != (enable & test_sr_bit))
    {
        return -1;
    }

    ex10_ex_printf("%s protected mode successfully\n",
                   enable ? "Entered" : "Left");
    return 0;
}

static uint32_t run_protected_mode_example(uint8_t* non_zero_password_array)
{
    // Note: This example assumes a single tag in field of view.
    // Note: This example assumes a starting password of all 0s

    const uint32_t non_zero_pass_in = *((uint32_t*)non_zero_password_array);

    struct BitSpan non_zero_access_info = {non_zero_password_array, 32};

    struct SelectCommandArgs non_zero_select_args = {0};
    struct Ex10Result        ex10_result =
        create_select_args(&non_zero_access_info, &non_zero_select_args);
    if (ex10_result.error)
    {
        return PROTECTED_CREATE_SELECT_ERROR;
    }

    // The non zero select is must be sent to force a protected mode tag to
    // reveal itself.
    struct Gen2CommandSpec non_zero_select = {
        .command = Gen2Select,
        .args    = &non_zero_select_args,
    };

    get_ex10_protocol()->set_event_fifo_threshold(0);

    // clang-format off
    ex10_ex_printf("Find the tag and change the password\n");
    if(inventory_and_halt(NULL, true)) { return PROTECTED_HALT_ERROR; }
    if(change_access_pwd(non_zero_pass_in)) { return PROTECTED_CHANGE_PWD_ERROR; }
    struct Ex10Reader const* reader = get_ex10_reader();
    reader->stop_transmitting();
    // Note the sleep after all stop_transmitting to ensure the tag state
    // settles.
    sleep(2);

    ex10_ex_printf("Enter the password, then set protected mode\n");
    if(inventory_and_halt(NULL, true)) { return PROTECTED_HALT_ERROR; }
    if(enter_access_mode(non_zero_pass_in)) { return PROTECTED_ENTER_ACCESS_ERROR; }
    if(set_mode_state(true)) { return PROTECTED_SET_MODE_ERROR; }
    reader->stop_transmitting();
    sleep(2);

    ex10_ex_printf("Ensure we can not see the tag\n");
    if(inventory_and_halt(NULL, false)) { return PROTECTED_HALT_ERROR; }
    reader->stop_transmitting();
    sleep(2);

    ex10_ex_printf("Show how to find it again and return it to not-protected mode\n");
    if(inventory_and_halt(&non_zero_select, true)) { return PROTECTED_HALT_ERROR; }
    if(enter_access_mode(non_zero_pass_in)) { return PROTECTED_ENTER_ACCESS_ERROR; }
    if(set_mode_state(false)) { return PROTECTED_SET_MODE_ERROR; }
    if(change_access_pwd(zero_access_pwd)) { return PROTECTED_CHANGE_PWD_ERROR; }
    reader->stop_transmitting();
    sleep(2);

    ex10_ex_printf("Show we can find the tag normally now\n");
    if(inventory_and_halt(NULL, true)) { return PROTECTED_HALT_ERROR; }
    reader->stop_transmitting();
    sleep(2);
    // clang-format on

    return PROTECTED_NO_ERROR;
}

static uint32_t protected_bit_recovery(uint8_t* non_zero_password_array)
{
    const uint32_t non_zero_password_in = *((uint32_t*)non_zero_password_array);

    struct BitSpan non_zero_access_info = {non_zero_password_array, 32};

    struct SelectCommandArgs non_zero_select_args = {0};
    struct Ex10Result        ex10_result =
        create_select_args(&non_zero_access_info, &non_zero_select_args);
    if (ex10_result.error)
    {
        return PROTECTED_CREATE_SELECT_ERROR;
    }
    // The non zero select is must be sent to force a protected mode tag to
    // reveal itself.
    struct Gen2CommandSpec non_zero_select = {
        .command = Gen2Select,
        .args    = &non_zero_select_args,
    };

    // Need to find the tag with the non-zero pwd protected select
    // clang-format off
    ex10_ex_printf("Enter with non zero pwd and return to not-protected mode\n");
    if (inventory_and_halt(&non_zero_select, true)) { return PROTECTED_HALT_ERROR; }
    if (enter_access_mode(non_zero_password_in)) { return PROTECTED_ENTER_ACCESS_ERROR; }
    if (set_mode_state(false)) { return PROTECTED_SET_MODE_ERROR; }
    // clang-format on
    get_ex10_reader()->stop_transmitting();
    sleep(2);

    return PROTECTED_NO_ERROR;
}

static uint32_t access_password_recovery(uint32_t non_zero_pass_in)
{
    // Assumes we are not in protected mode
    // clang-format off
    ex10_ex_printf("Enter with non zero pwd and change back\n");
    if (inventory_and_halt(NULL, true)) { return PROTECTED_HALT_ERROR; }
    if (enter_access_mode(non_zero_pass_in)) { return PROTECTED_ENTER_ACCESS_ERROR; }
    if (change_access_pwd(zero_access_pwd)) { return PROTECTED_CHANGE_PWD_ERROR; }
    // clang-format on
    get_ex10_reader()->stop_transmitting();
    sleep(2);

    return PROTECTED_NO_ERROR;
}

int main(int argc, char* argv[])
{
    ex10_ex_printf("Starting protected mode example\n");

    struct Ex10Result const ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

    if (ex10_result.error)
    {
        ex10_ex_eprintf("ex10_typical_board_setup() failed:\n");
        print_ex10_result(ex10_result);
        ex10_typical_board_teardown();
        return -1;
    }

    if (argc <= 1)
    {
        ex10_ex_eprintf("You must pass an appropriate command\n");
        return NO_ARGUMENT_ERROR;
    }

    char     arg_in      = *(argv[1]);
    uint32_t test_return = PROTECTED_NO_ERROR;

    // Create selects to use based on the password of the tag
    // The span is packed in LSB, but the tag interprets it MSB (MSB word is
    // writted to high address) 0 of the span should reflect the MSB of the
    // password. span[] = {1,2,3,4} encoded[] = 1,2,3,4 password at tag =
    // 0x01020304 The correct password to use is password_array_good but the
    // rescovery methods utilize different configurations in case a user is
    // experimenting, the write fails, there are rssi issues, etc.
    uint8_t password_array_good[4] = {
        (uint8_t)(non_zero_access_pwd >> 24),
        (uint8_t)(non_zero_access_pwd >> 16),
        (uint8_t)(non_zero_access_pwd >> 8),
        (uint8_t)(non_zero_access_pwd),
    };
    uint8_t password_array_bad1[4] = {
        (uint8_t)(non_zero_access_pwd >> 8),
        (uint8_t)(non_zero_access_pwd),
        (uint8_t)(non_zero_access_pwd >> 24),
        (uint8_t)(non_zero_access_pwd >> 16),
    };
    uint8_t password_array_bad2[4] = {
        (uint8_t)(non_zero_access_pwd),
        (uint8_t)(non_zero_access_pwd >> 8),
        (uint8_t)(non_zero_access_pwd >> 16),
        (uint8_t)(non_zero_access_pwd >> 24),
    };

    // Used to recover from a bad script exit
    // Ended the script with the protected mode bit set, which must have a
    // non-zero access password
    if (arg_in == 'l')
    {
        ex10_ex_printf("Protected bit recovery\n");
        test_return = protected_bit_recovery(password_array_good);
        ex10_ex_printf("Password: %d, %d, %d, %d -> Recovery code: %d\n",
                       password_array_good[0],
                       password_array_good[1],
                       password_array_good[2],
                       password_array_good[3],
                       test_return);

        test_return = protected_bit_recovery(password_array_bad1);
        ex10_ex_printf("Password: %d, %d, %d, %d -> Recovery code: %d\n",
                       password_array_bad1[0],
                       password_array_bad1[1],
                       password_array_bad1[2],
                       password_array_bad1[3],
                       test_return);

        test_return = protected_bit_recovery(password_array_bad2);
        ex10_ex_printf("Password: %d, %d, %d, %d -> Recovery code: %d\n",
                       password_array_bad2[0],
                       password_array_bad2[1],
                       password_array_bad2[2],
                       password_array_bad2[3],
                       test_return);

        // No actual error from this
        test_return = PROTECTED_NO_ERROR;
    }
    // Used to recover from a bad script exit
    // Ended the script with a non-zero access password
    else if (arg_in == 'a')
    {
        ex10_ex_printf("Password recovery\n");
        uint32_t password = *((uint32_t*)password_array_good);
        test_return       = access_password_recovery(password);
        ex10_ex_printf(
            "Password: 0x%x -> Recovery code: %d\n", password, test_return);

        password    = *((uint32_t*)password_array_bad1);
        test_return = access_password_recovery(password);
        ex10_ex_printf(
            "Password: 0x%x -> Recovery code: %d\n", password, test_return);

        password    = *((uint32_t*)password_array_bad2);
        test_return = access_password_recovery(password);
        ex10_ex_printf(
            "Password: 0x%x -> Recovery code: %d\n", password, test_return);

        // No actual error from this
        test_return = PROTECTED_NO_ERROR;
    }
    else if (arg_in == 't')
    {
        test_return = run_protected_mode_example(password_array_good);
    }
    else
    {
        ex10_ex_eprintf("No valid arg passed in.\n");
    }

    ex10_typical_board_teardown();
    ex10_ex_printf("Ending protected mode example\n");
    return (int)test_return;
}
