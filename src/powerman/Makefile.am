AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/liblsd \
	-I$(top_srcdir)/src/libcommon

bin_PROGRAMS = \
	powerman \
	pm$(EXEEXT)

sbin_PROGRAMS = \
	powermand

lib_LTLIBRARIES = \
	libpowerman.la

include_HEADERS = \
	libpowerman.h

powerman_SOURCES = \
	powerman.c

powerman_LDADD = \
	$(top_builddir)/src/liblsd/liblsd.a \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(LIBGENDERS) \
	$(LIBFORKPTY)

pm_SOURCES =

pm$(EXEEXT): powerman
	rm -f $@
	$(LN_S) $< $@

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
	$(top_builddir)/src/liblsd/liblsd.a \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(LIBWRAP) $(LIBFORKPTY)

AM_YFLAGS = -d

parse_lex.c : parse_tab.h

CLEANFILES = \
	parse_lex.c \
	parse_tab.c \
	parse_tab.h

libpowerman_la_SOURCES = \
	libpowerman.c

if WITH_PKG_CONFIG
pkgconfig_DATA = libpowerman.pc
endif
