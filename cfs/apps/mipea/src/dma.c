/*
 * dma.c
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#include <stdint.h>
#include <stddef.h>
#include <errno.h>

#include "dma.h"
#include "peripherals.h"
#include "mailbox_mod.h"

static const size_t DMA_OFFSET = 0x007000; // TODO: add channel 15
static const size_t DMA_SIZE = 0xFF4;

static volatile uint32_t *dma_base_ptr = NULL;

volatile struct dma_channel_register_map *DMAC0 = NULL;
volatile struct dma_channel_register_map *DMAC1 = NULL;
volatile struct dma_channel_register_map *DMAC2 = NULL;
volatile struct dma_channel_register_map *DMAC3 = NULL;
volatile struct dma_channel_register_map *DMAC4 = NULL;
volatile struct dma_channel_register_map *DMAC5 = NULL;
volatile struct dma_channel_register_map *DMAC6 = NULL;
volatile struct dma_channel_register_map *DMAC7 = NULL;
volatile struct dma_channel_register_map *DMAC8 = NULL;
volatile struct dma_channel_register_map *DMAC9 = NULL;
volatile struct dma_channel_register_map *DMAC10 = NULL;
volatile struct dma_channel_register_map *DMAC11 = NULL;
volatile struct dma_channel_register_map *DMAC12 = NULL;
volatile struct dma_channel_register_map *DMAC13 = NULL;
volatile struct dma_channel_register_map *DMAC14 = NULL;

volatile struct dma_register_map *DMA = NULL;

int dma_map(void)
{
    // open /dev/vcio which is used in mailbox
    _mbox_fd = mbox_open();
    if (_mbox_fd < 0)
        return -1;
    if (peripheral_map(&dma_base_ptr, DMA_OFFSET, DMA_SIZE) < 0)
        return -1;

    DMAC0 = (volatile struct dma_channel_register_map *)(dma_base_ptr);
    DMAC1 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x40);
    DMAC2 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x80);
    DMAC3 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0xc0);
    DMAC4 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x100);
    DMAC5 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x140);
    DMAC6 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x180);
    DMAC7 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x1c0);
    DMAC8 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x200);
    DMAC9 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x240);
    DMAC10 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x280);
    DMAC11 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x2c0);
    DMAC12 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x300);
    DMAC13 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x340);
    DMAC14 = (volatile struct dma_channel_register_map *)(dma_base_ptr + 0x380);

    DMA = (volatile struct dma_register_map *)(dma_base_ptr + 0x3f8);

    return 0;
}

void dma_unmap(void)
{
    peripheral_unmap(dma_base_ptr, DMA_SIZE);
    mbox_close(_mbox_fd); // close it again too
}

void dma_configure(dma_channel_config *config)
{
    config->channel->CS = config->dma_register.cs_register << 16; //OSK
}
void dma_enable(volatile struct dma_channel_register_map *channel)
{
    channel->CS |= 1;
}
void dma_disable(volatile struct dma_channel_register_map *channel)
{
    channel->CS &= ~1;
}

uint32_t dma_virt_to_phy(dma_phy_mem_blk *block, void *addr)
{
    unsigned int offset = (uint8_t *)addr - (uint8_t *)block->mem;
	return block->bus_addr + offset;
}

void dma_alloc_phy_mem(dma_phy_mem_blk *block, unsigned int size)
{
    block->size = (size / PAGE_SIZE + 1) * 4096; // round to 4096
    block->handle = mbox_alloc(_mbox_fd, block->size, PAGE_SIZE,
        MEM_FLAG_L1_NONALLOCATING | MEM_FLAG_ZERO);
    block->bus_addr = mbox_lock(_mbox_fd, block->handle);
    block->mem = mbox_map(BUS_TO_PHYS(block->bus_addr), block->size);
}

void dma_free_phy_mem(dma_phy_mem_blk *block)
{
    mbox_unmap(block->mem, block->size);
    mbox_unlock(_mbox_fd, block->handle);
    mbox_free(_mbox_fd, block->handle);
    block->mem = NULL;
}
