Name: 
Version:
Release:

Summary: PowerMan - centralized power control for clusters
License: GPL
Group: Applications/System
Url: http://sourceforge.net/projects/powerman
Source0: %{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%if 0%{?rhel}
%define _with_httppower 1
%define _with_genders 0
%define _with_tcp_wrappers 1
%endif

%if 0%{?chaos}
%define _with_httppower 1
%define _with_genders 1
%define _with_tcp_wrappers 1
%endif

%if 0%{?fedora}
%define _with_httppower 1
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

%description
PowerMan is a tool for manipulating remote power control (RPC) devices from a 
central location. Several RPC varieties are supported natively by PowerMan and 
Expect-like configurability simplifies the addition of new devices.

%prep
%setup

%build
%configure \
  %{?_with_genders: --with-genders} \
  %{?_with_httppower: --with-httppower} \
  --program-prefix=%{?_program_prefix:%{_program_prefix}}
make

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT 

%clean
rm -rf $RPM_BUILD_ROO

%post
if [ -x /sbin/chkconfig ]; then /sbin/chkconfig --add powerman; fi

%preun
if [ "$1" = 0 ]; then
  %{_sysconfdir}/init.d/powerman stop >/dev/null 2>&1 || :
  if [ -x /sbin/chkconfig ]; then /sbin/chkconfig --del powerman; fi
fi

%postun
if [ "$1" -ge 1 ]; then
  %{_sysconfdir}/init.d/powerman condrestart >/dev/null 2>&1 || :
fi

%files
%defattr(-,root,root,-)
%doc ChangeLog 
%doc DISCLAIMER 
%doc COPYING
%doc NEWS
%doc TODO
%{_bindir}/powerman
%{_bindir}/pm
%{_sbindir}/powermand
%if 0%{?_with_httppower}
%{_sbindir}/httppower
%endif
%{_sbindir}/plmpower
%dir %config %{_sysconfdir}/powerman
%{_sysconfdir}/powerman/*.dev
%{_sysconfdir}/powerman/powerman.conf.example
%{_mandir}/man1/*
%{_mandir}/man5/*
%{_mandir}/man7/*
%{_mandir}/man8/*
%{_sysconfdir}/init.d/powerman
%{_libdir}/*
%{_includedir}/*

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
