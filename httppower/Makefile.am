##*****************************************************************************
## $Id: Makefile.am,v 1.72 2005/08/25 22:04:32 achu Exp $
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = -I$(top_srcdir)/libcommon

sbin_PROGRAMS = httppower

httppower_SOURCES = httppower.c

httppower_LDADD = $(top_builddir)/libcommon/libcommon.a $(LIBCURL) $(LIBFORKPTY)
