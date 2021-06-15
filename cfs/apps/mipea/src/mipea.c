/*
 * mipea.c
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

#include "mipea.h"

int mipea_map(void)
{
    if (dma_map() < 0 || gpio_map() < 0 || i2c_map() < 0 || pwm_map() < 0
        || spi_map() < 0 || timer_map() < 0) {
        return -1;
    }
    return 0;
}

void mipea_unmap(void)
{
    dma_unmap();
    gpio_unmap();
    i2c_unmap();
    pwm_unmap();
    spi_unmap();
    timer_unmap();
}
