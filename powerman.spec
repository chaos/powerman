# $Id$

%define name    powerman
%define version 0.1.1
%define release 1

Name: %name
Version: %version
Release: %release
Summary: PowerMan - Cluster Power Management
#URL: FOO
Group: Applications/SystemUtilities
Copyright: LLNL/Internal-Use-Only
BuildRoot: %{_tmppath}/%{name}-%{version}

Source0: %{name}-%{version}.tgz

%description
PowerMan is currently available for LLNL internal use only.
An unrestricted GPL release of PowerMan is pending Review & Release.

%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" LDFLAGS="-s" ./configure --prefix=/usr
make

%install
rm -rf "$RPM_BUILD_ROOT"
mkdir -p "$RPM_BUILD_ROOT"
DESTDIR="$RPM_BUILD_ROOT" make install

%clean
rm -rf "$RPM_BUILD_ROOT"

%pre

%post

%preun

%files
%defattr(-,root,root,0755)
%config(noreplace) /etc/powerman.conf
/usr/bin/powerman
/usr/doc/powerman-0.1.1/DISCLAIMER
/usr/doc/powerman-0.1.1/README
/usr/doc/powerman-0.1.1/TOUR.SH
/usr/lib/powerman/bin/bogus_check
/usr/lib/powerman/bin/bogus_set
/usr/lib/powerman/bin/digi
/usr/lib/powerman/bin/etherwake
/usr/lib/powerman/bin/rmc
/usr/lib/powerman/bin/wti
/usr/lib/powerman/etc/bogus.conf
/usr/lib/powerman/etc/digi.conf
/usr/lib/powerman/etc/etherwake.conf
/usr/lib/powerman/etc/powerman.conf
/usr/lib/powerman/etc/wti.conf
/usr/lib/powerman/lib/ether-wake
/usr/lib/powerman/man/man1/digi.1
/usr/lib/powerman/man/man1/etherwake.1
/usr/lib/powerman/man/man1/pm.1
/usr/lib/powerman/man/man1/rmc.1
/usr/lib/powerman/man/man1/wti.1
/usr/lib/powerman/man/man5/digi.conf.5
/usr/lib/powerman/man/man5/etherwake.conf.5
/usr/lib/powerman/man/man5/powerman.conf.5
/usr/lib/powerman/man/man5/wti.conf.5


