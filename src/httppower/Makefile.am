AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/libcommon

sbin_PROGRAMS = httppower

httppower_SOURCES = httppower.c
httppower_LDADD = \
	$(top_builddir)/src/libcommon/libcommon.la \
	$(LIBCURL) \
	$(LIBFORKPTY)
