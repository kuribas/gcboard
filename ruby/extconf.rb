begin
  require 'mkmf-gnome2'
rescue LoadError
  puts "Couldn't find mkmf-gnome2.  Please check your ruby-gnome2 installation."
  exit(1)
end

pkg_config('gtk+-2.0')
dir_config("gtk2.0")

$LIBPATH += ["../.libs/"]
$CPPFLAGS = "-I ../ " + $CPPFLAGS
$LIBS = "-lgtkcboard " + $LIBS

create_makefile("gtkcboard")
