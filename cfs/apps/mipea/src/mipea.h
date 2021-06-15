/*
 * mipea.h
 *
 * Copyright (C) 2018 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

 #ifndef _MIPEA_H_
 #define _MIPEA_H_

#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "pwm.h"
#include "spi.h"
#include "timer.h"

 #ifdef __cplusplus
 extern "C" {
 #endif//__cplusplus

int     mipea_map(void);
void    mipea_unmap(void);

 #ifdef __cplusplus
 }
 #endif//__cplusplus

 #endif//MIPEA_H_
