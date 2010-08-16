##*****************************************************************************
## $Id: Makefile.am,v 1.72 2005/08/25 22:04:32 achu Exp $
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = -I$(top_srcdir)/liblsd -I$(top_srcdir)/libcommon

sbin_PROGRAMS = powermand

powermand_SOURCES = \
	arglist.c \
	arglist.h \
	client.c \
	client.h \
	daemon.c \
	daemon.h \
	device.c \
	device.h \
	device_pipe.c \
	device_pipe.h \
	device_private.h \
	device_serial.c \
	device_serial.h \
	device_tcp.c \
	device_tcp.h \
	parse_lex.l \
	parse_tab.y \
	parse_util.c \
	parse_util.h \
	powermand.c

powermand_LDADD = \
	$(top_builddir)/liblsd/liblsd.a \
	$(top_builddir)/libcommon/libcommon.a \
	$(LIBWRAP) $(LIBFORKPTY)

AM_YFLAGS = -d

parse_lex.c : parse_tab.h

CLEANFILES = parse_lex.c parse_tab.c parse_tab.h
