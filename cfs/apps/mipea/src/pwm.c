/*
 * pwm.c
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#include <stdint.h>
#include <stddef.h>

#include "pwm.h"
#include "peripherals.h"
#include "clock_manager.h"

static const size_t PWM_OFFSET = 0x20C000;
static const size_t PWM_SIZE = 0x24;

static volatile uint32_t *pwm_base_ptr = NULL;
volatile struct pwm_register_map *PWM = NULL;

int pwm_map(void)
{
    if (clock_map() < 0)
        return -1;
    if (peripheral_map(&pwm_base_ptr, PWM_OFFSET, PWM_SIZE) < 0)
        return -1;
    PWM = (volatile struct pwm_register_map *)pwm_base_ptr;
    return 0;
}

void pwm_unmap(void)
{
    pwm_disable(PWM_CHANNEL0);
    pwm_disable(PWM_CHANNEL1);
    clock_disable(&CM->PWMCTL);

    peripheral_unmap(pwm_base_ptr, PWM_SIZE);
    clock_unmap(); // unmap clock too
}

void pwm_enable(int channel)
{
    if (channel == PWM_CHANNEL0) {
        PWM->CTL |= 1;
    } else {
        PWM->CTL |= 0x100;
    }
}

void pwm_disable(int channel)
{
    if (channel == PWM_CHANNEL0) {
        PWM->CTL &= ~1;
    } else {
        PWM->CTL &= ~0x100;
    }
}

void pwm_configure(int channel, pwm_channel_config *config)
{
    clock_configure(&CM->PWMCTL, CLOCK_PLLD, config->divisor, 0);
    clock_enable(&CM->PWMCTL);

    if (channel == PWM_CHANNEL0) {
        PWM->CTL &= ~0xff; // clear all pwm0 bits
        PWM->CTL |= config->pwm_register.ctl_register;

        PWM->RNG1 = config->range;

    } else {
        PWM->CTL &= ~0xff00; // clear all pwm1 bits
        PWM->CTL |= config->pwm_register.ctl_register << 8;

        PWM->RNG2 = config->range;
    }
}
