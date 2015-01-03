require 'mkmf'

pkg_config('gtk+-2.0')

dir_config("gtk2.0")
dir_config("gtkcboard")

if not have_header('gtkcboard.h') or not have_library('gtkcboard', 'gtk_cboard_new')
   puts <<EOL
You must install gtkcboard first.
If you installed gtkcboard in a nonstandard location invoke
this program with the following options:
  --with-gtkcboard-libs=     location of library
  --with-gtkcboard-include=  location of header file
EOL
   exit(-1)
end

find_header("rbgtk.h", '$(sitearchdir)')
create_makefile("gtkcboard")
