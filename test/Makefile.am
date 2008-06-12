check_PROGRAMS = tpl vpcd tregex targv baytech

TESTS_ENVIRONMENT = env 
TESTS_ENVIRONMENT += "PATH_POWERMAN=$(top_builddir)/src/powerman"
TESTS_ENVIRONMENT += "PATH_POWERMAND=$(top_builddir)/src/powermand"
TESTS_ENVIRONMENT += "PATH_ETC=$(top_builddir)/etc"
TESTS = t00 t01 t02 t03 t04 t05 t06 t07 t08 t09 t10 t11 t12 t13 t14 t15 t16 t17 t18 t19 t20 t21 t22 t23 t24

CLEANFILES = *.out *.err *.diff

common_sources = \
	$(top_srcdir)/src/xread.c \
	$(top_srcdir)/src/error.c \
	$(top_srcdir)/src/debug.c \
	$(top_srcdir)/src/pluglist.c \
	$(top_srcdir)/src/hostlist.c \
	$(top_srcdir)/src/list.c \
	$(top_srcdir)/src/hprintf.c \
	$(top_srcdir)/src/xregex.c \
	$(top_srcdir)/src/xmalloc.c \
	$(top_srcdir)/src/argv.c

AM_CFLAGS = -I$(top_srcdir)/src

vpcd_SOURCES = vpcd.c $(common_sources)
vpcd_LDADD = $(LIBFORKPTY)

tpl_SOURCES = tpl.c $(common_sources)
tpl_LDADD = $(LIBFORKPTY)

tregex_SOURCES = tregex.c $(common_sources)
tregex_LDADD = $(LIBFORKPTY)

targv_SOURCES = targv.c $(common_sources)
targv_LDADD = $(LIBFORKPTY)

baytech_SOURCES = baytech.c $(common_sources)
baytech_LDADD = $(LIBFORKPTY)
