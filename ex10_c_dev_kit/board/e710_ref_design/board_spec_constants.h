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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "ex10_api/event_fifo_packet_types.h"

/** The E710 evaluation board crystal oscillator frequency. */
#define TCXO_FREQ_KHZ ((uint32_t)24000u)

/**
 * The maximum SPI transaction size supported by the host processor, in bytes.
 * @note The maximum Ex10 application command size is 1023 bytes.
 *       The EX10_SPI_BURST_SIZE must not be larger than the command size.
 */
#define EX10_SPI_BURST_SIZE ((size_t)1023u)

static_assert(EX10_SPI_BURST_SIZE <= 1023u,
              "EX10_SPI_BURST_SIZE > Ex10 application max command size");

/**
 * The value used for the fifo threshold will dictate how often the
 * SDK reads from the device. This, in accordance with number of tags being
 * read, dictates how many SDK event FIFO buffers are being used at a given
 * time. A smaller value will use more buffers, but the amount in that buffer
 * will be smaller and therefore faster to process. There is a tradeoff based
 * on memory available, traffic to the device, and tags being read.
 */
static uint16_t const DEFAULT_EVENT_FIFO_THRESHOLD = EX10_EVENT_FIFO_SIZE / 2;

/**
 * Define the PA power range transmit power threshold.
 * This value will determine which PA power range is used when ramping up CW
 * to its target power value.
 *
 * When the Tx power, in units of cdBm, is greater than or equal to this value,
 * then the high power PA setting is used.
 *
 * When the Tx power, in units of cdBm, is less than this value, then the low
 * power PA setting is used.
 *
 * On the Impinj Reader Chip development board schematic the signal
 * DIGITAL_IO[15] PWR_RANGE controls the PA power range setting.
 */
static int16_t const LOW_BIAS_TX_POWER_MAX_CDBM = 2700u;

/**
 * Define the number of antenna ports supported by the board
 */
#define ANTENNA_PORT_COUNT ((size_t)2u)

/**
 * Returns the RX insertion loss based on the board. This insertion loss
 * should be modified to match the used board. This insertion loss is
 * the loss in cdB of the output power as seen at the reverse power
 * detectors. This includes the 10dB loss from directional coupler
 * between TX and RX.
 */
static uint16_t const BOARD_INSERTION_LOSS_RX = 1100u;

/**
 * Returns the LO insertion loss based on the board. This insertion loss
 * should be modified to match the used board. This insertion loss is
 * the loss in cdB of the output power as seen at the reverse power
 * detectors.
 */
static uint16_t const BOARD_INSERTION_LOSS_LO = 1440u;
