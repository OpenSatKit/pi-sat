/*
 * pwm.h
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#ifndef _PWM_H_
#define _PWM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

struct pwm_register_map {
    uint32_t CTL;
    uint32_t STA;
    uint32_t DMAC;
    uint32_t: 32;
    uint32_t RNG1;
    uint32_t DAT1;
    uint32_t FIF1;
    uint32_t: 32;
    uint32_t RNG2;
    uint32_t DAT2;
};

#define RNG_CHANNEL0    PWM->RNG1
#define DAT_CHANNEL0    PWM->DAT1
#define RNG_CHANNEL1    PWM->RNG2
#define DAT_CHANNEL1    PWM->DAT2

enum {
    PWM_CHANNEL0, PWM_CHANNEL1
};

typedef struct {
    union {
        struct {
            uint32_t: 1;        // use pwm_enable / pwm_disable
            uint32_t mode: 1;
            uint32_t rptl: 1;
            uint32_t sbit: 1;
            uint32_t pola: 1;
            uint32_t usef: 1;
            uint32_t: 1;
            uint32_t msen: 1;
        } pwm_bitfield;  //osk
        uint32_t ctl_register;
    } pwm_register;  //osk
    unsigned int divisor;
    uint32_t range;
} pwm_channel_config;

extern volatile struct pwm_register_map *PWM;

int     pwm_map(void);
void    pwm_unmap(void);

void pwm_configure(int channel, pwm_channel_config *config);

void pwm_enable(int channel);
void pwm_disable(int channel);

/******* CTL Register bit values *******/
enum {
    PWM_CTL_MODE_PWM = 0x0,
    PWM_CTL_MODE_SERIALISER = 0x1
};
enum {
    PWM_RPTL_STOP = 0x0,
    PWM_RPTL_REPEAT = 0x1
};
enum {
    PWM_SBIT_LOW = 0x0,
    PWM_SBIT_HIGH = 0x1
};
enum {
    PWM_POLA_DEFAULT = 0x0,
    PWM_POLA_INVERTED = 0x1
};
enum {
    PWM_USEF_DATA = 0x0,
    PWM_USEF_FIFO = 0x1
};
enum {
    PWM_MSEN_PWMALGORITHM = 0x0,
    PWM_MSEN_MSRATIO = 0x1
};

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_PWM_H_
