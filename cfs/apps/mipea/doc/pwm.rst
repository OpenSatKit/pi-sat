.. index::
    single: PWM

***
PWM
***

.. note:: The Datasheet_ specifies PWM channels 0 and 1. The Raspberry Pi has \
    pins for PWM channels 1 and 2, you just need to add one.

Macros
======

.. macro:: RNG_CHANNEL0
.. macro:: DAT_CHANNEL0
.. macro:: RNG_CHANNEL1
.. macro:: DAT_CHANNEL1

    ::

        #define RNG_CHANNEL0    PWM->RNG1
        #define DAT_CHANNEL0    PWM->DAT1
        #define RNG_CHANNEL1    PWM->RNG2
        #define DAT_CHANNEL1    PWM->DAT2

    To prevent confusion (because the Datasheet_ calls the PWM channels 1 and 2
    and the Raspberry Pi 0 and 1) the values of the registers which need to be
    used "on the fly" are :code;`defined` from 2 to 1 and from 1 to 0.

Registers
=========

.. type:: struct pwm_register_map

    This struct maps the registers of the PWM.
    The names of the struct members correspond to the registers
    from the Datasheet_::

        struct pwm_register_map {
            uint32_t CTL;
            uint32_t STA;
            uint32_t DMAC;
            uint32_t: 32;
            uint32_t RNG1;
            uint32_t DAT1;
            uint32_t FIF1;
            uint32_t: 32;
            uint32_t RNG2;
            uint32_t DAT2;
        };

.. var:: extern volatile struct pwm_register_map *PWM

    ::

        PWM = (volatile struct pwm_register_map *)pwm_base_ptr;

    By using this variable, the registers of the PWM can be accessed like this
    :code:`PWM->RNG1`.

Enums
=====

PWM channel number
------------------

    This enum holds the values distinguishing PWM channel 0 and 1::

        enum {
            PWM_CHANNEL0, PWM_CHANNEL1
        };

Structs
=======

.. type:: pwm_channel_config

    This struct is used to configure a PWM channel::

        typedef struct {
            union {
                struct {
                    uint32_t: 1;
                    uint32_t mode: 1;
                    uint32_t rptl: 1;
                    uint32_t sbit: 1;
                    uint32_t pola: 1;
                    uint32_t usef: 1;
                    uint32_t: 1;
                    uint32_t msen: 1;
                };
                uint32_t ctl_register;
            };
            unsigned int divisor;
            uint32_t range;
        } pwm_channel_config;

    .. member:: uint32_t ctl_register

        This member can be directly edited by the anonymous struct inside
        this union. This register maps directly to the :code:`CTL` register,
        with some offset for PWM 1. The settings of this register are described
        in the `Macros`_.

    .. member:: unsigned int divisor

        The divisor which is passed to the :doc:`clock_manager`.

    .. member:: uint32_t range

        The range to which the PWM counter counts before it starts over.

Functions
=========

.. function:: int pwm_map(void)

    This function maps the PWM registers. It calls :func:`peripheral_map` with
    the values :macro:`PWM_OFFSET` and :macro:`PWM_SIZE`. On error
    :code:`-1` is returned.

.. function:: void pwm_unmap(void)

    This function unmaps the PWM registers.

.. function:: void pwm_configure(int channel, pwm_channel_config *config)

    This function configures :code:`channel` with a
    :type:`pwm_channel_config` pointed to by :code:`config`.

.. function:: void pwm_enable(int channel)

    This function enables :code:`channel`.

.. function:: void pwm_disable(int channel)

    This function disables :code:`channnel`.

Configuration Values
--------------------

=============================== ====================================
:data:`PWM_CTL_MODE_PWM`        Use PWM mode
:data:`PWM_CTL_MODE_SERIALISER` Use serialiser mode
:data:`PWM_RPTL_STOP`           If serialiser mode: Transmission stops when fifo empty
:data:`PWM_RPTL_REPEAT`         If serialiser mode: Repeat last data when fifo empty
:data:`PWM_SBIT_LOW`            Output low when no transmission active
:data:`PWM_SBIT_HIGH`           Output high when no transmission active
:data:`PWM_POLA_DEFAULT`        Polarity is default
:data:`PWM_POLA_INVERTED`       Polarity is innverted
:data:`PWM_USEF_DATA`           Data register is transmitted
:data:`PWM_USEF_FIFO`           Data from fifo is transmitted
:data:`PWM_MSEN_PWMALGORITHM`   Use PWM algorithm
:data:`PWM_MSEN_MSRATIO`        Use MS ratio
=============================== ====================================

.. _Datasheet: https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf
.. _this file: https://github.com/bootc/linux/blob/073993b3f3e23fb8d376f9e159eee410968e0c57/arch/arm/mach-bcm2708/bcm2708.c#L208
