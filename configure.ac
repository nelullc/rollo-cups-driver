dnl
dnl Configuration script for the Rollo CUPS Printer Driver.
dnl
dnl Copyright © 2024 by Nelu LLC.
dnl
dnl This program is free software.  Distribution and use rights are outlined in
dnl the file "COPYING".
dnl


dnl We need at least autoconf 2.60...
AC_PREREQ(2.60)


dnl Package name and version...
AC_INIT([rollo-driver], [1.8.4], [https://support.rollo.com/support/home], [rollo-driver], [https://support.rollo.com/support/home])

ROLLO_VERSION="AC_PACKAGE_VERSION"
AC_SUBST([ROLLO_VERSION])
AC_DEFINE_UNQUOTED([ROLLO_VERSION], "$ROLLO_VERSION", [Version number])

ROLLO_MAJOR="$(echo AC_PACKAGE_VERSION | awk -F. '{print $1}')"
ROLLO_MINOR="$(echo AC_PACKAGE_VERSION | awk -F. '{print $2}')"
ROLLO_PATCH="$(echo AC_PACKAGE_VERSION | awk -F. '{print $3}')"

AC_SUBST([ROLLO_MAJOR])
AC_SUBST([ROLLO_MINOR])
AC_SUBST([ROLLO_PATCH])


dnl This line is provided to ensure that you don't run the autoheader program
dnl against this project.  Doing so is completely unsupported and WILL cause
dnl problems!
AH_TOP([#error "Somebody ran autoheader on this project which is unsupported and WILL cause problems."])


dnl Get the build and host platforms and split the host_os value
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

[host_os_name=$(echo $host_os | sed -e '1,$s/[0-9.]*$//g')]
[host_os_version=$(echo $host_os | sed -e '1,$s/^[^0-9.]*//g' | awk -F. '{print $1 $2}')]
# Linux often does not yield an OS version we can use...
AS_IF([test "x$host_os_version" = x], [
    host_os_version="0"
])


dnl Compiler options...
CFLAGS="${CFLAGS:=}"
CPPFLAGS="${CPPFLAGS:=}"
LDFLAGS="${LDFLAGS:=}"
LIBS="${LIBS:=}"
OPTIM="${OPTIM:=}"

AC_SUBST([OPTIM])


dnl Standard programs...
AC_PROG_CC
AC_PATH_PROG([RM], [rm])


dnl install-sh
AC_MSG_CHECKING([for install-sh script])
INSTALL="$(pwd)/install-sh"
AC_SUBST([INSTALL])
AC_MSG_RESULT([using $INSTALL])


dnl Check for pkg-config, which is used for some other tests later on...
AC_PATH_TOOL([PKGCONFIG], [pkg-config])


dnl CUPS library...
AC_PATH_TOOL([CUPSCONFIG], [cups-config])
AC_MSG_CHECKING([for CUPS library v2.2 or higher])
AS_IF([$PKGCONFIG --exists cups], [
    CUPS_VERSION="$($PKGCONFIG --modversion cups)"
    AC_MSG_RESULT([yes, v$CUPS_VERSION])

    CFLAGS="$CFLAGS $($PKGCONFIG --cflags cups)"
    LIBS="$LIBS $($PKGCONFIG --libs cups)"

    AC_MSG_CHECKING([for CUPS datadir])
    CUPS_DATADIR="$($PKGCONFIG --variable=cups_datadir cups)"
    AC_MSG_RESULT([$CUPS_DATADIR])

    AC_MSG_CHECKING([for CUPS serverbin])
    CUPS_SERVERBIN="$($PKGCONFIG --variable=cups_serverbin cups)"
    AC_MSG_RESULT([$CUPS_SERVERBIN])
], [test "x$CUPSCONFIG" = x], [
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([CUPS 2.2 or later is required for the Rollo CUPS printer driver.])
], [
    CUPS_VERSION="$($CUPSCONFIG --version)"
    AS_CASE(["$CUPS_VERSION"],
        [1.* | 2.0* | 2.1*], [
            AC_MSG_RESULT(no)
	    AC_MSG_ERROR([CUPS 2.2 or later is required for the Rollo CUPS printer driver.])
    ])

    AC_MSG_RESULT([yes, v$CUPS_VERSION])
    CFLAGS="$CFLAGS $($CUPSCONFIG --cflags)"
    LDFLAGS="$LDFLAGSS $($CUPSCONFIG --ldflags)"
    LIBS="$LIBS $($CUPSCONFIG --image --libs)"

    AC_MSG_CHECKING([for CUPS datadir])
    CUPS_DATADIR="$($CUPSCONFIG --datadir)"
    AC_MSG_RESULT([$CUPS_DATADIR])

    AC_MSG_CHECKING([for CUPS serverbin])
    CUPS_SERVERBIN="$($CUPSCONFIG --serverbin)"
    AC_MSG_RESULT([$CUPS_SERVERBIN])
])

AC_SUBST([CUPS_DATADIR])
AC_SUBST([CUPS_SERVERBIN])


dnl Extra compiler options...
AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [turn on debugging, default=no]))
AC_ARG_ENABLE([maintainer], AS_HELP_STRING([--enable-maintainer], [turn on maintainer mode, default=no]))
AC_ARG_WITH([sanitizer], AS_HELP_STRING([--with-sanitizer], [build with address, leak, memory, or undefined sanitizer, default=no]), [], [with_sanitizer=no])
AS_IF([test "x$with_sanitizer" = xyes], [
    with_sanitizer="address"
], [test "$with_sanitizer" != address -a "$with_sanitizer" != leak -a "$with_sanitizer" != memory -a "$with_sanitizer" != no -a "$with_sanitizer" != undefined], [
    AC_MSG_ERROR([Unsupported --with-sanitizer value "$with_sanitizer" specified.])
])

AS_IF([test x$enable_debug = xyes], [
    CSFLAGS=""
    OPTIM="$OPTIM -g"
    OPTIONS="-DDEBUG -DDEBUG_GUARDS"
], [
    CSFLAGS="-o runtime"
    OPTIM="$OPTIM -g -Os"
    OPTIONS=""
])

AC_SUBST([CSFLAGS])
AC_SUBST([OPTIONS])

WARNINGS=""
AC_SUBST([WARNINGS])

AS_IF([test -n "$GCC"], [
    AS_IF([test x$with_sanitizer != xno], [
	# Use -fsanitize=FOO with debugging...
	OPTIM="$OPTIM -fsanitize=$with_sanitizer"
    ], [echo "$CPPFLAGS $CFLAGS" | grep -q _FORTIFY_SOURCE], [
        # Don't add _FORTIFY_SOURCE if it is already there
    ], [
	# Otherwise use the Fortify enhancements to catch any unbounded
	# string operations...
	CPPFLAGS="$CPPFLAGS -D_FORTIFY_SOURCE=3"
    ])

    dnl Show all standard warnings + unused variables when compiling...
    WARNINGS="-Wall -Wunused"

    dnl Drop some not-useful/unreliable warnings...
    for warning in char-subscripts deprecated-declarations format-truncation format-y2k switch unused-result; do
	AC_MSG_CHECKING([whether compiler supports -Wno-$warning])

	OLDCFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -Wno-$warning -Werror"

	AC_COMPILE_IFELSE([AC_LANG_PROGRAM()], [
	    AC_MSG_RESULT([yes])
	    WARNINGS="$WARNINGS -Wno-$warning"
        ], [
	    AC_MSG_RESULT([no])
	])

	CFLAGS="$OLDCFLAGS"
    done

    dnl Maintainer mode enables -Werror...
    AS_IF([test x$enable_maintainer = xyes], [
	WARNINGS="$WARNINGS -Werror"
    ])

    dnl See if PIE options are supported...
    AC_MSG_CHECKING(whether compiler supports -fPIE)
    OLDCFLAGS="$CFLAGS"
    AS_CASE(["$host_os_name"],
    [darwin*], [
	CFLAGS="$CFLAGS -fPIC -fPIE -Wl,-pie"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[
	    OLDCFLAGS="-fPIC $OLDCFLAGS"
	    LDFLAGS="-fPIE -Wl,-pie $LDFLAGS"
	    AC_MSG_RESULT([yes])
	],[
	    AC_MSG_RESULT([no])
	])
    ], [*], [
	CFLAGS="$CFLAGS -fPIC -fPIE -pie"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[
	    OLDCFLAGS="-fPIC $OLDCFLAGS"
	    LDFLAGS="-fPIE -pie $LDFLAGS"
	    AC_MSG_RESULT([yes])
	],[
	    AC_MSG_RESULT([no])
	])
    ])
    CFLAGS="$OLDCFLAGS"

    dnl OS-specific compiler options...
    AC_MSG_CHECKING([for OS-specific compiler options])
    AS_CASE(["$host_os_name"], [linux*], [
    	# Make sure we get the full set of Linux APIs from the headers...
	CPPFLAGS="$CPPFLAGS -D__USE_MISC -D_GNU_SOURCE"

	# Mark read-only sections as relocatable to random addresses...
	LDFLAGS="$LDFLAGS -Wl,-z,relro,-z,now"

        AC_MSG_RESULT([-D__USE_MISC -D_GNU_SOURCE -Wl,-z,relro,-z,now])
    ], [darwin*], [
        # When not building for debug, target macOS 11 or later, "universal"
        # binaries when possible...
        AS_IF([echo "$CPPFLAGS $CFLAGS $LDFLAGS $OPTIM" | grep -q "\\-arch "], [
            # Don't add architecture/min-version flags if they are already present
            AC_MSG_RESULT([none])
        ], [echo "$CPPFLAGS $CFLAGS $LDFLAGS $OPTIM" | grep -q "\\-mmacosx-version-"], [
            # Don't add architecture/min-version flags if they are already present
            AC_MSG_RESULT([none])
        ], [test "$host_os_version" -ge 200 -a x$enable_debug != xyes], [
	    # macOS 11.0 and higher support the Apple Silicon (arm64) CPUs
	    OPTIM="$OPTIM -mmacosx-version-min=10.10 -arch x86_64 -arch arm64"
            AC_MSG_RESULT([-mmacosx-version-min=10.10 -arch x86_64 -arch arm64])
        ], [test x$enable_debug != xyes], [
	    OPTIM="$OPTIM -mmacosx-version-min=10.10 -arch x86_64"
            AC_MSG_RESULT([-mmacosx-version-min=10.10 -arch x86_64])
	], [
	    # Don't add architecture/min-version flags if debug enabled
            AC_MSG_RESULT([none])
	])
    ], [*], [
        AC_MSG_RESULT([none])
    ])
])


dnl Extra linker options...
AC_ARG_WITH([ldflags], AS_HELP_STRING([--with-ldflags=...], [Specify additional LDFLAGS]), [
    LDFLAGS="$withval $LDFLAGS"
])

dnl Generate the Makefile...
AC_CONFIG_FILES([
    Makefile
])
AC_OUTPUT
