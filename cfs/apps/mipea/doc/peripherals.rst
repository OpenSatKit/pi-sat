.. index::
    single: Peripherals

***********
Peripherals
***********

The peripheral functions are something like the *core* of the library.
They map and unmap the memory used by all other parts.

Macros
======

.. macro:: PERIPHERAL_BASE_BCM2835

    ::

        0x20000000

    This macro holds the value of the peripheral base, when a BCM2835 is used.

.. macro:: PERIPHERAL_BASE_BCM2836_7

    ::

        0x3F000000

    This macro holds the value of the peripheral base, when a BCM2836 or
    BCM2837 is used.

Functions
=========

.. function:: int peripheral_map (volatile uint32_t **map, uint32_t offset, uint32_t size)

    This function maps a code memory block of size :code:`size` at offset
    :code:`offset` from the peripheral base.

    .. note:: The :code:`offset` must be a multiple of the page size which is \
        :code:`4096` on the Raspberry Pi.


    The function returns 0 on success and :code:`-1` on error.

.. function:: void peripheral_unmap (void* map, uint32_t size)

    This function unmaps the memory mapped to pointer :code:`map` with
    size :code:`size`.

.. function:: int peripheral_ismapped (void *map, uint32_t size)

    This function checks if a pointer :code:`map` is already mapped to a
    memory region with the size :code:`size`. It returns :code:`true` if
    the pointer is already mapped and :code:`false` if not.
