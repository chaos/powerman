Name: powerman
Version: 1.0.22
Release: 1
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
  if test x$WASRUNNING = x1; then
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
%defattr(0644,root,root)
%doc ChangeLog DISCLAIMER
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
