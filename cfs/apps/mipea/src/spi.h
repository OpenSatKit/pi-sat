/*
 * spi.h
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

struct spi_register_map {
    uint32_t CS;
    uint32_t FIFO;
    uint32_t CLK;
    uint32_t DLEN;
    uint32_t LTOH;
    uint32_t DC;
};

typedef struct {
    union {
        struct {
            uint32_t: 2;
            uint32_t cpha: 1;
            uint32_t cpol: 1;
            uint32_t: 2;        // unimplemented / unused
            uint32_t cspol: 1;  // Not changing anything?
            uint32_t: 14;       // unimplemented / unused
            uint32_t cspol0: 1;
            uint32_t cspol1: 1;
            uint32_t cspol2: 1;
        } spi_bitfield;   //osk
        uint32_t cs_register;   // cs = control and status
    } spi_register;   //osk

    uint16_t divisor;
} spi_channel_config;

extern volatile struct spi_register_map *SPI;

int     spi_map(void);
void    spi_unmap(void);

void spi_configure(spi_channel_config *config);
void spi_set_ce(uint8_t ce);

extern void     spi_transfer_start(void);
extern void     spi_transfer_stop(void);
extern uint8_t  spi_transfer_byte(uint8_t data);

extern uint8_t  spi_send2_recv1(uint8_t data0, uint8_t data1);

/******** CS Register bit values ********/
enum {
    SPI_CS_CE0 = 0x0,
    SPI_CS_CE1 = 0x1,
    SPI_CS_CE2 = 0x2
};
enum {
    SPI_CPHA_CLK_BEGINNING = 0x0,
    SPI_CPHA_CLK_MIDDLE = 0x1
};
enum {
    SPI_CPOL_RESET_LOW = 0x0,
    SPI_CPOL_RESET_HIGH = 0x1
};
enum {
    SPI_CSPOL_ACTIVE_LOW = 0x0, // can also be used for CSPOL0, 1 or 2
    SPI_CSPOL_ACTIVE_HIGH = 0x1
};

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_SPI_H_
