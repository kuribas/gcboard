ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libgtkcboard.la
include_HEADERS = gtkcboard.h

libgtkcboard_la_SOURCES = gtkcboard.c gtkcboard-marshal.c
libgtkcboard_la_LIBADD = $(GTKCBOARD_LIBS)
libgtkcboard_la_LDFLAGS = -version-info 1:0:0
libgtkcboard_la_DEPENDENCIES = gcb_common.c gcb_common.h gtkcboard.h gtkcboard-marshal.h

EXTRA_DIST = gcb_common.c gcb_common.h gtkcboard-marshal.h
DISTCLEANFILES = .deps/*.P

AM_CPPFLAGS = $(GTKCBOARD_CPPFLAGS)
AM_CFLAGS = -Wall $(GTKCBOARD_CFLAGS)

if WITH_RUBY
  SUBDIRS = ruby
endif

dist-hook:
	mkdir $(distdir)/ruby
	cp -pr `cat $(srcdir)/ruby/.dist-files | sed 's|^|$(srcdir)/ruby/|'` $(distdir)/ruby
	-find $(distdir)/ruby -type d -name CVS -exec rm -rf {} \;

