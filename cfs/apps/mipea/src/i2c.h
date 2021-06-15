/*
 * i2c.h
 *
 * Copyright (C) 2019 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#ifndef _I2C_H_
#define _I2C_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

struct i2c_register_map {
    uint32_t C;
    uint32_t S;
    uint32_t DLEN;
    uint32_t A;
    uint32_t FIFO;
    uint32_t DIV;
    uint32_t DEL;
    uint32_t CLKT;
};

extern volatile struct i2c_register_map *I2C;

int     i2c_map(void);
void    i2c_unmap(void);

void i2c_set_address(uint8_t addr);
void i2c_set_clkdiv(uint16_t divisor);
void i2c_set_clkstr(uint16_t clkstr);

void i2c_start(void);
void i2c_stop(void);

extern void     i2c_write_byte(uint8_t byte);
extern uint8_t  i2c_read_byte(void);

extern void     i2c_write_data(const uint8_t *data, uint16_t length);
extern void     i2c_read_data(uint8_t *data, uint16_t length);

extern void     i2c_write_register(uint8_t reg, uint8_t data);
extern uint8_t  i2c_read_register(uint8_t reg);

/******** useful values ********/
enum {I2C_FIFO_SIZE = 16};
enum {
    I2C_C_I2CEN = 0x8000,
    I2C_C_ST = 0x80,
    I2C_C_CLEAR = 0x10,
    I2C_C_READ = 0x01
};
enum {
    I2C_S_RXS = 0x20,
    I2C_S_TXD = 0x10,
    I2C_S_DONE = 0x02
};

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_I2C_H_
