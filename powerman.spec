####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
#
# I may want to return to this preamble and define some more of the 
# standard tags.
#
# v. 0-1-2:  2001-09-05
#            Add the Post Install step setting digi to suid root
# v. 0-1-3:  2001-09-05
#            Update version to current value
# v. 0-1-4:  2001-09-05
#            Fix man page references so they can be ".1" or ".1.gz"
#            Make powerman/etc/*.conf be "noreplace" config files
# v. 0-1-5:  2001-09-05
# v. 0-1-6:  2001-09-07
#            add icebox and make a few changes to the code
# v. 0-1-7:  2001-09-17
#            fixed corner case for unfull icebox and one bug 
#            related to \r\n
#            This is the version released for the pca/b clusters
####################################################################
 
%define name    powerman
%define version 0.1.7
%define release 2

Name: %name
Version: %version
Release: %release
Summary: PowerMan - Cluster Power Management
#URL: FOO
Group: Applications/System
Copyright: LLNL/Internal-Use-Only
BuildRoot: %{_tmppath}/%{name}-%{version}

Source0: %{name}-%{version}.tgz

%description
PowerMan is currently available for LLNL internal use only.
An unrestricted GPL release of PowerMan is pending Review & Release.

%prep
%setup

%build
make

%install
rm -rf "$RPM_BUILD_ROOT"
mkdir -p "$RPM_BUILD_ROOT"
DESTDIR="$RPM_BUILD_ROOT" make install

%clean
rm -rf "$RPM_BUILD_ROOT"

%pre

%post
# Once I have a good powerman configuration script I'll probably want to
# run it here.

%preun
# If there'e anything in post that needs to be undone, or a backup of
# a config file to be made, I can perhaps do it here.

%files
%defattr(-,root,root,0755)
%config(noreplace) /etc/powerman.conf
%config(noreplace) /usr/lib/powerman/etc/*.conf
/usr/bin/pm
%doc DISCLAIMER
%doc README
%doc TOUR.SH
/usr/lib/powerman/bin
/usr/lib/powerman/lib
/usr/man/man1/digi.1*
/usr/man/man1/etherwake.1*
/usr/man/man1/icebox.1*
/usr/man/man1/pm.1*
/usr/man/man1/rmc.1*
/usr/man/man1/wti.1*
/usr/man/man5/digi.conf.5*
/usr/man/man5/etherwake.conf.5*
/usr/man/man5/icebox.conf.5*
/usr/man/man5/powerman.conf.5*
/usr/man/man5/wti.conf.5*
