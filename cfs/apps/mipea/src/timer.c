/*
 * timer.c
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#include <stdint.h>
#include <stddef.h>

#include "timer.h"
#include "peripherals.h"
#include "../config.h" // for inline

static const size_t TIMER_OFFSET = 0x003000;
static const size_t TIMER_SIZE = 0x1C;

static volatile uint32_t *timer_base_ptr = NULL;

volatile struct timer_register_map *TMR = NULL;

int timer_map(void)
{
    if (peripheral_map(&timer_base_ptr, TIMER_OFFSET, TIMER_SIZE) < 0)
        return -1;
    TMR = (volatile struct timer_register_map *)timer_base_ptr;
    return 0;
}

void timer_unmap(void)
{
    peripheral_unmap(timer_base_ptr, TIMER_SIZE);
}

inline void timer_read(uint64_t *counter)
{
    *counter = TMR->CHI;
    *counter = *counter << 32;
    *counter |= TMR->CLO;
}
