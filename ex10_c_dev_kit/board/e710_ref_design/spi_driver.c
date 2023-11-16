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
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "board/spi_driver.h"
#include "ex10_api/ex10_print.h"

static uint32_t const default_clock_freq_hz = 4000000u;

struct SpiParameters
{
    int           fd;
    unsigned char bits_per_word;
    unsigned int  clock_freq_hz;
};

static struct SpiParameters spi_0 = {-1, 0, 0};

static int32_t spi_open(uint32_t clock_freq_hz)
{
    // SPI_MODE_1 uses CPOL = 0, CPHA = 1
    const uint8_t spi_mode = SPI_MODE_1;

    spi_0.bits_per_word = 8u;
    spi_0.clock_freq_hz =
        (clock_freq_hz == 0) ? default_clock_freq_hz : clock_freq_hz;

    char const* spi_dev_name = "/dev/spidev0.0";
    spi_0.fd                 = open(spi_dev_name, O_RDWR);

    if (spi_0.fd < 0)
    {
        ex10_eprintf(
            "open(%s) failed: %s: %d\n", spi_dev_name, strerror(errno), errno);
        return -errno;
    }

    int retval = 0;
    retval     = ioctl(spi_0.fd, SPI_IOC_WR_MODE, &spi_mode);
    if (retval < 0)
    {
        ex10_eprintf(
            "ioctl(SPI_IOC_WR_MODE) failed: %s: %d\n", strerror(errno), errno);
        return -errno;
    }

    retval = ioctl(spi_0.fd, SPI_IOC_WR_BITS_PER_WORD, &spi_0.bits_per_word);
    if (retval < 0)
    {
        ex10_eprintf("ioctl(SPI_IOC_WR_BITS_PER_WORD) failed: %s: %d\n",
                     strerror(errno),
                     errno);
        return -errno;
    }

    retval = ioctl(spi_0.fd, SPI_IOC_RD_MODE, &spi_mode);
    if (retval < 0)
    {
        ex10_eprintf(
            "ioctl(SPI_IOC_RD_MODE) failed: %s: %d\n", strerror(errno), errno);
        return -errno;
    }

    retval = ioctl(spi_0.fd, SPI_IOC_RD_BITS_PER_WORD, &spi_0.bits_per_word);
    if (retval < 0)
    {
        ex10_eprintf("ioctl(SPI_IOC_RD_BITS_PER_WORD) failed: %s: %d\n",
                     strerror(errno),
                     errno);
        return -errno;
    }

    retval = ioctl(spi_0.fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_0.clock_freq_hz);
    if (retval < 0)
    {
        ex10_eprintf("ioctl(SPI_IOC_WR_MAX_SPEED_HZ) failed: %s: %d\n",
                     strerror(errno),
                     errno);
        return -errno;
    }

    retval = ioctl(spi_0.fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_0.clock_freq_hz);
    if (retval < 0)
    {
        ex10_eprintf("ioctl(SPI_IOC_RD_MAX_SPEED_HZ) failed: %s: %d\n",
                     strerror(errno),
                     errno);
        return -errno;
    }

    return 0;
}

static void spi_close(void)
{
    if (spi_0.fd == -1)
    {
        return;
    }
    int spi_0_fd_to_close = spi_0.fd;
    spi_0.fd              = -1;
    if (close(spi_0_fd_to_close) < 0)
    {
        ex10_eprintf("spi_close() failed: %s: %d\n", strerror(errno), errno);
    }
}

static int32_t spi_write(const void* tx_buff, size_t length)
{
    if (spi_0.fd == -1)
    {
        return -1;
    }
    ssize_t const retval = write(spi_0.fd, tx_buff, length);
    if (retval < 0)
    {
        ex10_eprintf(
            "spi_write(%zu) failed: %s: %d\n", length, strerror(errno), errno);
        return -1;
    }
    else if ((size_t)retval != length)
    {
        ex10_eprintf("spi_write(%zd) != %zu, unexpected bytes written\n",
                     retval,
                     length);
        return -1;
    }
    return retval;
}

static int32_t spi_read(void* rx_buff, size_t length)
{
    if (spi_0.fd == -1)
    {
        return -1;
    }
    ssize_t const retval = read(spi_0.fd, rx_buff, length);

    if (retval < 0)
    {
        ex10_eprintf("spi_read() failed: %s: %d\n", strerror(errno), errno);
        return -1;
    }
    else if ((size_t)retval != length)
    {
        ex10_eprintf(
            "spi_read(%zd) != %zu, unexpected bytes read\n", retval, length);
        return -1;
    }
    return retval;
}

static struct Ex10SpiDriver const ex10_spi_driver = {
    .spi_open  = spi_open,
    .spi_close = spi_close,
    .spi_write = spi_write,
    .spi_read  = spi_read,
};

struct Ex10SpiDriver const* get_ex10_spi_driver(void)
{
    return &ex10_spi_driver;
}
