if HAVE_SYSTEMD
systemdsystemunit_DATA = powerman.service
systemdtmpdir_ddir = ${prefix}/lib/tmpfiles.d
systemdtmpdir_d_DATA = tmpfiles.d/powerman.conf
endif

if WITH_PKG_CONFIG
pkgconfig_DATA = libpowerman.pc
endif

check_DATA = \
	devices/vpc.dev

pkgsysconfdir = $(sysconfdir)/powerman

pkgsysconf_DATA = \
	powerman.conf.example \
	devices/apc7900.dev \
	devices/apc7900v3.dev \
	devices/apc7920.dev \
	devices/apc8941.dev \
	devices/apc.dev \
	devices/apc-snmp.dev \
	devices/apcnew.dev \
	devices/apcold.dev \
	devices/apcpdu3.dev \
	devices/apcpdu.dev \
	devices/appro-greenblade.dev \
	devices/appro-gb2.dev \
	devices/bashfun.dev \
	devices/baytech.dev \
	devices/baytech-snmp.dev \
	devices/baytech-rpc18d-nc.dev \
	devices/baytech-rpc22.dev \
	devices/baytech-rpc28-nc.dev \
	devices/baytech-rpc3-nc.dev \
	devices/cb-7050.dev \
	devices/cyclades-pm8.dev \
	devices/cyclades-pm10.dev \
	devices/cyclades-pm20.dev \
	devices/cyclades-pm42.dev \
	devices/dli.dev \
	devices/dli4.dev \
	devices/eaton-revelation-snmp.dev \
	devices/eaton-epdu-blue-switched.dev \
	devices/hp3488.dev \
	devices/hpilo.dev \
	devices/hpmp.dev \
	devices/hpmpblade.dev \
	devices/hpmpcell.dev \
	devices/hpmpdome.dev \
	devices/ibmbladecenter.dev \
	devices/icebox.dev \
	devices/icebox3.dev \
	devices/ics8064.dev \
	devices/ilom.dev \
	devices/lom.dev \
	devices/ipmi.dev \
	devices/ipmipower.dev \
	devices/ipmipower-serial.dev \
	devices/kvm.dev \
	devices/kvm-ssh.dev \
	devices/openbmc.dev \
	devices/redfish-supermicro.dev \
	devices/redfishpower-cray-r272z30.dev \
	devices/redfishpower-supermicro.dev \
	devices/redfishpower-cray-windom.dev \
	devices/phantom.dev \
	devices/plmpower.dev \
	devices/powerman.dev \
	devices/rancid-cisco-poe.dev \
	devices/rancid-edgemax-poe.dev \
	devices/raritan-px4316.dev \
	devices/raritan-px5523.dev \
	devices/sentry_cdu.dev \
	devices/swpdu.dev \
	devices/wti.dev \
	devices/wti-rps10.dev

EXTRA_DIST = $(pkgsysconf_DATA) $(check_DATA)
