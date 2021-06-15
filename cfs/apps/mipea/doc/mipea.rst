.. index::
    single: Mipea Wrapper

*************
Mipea Wrapper
*************

The mipea.c / h files are just a wrapper for all the other parts of the library.
If you are lazy (or need all peripherals mapped) than this wrapper is usefull.

Functions
=========

.. function:: int mipea_map(void)

    This function maps all the peripherals and returns :code:`-1` on error.

.. function:: void mipea_unmap(void)

    This function unmaps all the peripherals.

.. _Datasheet: https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf
