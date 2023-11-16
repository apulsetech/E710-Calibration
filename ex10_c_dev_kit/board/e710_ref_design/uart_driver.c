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

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "board/ex10_osal.h"
#include "board/uart_driver.h"
#include "ex10_api/ex10_print.h"

static speed_t const  default_speed_enum = B115200;
static tcflag_t const default_width      = CS8;

struct UartParameters
{
    int      fd;
    tcflag_t bits_per_word;
    speed_t  speed;
};

static struct UartParameters uart_0;

static int32_t uart_open(enum AllowedBpsRates bitrate)
{
    speed_t speed = default_speed_enum;

    char const* uart_dev_name = "/dev/ttyS0";
    uart_0.fd = open(uart_dev_name, O_RDWR | O_NOCTTY | O_NDELAY);

    if (uart_0.fd < 0)
    {
        ex10_eprintf(
            "open(%s) failed: %s: %d\n", uart_dev_name, strerror(errno), errno);
        return -errno;
    }

    fcntl(uart_0.fd, F_SETFL, 0);

    switch (bitrate)
    {
        case Bps_9600:
            speed = B9600;
            break;
        case Bps_19200:
            speed = B19200;
            break;
        case Bps_38400:
            speed = B38400;
            break;
        case Bps_57600:
            speed = B57600;
            break;
        case Bps_115200:
            speed = B115200;
            break;
        default:
            // Do nothing
            break;
    }

    struct termios uart_opts;
    ex10_memzero(&uart_opts, sizeof(uart_opts));
    tcgetattr(uart_0.fd, &uart_opts);
    cfsetispeed(&uart_opts, speed);
    cfsetospeed(&uart_opts, speed);
    uart_opts.c_cflag |= (CLOCAL | CREAD | default_width);
    uart_opts.c_lflag &= (tcflag_t) ~(ICANON | ECHO | ECHOE | ISIG);
    uart_opts.c_oflag |= OPOST;
    tcsetattr(uart_0.fd, TCSANOW, &uart_opts);

    uart_0.speed         = speed;
    uart_0.bits_per_word = default_width;

    return 0;
}

static void uart_close(void)
{
    if (close(uart_0.fd) < 0)
    {
        ex10_eprintf("uart_close() failed: %s: %d\n", strerror(errno), errno);
    }
}

static int32_t uart_write(const void* tx_buff, size_t length)
{
    ssize_t const retval = write(uart_0.fd, tx_buff, length);

    if (retval < 0)
    {
        ex10_eprintf(
            "uart_write(%zu) failed: %s: %d", length, strerror(errno), errno);
        return -1;
    }
    else if ((size_t)retval != length)
    {
        ex10_eprintf("uart_write(%zd) != %zu, unexpected bytes written\n",
                     retval,
                     length);
        return -1;
    }

    return retval;
}

static int32_t uart_read(void* rx_buff, size_t length)
{
    ssize_t const retval = read(uart_0.fd, rx_buff, length);

    if (retval < 0)
    {
        ex10_eprintf("uart_read() failed: %s: %d", strerror(errno), errno);
        return -1;
    }

    return (int32_t)retval;
}

static struct Ex10UartDriver const ex10_uart_driver = {
    .uart_open  = uart_open,
    .uart_close = uart_close,
    .uart_write = uart_write,
    .uart_read  = uart_read,
};

struct Ex10UartDriver const* get_ex10_uart_driver(void)
{
    return &ex10_uart_driver;
}
