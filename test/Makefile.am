sbin_PROGRAMS = vpcd

check_PROGRAMS = tpl tregex targv \
		 baytech icebox gpib dli phantom cyclades ipmipower cli \
                 ilom lom swpdu

TESTS_ENVIRONMENT = env 
TESTS_ENVIRONMENT += "PATH_POWERMAN=$(top_builddir)/powerman/powerman"
TESTS_ENVIRONMENT += "PATH_POWERMAND=$(top_builddir)/powermand/powermand"
TESTS_ENVIRONMENT += "PATH_ETC=$(top_builddir)/etc"
TESTS = t00 t01 t02 t03 t04 t05 t06 t07 t08 t09 t10 t11 t12 t13 t14 t15 t16 t17 t18 t19 t20 t21 t22 t23 t24 t25 t26 t27 t28 t29 t30 t31 t32 t33 t34 t35 t36 t37 t38 t39 t40 t41 t42 t43 t44 t45 t46 t47 t48 t49 t50 t51 t52
XFAIL_TESTS = 

EXTRA_DIST = $(TESTS) *.exp *.conf

CLEANFILES = *.out *.err *.diff

AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = \
	-I$(top_srcdir)/libcommon \
	-I$(top_srcdir)/libpowerman \
	-I$(top_srcdir)/liblsd

common_ldadd = \
	$(top_srcdir)/liblsd/liblsd.a \
	$(top_srcdir)/libcommon/libcommon.a \
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

phantom_SOURCES = phantom.c
phantom_LDADD = $(common_ldadd) $(LIBCURSES)

cyclades_SOURCES = cyclades.c
cyclades_LDADD = $(common_ldadd)

ipmipower_SOURCES = ipmipower.c
ipmipower_LDADD = $(common_ldadd)

cli_SOURCES = cli.c
cli_LDADD = -L../libpowerman -lpowerman
