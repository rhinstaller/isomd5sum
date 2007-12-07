PYVER  := $(shell python -c 'import sys; print sys.version[0:3]')
PYTHON = python$(PYVER)
PYTHONINCLUDE = /usr/include/$(PYTHON)

ifneq (,$(filter ppc64 x86_64 s390x,$(shell uname -m)))
LIBDIR = lib64
else
LIBDIR = lib
endif

CFLAGS = $(RPM_OPT_FLAGS) -Wall -Werror -D_GNU_SOURCE=1 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -fPIC -I$(PYTHONINCLUDE)

OBJECTS = md5.o libimplantisomd5.o checkisomd5.o implantisomd5
SOURCES = $(patsubst %.o,%.c,$(OBJECTS))
LDFLAGS += -lpopt -fPIC

PYOBJS = pyisomd5sum.o libcheckisomd5.a libimplantisomd5.a

all: implantisomd5 checkisomd5 pyisomd5sum.so libimplantisomd5.a libcheckisomd5.a

%.o: %.c
	gcc -c -O $(CFLAGS) -o $@ $<

implantisomd5: implantisomd5.o libimplantisomd5.a

checkisomd5: checkisomd5.o libcheckisomd5.a

libimplantisomd5.a: libimplantisomd5.a(libimplantisomd5.o md5.o)

libcheckisomd5.a: libcheckisomd5.a(libcheckisomd5.o md5.o)

pyisomd5sum.so: $(PYOBJS)
	gcc -shared -g -o pyisomd5sum.so -fpic $(PYOBJS) $(LDFLAGS)

install:
	mkdir -p $(DESTDIR)/usr/$(LIBDIR)/$(PYTHON)/site-packages
	mkdir -p $(DESTDIR)/usr/include
	mkdir -p $(DESTDIR)/usr/bin
	install -m 755 implantisomd5 $(DESTDIR)/usr/bin
	install -m 755 checkisomd5 $(DESTDIR)/usr/bin
	install -m 755 pyisomd5sum.so $(DESTDIR)/usr/$(LIBDIR)/$(PYTHON)/site-packages
#	ln -s ../../bin/implantisomd5 $(DESTDIR)/usr/lib/anaconda-runtime/implantisomd5
#	ln -s ../../bin/checkisomd5 $(DESTDIR)/usr/lib/anaconda-runtime/checkisomd5
	install -m 644 libimplantisomd5.h $(DESTDIR)/usr/include/
	install -m 644 libcheckisomd5.h $(DESTDIR)/usr/include/
	install -m 644 libimplantisomd5.a $(DESTDIR)/usr/$(LIBDIR)
	install -m 644 libcheckisomd5.a $(DESTDIR)/usr/$(LIBDIR)

clean:
	rm -f *.o *.so *.pyc *.a .depend *~
	rm -f implantisomd5 checkisomd5 
