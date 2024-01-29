check_PROGRAMS = \
	vpcd \
	baytech \
	icebox \
	dli \
	cyclades \
	ipmipower \
	redfishpower \
	ilom \
	lom \
	swpdu \
	openbmc-httppower \
	redfish-httppower

dist_check_SCRIPTS = \
	pm-sim \
	mkconf.pl \
	$(TESTS)

TESTS_ENVIRONMENT = env
TESTS_ENVIRONMENT += "PATH_POWERMAN=$(top_builddir)/src/powerman/powerman"
TESTS_ENVIRONMENT += "PATH_POWERMAND=$(top_builddir)/src/powerman/powermand"
TESTS_ENVIRONMENT += "PATH_POWERMAN_STONITH=$(top_srcdir)/heartbeat/powerman"
TESTS_ENVIRONMENT += "TEST_SRCDIR=$(top_srcdir)/test"
TESTS_ENVIRONMENT += "TEST_BUILDDIR=$(top_builddir)/test"

TESTS = t35 t36 \
	t42 t43 t44 t47 t50 t51 t52 t53 t54 t55 \
	t56 t57 t58 t59 t60 t61 t62 t63

XFAIL_TESTS =

CLEANFILES = *.out *.err *.diff

AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/libcommon \
	-I$(top_srcdir)/src/liblsd \
	-I$(top_srcdir)/src/powerman

common_ldadd = \
	$(top_builddir)/src/liblsd/liblsd.la \
	$(top_builddir)/src/libcommon/libcommon.la \
	$(LIBFORKPTY)

vpcd_SOURCES = vpcd.c
vpcd_LDADD = $(common_ldadd)

baytech_SOURCES = baytech.c
baytech_LDADD = $(common_ldadd)

icebox_SOURCES = icebox.c
icebox_LDADD = $(common_ldadd)

swpdu_SOURCES = swpdu.c
swpdu_LDADD = $(common_ldadd)

dli_SOURCES = dli.c
dli_LDADD = $(common_ldadd)

ilom_SOURCES = ilom.c
ilom_LDADD = $(common_ldadd)

lom_SOURCES = lom.c
lom_LDADD = $(common_ldadd)

cyclades_SOURCES = cyclades.c
cyclades_LDADD = $(common_ldadd)

ipmipower_SOURCES = ipmipower.c
ipmipower_LDADD = $(common_ldadd)

redfishpower_SOURCES = redfishpower.c
redfishpower_LDADD = $(common_ldadd)

openbmc_httppower_SOURCES = openbmc-httppower.c
openbmc_httppower_LDADD = $(common_ldadd)

redfish_httppower_SOURCES = redfish-httppower.c
redfish_httppower_LDADD = $(common_ldadd)

check_DATA = \
	mcr.conf sierra.conf \
	t35.conf t36.conf \
	t42.conf t43.conf t44.conf t47.conf \
	t50.conf t51.conf t53.conf t54.conf t55.conf t60.conf \
	t61.conf t62.conf t63.conf test4.conf test.conf

EXTRA_DIST = $(TESTS:%=%.exp) t53.dev

