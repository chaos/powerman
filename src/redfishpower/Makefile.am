AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/liblsd \
	-I$(top_srcdir)/src/libczmq \
	-I$(top_srcdir)/src/libcommon

sbin_PROGRAMS = redfishpower

redfishpower_SOURCES = \
	redfishpower.c \
	plugs.h \
	plugs.c

redfishpower_LDADD = \
	$(top_builddir)/src/liblsd/liblsd.la \
	$(top_builddir)/src/libczmq/libczmq.la \
	$(top_builddir)/src/libcommon/libcommon.la \
	$(LIBCURL) \
	$(LIBJANSSON)
