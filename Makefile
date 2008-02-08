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
LDFLAGS += -fPIC

PYOBJS = pyisomd5sum.o libcheckisomd5.a libimplantisomd5.a

all: implantisomd5 checkisomd5 pyisomd5sum.so libimplantisomd5.a libcheckisomd5.a

%.o: %.c
	$(CC) -c -O $(CFLAGS) -o $@ $<

implantisomd5: implantisomd5.o libimplantisomd5.a
	$(CC) -lpopt $(CFLAGS) implantisomd5.o libimplantisomd5.a -o implantisomd5

checkisomd5: checkisomd5.o libcheckisomd5.a
	$(CC) -lpopt $(CFLAGS) checkisomd5.o libcheckisomd5.a -o checkisomd5

libimplantisomd5.a: libimplantisomd5.a(libimplantisomd5.o md5.o)

libcheckisomd5.a: libcheckisomd5.a(libcheckisomd5.o md5.o)

pyisomd5sum.so: $(PYOBJS)
	$(CC) -shared -g -o pyisomd5sum.so -fpic $(PYOBJS) $(LDFLAGS)

install: all
	mkdir -p $(DESTDIR)/usr/$(LIBDIR)/$(PYTHON)/site-packages
	mkdir -p $(DESTDIR)/usr/include
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/usr/share/man/man1
	install -m 755 implantisomd5 $(DESTDIR)/usr/bin
	install -m 755 checkisomd5 $(DESTDIR)/usr/bin
	install -m 755 implantisomd5.1 $(DESTDIR)/usr/share/man/man1
	install -m 755 checkisomd5.1 $(DESTDIR)/usr/share/man/man1
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

VERSION=$(shell awk '/Version:/ { print $$2 }' isomd5sum.spec)
RELEASE=$(shell awk '/Release:/ { print $$2 }' isomd5sum.spec |sed -e 's/%{?dist}//')

rpmlog:
	@echo "* `date "+%a %b %d %Y"` `git-config user.name` <`git-config user.email`>"
	@git-log --pretty="format:- %s (%ae)" $(VERSION)-$(RELEASE).. |sed -e 's/@.*)/)/'
	@echo

tag:
	@git tag -a -m "Tag as $(VERSION)-$(RELEASE)" -f $(VERSION)-$(RELEASE)
	@echo "Tagged as $(VERSION)-$(RELEASE)"

archive:
	@git-archive --format=tar --prefix=isomd5sum-$(VERSION)/ HEAD |bzip2 > isomd5sum-$(VERSION).tar.bz2
	@echo "The final archive is in isomd5sum-$(VERSION).tar.bz2"

src: tag create-archive
	@rpmbuild -ts --nodeps booty-$(VERSION).tar.bz2
