# Process this file with autoconf to produce a configure script.
AC_INIT(gcboard, [0.3.1])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])
AC_ARG_ENABLE([ruby],
	       AC_HELP_STRING([--enable-ruby],
	         [build the ruby language bindings @<:@default=disable@:>@]),
		 [], [enable_ruby=no])
AS_IF([test x"$enable_ruby" != xno],
  [AC_PATH_PROG([RUBY], [ruby])
   AS_IF([test x"$RUBY" != x],
   [cd ruby;
   AS_IF([$RUBY extconf.rb],
   	 [with_ruby=yes],
	 [exit 1])
   cd ..;])]
   )

AM_CONDITIONAL(WITH_RUBY, test x"$with_ruby" = xyes)
   
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

