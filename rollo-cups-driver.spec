#
# RPM "spec" file for the Rollo CUPS Driver.
#
# Copyright © 2024 by Nelu LLC.
#
# This program is free software.  Distribution and use rights are outlined in
# the file "COPYING".
#

Summary: Rollo CUPS Driver
Name: rollo-cups-driver
Version: 1.8.3
Release: 0
License: GPL
Group: System Environment/Daemons
Source: https://github.com/nelullc/rollo-cups-driver/releases/download/v%{version}/rollo-cups-driver-%{version}.tar.gz
Url: https://github.com/nelullc/rollo-cups-driver
Packager: Rollo Support <support@rollo.com>
Vendor: Nelu LLC

# Package names are as defined for Red Hat (and clone) distributions
BuildRequires: libcups2-dev, libcupsimage2-dev

# Use buildroot so as not to disturb the version already installed
BuildRoot: /tmp/%{name}-root

%description
A CUPS printer driver for the Rollo X1038 label printer.

%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" LDFLAGS="$RPM_OPT_FLAGS" \
    ./configure
# If we got this far, all prerequisite libraries must be here.
make

%install
# Make sure the RPM_BUILD_ROOT directory exists.
rm -rf $RPM_BUILD_ROOT

make BUILDROOT=$RPM_BUILD_ROOT install

%post

%clean
rm -rf $RPM_BUILD_ROOT

%files
%dir /usr/lib/cups/filter
/usr/lib/cups/filter/*
%dir /usr/share/cups/model
/usr/share/cups/model/*