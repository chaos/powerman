AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = -I$(top_srcdir)/liblsd -I$(top_srcdir)/libcommon

bin_PROGRAMS = powerman pm$(EXEEXT)

common_ldadd = \
	$(top_builddir)/liblsd/liblsd.a \
	$(top_builddir)/libcommon/libcommon.a

powerman_SOURCES = powerman.c

powerman_LDADD = $(common_ldadd) $(LIBGENDERS) $(LIBFORKPTY) 

pm_SOURCES =

pm$(EXEEXT): powerman
	rm -f $@
	$(LN_S) $< $@
