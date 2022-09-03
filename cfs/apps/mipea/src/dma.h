/*
 * dma.h
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#ifndef _DMA_H_
#define _DMA_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

static const size_t PERIPHERAL_BASE_PHY = 0x7E000000;

struct dma_channel_register_map {
    uint32_t CS;
    uint32_t CONBLK_AD;
    uint32_t TI;
    uint32_t SOURCE_AD;
    uint32_t DEST_AD;
    uint32_t TXFR_LEN;
    uint32_t STRIDE;
    uint32_t NEXTCONBK;
    uint32_t DEBUG;
};

struct dma_register_map {
    uint32_t INT_STATUS;
    uint32_t: 32;
    uint32_t: 32;
    uint32_t: 32;
    uint32_t ENABLE;
};

typedef struct {
    uint32_t TI;
    uint32_t SOURCE_AD;
    uint32_t DEST_AD;
    uint32_t TXFR_LEN;
    uint32_t STRIDE;
    uint32_t NEXTCONBK;
    uint32_t: 32; // not needed?
    uint32_t: 32;
} dma_cb;

typedef struct {
    uint32_t handle;
    uint32_t bus_addr;
    void *mem;
    unsigned int size;
} dma_phy_mem_blk;

typedef struct {
    struct dma_channel_register_map *channel;
    union {
        struct {
            uint32_t DISDEBUG: 1;
            uint32_t WTOUTWRT: 1; // wait for outstanding writes
            uint32_t: 4; // reserved
            uint32_t PANIC_PRIORITY: 4;
            uint32_t PRIOTITY: 4;
        } dma_bitfield;           //OSK
        uint32_t cs_register;
    } dma_register;               //OSK
} dma_channel_config;

extern volatile struct dma_channel_register_map *DMAC0;
extern volatile struct dma_channel_register_map *DMAC1;
extern volatile struct dma_channel_register_map *DMAC2;
extern volatile struct dma_channel_register_map *DMAC3;
extern volatile struct dma_channel_register_map *DMAC4;
extern volatile struct dma_channel_register_map *DMAC5;
extern volatile struct dma_channel_register_map *DMAC6;
extern volatile struct dma_channel_register_map *DMAC7;
extern volatile struct dma_channel_register_map *DMAC8;
extern volatile struct dma_channel_register_map *DMAC9;
extern volatile struct dma_channel_register_map *DMAC10;
extern volatile struct dma_channel_register_map *DMAC11;
extern volatile struct dma_channel_register_map *DMAC12;
extern volatile struct dma_channel_register_map *DMAC13;
extern volatile struct dma_channel_register_map *DMAC14;

extern volatile struct dma_register_map *DMA;

int     dma_map(void);
void    dma_unmap(void);

void    dma_configure(dma_channel_config *config);
void    dma_enable(volatile struct dma_channel_register_map *channel);
void    dma_disable(volatile struct dma_channel_register_map *channel);

uint32_t dma_virt_to_phy(dma_phy_mem_blk *block, void *addr);

void dma_alloc_phy_mem(dma_phy_mem_blk *block, unsigned int size);
void dma_free_phy_mem(dma_phy_mem_blk *block);

extern int _mbox_fd;

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//_DMA_H_
