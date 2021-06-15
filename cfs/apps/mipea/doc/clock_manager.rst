.. index::
    single: Clock Manager

*************
Clock Manager
*************

Registers
=========

.. type:: struct clock_manager_register_map

    This struct maps the registers of the clock manager.
    The names of the struct members correspond to the registers. Unfortunately,
    the official datasheet does not feature this chapter. But there is an
    upload of this chapter here: `BCM2835 clocks`_::

        struct clock_manager_register_map {
            uint32_t GP0CTL;
            uint32_t GP0DIV;
            uint32_t GP1CTL;
            uint32_t GP1DIV;
            uint32_t GP2CTL;
            uint32_t GP2DIV;
            uint32_t: 32;
            uint32_t: 32;
            uint32_t: 32;
            uint32_t: 32;
            uint32_t PCMCTL;
            uint32_t PCMDIV;
            uint32_t PWMCTL;
            uint32_t PWMDIV;
        }

.. var:: extern volatile struct clock_manager_register_map *CM

    ::

        CM = (volatile struct clock_manager_register_map *)(clock_manager_base_ptr + 28);

    By using this variable, the registers of the clock manager can be accessed
    like this :code:`CM->PWMCTL`.

Enums
=====

Clock sources
-------------

    This enum holds the values for the different clock sources::

        enum {
            CLOCK_GND,
            CLOCK_OSC,
            CLOCK_TST0,
            CLOCK_TST1,
            CLOCK_PLLA,
            CLOCK_PLLC,
            CLOCK_PLLD,
            CLOCK_HDMI
        };

Global Variables
================

.. var:: extern const uint32_t CM_PASSWD;

    ::

        const uint32_t CM_PASSWD = 0x5A000000;

    This variable holds the clock manager password. This value must always be
    present when writing to a clock manager register
    (e.g. by OR with the value).

Functions
=========

.. function:: int clock_map(void)

    This function maps the clock manager registers.
    It calls :func:`peripheral_map` with the values
    :macro:`CLOCK_MANAGER_OFFSET` and :macro:`CLOCK_MANAGER_SIZE`. On error
    :code:`-1` is returned.

.. function:: void clock_unmap(void)

    This function unmaps the clock manager.

The following functions all take a pointer to a clock manager register as an
argument because all the registers for the *different clocks* have the
*same structure*. This means that you just need to tell the clock manager which
clock to use (by pointing to the right register). For example:
:code:`clock_enable(&CM->PWMCTL);`

.. function:: void clock_enable(volatile uint32_t *reg)

    This function enables the clock with the register pointed to by :code:`reg`.

.. function:: void clock_disable(volatile uint32_t *reg)

    This function disables the clock with the register pointed to by :code:`reg`.

.. function:: void clock_configure(volatile uint32_t *reg, clock_source_t src, \
    unsigned int divisor, unsigned int mash)

    This function configures the clock with the register pointed to by
    :code:`reg` and sets up the :type:`clock_source_t` :code:`src`, the divisor
    :code:`divisor` with the mash factor :code:`mash`.

.. todo:: Add a decimal places to the divisor.

.. _BCM2835 clocks: https://de.scribd.com/doc/127599939/BCM2835-Audio-clocks
