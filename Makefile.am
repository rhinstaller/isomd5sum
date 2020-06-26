ACLOCAL_AMFLAGS = -I m4

lib_LIBRARIES = libcheckisomd5.a libimplantisomd5.a
bin_PROGRAMS = implantisomd5 checkisomd5

libcheckisomd5_a_SOURCES = libcheckisomd5.c libcheckisomd5.h md5.c md5.h utilities.c utilities.h
libimplantisomd5_a_SOURCES = libimplantisomd5.c libimplantisomd5.h md5.c md5.h utilities.c utilities.h

checkisomd5_SOURCES = checkisomd5.c
checkisomd5_LDADD = libcheckisomd5.a
checkisomd5_LDFLAGS = -lpopt

implantisomd5_SOURCES = implantisomd5.c
implantisomd5_LDADD = libimplantisomd5.a
implantisomd5_LDFLAGS = -lpopt

include_HEADERS = libcheckisomd5.h libimplantisomd5.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = isomd5sum.pc

man_MANS = checkisomd5.1 implantisomd5.1
EXTRA_DIST = $(man_MANS) pyisomd5sum.c testpyisomd5sum.py
