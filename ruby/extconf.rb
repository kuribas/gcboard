require 'mkmf'

pkg_config('gtk+-2.0')
dir_config("gtk2.0")

find_header("rbgtk.h", '$(sitearchdir)')
find_header("rbgtk.h", '$(vendorarchdir)')

$LIBPATH += ["../.libs/"]
$CPPFLAGS = "-I ../ " + $CPPFLAGS
$LIBS = "-lgtkcboard " + $LIBS

create_makefile("gtkcboard")
