# Process this file with autoconf to produce a configure script.
AC_INIT(gcb_common.c)
AM_INIT_AUTOMAKE(gcboard, 0.3.0)
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.
GTK_REQUIRED=2.0.0
PKG_CHECK_MODULES(GTKCBOARD, gtk+-2.0 >= $GTK_REQUIRED)
AC_SUBST(GTKCBOARD_CFLAGS)
AC_SUBST(GTKCBOARD_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(malloc.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_CHECK_FUNCS(malloc)
AC_CHECK_FUNCS(memset)

AC_CONFIG_FILES(Makefile)
AC_OUTPUT

