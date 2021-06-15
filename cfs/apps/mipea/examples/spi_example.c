/*
 * spi_example.c
 *
 * Copyright (C) 2019 Jaslo Ziska
 * All rights reserved.
 *
 * This source code is licensed under BSD 3-Clause License.
 * A copy of this license can be found in the LICENSE.txt file.
 */

/*
 * This program uses SPI to communicate with the MCP3002 ADC
 * To compile use:
 * gcc -o spi_example -Wall spi_example.c -lmipea
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <mipea/gpio.h>
#include <mipea/spi.h>

union spi_mcp3002_transfer {
    struct {
        uint8_t: 4; // Don't care
        uint8_t odd: 1;  // ODD/!SGN bit
        uint8_t sgl: 1;  // SGL/!DIFF bit
        uint8_t lst: 2;  // Leading zero and starting bit
    };
    uint8_t val;
};

int main(void)
{
    if (gpio_map() < 0 || spi_map() < 0) { // map peripherals
        return 1; // return if mapping fails
    }

    uint32_t sclk = 11;    // connected to CLK
	uint32_t mosi = 10;    // connected to D_in
	uint32_t miso = 9;     // connected to D_out
	uint32_t ce0 = 8;      // connected to !CS/SHDN

    // tell the GPIOs to use SPI on them
	gpio_func(sclk, ALT0);
	gpio_func(mosi, ALT0);
	gpio_func(miso, ALT0);
	gpio_func(ce0, ALT0);

	spi_channel_config conf = {
        {{  /*
             * these strange double braces are not needed,
             * but else the compiler gives a warning
             * because of the way 'spi_channel_config' is implemented
             */
            SPI_CPHA_CLK_BEGINNING, // data on clock leading or trailing edge
            SPI_CPOL_RESET_LOW,     // clock polarity: rest state low or high
            SPI_CSPOL_ACTIVE_LOW,   /*
                                     * chip select polarity: active low or high
                                     * -> I tested this on CE0 and CE1 and
                                     * it did NOT have any effect on
                                     * the polarity of the chip select.
                                     * But I would set this bit anyway just to
                                     * be save.
                                     */
            SPI_CSPOL_ACTIVE_LOW,    // CE0 polarity: active low or high
            SPI_CSPOL_ACTIVE_LOW,    // CE1 similar to CE0
            SPI_CSPOL_ACTIVE_LOW,    // CE2 similar to CE0
        }},
        1000    /*
                 * clock divisor: the RaspberryPi Zero runs at 1GHz and the
                 * MCP3002 has a maximum clock frequency of 1Mhz
                 * => when looking at the signal with a logic analyser it
                 * appears that the clock is only running at 250Mhz although
                 * cpuinfo_cur_freq returns 1000000 ?
                 */
	};

	spi_configure(&conf);
    spi_set_ce(SPI_CS_CE0); // set which chip enable line to use

    union spi_mcp3002_transfer t;
    t.lst = 0b01; // Leading zero and starting bit
    t.sgl = 1;
    t.odd = 0;

    uint16_t adc = 0x300;   /*
                             * Because only the last 2 bit of the first
                             * transmission are important we set adc to
                             * 0b000000110000000 and AND the first result
                             * to it. If there are some errors in the
                             * first 6 bits we do not have errors in the result.
                             */

    while(adc > 1) {
        adc = 0x300;
        spi_transfer_start();           // Start a new transfer -> CS */
        adc &= spi_transfer_byte(t.val) << 8; // Transfer the first byte
                                              // and return the MS 2 bits
        adc |= spi_transfer_byte(0); // Send don't care and recive the last byte
        spi_transfer_stop();

        printf("ADC: %d\n", adc);
        usleep(100000);
    }

    gpio_unmap();
    spi_unmap();

    return 0;
}
