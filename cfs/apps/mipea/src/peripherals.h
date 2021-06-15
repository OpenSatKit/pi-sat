/*
 * peripherals.h
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

int peripheral_map(volatile uint32_t **map, uint32_t offset, uint32_t size);
void peripheral_unmap(volatile uint32_t *map, uint32_t size);

int peripheral_ismapped(volatile uint32_t *map, uint32_t size);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_PERIPHERALS_H_
