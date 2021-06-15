.. index::
    single: Installation

************
Installation
************

The source code is hosted on GitHub_. mipea uses autotools (autoconf,
automake, libtool) to build and install the library. The git repository
does *not* include the :code:`configure` script and :code:`Makefile.in` which
means that you have two options for installing the library.

Download the configure script
=============================

On GitHub_, when you look under the tab releases_
you will find some release with a name like for example "v2.0.0".
Then you can download the corresponding file named "mipea_x.x.x"
which *includes the configure script and Makefile.in*.
After downloading just run these commands from inside the downloaded directory::

    $ ./configure
    $ make
    $ sudo make install

Building the configure script
=============================

When you have the GNU autotools installed you can simply clone this repository
and build the :code:`configure` script and :code:`Makefile.in` yourself::

    $ git clone https://github.com/jasLogic/mipea.git
    $ cd mipea
    $ autoreconf --install
    $ ./configure
    $ make
    $ sudo make install

.. index::
    single: Troubleshooting

Troubleshooting
===============

Configure script can not find /proc/cpuinfo
-------------------------------------------

If the :code:`configure` script prints this warning::

    configure: WARNING: cannot find file /proc/cpuinfo

than the script was unable to find the :code:`cpuinfo` file which is needed
to determine the SoC (BCM2835 or BCM2836/7) and the revision. Pis with a
revision number where the last four digits are less than :code:`0004`
use I2C bus 0 instead of 1, like the new ones.

This error can be fixed by editing the :code:`config.h` file ensuring that it
contains these lines (depending on your Pi)::

    #define BCM2835         1
    #define BCM2836_7       1
    #define USE_I2C_BUS_0   1

When running a program, the shared library file is not found
------------------------------------------------------------

I noticed that sometimes the library can be linked, but when running a program
an error message appears saying: :code:`File or directory not found`.
If you havethis problem just run :code:`ldconfig`
or follow the output from :code:`sudo make install`::

    ----------------------------------------------------------------------
    Libraries have been installed in:
       /usr/local/lib

    If you ever happen to want to link against installed libraries
    in a given directory, LIBDIR, you must either use libtool, and
    specify the full pathname of the library, or use the `-LLIBDIR'
    flag during linking and do at least one of the following:
       - add LIBDIR to the `LD_LIBRARY_PATH' environment variable
         during execution
       - add LIBDIR to the `LD_RUN_PATH' environment variable
         during linking
       - use the `-Wl,-rpath -Wl,LIBDIR' linker flag
       - have your system administrator add LIBDIR to `/etc/ld.so.conf'

    See any operating system documentation about shared libraries for
    more information, such as the ld(1) and ld.so(8) manual pages.
    ----------------------------------------------------------------------

Wifi stops working when using the library
-----------------------------------------

In versions 2.1.0 and below the GPIO map and unmap functions cleared *all*
pullup / -downs on all pins. This could lead to the wifi not working until a
reboot. This issue should be fixed with version 2.1.1.

.. _GitHub: https://github.com/jasLogic/mipea
.. _releases: https://github.com/jasLogic/mipea/releases
