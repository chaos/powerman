##*****************************************************************************
## $Id: Makefile.am,v 1.72 2005/08/25 22:04:32 achu Exp $
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = -I$(top_srcdir)/libcommon

lib_LTLIBRARIES = libpowerman.la

libpowerman_la_SOURCES = \
	libpowerman.c

if WITH_PKG_CONFIG
pkgconfig_DATA = libpowerman.pc
endif
include_HEADERS = libpowerman.h
