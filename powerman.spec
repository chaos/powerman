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
 
Name: 
Version: 
Release: 
Summary: PowerMan - Power to the Cluster 
Group: Applications/System
License: GPL
URL: http://www.llnl.gov/linux/powerman/

BuildRoot: %{_tmppath}/%{name}-%{version}

Source0: %{name}-%{version}.tgz

%description
PowerMan is a tool for manipulating remote power control (RPC) devices from a 
central location. Several RPC varieties are supported natively by PowerMan and 
Expect-like configurability simplifies the addition of new devices.

%prep
%setup

%build
make VERSION=%{version} RELEASE=%{release}

%install
rm -rf "$RPM_BUILD_ROOT"
mkdir -p "$RPM_BUILD_ROOT"
DESTDIR="$RPM_BUILD_ROOT" make install

%clean
rm -rf "$RPM_BUILD_ROOT"

%post
if [ -x /etc/rc.d/init.d/powerman ]; then
  if /etc/rc.d/init.d/powerman status | grep running >/dev/null 2>&1; then
    /etc/rc.d/init.d/powerman stop
    WASRUNNING=1
  fi
  [ -x /sbin/chkconfig ] && /sbin/chkconfig --del powerman
  [ -x /sbin/chkconfig ] && /sbin/chkconfig --add powerman
  if test $WASRUNNING = 1; then
    /etc/rc.d/init.d/powerman start
  fi
fi

%preun
if [ "$1" = 0 ]; then
  if [ -x /etc/rc.d/init.d/powerman ]; then
    [ -x /sbin/chkconfig ] && /sbin/chkconfig --del powerman
    if /etc/rc.d/init.d/powerman status | grep running >/dev/null 2>&1; then
      /etc/rc.d/init.d/powerman stop
    fi
  fi
fi

%files
%doc ChangeLog DISCLAIMER
%defattr(0644,root,root)
%attr(0755,root,root)/usr/bin/powerman
%attr(0755,root,root)/usr/bin/pm
%attr(0755,root,root)/usr/sbin/powermand
%dir %attr(0755,root,root) /etc/powerman
%dir %attr(0755,root,root) /usr/man/man1
%dir %attr(0755,root,root) /usr/man/man5
%dir %attr(0755,root,root) /usr/man/man7
/etc/powerman/*
/usr/man/man1/*
/usr/man/man5/*
/usr/man/man7/*
%config(noreplace) %attr(0755,root,root)/etc/rc.d/init.d/powerman
