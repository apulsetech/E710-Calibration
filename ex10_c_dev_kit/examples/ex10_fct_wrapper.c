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
 * @file ex10_fct_wrapper.c
 * @details The Ex10 FCT wrapper is a program that is used in the FCT process
 *  and brings up the SDK to communicate with the Impinj reader chip, then
 *  opens up a UART on the Raspberry Pi to communicate with a PC that is
 *  running the main FCT scripts.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "board/board_spec.h"
#include "board/e710_ref_design/calibration.h"
#include "board/ex10_gpio.h"
#include "board/ex10_osal.h"
#include "board/ex10_rx_baseband_filter.h"
#include "board/time_helpers.h"
#include "board/uart_helpers.h"

#include "ex10_api/application_registers.h"
#include "ex10_api/board_init.h"
#include "ex10_api/event_fifo_printer.h"
#include "ex10_api/event_packet_parser.h"
#include "ex10_api/ex10_active_region.h"
#include "ex10_api/ex10_helpers.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/ex10_reader.h"
#include "ex10_api/ex10_rf_power.h"
#include "ex10_api/ex10_utils.h"
#include "ex10_api/rf_mode_definitions.h"
#include "ex10_api/version_info.h"
#include "ex10_regulatory/ex10_default_region_names.h"

#include "ex10_modules/ex10_listen_before_talk.h"
#include "ex10_modules/ex10_ramp_module_manager.h"

enum CalibrationCommands
{
    FirmwareUpgrade   = '^',
    VersionNumber     = '#',
    SetAnalogRxConfig = 'a',
    StartPrbs         = 'b',
    SetTxCoarseGain   = 'c',
    RampDown          = 'd',
    EnableRadio       = 'e',
    SetTxFineGain     = 'f',
    SetGpo            = 'g',
    PrintHelp         = 'h',
    PrintHelpAlt      = '?',
    ReadDumpInfoPage  = 'i',
    CompRssi          = 'j',
    GetDeviceSku      = 'k',
    LockSynthesizer   = 'l',
    MeasureAdc        = 'm',
    Reset             = 'n',
    LbtRssi           = 'o',
    RadioPowerControl = 'p',
    QuitWrapper       = 'q',
    QuitWrapperAlt    = 0x03,  /// ^C
    SetRegion         = 'r',
    ReadRssi          = 's',
    CwTest            = 't',
    RampUp            = 'u',
    SetVerbose        = 'v',
    WriteCalInfoPage  = 'w',
    StopTx            = 'x',
    SetAtestMux       = 'y',
    DoShortInventory  = 'z',
};

enum FirmwareUpgradeParam
{
    UpgradeStart    = 's',
    UpgradeContinue = 'c',
    UpgradeComplete = 'e',
};

enum InterfaceMode
{
    ModeNormal = 0,
    ModeRxCal  = 1,
    ModePrbs   = 2,
    // ModeRxFw   = 3,
};

enum ReadInfoPageParam
{
    ReadCalInfoPage            = 'c',
    ReadFeatureInfoPage        = 'f',
    ReadManufacturingInfoPage  = 'm',
    ReadStoredSettingsInfoPage = 's',
};

static const char* const adc_channel_names[] = {
    "PowerLo0",
    "PowerLo1",
    "PowerLo2",
    "PowerLo3",
    "PowerRx0",
    "PowerRx1",
    "PowerRx2",
    "PowerRx3",
    "TestMux0",
    "TestMux1",
    "TestMux2",
    "TestMux3",
    "Temperature",
    "PowerLoSum",
    "PowerRxSum",
};

enum ReturnCodes
{
    ReturnError        = 1,  /** Function failed */
    ReturnSuccess      = 0,  /** Function succeeded */
    ReturnExitError    = -1, /** Error in function, program should exit */
    ReturnGracefulExit = -2, /** No error, but program should exit */
};

void enable_sdd(void);

/* Global state */
#define MAX_REGION_SIZE 11u
static char               region[MAX_REGION_SIZE] = {0u};
static bool               verbose                 = true;
static enum InterfaceMode mode                    = ModeNormal;

static uint8_t info_page[EX10_INFO_PAGE_SIZE];


/**
 * Simple tolower() case implementation to allow human users to type in
 * UPPER/lower/MiXeD cAsE.
 */
static char* lower(char* buffer)
{
    if (!buffer)
    {
        return buffer;
    }

    char* ch = buffer;

    while (*ch != '\0')
    {
        *ch = (char)tolower(*ch);
        ch++;
    }

    return buffer;
}

/**
 * Ignore leading and trailing whitespace, and handle empty buffer case.
 */
static char* trim(char* buffer)
{
    if (!buffer)
    {
        return buffer;
    }

    while (isspace(*buffer))
    {
        buffer++;
    }

    if (!*buffer)
    {
        return buffer;
    }

    while (isspace(*(buffer + strlen(buffer) - 1u)))
    {
        *(buffer + strlen(buffer) - 1u) = 0;
    }

    return buffer;
}

/**
 * Send null terminated message back to user/caller via UART. Suppress sending
 * if user turns off verbose mode.
 *
 * Return int ReturnError if the passed in arguments are null. Otherwise return
 * ReturnSuccess
 */
static int uartsend(const struct Ex10UartHelper* uart, const char* msg)
{
    if (!uart || !msg)
    {
        return ReturnError;
    }

    if (verbose)
    {
        uart->send(msg);
        uart->send("\n");
    }
    else
    {
        // Do nothing
    }

    return ReturnSuccess;
}

/**
 * Send single character back to user/caller via UART. Suppress sending if user
 * turns off verbose mode.
 *
 * Return int ReturnError if the given argumet uart is null. Otherwise, return
 * ReturnSuccess
 */
static int uartecho(const struct Ex10UartHelper* uart, const char ch)
{
    if (!uart)
    {
        return ReturnError;
    }

    if (verbose)
    {
        char msg[2u] = {0};
        msg[0u]      = ch;
        uart->send(msg);
    }
    else
    {
        // Do nothing
    }

    return ReturnSuccess;
}

/**
 * Check op status for error, and parse if found
 *
 * Return true if error occurred. Otherwise, return false.
 */
static bool parse_ex10_result(const struct Ex10Result      ex10_result,
                              const struct Ex10UartHelper* uart)
{
    if (ex10_result.error)
    {
        if (uartsend(uart, "Ex10 result: Error") != ReturnSuccess)
        {
            return true;
        }

        char err[100u] = {0};
        snprintf(err,
                 sizeof(err),
                 "Error in module %s\n ",
                 get_ex10_module_string(ex10_result.module));
        if (uartsend(uart, err) != ReturnSuccess)
        {
            return true;
        }

        if (ex10_result.module == Ex10ModuleDevice)
        {
            switch (ex10_result.result_code.device)
            {
                case Ex10DeviceErrorCommandsNoResponse:
                    snprintf(
                        err,
                        sizeof(err),
                        "Command (resp-less) err: "
                        "result: %d, cmd: %d, num_of_cmds: %d",
                        ex10_result.device_status.cmd_result.failed_result_code,
                        ex10_result.device_status.cmd_result
                            .failed_command_code,
                        ex10_result.device_status.cmd_result
                            .commands_since_first_error);
                    if (uartsend(uart, err) != ReturnSuccess)
                    {
                        return true;
                    }
                    break;

                case Ex10DeviceErrorCommandsWithResponse:
                    snprintf(err,
                             sizeof(err),
                             "Command (with resp) err: "
                             "cmd_result: %d, cmd_id: %d, host_result: %d",
                             ex10_result.device_status.cmd_host_result
                                 .failed_result_code,
                             ex10_result.device_status.cmd_host_result
                                 .failed_command_code,
                             ex10_result.device_status.cmd_host_result
                                 .failed_host_result_code);
                    if (uartsend(uart, err) != ReturnSuccess)
                    {
                        return true;
                    }
                    break;

                case Ex10DeviceErrorOps:
                    snprintf(err,
                             sizeof(err),
                             "Op err - op_id: %d, busy: %d, error %d",
                             ex10_result.device_status.ops_status.op_id,
                             ex10_result.device_status.ops_status.busy,
                             ex10_result.device_status.ops_status.error);
                    if (uartsend(uart, err) != ReturnSuccess)
                    {
                        return true;
                    }
                    break;

                case Ex10DeviceErrorOpsTimeout:
                    snprintf(err,
                             sizeof(err),
                             "Op timeout - op_id: %d, busy: %d, error %d",
                             ex10_result.device_status.ops_status.op_id,
                             ex10_result.device_status.ops_status.busy,
                             ex10_result.device_status.ops_status.error);
                    if (uartsend(uart, err) != ReturnSuccess)
                    {
                        return true;
                    }
                    break;

                default:
                    if (uartsend(uart, "Unknown device result") !=
                        ReturnSuccess)
                    {
                        return true;
                    }
            }
        }
        else
        {
            snprintf(err,
                     sizeof(err),
                     "result code: %d",
                     ex10_result.result_code.sdk);
            if (uartsend(uart, err) != ReturnSuccess)
            {
                return true;
            }
        }
    }
    return false;
}

/**
 * After each op, wait for op completion and check for error.
 */
static bool op_result(const struct Ex10UartHelper* uart)
{
    struct Ex10Result ex10_result = get_ex10_ops()->wait_op_completion();
    return parse_ex10_result(ex10_result, uart);
}

