AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = -I$(top_srcdir)/libcommon -I$(top_srcdir)/liblsd

sbin_PROGRAMS = h8power

h8power_SOURCES = \
	error.c \
	error.h \
	h8heartbeat.c \
	h8heartbeat.h \
	h8power.c \
	h8power.h \
	h8proto.c \
	h8proto.h \
	h8sniff.c \
	h8sniff.h \
	hexdump.c \
	hexdump.h \
	main.c \
	serial.c \
	serial.h \
	tcp.c \
	tcp.h \
	wrappers.c \
	wrappers.h

h8power_LDADD = \
	$(top_builddir)/libcommon/libcommon.a \
	$(top_builddir)/liblsd/liblsd.a

man1_MANS = h8power.1
