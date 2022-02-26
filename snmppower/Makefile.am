AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = -I$(top_srcdir)/libcommon

sbin_PROGRAMS = snmppower

snmppower_SOURCES = snmppower.c
snmppower_LDADD = $(top_builddir)/libcommon/libcommon.a $(LIBNETSNMP) $(LIBFORKPTY)
