AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = -I$(top_srcdir)/libcommon

sbin_PROGRAMS = httppower

httppower_SOURCES = httppower.c
httppower_LDADD = $(top_builddir)/libcommon/libcommon.a $(LIBCURL) $(LIBFORKPTY)
