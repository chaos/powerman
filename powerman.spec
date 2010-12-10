Name:  powerman
Version: 2.3.7
Release: 1

Summary: PowerMan - centralized power control for clusters
License: GPL
Group: Applications/System
Url: http://sourceforge.net/projects/powerman
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%if 0%{?rhel}
%define _with_httppower 1
%define _with_snmppower 1
%define _with_genders 0
%define _with_tcp_wrappers 1
%endif

%if 0%{?chaos}
%define _with_httppower 1
%define _with_snmppower 1
%define _with_genders 1
%define _with_tcp_wrappers 1
%endif

%if 0%{?fedora}
%define _with_httppower 1
%define _with_snmppower 1
%define _with_genders 0
%define _with_tcp_wrappers 1
%endif

%if 0%{?_with_tcp_wrappers}
BuildRequires: tcp_wrappers
%endif
%if 0%{?_with_genders}
BuildRequires: genders
%endif
%if 0%{?_with_httppower}
BuildRequires: curl-devel
%endif
%if 0%{?_with_snmppower}
BuildRequires: net-snmp-devel
%endif

%package devel
Requires: %{name} = %{version}-%{release}
Summary: Headers and libraries for developing applications using PowerMan
Group: Development/Libraries

%package libs
Requires: %{name} = %{version}-%{release}
Summary: Libraries for applications using PowerMan
Group: System Environment/Libraries

%description
PowerMan is a tool for manipulating remote power control (RPC) devices from a 
central location. Several RPC varieties are supported natively by PowerMan and 
Expect-like configurability simplifies the addition of new devices.

%description devel
A header file and static library for developing applications using PowerMan.

%description libs
A shared library for applications using PowerMan.

%prep
%setup

%build
%configure \
  %{?_with_genders: --with-genders} \
  %{?_with_httppower: --with-httppower} \
  %{?_with_snmppower: --with-snmppower} \
  --program-prefix=%{?_program_prefix:%{_program_prefix}}
make

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT 

%clean
rm -rf $RPM_BUILD_ROOT

%post
if [ -x /sbin/chkconfig ]; then /sbin/chkconfig --add powerman; fi

%post libs
if [ -x /sbin/ldconfig ]; then /sbin/ldconfig %{_libdir}; fi

%preun
if [ "$1" = 0 ]; then
  %{_sysconfdir}/init.d/powerman stop >/dev/null 2>&1 || :
  if [ -x /sbin/chkconfig ]; then /sbin/chkconfig --del powerman; fi
fi

%postun
if [ "$1" -ge 1 ]; then
  %{_sysconfdir}/init.d/powerman condrestart >/dev/null 2>&1 || :
fi

%postun libs
if [ -x /sbin/ldconfig ]; then /sbin/ldconfig %{_libdir}; fi

%files
%defattr(-,root,root,0755)
%doc DISCLAIMER 
%doc COPYING
%doc NEWS
%doc TODO
%{_bindir}/powerman
%{_bindir}/pm
%{_sbindir}/powermand
%{_sbindir}/vpcd
%if 0%{?_with_httppower}
%{_sbindir}/httppower
%endif
%if 0%{?_with_snmppower}
%{_sbindir}/snmppower
%endif
%{_sbindir}/plmpower
%dir %config %{_sysconfdir}/powerman
%{_sysconfdir}/powerman/*.dev
%{_sysconfdir}/powerman/powerman.conf.example
%{_mandir}/*1/*
%{_mandir}/*5/*
%{_mandir}/*8/*
%{_libdir}/stonith/plugins/external/powerman
%{_sysconfdir}/init.d/powerman
%dir %attr(0755,daemon,root) %config %{_localstatedir}/run/powerman

%files devel
%defattr(-,root,root,0755)
%{_includedir}/*
%{_libdir}/*.la
%{_mandir}/*3/*
%ifnos aix5.3 aix5.2 aix5.1 aix5.0 aix4.3
%{_libdir}/*.a
%{_libdir}/*.so
%{_libdir}/pkgconfig/*
%endif

%files libs
%defattr(-,root,root,0755)
%ifnos aix5.3 aix5.2 aix5.1 aix5.0 aix4.3
%{_libdir}/*.so.*
%else
%{_libdir}/*.a
%endif

%changelog

* Tue Feb 14 2006 Ben Woodard <woodard@redhat.com> 1.0.22-3
- Changed /usr/bin to bindir
- Changed /usr/sbin to sbindir
- Added COPYING to list of docs.
- Changed /etc/rc.d/init.d/ to initrddir
- Changed /usr/man to mandir
- Added a fully qualified path to the source file.
- Fixed buildroot
- Added a patch which should fix a fc4 build problem.

* Thu Feb 09 2006 Ben Woodard <woodard@redhat.com> 1.0.22-2
- changed the buildroot to match fedora guidlines
- changed permissions of spec and src files.
- added changelog to spec file
