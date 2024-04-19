Rollo CUPS Printer Driver
=========================

This repository contains a CUPS printer driver for the [Rollo X1038][X1038]
label printer.


Prerequisites
-------------

You need the following software to build this driver:

- A C99 compiler (GCC and Clang both work)
- The CUPS 2.x libraries and headers

CentOS 8/Fedora 23+/RHEL 8:

    sudo dnf groupinstall 'Development Tools'
    sudo dnf install cups-devel

Debian/Raspbian/Ubuntu:

    sudo apt-get install build-essential libcups2-dev libcupsimage2-dev


Building the Driver
-------------------

A "configure" script and makefile are used to build the driver:

    ./configure
    make

The configure script will look for both the "pkg-config" and "cups-config"
commands to learn the appropriate options for building a driver for the local
version of CUPS and where to install the PPD file and driver filter.  The
following additional (developer) options are supported:

- `--enable-debug`: Build a debugging version of the driver.
- `--enable-maintainer`: Build in "maintainer" mode where warnings are errors.
- `--with-sanitizer=address`: Build with the AddressSanitizer enabled.
- `--with-sanitizer=leak`: Build with the LeakSanitizer enabled.
- `--with-sanitizer=memory`: Build with the MemorySanitizer enabled.
- `--with-sanitizer=undefined`: Build with the UndefinedSanitizer enabled.

Once built you have built the driver you can install it with:

    sudo make install


Legal Stuff
-----------

This code was produced under contract by [Lakeside Robotics Corporation][LRC]
and is Copyright © 2018-2024 by Nelu LLC.

This program is free software: you can redistribute it and/or modify it under
the terms of version 3 of the GNU General Public License as published by the
Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <https://www.gnu.org/licenses/>.


[LRC]: https://www.lakesiderobotics.ca
[X1038]: https://www.rollo.com/intro-print/
