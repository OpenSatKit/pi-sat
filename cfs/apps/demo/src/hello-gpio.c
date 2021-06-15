#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include "cfe.h"

#include "gpio.h"

int gpio_loop(void)
{

    OS_printf("hello-gpio.c entry point.\n:");

    if (gpio_map() < 0) { // map peripherals
        OS_printf("WARNING: Could not perform gpio_map().\n");
        return 1;
    }

    int out_pin = 18;  // GPIO 18
    //int out_pin = 16;  // GPIO 23
    //int out_pin = 26;
    gpio_out(out_pin);

    while(true) {
        gpio_set(out_pin);
        OS_printf("GPIO on\n");
        usleep(3 * 1000 * 1000);
        gpio_clr(out_pin);
        OS_printf("GPIO off\n");
        usleep(3 * 1000 * 1000);
    }

}
