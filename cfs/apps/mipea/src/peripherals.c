/*
 * peripherals.c
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "peripherals.h"
#include "../config.h"

#if defined(BCM2835)
	static const size_t PERIPHERAL_BASE = 0x20000000;
#elif defined(BCM2836_7)
	static const size_t PERIPHERAL_BASE = 0x3F000000;
#elif defined(BCM2711)
	static const size_t PERIPHERAL_BASE = 0xFE000000;
#else
	#error "No SoC specified"
#endif//BCM2xxx

#define perror_inf()	fprintf(stderr, "%s:%d: In function %s:\n", __FILE__,  \
	__LINE__, __func__)

int peripheral_map(volatile uint32_t **map, uint32_t offset, uint32_t size)
{
	printf("osk: peripheral_map() - Peripheral Base 0x%08lX, Offset 0x%08X, Size 0x%04X\n", PERIPHERAL_BASE, offset, size);
	if (peripheral_ismapped(*map, size)) {
		printf("osk: peripheral_map() - Already mapped\n");
		return 0; // already mapped
	}

	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		perror_inf();
		perror("Failed to open /dev/mem");
		return -1;
	}

	*map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		PERIPHERAL_BASE + offset);

	close(fd);

	if (*map == MAP_FAILED) {
		perror_inf();
		perror("Failed mmaping peripheral");
		return -1;
	}

	return 0;
}

void peripheral_unmap(volatile uint32_t *map, uint32_t size)
{
	if (peripheral_ismapped(map, size)) {
		if (munmap((void *)map, size) < 0) {
			perror_inf();
			perror("Failed munmapping peripheral");
		}
	}
}

int peripheral_ismapped(volatile uint32_t *map, uint32_t size)
{
	if (map == NULL)
		return 0;
	if (msync((void *)map, size, 0) < 0)
		return 0;
	return 1;
}
