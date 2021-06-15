/*
 * spi.c
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#include <stdint.h>
#include <stddef.h>

#include "spi.h"
#include "peripherals.h"
#include "../config.h" // for inline

static const size_t SPI_OFFSET = 0x204000;
static const size_t SPI_SIZE = 0x14;

static volatile uint32_t *spi_base_ptr = NULL;
volatile struct spi_register_map *SPI = NULL;

int spi_map(void)
{
    if (peripheral_map(&spi_base_ptr, SPI_OFFSET, SPI_SIZE) < 0)
        return -1;
    SPI = (volatile struct spi_register_map *)spi_base_ptr;
    return 0;
}

void spi_unmap(void)
{
    peripheral_unmap(spi_base_ptr, SPI_SIZE);
}

void spi_configure(spi_channel_config *config)
{
    SPI->CS = config->spi_register.cs_register;
    SPI->CLK = config->divisor;
}

void spi_set_ce(uint8_t ce)
{
    SPI->CS = ce & 3; // only use the last 2 bits
}

inline void spi_transfer_start(void)
{
    SPI->CS |= 0x80; // set TA
}

inline void spi_transfer_stop(void)
{
    while(!(SPI->CS & 0x10000));// wait for DONE
    SPI->CS &= ~0x80; // clear TA
}

inline uint8_t spi_transfer_byte(uint8_t data)
{
    SPI->CS |= 0x30; // Clear FIFO
    SPI->FIFO = data;
    while(!(SPI->CS & 0x20000)); // wait for data in RX FIFO
    return (uint8_t) SPI->FIFO;
}

// send 2 bytes and return the data from the last transmission
inline uint8_t spi_send2_recv1(uint8_t data0, uint8_t data1)
{
    spi_transfer_start();
    spi_transfer_byte(data0);
    uint8_t ret = spi_transfer_byte(data1);
    spi_transfer_stop();
    return ret;
}
