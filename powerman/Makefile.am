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