/**
 * Convert hexadecimal parameter string to a uint32 value.
 */
static uint32_t HexStrToUint32(const struct Ex10UartHelper* uart,
                               char*                        param,
                               bool*                        parse_error)
{
    *parse_error = true;

    if (!param)
    {
        uartsend(uart, "Bad command");
        return 0;
    }
    if (strchr(param, '.') || strlen(param) > 8)
    {
        uartsend(uart, "Enter as 4-byte hex value");
        return 0;
    }

    for (size_t idx = 0; idx < strlen(param); ++idx)
    {
        if (isxdigit(param[idx]) == false)
        {
            uartsend(uart, "Invalid digit - must be hex with no leading '0x'");
            return 0;
        }
    }

    errno        = 0;
    uint32_t val = strtoul(param, NULL, 16);
    if (errno != 0)
    {
        uartsend(uart, "Error parsing hex value");
        return 0;
    }
    *parse_error = false;
    return val;
}

/**
 * User entered 'h' or '?':
 * Print a list of UART commands regardless of verbose mode setting.
 */
static int send_help(const struct Ex10UartHelper* uart)
{
    if (!uart)
    {
        return ReturnError;
    }

    uart->send("^ s <image_size>                  Upload firmware: start\n");
    uart->send("^ c <ascii_hex_chunk>             Upload firmware: continue\n");
    uart->send("^ e <checksum>                    Upload firmware: end\n");
    uart->send("#                                 Get firmware version\n");
    uart->send("a <RxGainControl>                 Op: SetAnalogRxConfig\n");
    uart->send("b                                 Op: StartPrbs\n");
    uart->send("c <coarse atten [0..30]>          Op: SetCoarseGain\n");
    uart->send("d                                 Op: TxRampDown\n");
    uart->send(
        "e <antenna [1|2]> <rf_mode [#]>   Ops: Get/set GPO, SetRfMode\n");
    uart->send("f <tx scalar [-2048..2047]>       Op: SetFineGain\n");
    uart->send("g <GPO# [0..31]> <state [0|1]>    Op: SetGPO\n");
    uart->send(
        "h, ?                              Print this help information\n");
    uart->send(
        "i <info page ID [c|f|m|s]>        "
        "Dump cal, feature, manufacturing, or settings info page\n");
    uart->send("k                                 Get device SKU\n");
    uart->send("l <freq_khz>                      Op: LockSynthesizer\n");
    uart->send("m <channel [0..14]>               Op: MeasureAdc\n");
    uart->send(
        "n <context [b|a]>                 "
        "Reset into bootloader or application\n");
    uart->send("p <state [0|1]>                   Op: RadioPowerControl\n");
    uart->send("r <region [FCC|EU1|ETSI LOWER]>   Reinit region\n");
    uart->send("s                                 Op: MeasureRssi\n");
    uart->send(
        "t <antenna [1|2]> <rf_mode [#]> <pwr_cdbm> <freq_khz> <remain on "
        "[1,0]>\n");
    uart->send(
        "                                  Builds CW configs and calls "
        "cw_on aggregate op\n");
    uart->send(
        "u <dc_offset>                     Tx ramp with no regulatory timers "
        "Op:TxRampUp\n");
    uart->send(
        "v <verbosity [0|1]>               Set/toggle verbose output "
        "(optional)\n");
    uart->send(
        "w                                 (1) Begin sending cal data, "
        "(2) Write received cal data\n");
    uart->send("q, ^C                             Quit ex10_wrapper\n");
    uart->send(
        "x                                 Stop transmitting"
        "Ops:StopOp, TxRampDown\n");
    uart->send("y <mux0> <mux1> <mux2> <mux3>     Op: SetAtestMux\n");
    uart->send(
        "z <antenna [1|2]> <rf_mode [#]> <pwr_cdbm> <freq_khz> <duration "
        "ms [50,60000]>\n"
        "                                  Inventory\n");
    uart->send(
        "z                                 Default 50ms inventory on antenna "
        "1\n");

    return ReturnSuccess;
}

static uint8_t image_chunk[EX10_MAX_IMAGE_CHUNK_SIZE + 1];

static struct ConstByteSpan read_image_chunk(char* param)
{
    struct ConstByteSpan image = {
        .data   = 0u,
        .length = 0u,
    };

    if (!param)
    {
        return image;
    }

    size_t   chunk_len    = 0;
    char*    hex_byte_str = strtok(param, " \r\n");
    uint16_t byte_idx     = 0;

    while (hex_byte_str)
    {
        chunk_len++;
        // Convert hex string to a byte and put into buffer
        // Expected command format: "^ c 00 00 00 00 .....\n"
        long hex_byte         = strtol(hex_byte_str, NULL, 16);
        image_chunk[byte_idx] = 0xFF & hex_byte;
        byte_idx++;

        hex_byte_str = strtok(NULL, " \r\n");
    }

    if (chunk_len > EX10_MAX_IMAGE_CHUNK_SIZE)
    {
        ex10_ex_eprintf(
            "Given buffer size exceeds the maximum image chunk size. Max size: "
            "%u, received: %u",
            EX10_MAX_IMAGE_CHUNK_SIZE,
            chunk_len);
        return image;
    }

    image.data   = image_chunk;
    image.length = chunk_len;

    return image;
}

/**
 * Call after protocol->reset(Application) to reestablish IRQ mask, etc.
 * Return int ReturnError if deinit/init failed. Otherwise, return
 * ReturnSuccess
 */
static int reinit(const struct Ex10UartHelper* uart)
{
    struct Ex10Reader const* reader      = get_ex10_reader();
    struct Ex10Result        ex10_result = reader->deinit();
    if (ex10_result.error)
    {
        parse_ex10_result(ex10_result, uart);
        return ReturnError;
    }

    // Set interrupt mask, GPIO levels, etc.
    reader->init_ex10();

    // Configure EventFifo interrupt threshold
    struct EventFifoIntLevelFields const level_data = {
        .threshold = DEFAULT_EVENT_FIFO_THRESHOLD};
    get_ex10_protocol()->write(&event_fifo_int_level_reg, &level_data);

    return ReturnSuccess;
}

/**
 * User entered '^':
 * Update firmware using image data uploaded over serial.
 */
