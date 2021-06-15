/*
 * clock_manager.h
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#ifndef _CLOCK_MANAGER_H_
#define _CLOCK_MANAGER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

struct clock_manager_register_map {
    uint32_t GP0CTL;
    uint32_t GP0DIV;
    uint32_t GP1CTL;
    uint32_t GP1DIV;
    uint32_t GP2CTL;
    uint32_t GP2DIV;
    uint32_t: 32;
    uint32_t: 32;
    uint32_t: 32;
    uint32_t: 32;
    uint32_t PCMCTL;
    uint32_t PCMDIV;
    uint32_t PWMCTL;
    uint32_t PWMDIV;
};

enum {
    CLOCK_GND,
    CLOCK_OSC,
    CLOCK_TST0,
    CLOCK_TST1,
    CLOCK_PLLA,
    CLOCK_PLLC,
    CLOCK_PLLD,
    CLOCK_HDMI
};

extern const uint32_t CM_PASSWD;

extern volatile struct clock_manager_register_map *CM;

int     clock_map(void);
void    clock_unmap(void);

void clock_enable(volatile uint32_t *reg);
void clock_disable(volatile uint32_t *reg);
void clock_configure(volatile uint32_t *reg, int src, unsigned int divisor,
    unsigned int mash);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_CLOCK_MANAGER_H_
