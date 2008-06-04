check_PROGRAMS = tpl vpcd tregex

TESTS_ENVIRONMENT = env 
TESTS_ENVIRONMENT += "PATH_POWERMAN=$(top_builddir)/src/powerman"
TESTS_ENVIRONMENT += "PATH_POWERMAND=$(top_builddir)/src/powermand"
TESTS_ENVIRONMENT += "PATH_ETC=$(top_builddir)/etc"
TESTS = t00 t01 t02 t03 t04 t05 t06

CLEANFILES = *.out *.err *.diff t04.conf t06.conf

common_sources = \
	$(top_srcdir)/src/xread.c \
	$(top_srcdir)/src/error.c \
	$(top_srcdir)/src/debug.c \
	$(top_srcdir)/src/pluglist.c \
	$(top_srcdir)/src/hostlist.c \
	$(top_srcdir)/src/list.c \
	$(top_srcdir)/src/hprintf.c \
	$(top_srcdir)/src/xregex.c \
	$(top_srcdir)/src/xmalloc.c

AM_CFLAGS = -I$(top_srcdir)/src

vpcd_SOURCES = vpcd.c $(common_sources)
vpcd_LDADD = $(LIBFORKPTY)

tpl_SOURCES = tpl.c $(common_sources)
tpl_LDADD = $(LIBFORKPTY)

tregex_SOURCES = tregex.c $(common_sources)
tregex_LDADD = $(LIBFORKPTY)
