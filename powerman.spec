####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
####################################################################
#   Copyright (C) 2001-2002 The Regents of the University of California.
#   Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
#   Written by Andrew Uselton (uselton2@llnl.gov>
#   UCRL-CODE-2002-008.
#   
#   This file is part of PowerMan, a remote power management program.
#   For details, see <http://www.llnl.gov/linux/powerman/>.
#   
#   PowerMan is free software; you can redistribute it and/or modify it under
#   the terms of the GNU General Public License as published by the Free
#   Software Foundation; either version 2 of the License, or (at your option)
#   any later version.
#   
#   PowerMan is distributed in the hope that it will be useful, but WITHOUT 
#   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
#   for more details.
#   
#   You should have received a copy of the GNU General Public License along
#   with PowerMan; if not, write to the Free Software Foundation, Inc.,
#   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
####################################################################
 
%define name    powerman
%define version 1.0.0
%define release 1

Name: %name
Version: %version
Release: %release
Summary: PowerMan - Power to the Cluster 
#URL: FOO
Group: Applications/System
License:	GPL
URL:		http://www.llnl.gov/linux/powerman/

BuildRoot: %{_tmppath}/%{name}-%{version}

Source0: %{name}-%{version}.tgz

%description
  PowerMan is an open source project for power management of the
nodes in a cluster.  The daemon resides on a management workstation 
and communicates with power control hardware via TCP (raw or telnet).
It supports a wide vaiety of hardware and is customized through a
configuration file on the manaement workstation.  The client can
be anywhere (TCP wrappers permitting) and does not know anything
about the configuration of the target cluster, i.e. it determines 
cluster properties as part of its negotiation with the server.  


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
/usr/bin/powerman
/usr/bin/powermand
%defattr(-,root,root,0644)
/etc/powerman/baytech.dev
/etc/powerman/icebox.dev
/etc/powerman/wti.dev
/etc/powerman/vicebox.dev
%config(noreplace) /etc/powerman/powerman.conf
/usr/man/man1/powerman.1*
/usr/man/man1/powermand.1*
/usr/man/man5/powerman.conf.5*
%doc ChangeLog
%doc DISCLAIMER
%doc README
%doc TODO
%doc doc/powerman.fig
%doc doc/powermand.fig
