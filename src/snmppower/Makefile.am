AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/libcommon

sbin_PROGRAMS = snmppower

snmppower_SOURCES = snmppower.c
snmppower_LDADD = \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(LIBNETSNMP) \
	$(LIBFORKPTY)
