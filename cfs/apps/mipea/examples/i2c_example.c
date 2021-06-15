/*
 * i2c_example.c
 *
 * Copyright (C) 2019 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

/*
 * This program uses I2C to communicate with the L3GD20H gyro
 * To compile use:
 * gcc -o i2c_example -Wall i2c_example.c -lmipea
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <mipea/gpio.h>
#include <mipea/i2c.h>

int main(void)
{
    if (gpio_map() < 0 || i2c_map() < 0) { // map peripherals
        return 1; // return if mapping fails
    }

    uint32_t sda = 2;
    uint32_t scl = 3;

    // tell the GPIOs to use I2C on them
    gpio_func(sda, ALT0);
    gpio_func(scl, ALT0);

    i2c_set_address(0x6a); // set address
    i2c_set_clkdiv(4000); // clock divider -> clk must be < 100kHz for gyro
    i2c_set_clkstr(0x40); // can often be ignored and left on reset value

    i2c_start(); // start i2c

    i2c_write_register(0x20, 0x0F); // write to cntrl1 register and turn on power

    while(1) {
        int data_length = 6;
        int8_t data[data_length]; // data from gyro is signed

        i2c_write_byte(0x28 | 0x80); /* we want to read from address 0x28
                                        0x80 for consecutive reads */
        i2c_read_data((uint8_t *)data, data_length);

        printf("X: %d\nY: %d\nZ: %d\n\n", data[0] | (data[1] << 8),
            data[2] | (data[3] << 8), data[4] | (data[5] << 8));

        usleep(100000);
    }

    i2c_stop();

    gpio_unmap();
    i2c_unmap();
}
