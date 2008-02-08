%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Summary: Utilities for working with md5sum implanted in ISO images
Name: isomd5sum
Version: 1.0.4
Release: 1
Epoch: 1
License: GPLv2+
Group: Applications/System
URL: http://git.fedorahosted.org/git/?p=isomd5sum.git;a=summary
Source0: http://fedorahosted.org/releases/i/s/isomd5sum/%{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: python-devel popt-devel

%description
The isomd5sum package contains utilities for implanting and verifying
an md5sum implanted into an ISO9660 image.

%package devel
Summary: Development headers and library for using isomd5sum 
Group: Development/System
Requires: %{name} = %{epoch}:%{version}-%{release}

%description devel
This contains header files and a library for working with the isomd5sum
implanting and checking.


%prep
%setup -q

%build
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc COPYING
/usr/bin/implantisomd5
/usr/bin/checkisomd5
%{_mandir}/man*/*
%{python_sitearch}/pyisomd5sum.so

%files devel
%defattr(-,root,root,-)
%{_includedir}/*.h
%{_libdir}/*.a

%changelog
* Thu Feb  7 2008 Jeremy Katz <katzj@redhat.com> - 1:1.0.4-1
- Add man pages from Ryan Finnie (ryan AT finnie DOT org)
- Use popt in checkisomd5 (Ryan Finnie)
- Fix verbose/gauge interactions (Ryan Finnie)
- A few other little janitorial things (Ryan Finnie)

* Mon Dec 10 2007 Jeremy Katz <katzj@redhat.com> - 1:1.0.2-1
- The "fix the build after changing the API" release

* Mon Dec 10 2007 Jeremy Katz <katzj@redhat.com> - 1:1.0.1-1
- Add some simple callback support in the library

* Fri Dec  7 2007 Jeremy Katz <katzj@redhat.com> - 1.0-1
- Initial build.

