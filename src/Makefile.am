##*****************************************************************************
## $Id: Makefile.am,v 1.72 2005/08/25 22:04:32 achu Exp $
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

AM_CFLAGS = @GCCWARN@ 

if WITH_HTTPPOWER
HTTPPOWER = httppower
endif

bin_PROGRAMS = powerman pm
sbin_PROGRAMS = powermand plmpower $(HTTPPOWER)

noinst_HEADERS = \
	arglist.h \
	argv.h \
	cbuf.h \
	client.h \
        client_proto.h \
	daemon.h \
	debug.h \
	device.h \
	device_pipe.h \
	device_private.h \
	device_serial.h \
	device_tcp.h \
	error.h \
	hash.h \
	hostlist.h \
	hprintf.h \
	list.h \
	parse_util.h \
	pluglist.h \
	powerman.h \
	xread.h \
	xpoll.h \
	xmalloc.h \
	xregex.h \
	xtime.h \
	xtypes.h \
	xpty.h \
	xsignal.h

COMMON_SOURCE = \
	debug.c \
	error.c \
	hostlist.c \
	list.c \
	hprintf.c \
	argv.c \
	xread.c \
	xpoll.c \
	xmalloc.c \
	xregex.c \
	xpty.c \
	xsignal.c

powerman_SOURCES = powerman.c $(COMMON_SOURCE)

powerman_LDADD = $(LIBGENDERS) $(LIBFORKPTY)

if WITH_HTTPPOWER
httppower_SOURCES = httppower.c $(COMMON_SOURCE)

httppower_LDADD = $(LIBCURL) $(LIBFORKPTY)
endif

plmpower_SOURCES = plmpower.c $(COMMON_SOURCE)

plmpower_LDADD = $(LIBFORKPTY)

powermand_SOURCES = \
	powermand.c \
	arglist.c \
	cbuf.c \
	client.c \
	daemon.c \
	device.c \
	device_pipe.c \
	device_serial.c \
	device_tcp.c \
	hash.c \
	parse_lex.l \
	parse_tab.y \
	parse_util.c \
	pluglist.c \
	$(COMMON_SOURCE)

powermand_LDADD = $(LIBWRAP) $(LIBFORKPTY)

AM_YFLAGS = -d

pm_SOURCES =

pm: powerman
	$(LN_S) powerman pm
