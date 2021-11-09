sbin_PROGRAMS = vpcd

check_PROGRAMS = \
	tpl \
	tregex \
	targv \
	baytech \
	icebox \
	gpib \
	dli \
	cyclades \
	ipmipower \
	redfishpower \
	cli \
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
TESTS_ENVIRONMENT += "PATH_POWERMAN=$(top_builddir)/powerman/powerman"
TESTS_ENVIRONMENT += "PATH_POWERMAND=$(top_builddir)/powermand/powermand"
TESTS_ENVIRONMENT += "PATH_ETC=$(top_builddir)/etc"
TESTS_ENVIRONMENT += "PATH_POWERMAN_STONITH=$(top_srcdir)/heartbeat/powerman"
TESTS_ENVIRONMENT += "TEST_SRCDIR=$(top_srcdir)/test"
TESTS_ENVIRONMENT += "TEST_BUILDDIR=$(top_builddir)/test"

TESTS = t00 t01 t02 t03 t04 t05 t06 t07 t08 t09 t10 t11 t12 t13 \
	t14 t15 t16 t17 t18 t19 t20 t21 t22 t23 t24 t25 t26 t27 \
	t28 t29 t30 t31 t32 t33 t34 t35 t36 t37 t38 t39 t40 t41 \
	t42 t43 t44 t46 t47 t48 t49 t50 t51 t52 t53 t54 t55 \
	t56 t57 t58 t59 t60 t61 t62 t63

XFAIL_TESTS = 

CLEANFILES = *.out *.err *.diff

AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = \
	-I$(top_srcdir)/libcommon \
	-I$(top_srcdir)/libpowerman \
	-I$(top_srcdir)/liblsd

common_ldadd = \
	$(top_builddir)/liblsd/liblsd.a \
	$(top_builddir)/libcommon/libcommon.a \
	$(LIBFORKPTY)

vpcd_SOURCES = vpcd.c
vpcd_LDADD = $(common_ldadd)

tpl_SOURCES = tpl.c
tpl_LDADD = $(common_ldadd)

tregex_SOURCES = tregex.c
tregex_LDADD = $(common_ldadd)

targv_SOURCES = targv.c
targv_LDADD = $(common_ldadd)

baytech_SOURCES = baytech.c
baytech_LDADD = $(common_ldadd)

icebox_SOURCES = icebox.c
icebox_LDADD = $(common_ldadd)

swpdu_SOURCES = swpdu.c
swpdu_LDADD = $(common_ldadd)

gpib_SOURCES = gpib.c
gpib_LDADD = $(common_ldadd)

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

cli_SOURCES = cli.c
cli_LDADD = -L$(top_builddir)/libpowerman -lpowerman

check_DATA = \
	mcr.conf sierra.conf \
	t07.conf t09.conf t10.conf t17.conf t18.conf t19.conf t20.conf \
	t21.conf t22.conf t23.conf t24.conf t25.conf t26.conf t27.conf \
	t28.conf t29.conf t30.conf t31.conf t32.conf t33.conf t34.conf \
	t35.conf t36.conf t37.conf t38.conf t39.conf t40.conf t41.conf \
	t42.conf t43.conf t44.conf t46.conf t47.conf t48.conf \
	t49.conf t50.conf t51.conf t53.conf t54.conf t55.conf t60.conf \
	t61.conf test4.conf test.conf

EXTRA_DIST = $(TESTS:%=%.exp) t53.dev

