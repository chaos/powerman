AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/liblsd

noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	argv.c \
	argv.h \
	debug.c \
	debug.h \
	client_proto.h \
	error.c \
	error.h \
	hprintf.c \
	hprintf.h \
	powerman.h \
	xmalloc.c \
	xmalloc.h \
	xpoll.c \
	xpoll.h \
	xpty.c \
	xpty.h \
	xread.c \
	xread.h \
	xregex.c \
	xregex.h \
	xsignal.c \
	xsignal.h \
	xtime.h \
	xtypes.h

TESTS = \
	test_argv.t \
	test_xregex.t

check_PROGRAMS = $(TESTS)

TEST_EXTENSIONS = .t
T_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	$(top_srcdir)/config/tap-driver.sh

test_argv_t_CPPFLAGS = \
	-I$(top_srcdir)/src/libtap
test_argv_t_SOURCES = test/argv.c
test_argv_t_LDADD = \
	$(builddir)/libcommon.la \
	$(top_builddir)/src/libtap/libtap.la

test_xregex_t_CPPFLAGS = \
	-I$(top_srcdir)/src/libtap
test_xregex_t_SOURCES = test/xregex.c
test_xregex_t_LDADD = \
	$(builddir)/libcommon.la \
	$(top_builddir)/src/libtap/libtap.la