static int firmware_upgrade(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    const char usage[] = "Upload requires param s,c,e";

    char*         param        = strtok(command, " ");
    static size_t image_length = 0;

    if (param)
    {
        switch (tolower(param[0]))
        {
            case UpgradeStart:
            {
                param        = strtok(NULL, " ");
                image_length = (size_t)atoi(param);
                if (image_length > EX10_MAX_IMAGE_BYTES)
                {
                    uart->send("Image size exceeds maximum");
                    return ReturnError;
                }

                // Crc calculated by the sender
                param                         = strtok(NULL, " ");
                uint16_t expected_chunk_crc16 = (uint16_t)atoi(param);

                // Read data, forward to protocol
                param = strtok(NULL, " ");
                // strtok has modified the string with NULL, undo the NULL so
                // that the full line is passed along.
                param[strlen(param)]       = ' ';
                struct ConstByteSpan chunk = read_image_chunk(param);

                // Check chunk crc
                uint16_t chunk_crc16 =
                    ex10_compute_crc16(chunk.data, chunk.length);
                if (chunk_crc16 != expected_chunk_crc16)
                {
                    return ReturnError;
                }

                get_ex10_protocol()->upload_start(
                    UploadFlash, image_length, chunk);
                break;
            }
            case UpgradeContinue:
            {
                // Crc calculated by the sender
                param                         = strtok(NULL, " ");
                uint16_t expected_chunk_crc16 = (uint16_t)atoi(param);

                // Read data, forward to protocol
                param = strtok(NULL, " ");
                // strtok has modified the string with NULL, undo the NULL so
                // that the full line is passed along.
                param[strlen(param)]       = ' ';
                struct ConstByteSpan chunk = read_image_chunk(param);

                // Check chunk crc
                uint16_t chunk_crc16 =
                    ex10_compute_crc16(chunk.data, chunk.length);
                if (chunk_crc16 != expected_chunk_crc16)
                {
                    return ReturnError;
                }

                get_ex10_protocol()->upload_continue(chunk);
                break;
            }
            case UpgradeComplete:
            {
                get_ex10_protocol()->upload_complete();
                break;
            }
            default:
                return ReturnError;
                break;
        }
    }
    else
    {
        uartsend(uart, usage);
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered '#':
 * Get the firmware version
 */
static int get_firmware_version(const struct Ex10UartHelper* uart,
                                char*                        command)
{
    if (!uart)
    {
        return ReturnError;
    }
    (void)command;

    char                       ver_info[VERSION_STRING_SIZE];
    struct ImageValidityFields image_validity;
    struct RemainReasonFields  remain_reason;

    // Reset into the Application
    get_ex10_protocol()->reset(Application);
    if (reinit(uart) != ReturnSuccess)
    {
        return ReturnError;
    }

    get_ex10_version()->get_application_info(
        ver_info, sizeof(ver_info), &image_validity, &remain_reason);
    if (uartsend(uart, ver_info) != ReturnSuccess)
    {
        return ReturnError;
    }

    if ((image_validity.image_valid_marker) &&
        !(image_validity.image_non_valid_marker))
    {
        uartsend(uart, "Application image VALID");
    }
    else
    {
        uartsend(uart, "Application image INVALID");
    }

    const char* remain_reason_str =
        get_ex10_helpers()->get_remain_reason_string(
            remain_reason.remain_reason);
    snprintf(ver_info,
             sizeof(ver_info),
             "Remain in bootloader reason: %s",
             remain_reason_str);
    if (uartsend(uart, ver_info) != ReturnSuccess)
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'a':
 * Parse Analog RX config parameters and call SetRxGainOp
 */
static int set_analog_rx_config(const struct Ex10UartHelper* uart,
                                char*                        command)
{
    if (!uart)
    {
        return ReturnError;
    }

    char* param = strtok(command, " ");

    bool           parse_error;
    uint16_t const val = (uint16_t)HexStrToUint32(uart, param, &parse_error);
    if (parse_error)
    {
        return ReturnError;
    }

    struct RxGainControlFields rx_config;
    ex10_memcpy(&rx_config, sizeof(rx_config), &val, sizeof(val));
    static_assert(sizeof(rx_config) == sizeof(val), "");

    char parsed_rx_config[80u] = {0};
    sprintf(parsed_rx_config,
            "RxAtten:%d, PGA1:%d, PGA2:%d, PGA3:%d, mixer:%d, PGA1_select: %d, "
            "mixer_bw: %d",
            rx_config.rx_atten,
            rx_config.pga1_gain,
            rx_config.pga2_gain,
            rx_config.pga3_gain,
            rx_config.mixer_gain,
            rx_config.pga1_rin_select,
            rx_config.mixer_bandwidth);
    if (uartsend(uart, parsed_rx_config) != ReturnSuccess)
    {
        return ReturnError;
    }

    struct Ex10Result ex10_result =
        get_ex10_rf_power()->set_analog_rx_config(&rx_config);
    if (ex10_result.error)
    {
        parse_ex10_result(ex10_result, uart);
        return ReturnError;
    }
    if (op_result(uart))
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'c':
 * Parse TX atten and call set_tx_coarse_gain.
 */
static int set_tx_coarse_gain(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    int32_t req_tx_atten = 0;
    char*   param        = strtok(command, " ");

    if (param)
    {
        if (strchr(param, '.'))
        {
            uartsend(uart, "Enter TX coarse atten as a whole number [0,30]");
            return ReturnError;
        }

        req_tx_atten = atoi(param);
        if (req_tx_atten < 0 || req_tx_atten > 30)
        {
            uartsend(uart, "TX coarse atten out of range [0,30]");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    struct Ex10Result ex10_result =
        get_ex10_ops()->set_tx_coarse_gain((uint8_t)req_tx_atten);
    if (ex10_result.error)
    {
        parse_ex10_result(ex10_result, uart);
        return ReturnError;
    }
    if (op_result(uart))
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'e':
 * Parse antenna and RF mode, and call get/set GPIO levels, and set RF mode.
 */
static int enable_radio(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    uint8_t req_antenna = 0u;
    char*   param       = strtok(command, " ");

    if (param)
    {
        req_antenna = (uint8_t)atoi(param);
        if ((req_antenna != 1u) && (req_antenna != 2u))
        {
            uartsend(uart, "Antenna must be 1 or 2");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    enum RfModes req_rf_mode = (enum RfModes)0u;
    param                    = strtok(NULL, " ");

    if (param)
    {
        int rf_mode_i = atoi(param);
        req_rf_mode   = (enum RfModes)rf_mode_i;
        if (rf_mode_i == 0u)
        {
            uartsend(uart, "Bad RF Mode. Suggestion: use RF mode 5");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    // Set GPIO config
    struct Ex10GpioHelpers const* const gpio_helpers = get_ex10_gpio_helpers();
    struct Ex10RxBasebandFilter const*  ex10_rx_baseband_filter =
        get_ex10_rx_baseband_filter();

    struct GpioPinsSetClear gpio_pins_set_clear = {0u, 0u, 0u, 0u};
    gpio_helpers->set_antenna_port(&gpio_pins_set_clear, req_antenna);
    const enum BasebandFilterType rx_baseband_filter =
        ex10_rx_baseband_filter->choose_rx_baseband_filter(req_rf_mode);
    gpio_helpers->set_rx_baseband_filter(&gpio_pins_set_clear,
                                         rx_baseband_filter);
    gpio_helpers->set_pa_bias_enable(&gpio_pins_set_clear, true);
    gpio_helpers->set_pa_power_range(&gpio_pins_set_clear, PowerRangeHigh);
    gpio_helpers->set_rf_power_supply_enable(&gpio_pins_set_clear, true);
    gpio_helpers->set_tx_rf_filter(&gpio_pins_set_clear,
                                   get_ex10_active_region()->get_rf_filter());

    struct Ex10Ops const* ops = get_ex10_ops();
    struct Ex10Result     ex10_result =
        ops->set_clear_gpio_pins(&gpio_pins_set_clear);
    if (ex10_result.error)
    {
        parse_ex10_result(ex10_result, uart);
        return ReturnError;
    }

    if (op_result(uart))
    {
        return ReturnError;
    }

    // Set RF mode
    ex10_result = ops->set_rf_mode(req_rf_mode);
    if (ex10_result.error)
    {
        parse_ex10_result(ex10_result, uart);
        return ReturnError;
    }

    if (op_result(uart))
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'f':
 * Parse TX scalar and call set_tx_fine_gain.
 */
static int set_tx_fine_gain(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    int32_t req_tx_scalar = 0;
    char*   param         = strtok(command, " ");

    if (param)
    {
        if (strchr(param, '.'))
        {
            uartsend(uart, "Enter TX scalar as a whole number [-2048,2047]");
            return ReturnError;
        }

        req_tx_scalar = atoi(param);

        if (req_tx_scalar < -2048 || req_tx_scalar > 2047)
        {
            uartsend(uart, "TX fine scalar out of range [-2048,2047]");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    struct Ex10Result ex10_result =
        get_ex10_ops()->set_tx_fine_gain((int16_t)req_tx_scalar);
    if (ex10_result.error)
    {
        parse_ex10_result(ex10_result, uart);
        return ReturnError;
    }
    if (op_result(uart))
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'g':
 * Set general purpose output.
 */
static int set_gpo(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    char* param = strtok(command, " ");

    int pin_num = -1;
    if (param)
    {
        pin_num = atoi(param);
        if ((pin_num < 0) || (pin_num > 31))
        {
            uartsend(uart, "Pin number must be from 0 to 31");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad pin number");
        return ReturnError;
    }

    param       = strtok(NULL, " ");
    int pin_val = -1;
    if (param)
    {
        pin_val = atoi(param);
        if ((pin_val != 0u) && (pin_val != 1u))
        {
            uartsend(uart, "Pin value must be 0 or 1");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Pin value must be 0 or 1");
        return ReturnError;
    }

    struct GpioPinsSetClear const gpio_pins_set_clear = {
        .output_level_set    = pin_val ? (1u << pin_num) : 0u,
        .output_level_clear  = pin_val ? 0u : (1u << pin_num),
        .output_enable_set   = (1u << pin_num),
        .output_enable_clear = 0u,
    };

    struct Ex10Result ex10_result =
        get_ex10_ops()->set_clear_gpio_pins(&gpio_pins_set_clear);
    if (ex10_result.error)
    {
        parse_ex10_result(ex10_result, uart);
        return ReturnError;
    }

    if (op_result(uart))
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'k'
 * Read using SKU , parse packets and report RSSI.
 */
static int read_device_sku(const struct Ex10UartHelper* uart)
{
    if (!uart)
    {
        return ReturnError;
    }

    enum ProductSku sku = get_ex10_protocol()->get_sku();

    char result_str[20u] = {0};
    sprintf(result_str, "Result: %04X\n", sku);
    uart->send(result_str);

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'l':
 * Parse frequency (in kHz) and lock synthesizer
 */
static int lock_synthesizer(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    uint32_t req_frequency_khz = 0u;
    char*    param             = strtok(command, " ");

    if (param)
    {
        if (strchr(param, '.'))
        {
            uartsend(uart, "Enter frequency in kHz");
            return ReturnError;
        }

        req_frequency_khz = (uint32_t)atoi(param);
        if ((strcmp(region, "FCC") == 0 &&
             (req_frequency_khz < 902000 || req_frequency_khz > 928000)) ||
            (strcmp(region, "ETSI_LOWER") == 0 &&
             (req_frequency_khz < 865000 || req_frequency_khz > 868000)))
        {
            uartsend(uart, "Frequency out of band");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    // Get synth params
    struct SynthesizerParams synth_params = {0};
    get_ex10_active_region()->get_synthesizer_params(req_frequency_khz,
                                                     &synth_params);

    struct Ex10Result ex10_result = get_ex10_ops()->lock_synthesizer(
        synth_params.r_divider_index, synth_params.n_divider);
    if (ex10_result.error)
    {
        parse_ex10_result(ex10_result, uart);
        return ReturnError;
    }
    if (op_result(uart))
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'm' followed by an ADC request number
 * Get ADC can read either power or temperature during calibration.
 * The other ADC channels are not supported, as they are not used in the
 * calibration process.
 */
static int measure_adc(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    int32_t request = 0;
    char*   param   = strtok(command, " ");

    if (param)
    {
        if (strchr(param, '.'))
        {
            uartsend(uart,
                     "Enter requested ADC channel as whole number [0,14]");
            return ReturnError;
        }

        request = atoi(param);

        if (request < 0 || request > 14)
        {
            uartsend(uart, "Requested ADC channel out of range [0,14]");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    char msg[30u] = {0};
    sprintf(msg, "Measure ADC (%s)", adc_channel_names[request]);
    if (uartsend(uart, msg) != ReturnSuccess)
    {
        return ReturnError;
    }

    uint16_t adc_result = 0u;
    get_ex10_rf_power()->measure_and_read_aux_adc(
        (enum AuxAdcResultsAdcResult)request, 1u, &adc_result);

    char result_str[20u] = {0};
    sprintf(result_str, "Result: %d\n", adc_result);
    uart->send(result_str);

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'n' followed by 'b' or 'a':
 * Reset into bootloader or application.  This uses the reset command and can
 * be used to watch the initialization process.
 */
static int reset(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart)
    {
        return ReturnError;
    }

    struct Ex10Protocol const* protocol = get_ex10_protocol();
    const char                 usage[] =
        "Reset type must be 'b' for bootloader "
        "or 'a' for application";

    char* param = strtok(command, " ");
    if (param)
    {
        if (tolower(param[0] == 'b'))
        {

            protocol->reset(Bootloader);
            if (protocol->get_running_location() != Bootloader)
            {
                return ReturnError;
            }
        }
        else if (tolower(param[0] == 'a'))
        {
            protocol->reset(Application);
            if (reinit(uart) != ReturnSuccess)
            {
                return ReturnError;
            }
            if (protocol->get_running_location() != Application)
            {
                return ReturnError;
            }
        }
        else
        {
            uartsend(uart, usage);
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, usage);
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'p':
 * Parse radio power control parameter as boolean, call radio power control.
 */
static int radio_power_control(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    uint8_t enable = 0u;
    char*   param  = strtok(command, " ");

    if (param)
    {
        enable = (uint8_t)atoi(param);
        if ((enable != 0u) && (enable != 1u))
        {
            uartsend(uart, "Value must be 0 or 1");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    char msg[30u] = {0};
    sprintf(
        msg, "Radio Power Control: %s", (enable == 1) ? "Enable" : "Disable");
    if (uartsend(uart, msg) != ReturnSuccess)
    {
        return ReturnError;
    }

    struct Ex10Result ex10_result =
        get_ex10_ops()->radio_power_control((enable == 1) ? true : false);
    if (ex10_result.error)
    {
        parse_ex10_result(ex10_result, uart);
        return ReturnError;
    }
    if (op_result(uart))
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'r':
 * Parse region string, and reset ops and reader SDK layers with new region
 */
static int set_region(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    get_ex10_helpers()->discard_packets(false, true, false);
    if (strcmp(lower(trim(command)), "fcc") == 0)
    {
        uartsend(uart, "Set Region to FCC");

        ex10_typical_board_teardown();
        get_ex10_regulatory()->set_region(REGION_FCC, NULL);
        const struct Ex10Result ex10_result =
            ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

        if (ex10_result.error)
        {
            return ReturnError;
        }
        strncpy(region, "FCC", MAX_REGION_SIZE);
    }
    else if ((strcmp(lower(trim(command)), "eu1") == 0) ||
             strcmp(lower(trim(command)), "etsi lower") == 0)
    {
        uartsend(uart, "Set Region to ETSI Lower");

        ex10_typical_board_teardown();
        get_ex10_regulatory()->set_region(REGION_ETSI_LOWER, NULL);
        const struct Ex10Result ex10_result =
            ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_ETSI_LOWER);

        if (ex10_result.error)
        {
            return ReturnError;
        }
        strncpy(region, "ETSI_LOWER", MAX_REGION_SIZE);
    }
    else if (strcmp(lower(trim(command)), "japan") == 0)
    {
        uartsend(uart, "Set Region to Japan");

        ex10_typical_board_teardown();
        get_ex10_regulatory()->set_region(REGION_JAPAN2, NULL);
        const struct Ex10Result ex10_result =
            ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_JAPAN2);

        if (ex10_result.error)
        {
            return ReturnError;
        }
        strncpy(region, "JAPAN2", MAX_REGION_SIZE);
    }
    else
    {
        uartsend(uart, "Unknown region");
        return ReturnError;
    }

    // Unregister rampup callbacks since we don't care about reverse power,
    // antenna disconnect detection.
    get_ex10_ramp_module_manager()->unregister_ramp_callbacks();

    enable_sdd();

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 's'
 * Read RSSI with MeasureRssiOp, parse packets and report.
 */
static int read_rssi(const struct Ex10UartHelper* uart)
{
    if (!uart)
    {
        return ReturnError;
    }

    struct Ex10Helpers const* helpers = get_ex10_helpers();
    helpers->discard_packets(false, true, false);
    uint16_t rssi_result = helpers->read_rssi_value_from_op(0x0Fu);

    if (rssi_result == 0)
    {  /// Measure RSSI Op returned error
        uartsend(uart, "MeasureRssiOp error");
        return ReturnError;
    }

    char result_str[20u] = {0};
    sprintf(result_str, "Result: %d\n", rssi_result);
    uart->send(result_str);

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'j'
 * Read RSSI with MeasureRSSIOp, convert to cdBm with compensated RSSI.
 */
static int read_compensated_rssi(const struct Ex10UartHelper* uart,
                                 char*                        command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    uint8_t req_antenna = 0u;
    char*   param       = strtok(command, " ");

    if (param)
    {
        req_antenna = (uint8_t)atoi(param);
        if ((req_antenna != 1u) && (req_antenna != 2u))
        {
            uartsend(uart, "Antenna must be 1 or 2");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    enum RfModes req_rf_mode = (enum RfModes)0u;
    param                    = strtok(NULL, " ");

    if (param)
    {
        int rf_mode_i = atoi(param);
        req_rf_mode   = (enum RfModes)rf_mode_i;
        if (rf_mode_i == 0u)
        {
            uartsend(uart, "Bad RF Mode. Suggestion: use RF mode 5");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    struct Ex10Helpers const* helpers = get_ex10_helpers();
    helpers->discard_packets(false, true, false);
    uint16_t rssi_raw = helpers->read_rssi_value_from_op(0x0Fu);

    if (rssi_raw == 0)
    {  /// Measure RSSI Op returned error
        uartsend(uart, "MeasureRssiOp error");
        return ReturnError;
    }

    struct Ex10Calibration const*     calibration = get_ex10_calibration();
    const struct RxGainControlFields* rx_config =
        get_ex10_reader()->get_current_analog_rx_fields();

    uint16_t curr_temp_adc = 0;
    get_ex10_rf_power()->measure_and_read_aux_adc(
        AdcResultTemperature, 1, &curr_temp_adc);

    int16_t compensated_rssi = calibration->get_compensated_rssi(
        rssi_raw,
        req_rf_mode,
        rx_config,
        req_antenna,
        get_ex10_active_region()->get_rf_filter(),
        curr_temp_adc);

    char result_str[20u] = {0};
    sprintf(result_str, "Result: %d\n", compensated_rssi);

    uart->send(result_str);

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'o'
 * Read RSSI with reader->get_listen_before_talk_rssi.
 */
static int read_lbt_rssi(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    uint8_t req_antenna = 0u;
    char*   param       = strtok(command, " ");

    // antenna, frequency_khz, lbt_offset, rssi_count, override_used
    if (param)
    {
        req_antenna = (uint8_t)atoi(param);
        if ((req_antenna != 1u) && (req_antenna != 2u))
        {
            uartsend(uart, "Antenna must be 1 or 2");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    uint32_t req_frequency_khz = 0u;
    param                      = strtok(NULL, " ");

    if (param)
    {
        req_frequency_khz = (uint32_t)atoi(param);
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    int32_t req_lbt_offset = 0u;
    param                  = strtok(NULL, " ");

    if (param)
    {
        req_lbt_offset = (int32_t)atoi(param);
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    uint8_t req_rssi_count = 0u;
    param                  = strtok(NULL, " ");

    if (param)
    {
        req_rssi_count = (uint8_t)atoi(param);
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    uint8_t req_override_used = 0u;
    param                     = strtok(NULL, " ");

    if (param)
    {
        req_override_used = (uint8_t)atoi(param);
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    struct LbtControlFields lbt_settings = {
        .override              = req_override_used,
        .narrow_bandwidth_mode = false,
        .num_rssi_measurements = 1,
        .measurement_delay_us  = 0,
    };

    int16_t rssi_result = 0;
    get_ex10_reader()->listen_before_talk_multi(req_antenna,
                                                req_rssi_count,
                                                lbt_settings,
                                                &req_frequency_khz,
                                                &req_lbt_offset,
                                                &rssi_result);

    if (rssi_result == 0)
    {  /// Measure RSSI Op returned error
        uartsend(uart, "MeasureRssiOp error");
        return ReturnError;
    }

    char result_str[20u] = {0};
    sprintf(result_str, "Result: %d\n", rssi_result);
    uart->send(result_str);

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 't':
 * Parse cw_test parameters and call reader api
 */
static int cw_test(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    uint8_t req_antenna = 0u;
    char*   param       = strtok(command, " ");

    if (param)
    {
        req_antenna = (uint8_t)atoi(param);
        if ((req_antenna != 1u) && (req_antenna != 2u))
        {
            uartsend(uart, "Antenna must be 1 or 2");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    enum RfModes req_rf_mode = (enum RfModes)0u;
    param                    = strtok(NULL, " ");

    if (param)
    {
        int rf_mode_i = atoi(param);
        req_rf_mode   = (enum RfModes)rf_mode_i;
        if (rf_mode_i == 0u)
        {
            uartsend(uart, "Bad RF Mode. Suggestion: use RF mode 5");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    int16_t req_power_cdbm = 0;
    param                  = strtok(NULL, " ");

    if (param)
    {
        req_power_cdbm = (int16_t)atoi(param);
        if (req_power_cdbm < 0 || req_power_cdbm > 3200)
        {
            uartsend(uart, "Power_cdbm must be within [0,3200]");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    uint32_t req_frequency_khz = 0u;
    param                      = strtok(NULL, " ");

    if (param)
    {
        if (strchr(param, '.'))
        {
            uartsend(uart, "Enter frequency in kHz");
            return ReturnError;
        }
        req_frequency_khz = (uint32_t)atoi(param);
        if ((strcmp(region, "FCC") == 0 &&
             (req_frequency_khz < 902000 || req_frequency_khz > 928000)) ||
            (strcmp(region, "ETSI_LOWER") == 0 &&
             (req_frequency_khz < 865000 || req_frequency_khz > 868000)))
        {
            uartsend(uart, "Frequency out of band");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    uint8_t req_remain_on = 0u;
    param                 = strtok(NULL, " ");

    if (param)
    {
        req_remain_on = (uint8_t)atoi(param);
        if ((req_remain_on != 0u) && (req_remain_on != 1u))
        {
            uartsend(uart, "Remain on value must be 0 or 1");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    get_ex10_ops()->wait_op_completion();

    enum Ex10RegionId region_id =
        get_ex10_default_region_names()->get_region_id(region);
    // Set to Null to clear overrides and custom settings
    get_ex10_regulatory()->set_region(region_id, NULL);
    // Now set the active region again to the base region
    get_ex10_active_region()->set_region(region_id, TCXO_FREQ_KHZ);

    struct Ex10Result ex10_result =
        get_ex10_reader()->cw_test(req_antenna,
                                   req_rf_mode,
                                   req_power_cdbm,
                                   req_frequency_khz,
                                   req_remain_on);
    if (parse_ex10_result(ex10_result, uart))
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'u':
 * Parse DC offset and call tx ramp up.
 */
static int cal_tx_ramp_up(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    int32_t req_dc_offset = 0;
    char*   param         = strtok(command, " ");

    if (param)
    {
        if (strchr(param, '.'))
        {
            uartsend(uart,
                     "Enter DC offset as a whole number [-524288,524287]");
            return ReturnError;
        }

        req_dc_offset = atoi(param);
        if (req_dc_offset < -524288 || req_dc_offset > 524287)
        {
            uartsend(uart, "DC offset out of range [-524288,524287]");
            return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    if (op_result(uart))
    {
        return ReturnError;
    }

    struct Ex10Result ex10_result = get_ex10_ops()->tx_ramp_down();
    (void)ex10_result;  /// @todo unhandled ex10_result
    if (op_result(uart))
    {
        return ReturnError;
    }

    struct Ex10RegulatoryTimers const timer_config = {0u};
    get_ex10_rf_power()->set_regulatory_timers(&timer_config);
    ex10_result = get_ex10_ops()->tx_ramp_up(req_dc_offset);
    if (ex10_result.error)
    {
        parse_ex10_result(ex10_result, uart);
        return ReturnError;
    }

    if (op_result(uart))
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'v':
 * Parse verbose parameter as boolean, or toggle if no parameter
 */
static int set_verbose_mode(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    uint8_t enable = 0u;
    char*   param  = strtok(command, " ");

    if (param)
    {
        enable = (uint8_t)atoi(param);
        if ((enable != 0u) && (enable != 1u))
        {
            uartsend(uart, "Value must be 0 or 1");
            return ReturnError;
        }
    }
    else
    {
        uart->send("Toggle verbose output\n");
        verbose = !verbose;
        if (verbose)
        {
            uart->send("Enabled\n");
        }
        else
        {
            uart->send("Disabled\n");
        }
        return ReturnSuccess;
    }

    verbose       = (enable == 1) ? true : false;
    char msg[30u] = {0};
    sprintf(
        msg, "Set verbose mode: %s\n", (enable == 1) ? "Enabled" : "Disabled");
    uart->send(msg);

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * Hex dump of info page
 */
static void hex_dump_info_page(uint32_t                     base_address,
                               const struct Ex10UartHelper* uart)
{
    for (size_t offset = 0; offset < EX10_INFO_PAGE_SIZE; offset += 16)
    {
        char line[180] = {0};

        for (size_t col = 0; col < 16; col++)
        {
            char item[4] = {0};
            if (!col)
            {
                sprintf(line, "%08zX:", base_address + offset);
            }
            sprintf(item, " %02X", *(info_page + offset + col));
            strcat(line, item);
            if (!((col + 1) % 8))
            {
                strcat(line, "  ");
            }
        }
        strcat(line, "\n");
        uart->send(line);
    }
}

/**
 * Info page address offsets, indexed by page Id.
 */
static const uint32_t info_page_offsets[] = {
    0x10010000,  // MainBlock
    0x1ffd0000,  // features
    0x1ffd4000,  // mfg
    0x1ffd8000,  // cal
    0x1ffdc000,  // stored settings
};

/**
 * Get feature info page and send to UART
 */
static int read_feature_info_page(const struct Ex10UartHelper* uart)
{
    ex10_memzero(info_page, EX10_INFO_PAGE_SIZE);
    const uint32_t base_addr = info_page_offsets[FeatureControlsId];

    struct Ex10Result ex10_result =
        get_ex10_protocol()->read_info_page_buffer(base_addr, info_page);
    if (parse_ex10_result(ex10_result, uart))
    {
        return ReturnError;
    }

    hex_dump_info_page(base_addr, uart);

    if (uartsend(uart, "Done") != ReturnSuccess)
    {
        return ReturnError;
    }
    return ReturnSuccess;
}

/**
 * Get manufacturing info page and send to UART
 */
static int read_manufacturing_info_page(const struct Ex10UartHelper* uart)
{
    ex10_memzero(info_page, EX10_INFO_PAGE_SIZE);
    const uint32_t base_addr = info_page_offsets[ManufacturingId];

    struct Ex10Result ex10_result =
        get_ex10_protocol()->read_info_page_buffer(base_addr, info_page);
    if (parse_ex10_result(ex10_result, uart))
    {
        return ReturnError;
    }

    hex_dump_info_page(base_addr, uart);

    if (uartsend(uart, "Done") != ReturnSuccess)
    {
        return ReturnError;
    }
    return ReturnSuccess;
}

/**
 * Get calibration info page content and send to UART
 */
static int read_cal_info_page(const struct Ex10UartHelper* uart)
{
    ex10_memzero(info_page, EX10_INFO_PAGE_SIZE);
    const uint32_t base_addr = info_page_offsets[CalPageId];

    struct Ex10Result ex10_result =
        get_ex10_protocol()->read_info_page_buffer(base_addr, info_page);
    if (parse_ex10_result(ex10_result, uart))
    {
        return ReturnError;
    }

    hex_dump_info_page(base_addr, uart);

    if (uartsend(uart, "Done") != ReturnSuccess)
    {
        return ReturnError;
    }
    return ReturnSuccess;
}

/**
 * Get stored settings info page and send to UART
 */
static int read_stored_settings_info_page(const struct Ex10UartHelper* uart)
{
    ex10_memzero(info_page, EX10_INFO_PAGE_SIZE);
    const uint32_t base_addr = info_page_offsets[StoredSettingsId];

    struct Ex10Result ex10_result =
        get_ex10_protocol()->read_info_page_buffer(base_addr, info_page);
    if (parse_ex10_result(ex10_result, uart))
    {
        return ReturnError;
    }

    hex_dump_info_page(base_addr, uart);

    if (uartsend(uart, "Done") != ReturnSuccess)
    {
        return ReturnError;
    }
    return ReturnSuccess;
}

/**
 * User entered 'i':
 * Get info page content and send to UART
 */
static int read_info_page(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    int   result = ReturnSuccess;
    char* param  = strtok(command, " ");

    if (param)
    {
        switch (tolower(param[0]))
        {
            case ReadCalInfoPage:
                uartsend(uart, "Read cal info page");
                result = read_cal_info_page(uart);
                break;
            case ReadFeatureInfoPage:
                uartsend(uart, "Read feature info page");
                result = read_feature_info_page(uart);
                break;
            case ReadManufacturingInfoPage:
                uartsend(uart, "Read manufacturing info page");
                result = read_manufacturing_info_page(uart);
                break;
            case ReadStoredSettingsInfoPage:
                uartsend(uart, "Read stored settings info page");
                result = read_stored_settings_info_page(uart);
                break;
            default:
                uartsend(uart, "Bad command");
                return ReturnError;
        }
    }
    else
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    return result;
}

/**
 * User entered 'w':
 * Receive info page bytes as ASCII hex over UART, then write info page
 */
static int write_info_page(const struct Ex10UartHelper* uart)
{
    if (!uart)
    {
        return ReturnError;
    }

    if (mode == ModeNormal)
    {  // Note: The following is only printed in verbose mode
        uartsend(uart, "Entering data receive mode");
        uartsend(uart, "Send data format:");
        uartsend(uart, "oooo: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff");
        uartsend(uart, "followed by CR, where:");
        uartsend(uart, "    oooo = page offset in hex [0,7FF]");
        uartsend(uart, "    ff   = hex ASCII byte");
        mode = ModeRxCal;

        ex10_memset(info_page,
                    calibration_info_reg.length,
                    0xFF,
                    calibration_info_reg.length);

        uartsend(uart, "Done");
        return ReturnSuccess;
    }
    else
    {
        if (mode == ModeRxCal)
        {
            uartsend(uart, "Exiting data receive mode");
            uartsend(uart, "Writing received data to calibration page");
            mode = ModeNormal;

            // Calibration data now in info_page, padded to .length with
            // 0xff Find the end of the actual data
            size_t length = sizeof(info_page);
            while (info_page[length - 1u] == 0xFF)
            {
                length--;
                if (!length)
                {
                    break;
                }
            }

            struct Ex10Protocol const* ex10_protocol = get_ex10_protocol();
            ex10_protocol->reset(Bootloader);
            ex10_protocol->write_calibration_page(info_page, length);
            ex10_protocol->reset(Application);
            if (reinit(uart) != ReturnSuccess)
            {
                return ReturnError;
            }

            uartsend(uart, "Done");
            return ReturnSuccess;
        }
        else
        {  // In some other mode and 'w' is invalid
            uartsend(uart, "Bad command");
            return ReturnError;
        }
    }
}

/**
 * User entered 'x':
 * Stop transmitting
 */
static int stop_transmitting(const struct Ex10UartHelper* uart)
{
    if (!uart)
    {
        return ReturnError;
    }

    get_ex10_reader()->stop_transmitting();
    if (op_result(uart))
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * Route an output to the ATEST bus
 */
static int RouteAtest(const struct Ex10UartHelper* uart,
                      uint32_t                     mux0,
                      uint32_t                     mux1,
                      uint32_t                     mux2,
                      uint32_t                     mux3)
{
    struct Ex10Ops const* ops     = get_ex10_ops();
    struct Ex10Result ex10_result = ops->set_atest_mux(mux0, mux1, mux2, mux3);
    if (ex10_result.error)
    {
        parse_ex10_result(ex10_result, uart);
        return ReturnExitError;
    }

    ex10_result = ops->wait_op_completion();
    if (ex10_result.error)
    {
        return ReturnExitError;
    }

    return ReturnSuccess;
}

/**
 * User entered 'y':
 * Route outputs to ATEST bus
 */
static int set_atest_mux(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    const char* delim = " ";
    char*       param = strtok(command, delim);

    if (!param)
    {
        uartsend(uart, "Bad command");
        return ReturnError;
    }

    bool     parse_error;
    uint32_t mux0 = HexStrToUint32(uart, param, &parse_error);
    if (parse_error)
    {
        uartsend(uart, "Bad mux0 value.");
        return ReturnError;
    }

    param         = strtok(NULL, delim);
    uint32_t mux1 = HexStrToUint32(uart, param, &parse_error);
    if (parse_error)
    {
        uartsend(uart, "Bad mux1 value.");
        return ReturnError;
    }

    param         = strtok(NULL, delim);
    uint32_t mux2 = HexStrToUint32(uart, param, &parse_error);
    if (parse_error)
    {
        uartsend(uart, "Bad mux2 value.");
        return ReturnError;
    }

    param         = strtok(NULL, delim);
    uint32_t mux3 = HexStrToUint32(uart, param, &parse_error);
    if (parse_error)
    {
        uartsend(uart, "Bad mux3 value.");
        return ReturnError;
    }

    if (RouteAtest(uart, mux0, mux1, mux2, mux3))
    {
        return ReturnError;
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * User entered 'z':
 * Start inventory, using simple inventory helper. No way to interrupt
 * The intent here is just to provide some tag reads for a factory check
 */
static int inventory(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    // Defaults for fixed q inventory
    const uint8_t initial_q        = 4u;
    const uint8_t select_all       = 0u;
    const bool    dual_target      = true;
    const uint8_t session          = 0u;
    const bool    tag_focus_enable = false;
    const bool    fast_id_enable   = false;
    const bool    halt_on_all_tags = false;

    // Inventory target_state state
    uint8_t target_state = 0u;

    uint8_t      inv_antenna       = 0u;
    enum RfModes inv_rf_mode       = (enum RfModes)0u;
    int16_t      inv_power_cdbm    = 0;
    uint32_t     inv_frequency_khz = 0u;
    uint32_t     inv_duration_ms   = 0u;

    if (command[0] == '\0')
    {
        // Assume some simple defaults
        inv_antenna       = 1u;
        inv_rf_mode       = mode_5;
        inv_power_cdbm    = 3000;
        inv_frequency_khz = 0u;
        inv_duration_ms   = 50u;
    }
    else
    {
        // Parse all the parameters given for custom inventory
        char* param = strtok(command, " ");
        if (param)
        {
            inv_antenna = (uint8_t)atoi(param);
            if ((inv_antenna != 1u) && (inv_antenna != 2u))
            {
                uartsend(uart, "Antenna must be 1 or 2");
                return ReturnError;
            }
        }
        else
        {
            uartsend(uart, "Bad command");
            return ReturnError;
        }

        param = strtok(NULL, " ");
        if (param)
        {
            int rf_mode_i = atoi(param);
            inv_rf_mode   = (enum RfModes)rf_mode_i;
            if (inv_rf_mode == 0u)
            {
                uartsend(uart, "Bad RF Mode. Suggestion: use RF mode 5");
                return ReturnError;
            }
        }
        else
        {
            uartsend(uart, "Bad command");
            return ReturnError;
        }

        param = strtok(NULL, " ");
        if (param)
        {
            inv_power_cdbm = (int16_t)atoi(param);
            if (inv_power_cdbm < 0 || inv_power_cdbm > 3200)
            {
                uartsend(uart, "Power_cdbm must be within [0,3200]");
                return ReturnError;
            }
        }
        else
        {
            uartsend(uart, "Bad command");
            return ReturnError;
        }

        param = strtok(NULL, " ");
        if (param)
        {
            if (strchr(param, '.'))
            {
                uartsend(uart,
                         "Enter frequency in kHz, or 0 to use region frequency "
                         "table");
                return ReturnError;
            }
            inv_frequency_khz = (uint32_t)atoi(param);
            if (inv_frequency_khz != 0u)
            {
                if ((strcmp(region, "FCC") == 0 &&
                     (inv_frequency_khz < 902000 ||
                      inv_frequency_khz > 928000)) ||
                    (strcmp(region, "ETSI_LOWER") == 0 &&
                     (inv_frequency_khz < 865000 ||
                      inv_frequency_khz > 868000)))
                {
                    uartsend(uart, "Frequency out of band");
                    return ReturnError;
                }
            }
        }
        else
        {
            uartsend(uart, "Bad command");
            return ReturnError;
        }

        param = strtok(NULL, " ");
        if (param)
        {
            inv_duration_ms = (uint32_t)atoi(param);
            if ((inv_duration_ms < 50) || (inv_duration_ms > 60 * 1000))
            {
                uartsend(uart, "Duration value must be between [50, 60000]");
                return ReturnError;
            }
        }
        else
        {
            uartsend(uart, "Bad command");
            return ReturnError;
        }
    }

    /* Used for info in reading out the event FIFO */
    struct InfoFromPackets packet_info = {0u, 0u, 0u, 0u, {0u}};

    struct InventoryRoundControlFields inventory_config = {
        .initial_q            = initial_q,
        .max_q                = initial_q,
        .min_q                = initial_q,
        .num_min_q_cycles     = 1,
        .fixed_q_mode         = true,
        .q_increase_use_query = false,
        .q_decrease_use_query = false,
        .session              = session,
        .select               = select_all,
        .target               = target_state,
        .halt_on_all_tags     = halt_on_all_tags,
        .tag_focus_enable     = tag_focus_enable,
        .fast_id_enable       = fast_id_enable,
    };

    struct InventoryRoundControl_2Fields inventory_config_2 = {
        .max_queries_since_valid_epc = 0};

    struct InventoryHelperParams ihp = {
        .antenna               = inv_antenna,
        .rf_mode               = inv_rf_mode,
        .tx_power_cdbm         = inv_power_cdbm,
        .inventory_config      = &inventory_config,
        .inventory_config_2    = &inventory_config_2,
        .send_selects          = false,
        .remain_on             = false,
        .dual_target           = dual_target,
        .inventory_duration_ms = inv_duration_ms,
        .packet_info           = &packet_info,
        .verbose               = true,
    };

    bool           round_done = true;
    uint32_t const start_time = get_ex10_time_helpers()->time_now();

    // Clear the number of tags found so that if we halt, we can return
    struct Ex10Helpers const* helpers = get_ex10_helpers();
    struct Ex10Reader const*  reader  = get_ex10_reader();
    helpers->clear_info_from_packets(ihp.packet_info);
    helpers->discard_packets(false, true, false);

    if (inv_frequency_khz != 0)
    {
        // setup the region for a single frequency
        get_ex10_active_region()->set_single_frequency(inv_frequency_khz);
    }
    while (get_ex10_time_helpers()->time_elapsed(start_time) <
           ihp.inventory_duration_ms)
    {
        if (ihp.packet_info->total_singulations &&
            ihp.inventory_config->halt_on_all_tags)
        {
            break;
        }
        if (round_done)
        {
            round_done = false;
            struct Ex10Result ex10_result =
                reader->inventory(ihp.antenna,
                                  ihp.rf_mode,
                                  ihp.tx_power_cdbm,
                                  ihp.inventory_config,
                                  ihp.inventory_config_2,
                                  ihp.send_selects,
                                  ihp.remain_on);
            if (ex10_result.error)
            {
                ex10_discard_packets(true, true, true);
                return ReturnError;
            }

            if (ihp.dual_target)
                ihp.inventory_config->target = !ihp.inventory_config->target;
        }

        struct EventFifoPacket const* packet = reader->packet_peek();
        while (packet)
        {
            get_ex10_helpers()->examine_packets(packet, ihp.packet_info);

            if (packet->packet_type == TagRead)
            {
                char                 line[120] = {0};
                struct TagReadFields tag_read =
                    get_ex10_event_parser()->get_tag_read_fields(
                        packet->dynamic_data,
                        packet->dynamic_data_length,
                        packet->static_data->tag_read.type,
                        packet->static_data->tag_read.tid_offset);

                sprintf(line, "EPC ");
                uint8_t const* epc_data = tag_read.epc;
                for (size_t iter = 0u; iter < tag_read.epc_length; ++iter)
                {
                    char byte_hex[3] = {0};
                    sprintf(byte_hex, "%02x", *epc_data++);
                    strcat(line, byte_hex);
                }

                char pc_hex[10] = {0};
                sprintf(
                    pc_hex, "|PC %04x", helpers->swap_bytes(*(tag_read.pc)));
                strcat(line, pc_hex);

                if (tag_read.stored_crc)
                {
                    char           crc_hex[11] = {0};
                    uint16_t const stored_crc =
                        ex10_bytes_to_uint16(tag_read.stored_crc);
                    snprintf(crc_hex, sizeof(crc_hex), "|CRC %04X", stored_crc);
                    strcat(line, crc_hex);
                }

                int16_t compensated_rssi = reader->get_current_compensated_rssi(
                    packet->static_data->tag_read.rssi);
                char rssi_str[16] = {0};
                sprintf(rssi_str, "|RSSI %d\n", compensated_rssi);
                strcat(line, rssi_str);

                (void)line;
                // uart->send(line);

                if (ihp.inventory_config->halt_on_all_tags == false)
                {
                    // We have not set the halt bit, so should'nt be halted.
                    if (packet->static_data->tag_read.halted_on_tag == true)
                    {
                        ex10_ex_eprintf(
                            "Halted on a tag when the halt bit is not set\n");
                    }
                }
            }
            else if (packet->packet_type == InventoryRoundSummary)
            {
                round_done = true;
            }
            else if (packet->packet_type == TxRampDown)
            {
                // Note that session 0 is used and thus on a transmit power
                // down the tag state is reverted to A. If one chose to use
                // a session with persistence between power cycles, this
                // could go away.
                ihp.inventory_config->target = 0;
                round_done                   = true;
            }
            reader->packet_remove();
            packet = reader->packet_peek();
        }
    }
    char result[80] = {0};
    sprintf(result,
            "Result: %zu tags read, with average rate: %8.3f\n",
            ihp.packet_info->total_singulations,
            (float)ihp.packet_info->total_singulations /
                (float)(inv_duration_ms / 1000));
    uart->send(result);

    // If we are told to halt on tags we return to the user after halting, and
    // thus don't clean up
    if (false == ihp.inventory_config->halt_on_all_tags)
    {
        // Regulatory timers will automatically ramp us down, but we are being
        // explicit here.
        reader->stop_transmitting();

        while (false == round_done)
        {
            struct EventFifoPacket const* packet = reader->packet_peek();
            while (packet != NULL)
            {
                get_ex10_helpers()->examine_packets(packet, ihp.packet_info);
                if (packet->packet_type == InventoryRoundSummary)
                {
                    round_done = true;
                }
                reader->packet_remove();
                packet = reader->packet_peek();
            }
        }
    }

    uartsend(uart, "Done");
    return ReturnSuccess;
}

/**
 * This is the main user input capture function. It collects input characters
 * until the user hits Enter (chr(10)). Escape sequences (function keys, page
 * up/down, arrow keys, ins/del, home/end) are ignored. Backspace is handled.
 * Some effort has been made to handle odd terminal handling of rapid backspace
 * entry (white space is skipped when backspace is entered with typematic
 * repeat). Entering ^C or q<Enter> causes terminal to exit.
 */
static int wait_for_command(const struct Ex10UartHelper* uart,
                            char*                        rx_raw_buffer,
                            char*                        rx_buffer,
                            const size_t                 rx_length)
{
    if (!uart || !rx_buffer)
    {
        return ReturnError;
    }

    char*    rx_pointer      = rx_buffer;
    size_t   rx_buffer_avail = rx_length;
    bool     waiting         = true;
    bool     esc_sequence    = false;
    uint32_t time_diff       = get_ex10_time_helpers()->time_now();

    ex10_memzero(rx_buffer, rx_length);

    while (waiting)
    {
        ex10_memzero(rx_raw_buffer, rx_length);
        size_t const count = uart->receive(rx_raw_buffer, rx_buffer_avail);
        time_diff          = get_ex10_time_helpers()->time_elapsed(time_diff);

        if (count > 0)
        {
            if (strchr(rx_raw_buffer, 0x0A) || strchr(rx_raw_buffer, 0x0D) ||
                strchr(rx_raw_buffer, 0x03))  // Enter and ^C
            {
                waiting = false;
            }

            for (size_t iter = 0; iter < count; iter++)
            {
                if (!esc_sequence && (rx_raw_buffer[iter] == 27))  // ESC
                {
                    esc_sequence = true;
                }
                else if (esc_sequence && ((rx_raw_buffer[iter] == 126) ||
                                          (rx_raw_buffer[iter] == 27) ||
                                          (rx_raw_buffer[iter] == 65) ||
                                          (rx_raw_buffer[iter] == 66) ||
                                          (rx_raw_buffer[iter] == 67) ||
                                          (rx_raw_buffer[iter] == 68)))
                {
                    esc_sequence = false;
                }
                else if (!esc_sequence && (rx_raw_buffer[iter] == 127))  // BKSP
                {
                    while ((rx_pointer > rx_buffer) &&
                           isspace(*(rx_pointer - 1)) && time_diff < 100)
                    {
                        rx_buffer_avail++;
                        rx_pointer--;
                        *rx_pointer = '\0';
                    }

                    if (rx_pointer > rx_buffer)
                    {
                        rx_buffer_avail++;
                        uart->send(rx_raw_buffer + iter);
                        rx_pointer--;
                    }
                    *rx_pointer = '\0';
                }
                else if (!esc_sequence)
                {
                    rx_buffer_avail--;
                    if (!rx_buffer_avail)
                    {
                        ex10_ex_eprintf("Command buffer full\n");
                        return ReturnError;
                    }
                    if (uartecho(uart, rx_raw_buffer[iter]) != ReturnSuccess)
                    {
                        return ReturnError;
                    }
                    *rx_pointer = rx_raw_buffer[iter];
                    rx_pointer++;
                }
            }
        }
        get_ex10_time_helpers()->wait_ms(1u);
    }

    return ReturnSuccess;
}

/**
 * Given a command entered by the user, parse command using first character
 * and pass remaining command string to appropriate function for further
 * parsing and execution.
 */
static int do_command(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    int                   result      = ReturnSuccess;
    bool                  wait_for_op = true;
    struct Ex10Ops const* ops         = get_ex10_ops();

    if (command[0] != '\0')
    {
        switch (tolower(command[0]))
        {
            case FirmwareUpgrade:
                uartsend(uart, "Firmware upgrade");
                result = firmware_upgrade(uart, &command[1]);
                break;
            case VersionNumber:
                uartsend(uart, "Firmware version");
                result = get_firmware_version(uart, &command[1]);
                break;
            case SetAnalogRxConfig:
                uartsend(uart, "Set Analog RX config");
                result = set_analog_rx_config(uart, &command[1]);
                break;
            case StartPrbs:
            {
                uartsend(uart, "Start PRBS");
                struct Ex10Result const ex10_result = ops->start_prbs();
                if (ex10_result.error)
                {
                    parse_ex10_result(ex10_result, uart);
                    return ReturnError;
                }
                mode = ModePrbs;
                // Don't wait since OpEnded will come after PRBS is stopped.
                wait_for_op = false;
            }
            break;
            case SetTxCoarseGain:
                uartsend(uart, "Set TX coarse gain");
                result = set_tx_coarse_gain(uart, &command[1]);
                break;
            case RampDown:
            {

                uartsend(uart, "TX ramp down");
                if (op_result(uart))
                {
                    return ReturnError;
                }
                struct Ex10Result const ex10_result = ops->tx_ramp_down();
                if (ex10_result.error)
                {
                    parse_ex10_result(ex10_result, uart);
                    return ReturnError;
                }
                if (op_result(uart))
                {
                    return ReturnError;
                }
            }
            break;
            case EnableRadio:
                uartsend(uart, "Enable radio");
                result = enable_radio(uart, &command[1]);
                break;
            case SetTxFineGain:
                uartsend(uart, "Set TX fine gain");
                result = set_tx_fine_gain(uart, &command[1]);
                break;
            case SetGpo:
                uartsend(uart, "Set GPO");
                result = set_gpo(uart, &command[1]);
                break;
            case PrintHelp:
            case PrintHelpAlt:
                uart->send("Display help\n");
                result = send_help(uart);
                break;
            case ReadDumpInfoPage:
                result = read_info_page(uart, &command[1]);
                break;
            case GetDeviceSku:
                uartsend(uart, "Report device SKU");
                result = read_device_sku(uart);
                break;
            case LockSynthesizer:
                uartsend(uart, "Lock synthesizer [kHz]");
                result = lock_synthesizer(uart, &command[1]);
                break;
            case MeasureAdc:
                result = measure_adc(uart, &command[1]);
                break;
            case Reset:
                uartsend(uart, "Reset");
                result = reset(uart, &command[1]);
                break;
            case RadioPowerControl:
                result = radio_power_control(uart, &command[1]);
                break;
            case SetRegion:
                result = set_region(uart, &command[1]);
                break;
            case ReadRssi:
                uartsend(uart, "Read RSSI");
                result = read_rssi(uart);
                break;
            case CompRssi:
                uartsend(uart, "Comp RSSI");
                result = read_compensated_rssi(uart, &command[1]);
                break;
            case LbtRssi:
                uartsend(uart, "LBT RSSI");
                result = read_lbt_rssi(uart, &command[1]);
                break;
            case CwTest:
                uartsend(uart, "CW Test");
                result = cw_test(uart, &command[1]);
                break;
            case RampUp:
                uartsend(uart, "TX ramp up");
                result = cal_tx_ramp_up(uart, &command[1]);
                break;
            case WriteCalInfoPage:
                uartsend(uart, "Write calibration info page");
                result = write_info_page(uart);
                break;
            case QuitWrapper:
            case QuitWrapperAlt:
                uartsend(uart, "Quit");
                uartsend(uart, "Exiting");
                result = ReturnGracefulExit;
                break;
            case SetVerbose:
                result = set_verbose_mode(uart, &command[1]);
                break;
            case StopTx:
                uartsend(uart, "Stop transmitting");
                result = stop_transmitting(uart);
                break;
            case SetAtestMux:
                result = set_atest_mux(uart, &command[1]);
                break;
            case DoShortInventory:
                result = inventory(uart, &command[1]);
                break;
            default:
                uartsend(uart, "Unknown command");
                break;
        }
        if (wait_for_op &&
            get_ex10_protocol()->get_running_location() != Bootloader)
        {
            ops->wait_op_completion();
        }
    }

    // Controlling program (or human) will use OK to trigger next command
    if (result >= 0)
    {
        result == 0 ? uart->send("OK\n") : uart->send("ERROR\n");
    }

    return result;
}

/**
 * Given a line of calibration data in hex dump form, parse to binary
 * and store in info page buffer
 */
static int parse_and_store_cal_data(const struct Ex10UartHelper* uart,
                                    char*                        data)
{
    if (!uart || !data)
    {
        return ReturnError;
    }

    int result = ReturnSuccess;
    if (data[0] == '\0')
    {
        // Do nothing
    }
    else if (tolower(data[0]) == WriteCalInfoPage)
    {
        result = write_info_page(uart);
    }
    else if (!strchr(data, ':'))
    {
        uartsend(uart, "Bad data format.");
        result = ReturnError;
    }
    else
    {
        char* nextchar      = NULL;
        errno               = 0;
        size_t const offset = (size_t)strtoul(data, &nextchar, 16);
        if (errno != 0)
        {
            perror("Error parsing data offset: ");
            result = ReturnError;
        }
        else if (offset > 2047)
        {
            uartsend(uart, "Data offset is a hex value [0,7FF]");
            result = ReturnError;
        }
        else if (*nextchar == '\0' || nextchar == data)
        {
            uartsend(uart, "No offset found");
            result = ReturnError;
        }
        else
        {
            size_t count = 0;
            data         = nextchar;
            nextchar     = NULL;
            while (*data != '\0' && isxdigit(*data) == false)
            {
                data++;
            }
            while (*data != '\0' && nextchar != data)
            {
                errno            = 0;
                size_t const val = (size_t)strtoul(data, &nextchar, 16);
                if (errno != 0)
                {
                    perror("Error parsing hex data byte: ");
                    result = ReturnError;
                    break;
                }
                else if (val > 255)
                {
                    uartsend(uart, "Hex data value should be in range [0,ff]");
                    result = ReturnError;
                    break;
                }
                else if (*nextchar != '\n' && nextchar != data)
                {
                    info_page[offset + count] = (uint8_t)val;
                    count++;
                    data     = nextchar;
                    nextchar = NULL;
                }
            }
            // ex10_ex_printf("Parsed %zu values at offset %zu\n", count,
            // offset);
            uartsend(uart, "Done");
        }
    }

    // Controlling program (or human) will use OK to trigger next command
    if (result >= 0)
    {
        result == 0 ? uart->send("OK\n") : uart->send("ERROR\n");
    }

    return result;
}

/**
 * When in PRBS mode, check to see if 'x' is pressed to stop transmitting
 * and return to normal mode. Ignore all other commands (except quit)
 */
static int start_stop_prbs(const struct Ex10UartHelper* uart, char* command)
{
    if (!uart || !command)
    {
        return ReturnError;
    }

    int result = ReturnSuccess;
    if (command[0] != '\0')
    {
        switch (tolower(command[0]))
        {
            case QuitWrapper:
            case QuitWrapperAlt:
                uartsend(uart, "Quit");
                uartsend(uart, "Exiting");
                result = ReturnExitError;
                break;
            case StopTx:
                uartsend(uart, "Stop transmitting");
                result = stop_transmitting(uart);
                mode   = ModeNormal;
                break;
            default:
                uartsend(uart, "Ignored command");
                break;
        }
    }

    // Controlling program (or human) will use OK to trigger next command
    if (result >= 0)
    {
        result == 0 ? uart->send("OK\n") : uart->send("ERROR\n");
    }

    return result;
}

void enable_sdd(void)
{
    struct LogEnablesFields log_enables = {0};
    log_enables.op_logs                 = true;
    log_enables.ramping_logs            = true;
    log_enables.lmac_logs               = true;
    log_enables.insert_fifo_event_logs  = true;
    log_enables.host_irq_logs           = true;
    log_enables.regulatory_logs         = true;
    const uint8_t log_speed_mhz         = 12;
    get_ex10_reader()->enable_sdd_logs(log_enables, log_speed_mhz);
}

int main(void)
{
    // The longest string will be for an ascii-hex upload string: '^ s '
    // plus 6 chars for the image length, plus 3 chars for each of 1021
    // data bytes, plus some extra.
    char uart_rx_buffer[EX10_MAX_IMAGE_CHUNK_SIZE * 3 + 6 + 10] = {0};
    char uart_rx_raw_buffer[sizeof(uart_rx_buffer)]             = {0};

    ex10_ex_printf(
        "##############################################################\n");
    ex10_ex_printf(
        "# Ex10 Northbound serial interface wrapper                   #\n");
    ex10_ex_printf(
        "##############################################################\n");
    ex10_ex_printf("\n");

    const struct Ex10Result ex10_result =
        ex10_typical_board_setup(DEFAULT_SPI_CLOCK_HZ, REGION_FCC);

    if (ex10_result.error)
    {
        return ReturnError;
    }

    get_ex10_ramp_module_manager()->unregister_ramp_callbacks();

    get_ex10_protocol()->set_event_fifo_threshold(0u);

    ex10_typical_board_uart_setup(Bps_115200);
    const struct Ex10UartHelper* uart = get_ex10_uart_helper();

    enable_sdd();

    // READY indicates northbound interface is ready to parse commands
    uart->send("READY\n");

    int result = ReturnSuccess;
    while (result >= 0)
    {
        if (wait_for_command(uart,
                             uart_rx_raw_buffer,
                             uart_rx_buffer,
                             sizeof(uart_rx_buffer)) != ReturnSuccess)
        {
            return ReturnError;
        }

        switch (mode)
        {
            case ModeNormal:
                result = do_command(uart, trim(uart_rx_buffer));
                break;
            case ModeRxCal:
                parse_and_store_cal_data(uart, trim(uart_rx_buffer));
                break;
            case ModePrbs:
                start_stop_prbs(uart, trim(uart_rx_buffer));
                break;
            default:
                // Reset mode
                mode = ModeNormal;
                break;
        }
    }

    ex10_typical_board_uart_teardown();
    ex10_typical_board_teardown();

    if (result == ReturnGracefulExit)
    {
        return ReturnSuccess;
    }

    return result;
}
