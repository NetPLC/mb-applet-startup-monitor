# Process this file with autoconf to produce a configure script.
AC_INIT(mbmonitor.c)
AM_INIT_AUTOMAKE(mb-applet-startup-monitor, 0.1)
AM_CONFIG_HEADER(config.h)

MB_LIBS=""

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for header files.
AC_PATH_X
AC_PATH_XTRA
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([strdup])

PKG_CHECK_MODULES(MB, libstartup-notification-1.0 libmb)

AC_SUBST(MB_LIBS)
AC_SUBST(MB_CFLAGS)

if test "x$GCC" = "xyes"; then
        GCC_FLAGS="-g -Wall -fno-strict-aliasing"
fi

AC_SUBST(GCC_FLAGS)

AC_OUTPUT([Makefile data/Makefile])
