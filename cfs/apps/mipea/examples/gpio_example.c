/*
 * gpio_example.c
 *
 * Copyright (C) 2019 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

 /*
  * To compile use:
  * gcc -o gpio_example gpio_example.c -lmipea
  */

#include <unistd.h>
#include <stdio.h>

#include <mipea/gpio.h>

int main(void)
{
    if (gpio_map() < 0) { // map peripherals
        return 1; // return if mapping fails
    }

    int out_pin = 26; // LED on pin 26
    int in_pin = 19; // Button wired to GND on pin 19

    gpio_out(out_pin); // Make gpio 26 an output

    gpio_inp(in_pin); // Make gpio 19 an input
    gpio_pud(in_pin, PUD_UP); // Enable the internal pullup on pin 19

    while(gpio_tst(in_pin) != 0) { // While pin 19 is high
        gpio_set(out_pin); // Set pin 26
        sleep(1);
        gpio_clr(out_pin); // Clear pin 26
        sleep(1);
    }

    gpio_unmap();
    return 0;
}
