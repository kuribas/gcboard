require 'mkmf'

pkg_config('gtk+-2.0')
dir_config("gtk2.0")

unless find_header("rbgtk.h", '$(sitearchdir)') ||
      find_header("rbgtk.h", '$(vendorarchdir)')
   puts "couldn't find header files for ruby-gtk2"
   exit(1)
end

$LIBPATH += ["../.libs/"]
$CPPFLAGS = "-I ../ " + $CPPFLAGS
$LIBS = "-lgtkcboard " + $LIBS

create_makefile("gtkcboard")
