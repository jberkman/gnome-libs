# Note that this is NOT a relocatable package
%define ver      0.30
%define rel      SNAP
%define prefix   /usr

Summary: GNOME basic libraries
Name: gnome-libs
Version: %ver
Release: %rel
Copyright: LGPL
Group: X11/gnome
Source: ftp://ftp.gnome.org/pub/GNOME/sources/gnome-libs-%{ver}.tar.gz
BuildRoot: /var/tmp/gnome-libs-root
Obsoletes: gnome
Packager: Marc Ewing <marc@redhat.com>
URL: http://www.gnome.org/
Requires: gtk+ >= 1.1
Docdir: %{prefix}/doc

%description
Basic libraries you must have installed to use GNOME.

GNOME is the GNU Network Object Model Environment.  That's a fancy
name but really GNOME is a nice GUI desktop environment.  It makes
using your computer easy, powerful, and easy to configure.

%package devel
Summary: Libraries, includes, etc to develop GNOME applications
Group: X11/gnome
Requires: gnome-libs = %{PACKAGE_VERSION}
Obsoletes: gnome

%description devel
Libraries, include files, etc you can use to develop GNOME applications.

%changelog

* Fri Nov 20 1998 Pablo Saratxaga <srtxg@chanae.alphanet.ch>

- use --localstatedir=/var/lib in config state (score files for games
  for exemple will go there).
- added several more files to %files section, in particular language
  files and corba IDLs

* Wed Sep 23 1998 Michael Fulbright <msf@redhat.com>

- Updated to version 0.30

* Mon Apr 13 1998 Marc Ewing <marc@redhat.com>
- Added %{prefix}/lib/gnome-libs

* Fri Mar 13 1998 Marc Ewing <marc@redhat.com>

- Integrate into gnome-libs source tree

%prep
%setup

%build
# Needed for snapshot releases.
if [ ! -f configure ]; then
  CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh --prefix=%prefix --localstatedir=/var/lib
else
  CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%prefix --localstatedir=/var/lib
fi

if [ "$SMP" != "" ]; then
  (make "MAKE=make -k -j $SMP"; exit 0)
  make
else
  make
fi

%install
rm -rf $RPM_BUILD_ROOT

make prefix=$RPM_BUILD_ROOT%{prefix} install

%clean
rm -rf $RPM_BUILD_ROOT

%post 
if ! grep %{prefix}/lib /etc/ld.so.conf > /dev/null ; then
  echo "%{prefix}/lib" >> /etc/ld.so.conf
fi

/sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-, root, root)

%doc AUTHORS COPYING ChangeLog NEWS README
%{prefix}/lib/lib*.so.*
%{prefix}/bin/*
%{prefix}/sbin/*
%{prefix}/share/locale/*/*/*
%{prefix}/share/idl/*
%{prefix}/share/pixmaps/*
%config %{prefix}/share/gtkrc
%config %{prefix}/etc/*

%files devel
%defattr(-, root, root)

%{prefix}/lib/lib*.so
%{prefix}/lib/*.a
%{prefix}/lib/*.sh
%{prefix}/lib/gnome-libs
%{prefix}/include/*
%{prefix}/share/aclocal/*

