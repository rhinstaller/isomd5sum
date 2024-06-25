PYTHON ?= python3
PYTHONSITEPACKAGES := $(shell $(PYTHON) -c 'import site; print(site.getsitepackages()[0])')
PYTHONINCLUDE := $(shell $(PYTHON)-config --includes)

VERSION=1.2.5

ifneq (,$(filter sparc64 ppc64 ppc64le x86_64 s390x aarch64 mips64 riscv64,$(shell uname -m)))
LIBDIR = lib64
else
LIBDIR = lib
endif

CFLAGS += -std=gnu11 -Wall -D_GNU_SOURCE=1 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -fPIC $(PYTHONINCLUDE)

OBJECTS = md5.o libimplantisomd5.o checkisomd5.o implantisomd5
SOURCES = $(patsubst %.o,%.c,$(OBJECTS))
LDFLAGS += -fPIC

PYOBJS = pyisomd5sum.o libcheckisomd5.a libimplantisomd5.a

all: implantisomd5 checkisomd5 pyisomd5sum.so libimplantisomd5.a libcheckisomd5.a

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -O3 -o $@ $<

implantisomd5: implantisomd5.o libimplantisomd5.a
	$(CC) $(CPPFLAGS) $(CFLAGS) implantisomd5.o libimplantisomd5.a -lpopt $(LDFLAGS) -o implantisomd5

checkisomd5: checkisomd5.o libcheckisomd5.a
	$(CC) $(CPPFLAGS) $(CFLAGS) checkisomd5.o libcheckisomd5.a -lpopt $(LDFLAGS) -o checkisomd5

libimplantisomd5.a: libimplantisomd5.a(libimplantisomd5.o md5.o utilities.o)

libcheckisomd5.a: libcheckisomd5.a(libcheckisomd5.o md5.o utilities.o)

pyisomd5sum.so: $(PYOBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -shared -g -fpic $(PYOBJS) $(LDFLAGS) -o pyisomd5sum.so

install: all install-bin install-python install-devel

install-bin:
	install -d -m 0755 $(DESTDIR)/usr/bin
	install -d -m 0755 $(DESTDIR)/usr/share/man/man1
	install -m 0755 implantisomd5 $(DESTDIR)/usr/bin
	install -m 0755 checkisomd5 $(DESTDIR)/usr/bin
	install -m 0644 implantisomd5.1 $(DESTDIR)/usr/share/man/man1
	install -m 0644 checkisomd5.1 $(DESTDIR)/usr/share/man/man1

install-python:
	install -d -m 0755 $(DESTDIR)$(PYTHONSITEPACKAGES)
	install -m 0755 pyisomd5sum.so $(DESTDIR)$(PYTHONSITEPACKAGES)

install-devel:
	install -d -m 0755 $(DESTDIR)/usr/include
	install -d -m 0755 $(DESTDIR)/usr/$(LIBDIR)
	install -d -m 0755 $(DESTDIR)/usr/share/pkgconfig
	install -m 0644 libimplantisomd5.h $(DESTDIR)/usr/include/
	install -m 0644 libcheckisomd5.h $(DESTDIR)/usr/include/
	install -m 0644 libimplantisomd5.a $(DESTDIR)/usr/$(LIBDIR)
	install -m 0644 libcheckisomd5.a $(DESTDIR)/usr/$(LIBDIR)
	sed "s#@VERSION@#${VERSION}#g; s#@includedir@#/usr/include#g; s#@libdir@#/usr/${LIBDIR}#g" isomd5sum.pc.in > ${DESTDIR}/usr/share/pkgconfig/isomd5sum.pc

clean:
	rm -f *.o *.so *.pyc *.a .depend *~
	rm -f implantisomd5 checkisomd5 

tag:
	@git tag -a -m "Tag as $(VERSION)" -f $(VERSION)
	@echo "Tagged as $(VERSION)"

archive:
	@git archive --format=tar --prefix=isomd5sum-$(VERSION)/ HEAD |bzip2 > isomd5sum-$(VERSION).tar.bz2
	@echo "The final archive is in isomd5sum-$(VERSION).tar.bz2"

test:
	$(PYTHON) ./testpyisomd5sum.py
