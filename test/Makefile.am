check_PROGRAMS = tpl vpcd

TESTS_ENVIRONMENT = env 
TESTS_ENVIRONMENT += "PATH_POWERMAN=$(top_builddir)/src/powerman"
TESTS_ENVIRONMENT += "PATH_POWERMAND=$(top_builddir)/src/powermand"
TESTS = t00

CLEANFILES = *.out *.diff

common_sources = \
        $(top_srcdir)/src/wrappers.c \
        $(top_srcdir)/src/error.c \
        $(top_srcdir)/src/debug.c \
        $(top_srcdir)/src/pluglist.c \
        $(top_srcdir)/src/hostlist.c \
        $(top_srcdir)/src/list.c \
        $(top_srcdir)/src/hprintf.c

AM_CFLAGS = -I$(top_srcdir)/src

vpcd_SOURCES = vpcd.c
vpcd_LDADD = -lpthread
tpl_SOURCES = tpl.c $(common_sources)
tpl_LDADD = $(LIBFORKPTY)
