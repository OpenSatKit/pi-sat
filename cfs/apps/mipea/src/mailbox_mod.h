/*
 * mailbox_mod.h
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 *
 * Based on
 *      mailbox.h (https://github.com/raspberrypi/userland/blob/master/host_applications/linux/apps/hello_pi/hello_fft/mailbox.h)
 *      Copyright (c) 2012, Broadcom Europe Ltd.
 */

#ifndef _MAILBOX_MOD_H_
#define _MAILBOX_MOD_H_

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

// the macros are for backward compatibility
#define mbox_alloc  mem_alloc
#define mbox_free   mem_free
#define mbox_lock   mem_lock
#define mbox_unlock mem_unlock
#define mbox_map    mapmem
#define mbox_unmap  unmapmem
#define mbox_vc_execute     execute_code
#define mbox_qpu_enable     qpu_enable
#define mbox_qpu_execute    execute_qpu


#define MAJOR_NUM 100
#define IOCTL_MBOX_PROPERTY _IOWR(MAJOR_NUM, 0, uint8_t *)
#define DEVICE_FILE_NAME    "/dev/vcio"

#define PAGE_SIZE       4096
#define BUS_TO_PHYS(x)  ((x) & ~0xC0000000)

// From: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface#allocate-memory
#define MEM_FLAG_DISCARDABLE        0x01 // can be resized to 0 at any time. Use for cached data
#define MEM_FLAG_NORMAL             0x00 // normal allocating alias. Don't use from ARM
#define MEM_FLAG_DIRECT             0x04 // 0xC alias uncached
#define MEM_FLAG_COHERENT           0x08 // 0x8 alias. Non-allocating in L2 but coherent
#define MEM_FLAG_L1_NONALLOCATING   (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT) // Allocating in L2
#define MEM_FLAG_ZERO               0x10 // initialise buffer to all zeros
#define MEM_FLAG_NO_INIT            0x20 // don't initialise (default is initialise to all ones
#define MEM_FLAG_HINT_PERMALOCK     0x40 // Likely to be locked for long periods of time

typedef struct {
    uint32_t id;
    uint32_t data_size;
    uint32_t req_code; // 31st bit cleared for requests
} mbox_tag_t;

typedef struct {
    uint32_t size;
    uint32_t req_code; // 0x00000000 for requests
    mbox_tag_t tag;
} mbox_buffer_t;

int     mbox_open(void);
void    mbox_close(int fd);

uint32_t mbox_alloc(int fd, unsigned int size, unsigned int align, unsigned int flags);
uint32_t mbox_free(int fd, uint32_t handle);

uint32_t mbox_lock(int fd, uint32_t handle);
uint32_t mbox_unlock(int fd, uint32_t handle);

void *  mbox_map(uint32_t addr, unsigned int size);
void    mbox_unmap(void *ptr, unsigned int size);

uint32_t mbox_vc_execute(int fd, uint32_t func_ptr, uint32_t r0, uint32_t r1,
    uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5);
uint32_t mbox_qpu_enable(int fd, uint32_t state);
uint32_t mbox_qpu_execute(int fd, uint32_t num_qpus, uint32_t control,
    uint32_t noflush, uint32_t timeout);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//_MAILBOX_MOD_H_
