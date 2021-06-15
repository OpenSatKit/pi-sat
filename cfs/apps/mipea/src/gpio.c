/*
 * gpio.c
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#include <stdint.h>
#include <stddef.h>

#include "gpio.h"
#include "peripherals.h"
#include "../config.h" // for inline and BCM235/6/7 or BCM2711

static const size_t GPIO_OFFSET = 0x200000;

#if defined(BCM235) || defined(BCM2836_7)
	static const size_t GPIO_SIZE = 0xA0;
#else
	static const size_t GPIO_SIZE = 0xF4;
#endif//BCM2xxx

static volatile uint32_t *gpio_base_ptr = NULL;
volatile struct gpio_register_map *GP = NULL;

static inline void delay_cycles(unsigned int n)
{
	volatile unsigned int i; // do not optimize delay loop
	for (i = 0; i < n; ++i);
}

int gpio_map(void)
{
	if (peripheral_map(&gpio_base_ptr, GPIO_OFFSET, GPIO_SIZE) < 0)
		return -1;
	GP = (volatile struct gpio_register_map *)gpio_base_ptr;
	return 0;
}

void gpio_unmap(void)
{
	peripheral_unmap(gpio_base_ptr, GPIO_SIZE);
}

void gpio_func(uint32_t pin, int function)
{
	GP->FSEL[pin / 10] &= ~(7 << ((pin % 10) * 3)); // clear the 3 bits

	switch(function) {
		case INPUT:
			break;
		case OUTPUT:
			GP->FSEL[pin / 10] |= (0x01 << ((pin % 10) * 3));  //osk 0b001
			break;
		case ALT0:
			GP->FSEL[pin / 10] |= (0x04 << ((pin % 10) * 3));  //osk 0b100
			break;
		case ALT1:
			GP->FSEL[pin / 10] |= (0x05 << ((pin % 10) * 3));  //osk 0b101
			break;
		case ALT2:
			GP->FSEL[pin / 10] |= (0x06 << ((pin % 10) * 3));  //osk 0b110
			break;
		case ALT3:
			GP->FSEL[pin / 10] |= (0x07 << ((pin % 10) * 3));  //osk 0b111
			break;
		case ALT4:
			GP->FSEL[pin / 10] |= (0x03 << ((pin % 10) * 3));  //osk 0b011 
			break;
		case ALT5:
			GP->FSEL[pin / 10] |= (0x02 << ((pin % 10) * 3));  //osk 0b010
			break;
		default:
			break;
	}
}

inline void gpio_set(uint32_t pin)
{
	GP->SET[pin >> 5] = (1 << (pin & 0x1f));
}

inline void gpio_clr(uint32_t pin)
{
	GP->CLR[pin >> 5] = (1 << (pin & 0x1f));
}

inline uint32_t gpio_tst(uint32_t pin)
{
	return GP->LEV[pin >> 5] & (1 << (pin & 0x1f));
}

void gpio_pud(uint32_t pin, int pud)
{
#if defined(BCM235) || defined(BCM2836_7)
	GP->PUD = pud;
	delay_cycles(150);
	GP->PUDCLK[pin >> 5] = (1 << (pin & 0x1f));
	delay_cycles(150);
	GP->PUD = 0;
	GP->PUDCLK[pin >> 5] = 0;
#else
	// BCM2711 has PUD_UP = 1 and PUD_DOWN = 2 (other way around)
	if (pud == PUD_UP)
		pud = 1;
	else if (pud == PUD_DOWN)
		pud = 2;

	GP->PUPPDN[pin >> 4] &= ~(7 << ((pin & 0xf) << 1));
	GP->PUPPDN[pin >> 4] |= pud << ((pin & 0xf) << 1);
#endif//BCM2xxx
}


void gpio_inp(uint32_t pin)
{
	GP->FSEL[pin / 10] &= ~(7 << ((pin % 10) * 3));
}

void gpio_out(uint32_t pin)
{
	GP->FSEL[pin / 10] &= ~(7 << ((pin % 10) * 3)); // clear the 3 bits first
	GP->FSEL[pin / 10] |= (1 << ((pin % 10) * 3));
}
