/*
 * mailbox_mod.c
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 *
 * Based on
 *      mailbox.c (https://github.com/raspberrypi/userland/blob/master/host_applications/linux/apps/hello_pi/hello_fft/mailbox.c)
 *      Copyright (c) 2012, Broadcom Europe Ltd.
 */

#include "mailbox_mod.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define perror_inf()	fprintf(stderr, "%s:%d: In function %s:\n", __FILE__,  \
    __LINE__, __func__)

int mbox_open(void)
{
    int fd;
    if ((fd = open(DEVICE_FILE_NAME, 0)) < 0) {
        perror_inf();
        perror("Can't open device file: /dev/vcio");
        return -1;
    }
    return fd;
}

void mbox_close(int fd)
{
    close(fd);
}

uint32_t mbox_alloc(int fd, unsigned int size, unsigned int align,
        unsigned int flags)
{
    struct request_alloc {
        mbox_buffer_t buffer;
        union {
            uint32_t size;
            uint32_t handle;
        } xfer;      //osk
        uint32_t alignment;
        uint32_t flags;
        uint32_t end_tag;
    };

    volatile struct request_alloc __attribute__((aligned(16))) request;
    request.buffer.size = sizeof(struct request_alloc);
    request.buffer.req_code = 0;
    request.buffer.tag.id = 0x0003000c;
    request.buffer.tag.data_size = 3 * sizeof(uint32_t);
    request.buffer.tag.req_code = 0;

    request.xfer.size = size;   //osk
    request.alignment = align;
    request.flags = flags;

    request.end_tag = 0;

    ioctl(fd, IOCTL_MBOX_PROPERTY, &request);
    return request.xfer.handle;   //osk
}

uint32_t mbox_free(int fd, uint32_t handle)
{
    struct request_release {
        mbox_buffer_t buffer;
        union {
            uint32_t handle;
            uint32_t status; // 0 on success
        } xfer;   //osk
        uint32_t end_tag;
    };

    volatile struct request_release __attribute__((aligned(16))) request;
    request.buffer.size = sizeof(struct request_release);
    request.buffer.req_code = 0;
    request.buffer.tag.id = 0x0003000f;
    request.buffer.tag.data_size = sizeof(uint32_t);
    request.buffer.tag.req_code = 0;

    request.xfer.handle = handle;   //osk

    request.end_tag = 0;

    ioctl(fd, IOCTL_MBOX_PROPERTY, &request);
    return request.xfer.status;   //osk
}

uint32_t mbox_lock(int fd, uint32_t handle)
{
    struct request_lock {
        mbox_buffer_t buffer;
        union {
            uint32_t handle;
            uint32_t bus_addr;
        } xfer;    //osk
        uint32_t end_tag;
    };

    volatile struct request_lock __attribute__((aligned(16))) request;
    request.buffer.size = sizeof(struct request_lock);
    request.buffer.req_code = 0;
    request.buffer.tag.id = 0x0003000d;
    request.buffer.tag.data_size = sizeof(uint32_t);
    request.buffer.tag.req_code = 0;

    request.xfer.handle = handle;   //osk

    request.end_tag = 0;

    ioctl(fd, IOCTL_MBOX_PROPERTY, &request);
    return request.xfer.bus_addr;   //osk
}

uint32_t mbox_unlock(int fd, uint32_t handle)
{
    struct request_unlock {
        mbox_buffer_t buffer;
        union {
            uint32_t handle;
            uint32_t status; // 0 on success
        } xfer;   //osk
        uint32_t end_tag;
    };

    volatile struct request_unlock __attribute__((aligned(16))) request;
    request.buffer.size = sizeof(struct request_unlock);
    request.buffer.req_code = 0;
    request.buffer.tag.id = 0x0003000e;
    request.buffer.tag.data_size = sizeof(uint32_t);
    request.buffer.tag.req_code = 0;

    request.xfer.handle = handle;

    request.end_tag = 0;

    ioctl(fd, IOCTL_MBOX_PROPERTY, &request);
    return request.xfer.status;   //osk
}

