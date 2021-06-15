/*
 * i2c.c
 *
 * Copyright (C) 2019 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#include <stdint.h>

#include "i2c.h"
#include "peripherals.h"
#include "../config.h" // for information on the i2c

#if defined(USE_I2C_BUS_0)
    static const size_t I2C_OFFSET = 0x205000;
#else
    static const size_t I2C_OFFSET = 0x804000;
#endif//USE_I2C_BUS_0

static const size_t I2C_SIZE = 0x18;

static volatile uint32_t *i2c_base_ptr = NULL;
volatile struct i2c_register_map *I2C = NULL;

int i2c_map(void)
{
    if (peripheral_map(&i2c_base_ptr, I2C_OFFSET, I2C_SIZE) < 0)
        return -1;
    I2C = (volatile struct i2c_register_map *)i2c_base_ptr;
    return 0;
}

void i2c_unmap(void)
{
    peripheral_unmap(i2c_base_ptr, I2C_SIZE);
}

void i2c_set_address(uint8_t addr)
{
    I2C->A = addr & 0x7f; // only 7 bit
}

void i2c_set_clkdiv(uint16_t divisor)
{
    I2C->DIV = divisor;
}

void i2c_set_clkstr(uint16_t clkstr)
{
    I2C->CLKT = clkstr;
}

void i2c_start(void)
{
    I2C->C |= I2C_C_I2CEN; // clear fifo, enable bsc controller
    I2C->S = 0x50; // reset flag register
}
void i2c_stop(void)
{
    I2C->C &= ~I2C_C_I2CEN; // disable bsc controller
}

inline void i2c_write_byte(uint8_t byte)
{
    I2C->DLEN = 1; // one byte transfer
    I2C->C |= I2C_C_CLEAR; // clear fifo
    I2C->C &= ~I2C_C_READ; // clear read bit -> write

    I2C->FIFO = byte;
    I2C->C |= I2C_C_ST; // start transfer

    while(!(I2C->S & I2C_S_DONE)); // wait for done
    I2C->S |= I2C_S_DONE; // clear done flag
}

inline uint8_t i2c_read_byte(void)
{
    I2C->DLEN = 1; // one byte transfer
    I2C->C |= I2C_C_READ | I2C_C_CLEAR; // set read bit, clear FIFO

    I2C->C |= I2C_C_ST; // start transfer
    while(!(I2C->S & I2C_S_DONE)); // wait for done
    I2C->S |= I2C_S_DONE; // clear done flag

    return I2C->FIFO;
}

inline void i2c_write_data(const uint8_t *data, uint16_t length)
{
    uint16_t i;

    I2C->DLEN = length;
    I2C->C |= I2C_C_CLEAR; // clear fifo
    I2C->C &= ~I2C_C_READ; // clear read bit -> write

    // fill fifo before starting transfer
    for (i = 0; i < length && i < I2C_FIFO_SIZE; ++i) {
        I2C->FIFO = data[i];
    }
    I2C->C |= I2C_C_ST; // start transfer
    while(i < length && I2C->S & I2C_S_TXD) { // write data to fifo while there is data and space
        I2C->FIFO = data[i++];
    }
    while(!(I2C->S & I2C_S_DONE)); // wait for done
    I2C->S |= I2C_S_DONE; // clear done flag
}

inline void i2c_read_data(uint8_t *data, uint16_t length)
{
    uint16_t i = 0;

    I2C->DLEN = length;
    I2C->C |= I2C_C_READ | I2C_C_CLEAR; // set read bit, clear FIFO

    I2C->C |= I2C_C_ST; // start transfer

    while(!(I2C->S & I2C_S_DONE)) { // while not done
        while(i < length && I2C->S & I2C_S_RXS) { // while data left and fifo not empty
            data[i++] = I2C->FIFO;
        }
    }
    while(i < length && I2C->S & I2C_S_RXS) { // remaining data in fifo
        data[i++] = I2C->FIFO;
    }
    I2C->S |= I2C_S_DONE; // clear done flag
}

/*
 * the difference between this function and 2 i2c_read_byte is that
 * this function is only ONE transmission with two data packets, while
 * 2 times i2c_read_byte makes two individual transmissions which might not
 * work on some ics
 */
inline void i2c_write_register(uint8_t reg, uint8_t data)
{
    uint8_t tf[2] = {reg, data};
    i2c_write_data(tf, 2);
}

/*
 * different to i2c_write_register, this function can be replaced by
 * one i2c_write_byte and i2c_read_byte
 */
inline uint8_t i2c_read_register(uint8_t reg)
{
    i2c_write_byte(reg);
    return i2c_read_byte();
}
