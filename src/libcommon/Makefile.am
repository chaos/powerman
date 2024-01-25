AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/liblsd

noinst_LIBRARIES = libcommon.a

libcommon_a_SOURCES = \
	argv.c \
	argv.h \
	debug.c \
	debug.h \
	client_proto.h \
	error.c \
	error.h \
	hprintf.c \
	hprintf.h \
	pluglist.c \
	pluglist.h \
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

TESTS = test_argv

check_PROGRAMS = $(TESTS)

test_argv_SOURCES = test/argv.c
test_argv_LDADD = $(builddir)/libcommon.a