void *mbox_map(uint32_t addr, unsigned int size)
{
    unsigned int offset = addr % PAGE_SIZE; // maps must align to 4096
    addr -= offset;
    size += offset;

    int fd;
    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
        perror_inf();
        perror("Failed to open \"/dev/mem\"");
		return NULL;
    }

    void *map = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, addr);
    if (map == MAP_FAILED) {
        perror_inf();
		perror("Failed mmaping peripheral");
        close(fd);
		return NULL;
	}

    close(fd);
    return (char *)map + offset;
}

void mbox_unmap(void *ptr, unsigned int size)
{
    intptr_t* int_ptr = (intptr_t*) ptr;    //osk - Not sure if this is what the code really wants but needed something to compile ptr-= offset line. 
    intptr_t  offset = (intptr_t ) int_ptr % PAGE_SIZE; //osk
    int_ptr -= offset; // <- ? //osk
    size += offset;

    if (munmap(ptr, size) == -1) {
        perror_inf();
		perror("Failed to munmap");
	}
}

uint32_t mbox_vc_execute(int fd, uint32_t func_ptr, uint32_t r0, uint32_t r1,
        uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5)
{
    struct request_execute {
        mbox_buffer_t buffer;
        uint32_t function;
        uint32_t r0;
        uint32_t r1;
        uint32_t r2;
        uint32_t r3;
        uint32_t r4;
        uint32_t r5;
        uint32_t end_tag;
    };

    struct request_execute request;
    request.buffer.size = sizeof(struct request_execute);
    request.buffer.req_code = 0;
    request.buffer.tag.id = 0x00030010;
    request.buffer.tag.data_size = 7 * sizeof(uint32_t);
    request.buffer.tag.req_code = 0;

    request.function = func_ptr; // bus address!
    request.r0 = r0;
    request.r1 = r1;
    request.r2 = r2;
    request.r3 = r3;
    request.r4 = r4;
    request.r5 = r5;

    request.end_tag = 0;

    ioctl(fd, IOCTL_MBOX_PROPERTY, &request);
    return request.function; // request returns r0, but in the place of function
}

uint32_t mbox_qpu_enable(int fd, uint32_t state)
{
    struct request_qpu_enable {
        mbox_buffer_t buffer;
        uint32_t state;
        uint32_t end_tag;
    };

    struct request_qpu_enable request;
    request.buffer.size = sizeof(struct request_qpu_enable);
    request.buffer.req_code = 0;
    request.buffer.tag.id = 0x00030012;
    request.buffer.tag.data_size = sizeof(uint32_t);
    request.buffer.tag.req_code = 0;

    request.state = state;

    request.end_tag = 0;

    ioctl(fd, IOCTL_MBOX_PROPERTY, &request);
    return request.state;
}

uint32_t mbox_qpu_execute(int fd, uint32_t num_qpus, uint32_t control,
    uint32_t noflush, uint32_t timeout)
{
    struct request_qpu_execute {
        mbox_buffer_t buffer;
        uint32_t num_qpus;
        uint32_t control;
        uint32_t noflush;
        uint32_t timeout;
        uint32_t end_tag;
    };

    struct request_qpu_execute request;
    request.buffer.size = sizeof(struct request_qpu_execute);
    request.buffer.req_code = 0;
    request.buffer.tag.id = 0x00030011;
    request.buffer.tag.data_size = 4 * sizeof(uint32_t);
    request.buffer.tag.req_code = 0;

    request.num_qpus = num_qpus;
    request.control = control;
    request.noflush = noflush;
    request.timeout = timeout;

    request.end_tag = 0;

    ioctl(fd, IOCTL_MBOX_PROPERTY, &request);
    return request.num_qpus;
}
