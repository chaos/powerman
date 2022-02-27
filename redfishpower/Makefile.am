AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = \
	-I$(top_srcdir)/liblsd \
	-I$(top_srcdir)/libcommon

sbin_PROGRAMS = redfishpower

redfishpower_SOURCES = redfishpower.c
redfishpower_LDADD = \
	$(top_builddir)/liblsd/liblsd.a \
	$(top_builddir)/libcommon/libcommon.a \
	$(LIBCURL) \
	$(LIBJANSSON)
