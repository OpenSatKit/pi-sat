/*
 * pwm_example.c
 *
 * Copyright (C) 2019 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

/*
 * To compile use:
 * gcc -o pwm_example pwm_example.c -lmipea
 */

/*
 * Although you can drive one or two servos using this library and PWM it is now
 * possible to drive more. You could of course use `usleep()` but this is
 * not recommended, as it is not very accurate. Instead you could use my
 * mipea_servo library (https://github.com/jasLogic/mipea_servo)
 * which makes use of DMA and PWM to archieve a **very** accurate pulse
 * without CPU usage.
 */

#include <stdio.h>
#include <unistd.h>

#include <mipea/gpio.h>
#include <mipea/pwm.h>

int main(void)
{
    if (gpio_map() < 0 || pwm_map() < 0) { // Map peripherals
        return 1; // return if mapping fails
    }

    int servo = 18; // servo signal on pin 18
    gpio_func(servo, ALT5); // pin 18 uses alternate function 5 for pwm

    pwm_channel_config ch = {
        {{  /*
             * these strange double braces are not needed,
             * but else the compiler gives a warning because of
             * the way 'pwm_channel_config' is implemented
             */
            PWM_CTL_MODE_PWM,   // use pwm mode
            PWM_RPTL_STOP,      // if serial mode: repeat the last data
            PWM_SBIT_LOW,       // polarity when no transmission is active
            PWM_POLA_DEFAULT,   // do not invert the output
            PWM_USEF_DATA,      // if serial mode: use fifo?
            PWM_MSEN_MSRATIO,   /*
                                 * pwm algorithm: distribute the pwm as
                                 * even as possible, not useful for
                                 * servo control
                                 */
        }},
        1953,   // clock divisor
        5120    // the range of the counter
    };

    /*
     * Explanation of clock divisor and range value:
     *  - the input clock of the pwm is 500MHz
     *  - we need 50Hz
     *  -> 500Mhz / (range * divisor) = 50Hz
     *
     *  - we want to have exactly one byte accuracy for the servo
     *  -> 50Hz => 20ms
     *  -> one step is exactly (20ms / range) long
     *  - pwm is between 1ms and 2ms pulselength
     *  - so we have delta = 1ms of time to cover
     *  - we want to cover 1ms with 256 steps
     *  - becuase our pulse is always minimum of 1ms (maximum of 2ms)
     *    we also want 256 steps in this 1ms
     *  -> (20ms / range) * 256 = 0.001ms or (20ms / range) * 2 * 256 = 0.002ms
     *  => solves to: range = 5120
     *
     *  - putting this value into the function at the top
     *  -> 100Mhz / (5120 * divisor) = 50Hz
     *  => solves to: divisor = 1953.125
     */

    pwm_configure(PWM_CHANNEL0, &ch);
    pwm_enable(PWM_CHANNEL0);

    int going_up = 1;
    unsigned char num = 0;

    int i;
    for (i = 0; i < 10;) {
        if (going_up) {
            if (num == 255) {
                going_up = 0;
                ++i;
            } else {
                ++num;
            }
        } else {
            if (num == 0) {
                going_up = 1;
            } else {
                --num;
            }
        }
        DAT_CHANNEL0 = num + 256;
        usleep(10000);
    }

    gpio_unmap();
    pwm_unmap();

    return 0;
}
