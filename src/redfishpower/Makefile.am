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

TESTS = test_plugs.t

check_PROGRAMS = $(TESTS)

TEST_EXTENSIONS = .t
T_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	$(top_srcdir)/config/tap-driver.sh

test_plugs_t_CPPFLAGS = \
	-I$(top_srcdir)/src/liblsd \
	-I$(top_srcdir)/src/libtap
test_plugs_t_SOURCES = test/plugs.c
test_plugs_t_LDADD = \
	$(builddir)/plugs.o \
	$(top_builddir)/src/libcommon/libcommon.la \
	$(top_builddir)/src/liblsd/liblsd.la \
	$(top_builddir)/src/libczmq/libczmq.la \
	$(top_builddir)/src/libtap/libtap.la
