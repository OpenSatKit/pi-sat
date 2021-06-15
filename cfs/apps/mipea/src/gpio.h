/*
 * gpio.h
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

struct gpio_register_map {
	uint32_t FSEL[6];
	uint32_t: 32;
	uint32_t SET[2];
	uint32_t: 32;
	uint32_t CLR[2];
	uint32_t: 32;
	uint32_t LEV[2];
	uint32_t: 32;
	uint32_t EDS[2];
	uint32_t: 32;
	uint32_t REN[2];
	uint32_t: 32;
	uint32_t FEN[2];
	uint32_t: 32;
	uint32_t HEN[2];
	uint32_t: 32;
	uint32_t LEN[2];
	uint32_t: 32;
	uint32_t AREN[2];
	uint32_t: 32;
	uint32_t AFEN[2];
	uint32_t: 32; // 0x90
	uint32_t PUD;
	uint32_t PUDCLK[2];
// BCM2711 only
	uint32_t: 32; // 0xa0
	uint32_t: 32;
	uint32_t: 32;
	uint32_t: 32;
	uint32_t: 32; // 0xb0
	uint32_t: 32;
	uint32_t: 32;
	uint32_t: 32;
	uint32_t: 32; // 0xc0
	uint32_t: 32;
	uint32_t: 32;
	uint32_t: 32;
	uint32_t: 32; // 0xd0
	uint32_t: 32;
	uint32_t: 32;
	uint32_t: 32;
	uint32_t: 32; // 0xe0
	uint32_t PUPPDN[4];
};

enum {
	INPUT, OUTPUT, ALT0, ALT1, ALT2, ALT3, ALT4, ALT5
};
enum {
	PUD_DISABLE, PUD_DOWN, PUD_UP
};

extern volatile struct gpio_register_map *GP;

int 	gpio_map(void);
void 	gpio_unmap(void);

void gpio_func(uint32_t pin, int function);

extern void 	gpio_set(uint32_t pin);
extern void 	gpio_clr(uint32_t pin);
extern uint32_t gpio_tst(uint32_t pin);

void gpio_pud(uint32_t pin, int pud);

void gpio_inp(uint32_t pin);
void gpio_out(uint32_t pin);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_GPIO_H_
