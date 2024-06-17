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
Version: 1.8.4
Release: 0
License: GPLv3
Group: System Environment/Drivers
Source: https://github.com/nelullc/rollo-cups-driver/releases/download/v%{version}/rollo-cups-driver-%{version}.tar.gz
Url: https://github.com/nelullc/rollo-cups-driver
Packager: Rollo Support <support@rollo.com>
Vendor: Nelu LLC

# Package names are as defined for Red Hat (and clone) distributions
BuildRequires: cups-devel

# Use buildroot so as not to disturb the version already installed
BuildRoot: /tmp/%{name}-root

%description
A CUPS printer driver for the Rollo X1038 label printer.

%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" LDFLAGS="$RPM_OPT_FLAGS" ./configure
make

%install
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

%changelog

* Mon June 17 2024 Michael Sweet <msweet@lakesiderobotics.ca>

- Fixed dither buffering that caused black labels (Issue #1)

* Fri Apr 19 2024 Michael Sweet <msweet@lakesiderobotics.ca>

- Updated to 1.8.3.
