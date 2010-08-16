##*****************************************************************************
## $Id: Makefile.am,v 1.72 2005/08/25 22:04:32 achu Exp $
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = -I$(top_srcdir)/liblsd -I$(top_srcdir)/libcommon

bin_PROGRAMS = powerman pm

common_ldadd = \
	$(top_builddir)/liblsd/liblsd.a \
	$(top_builddir)/libcommon/libcommon.a

powerman_SOURCES = powerman.c

powerman_LDADD = $(common_ldadd) $(LIBGENDERS) $(LIBFORKPTY) 

pm_SOURCES =

pm: powerman
	$(LN_S) powerman pm
