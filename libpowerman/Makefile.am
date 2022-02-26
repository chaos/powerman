AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = -I$(top_srcdir)/libcommon

lib_LTLIBRARIES = libpowerman.la

libpowerman_la_SOURCES = \
	libpowerman.c

if WITH_PKG_CONFIG
pkgconfig_DATA = libpowerman.pc
endif
include_HEADERS = libpowerman.h
