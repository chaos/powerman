####################################################################
#   $Id$
####################################################################
#   Copyright (C) 2001-2002 The Regents of the University of California.
#   Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
#   Written by Andrew Uselton (uselton2@llnl.gov>
#   UCRL-CODE-2002-008.
#   
#   This file is part of PowerMan, a remote power management program.
#   For details, see <http://www.llnl.gov/linux/powerman/>.
#   
#   PowerMan is free software; you can redistribute it and/or modify it under
#   the terms of the GNU General Public License as published by the Free
#   Software Foundation; either version 2 of the License, or (at your option)
#   any later version.
#   
#   PowerMan is distributed in the hope that it will be useful, but WITHOUT 
#   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
#   for more details.
#   
#   You should have received a copy of the GNU General Public License along
#   with PowerMan; if not, write to the Free Software Foundation, Inc.,
#   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
####################################################################

PACKAGE= powerman
VERSION= 1.0.0
SHELL=   /bin/sh
MAKE=    /usr/bin/make
CC    = gcc
LEX   = /usr/bin/flex
YACC  = /usr/bin/bison
DEFS=
CFLAGS=
LDFLAGS=
LIB   = -lfl -lwrap -lnsl
INSTALL= /usr/bin/install -c
mkinstalldirs=	 $(SHELL) $(top_srcdir)/aux/mkinstalldirs
COPTS = -g -O2 -Wall
INC   = 

top_srcdir=     ..
srcbin=		${top_srcdir}/bin
prefix=	        /usr
etcdir=         /etc
exec_prefix=    ${prefix}
bindir=         ${exec_prefix}/bin
sbindir=        ${exec_prefix}/sbin
libdir=         ${exec_prefix}/lib
packagedir=     ${etcdir}/${PACKAGE}

SERVER_OBJS = action.o \
              buffer.o \
              config.o \
              daemon_init.o \
              device.o \
              exit_error.o \
              list.o \
              listener.o \
              log.o \
              main.o \
              parse_lex.o \
              parse_tab.o \
              pm_string.o \
              server.o \
              wrappers.o 

CLIENT_OBJS = client.o \
              list.o \
              pm_string.o \
              wrappers.o \
	      exit_error.o

COMMON_HEADERS = list.h \
                 exit_error.h \
                 pm_string.h \
                 buffer.h \
                 log.h \
                 wrappers.h

MODULE_HEADERS = action.h \
                 config.h \
                 daemon_init.h \
                 device.h \
                 listener.h \
                 main.h \
                 server.h \

all :  depend ${srcbin}/powerman ${srcbin}/powermand tags

${srcbin}/powermand : $(SERVER_OBJS)
	$(mkinstalldirs)	${srcbin}
	$(CC) $(COPTS) -o $@ $(SERVER_OBJS) $(INC) $(LIB)

${srcbin}/powerman : $(CLIENT_OBJS)
	$(mkinstalldirs)	${srcbin}
	$(CC) $(COPTS) -o $@ $(CLIENT_OBJS) $(INC) $(LIB)

.c.o:
	$(CC) $(COPTS) -c $<

parse_lex.c : parse.lex
	$(LEX) -oparse_lex.c parse.lex

parse_lex.o : parse_lex.c parse_tab.h $(MODULE_HEADERS)
	$(CC) $(COPTS) -c -o parse_lex.o parse_lex.c $(INC)


# Fix for annoying "yyval used uninitialized" warning
parse_tab.c parse_tab.h : parse.y
	$(YACC) -d -oparse_tab.c parse.y
	sed s/YYSTYPE\ yyval/YYSTYPE\ yyval\ =\ NULL/ parse_tab.c >foo.c
	mv foo.c parse_tab.c

parse_tab.o : parse_tab.c parse_tab.h powerman.h $(MODULE_HEADERS)
	$(CC) $(COPTS) -c -o parse_tab.o parse_tab.c $(INC)

tags: *.[ch]
	-ctags -e *.[chy] *.lex 2>/dev/null

depend:
	-makedepend -- $(COPTS) $(DEFS) *.[ch] 2>/dev/null

clean :
	rm -f *~ *.o .#* parse_* TAGS


# DO NOT DELETE THIS LINE -- make depend depends on it.

action.o: powerman.h /usr/include/stdio.h /usr/include/features.h
action.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
action.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
action.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
action.o: /usr/include/bits/sched.h /usr/include/libio.h
action.o: /usr/include/_G_config.h /usr/include/wchar.h
action.o: /usr/include/bits/wchar.h /usr/include/gconv.h
action.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
action.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
action.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
action.o: /usr/include/time.h /usr/include/endian.h
action.o: /usr/include/bits/endian.h /usr/include/sys/select.h
action.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
action.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
action.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
action.o: /usr/include/netinet/in.h /usr/include/stdint.h
action.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
action.o: /usr/include/limits.h
action.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
action.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
action.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
action.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
action.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
action.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
action.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
action.o: /usr/include/bits/uio.h /usr/include/netdb.h
action.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
action.o: /usr/include/bits/netdb.h /usr/include/stdlib.h
action.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
action.o: /usr/include/xlocale.h /usr/include/alloca.h /usr/include/regex.h
action.o: /usr/include/string.h /usr/include/ctype.h /usr/include/fcntl.h
action.o: /usr/include/bits/fcntl.h /usr/include/signal.h
action.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
action.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
action.o: /usr/include/bits/sigstack.h /usr/include/ucontext.h
action.o: /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h
action.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
action.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
action.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
action.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
action.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
action.o: /usr/include/getopt.h /usr/include/sys/time.h /usr/include/errno.h
action.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
action.o: /usr/include/asm/errno.h /usr/include/arpa/telnet.h list.h config.h
action.o: device.h main.h log.h action.h wrappers.h pm_string.h server.h
action.o: exit_error.h
buffer.o: powerman.h /usr/include/stdio.h /usr/include/features.h
buffer.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
buffer.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
buffer.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
buffer.o: /usr/include/bits/sched.h /usr/include/libio.h
buffer.o: /usr/include/_G_config.h /usr/include/wchar.h
buffer.o: /usr/include/bits/wchar.h /usr/include/gconv.h
buffer.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
buffer.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
buffer.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
buffer.o: /usr/include/time.h /usr/include/endian.h
buffer.o: /usr/include/bits/endian.h /usr/include/sys/select.h
buffer.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
buffer.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
buffer.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
buffer.o: /usr/include/netinet/in.h /usr/include/stdint.h
buffer.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
buffer.o: /usr/include/limits.h
buffer.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
buffer.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
buffer.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
buffer.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
buffer.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
buffer.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
buffer.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
buffer.o: /usr/include/bits/uio.h /usr/include/netdb.h
buffer.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
buffer.o: /usr/include/bits/netdb.h /usr/include/stdlib.h
buffer.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
buffer.o: /usr/include/xlocale.h /usr/include/alloca.h /usr/include/regex.h
buffer.o: /usr/include/string.h /usr/include/ctype.h /usr/include/fcntl.h
buffer.o: /usr/include/bits/fcntl.h /usr/include/signal.h
buffer.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
buffer.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
buffer.o: /usr/include/bits/sigstack.h /usr/include/ucontext.h
buffer.o: /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h
buffer.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
buffer.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
buffer.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
buffer.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
buffer.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
buffer.o: /usr/include/getopt.h /usr/include/sys/time.h /usr/include/errno.h
buffer.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
buffer.o: /usr/include/asm/errno.h /usr/include/arpa/telnet.h exit_error.h
buffer.o: wrappers.h log.h pm_string.h buffer.h
client.o: powerman.h /usr/include/stdio.h /usr/include/features.h
client.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
client.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
client.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
client.o: /usr/include/bits/sched.h /usr/include/libio.h
client.o: /usr/include/_G_config.h /usr/include/wchar.h
client.o: /usr/include/bits/wchar.h /usr/include/gconv.h
client.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
client.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
client.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
client.o: /usr/include/time.h /usr/include/endian.h
client.o: /usr/include/bits/endian.h /usr/include/sys/select.h
client.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
client.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
client.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
client.o: /usr/include/netinet/in.h /usr/include/stdint.h
client.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
client.o: /usr/include/limits.h
client.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
client.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
client.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
client.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
client.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
client.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
client.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
client.o: /usr/include/bits/uio.h /usr/include/netdb.h
client.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
client.o: /usr/include/bits/netdb.h /usr/include/stdlib.h
client.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
client.o: /usr/include/xlocale.h /usr/include/alloca.h /usr/include/regex.h
client.o: /usr/include/string.h /usr/include/ctype.h /usr/include/fcntl.h
client.o: /usr/include/bits/fcntl.h /usr/include/signal.h
client.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
client.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
client.o: /usr/include/bits/sigstack.h /usr/include/ucontext.h
client.o: /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h
client.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
client.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
client.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
client.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
client.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
client.o: /usr/include/getopt.h /usr/include/sys/time.h /usr/include/errno.h
client.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
client.o: /usr/include/asm/errno.h /usr/include/arpa/telnet.h list.h config.h
client.o: device.h exit_error.h pm_string.h buffer.h wrappers.h main.h
client.o: client.h
config.o: powerman.h /usr/include/stdio.h /usr/include/features.h
config.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
config.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
config.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
config.o: /usr/include/bits/sched.h /usr/include/libio.h
config.o: /usr/include/_G_config.h /usr/include/wchar.h
config.o: /usr/include/bits/wchar.h /usr/include/gconv.h
config.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
config.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
config.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
config.o: /usr/include/time.h /usr/include/endian.h
config.o: /usr/include/bits/endian.h /usr/include/sys/select.h
config.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
config.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
config.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
config.o: /usr/include/netinet/in.h /usr/include/stdint.h
config.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
config.o: /usr/include/limits.h
config.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
config.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
config.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
config.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
config.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
config.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
config.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
config.o: /usr/include/bits/uio.h /usr/include/netdb.h
config.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
config.o: /usr/include/bits/netdb.h /usr/include/stdlib.h
config.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
config.o: /usr/include/xlocale.h /usr/include/alloca.h /usr/include/regex.h
config.o: /usr/include/string.h /usr/include/ctype.h /usr/include/fcntl.h
config.o: /usr/include/bits/fcntl.h /usr/include/signal.h
config.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
config.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
config.o: /usr/include/bits/sigstack.h /usr/include/ucontext.h
config.o: /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h
config.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
config.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
config.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
config.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
config.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
config.o: /usr/include/getopt.h /usr/include/sys/time.h /usr/include/errno.h
config.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
config.o: /usr/include/asm/errno.h /usr/include/arpa/telnet.h exit_error.h
config.o: list.h config.h device.h main.h action.h wrappers.h pm_string.h
daemon_init.o: powerman.h /usr/include/stdio.h /usr/include/features.h
daemon_init.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
daemon_init.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
daemon_init.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
daemon_init.o: /usr/include/bits/sched.h /usr/include/libio.h
daemon_init.o: /usr/include/_G_config.h /usr/include/wchar.h
daemon_init.o: /usr/include/bits/wchar.h /usr/include/gconv.h
daemon_init.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
daemon_init.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
daemon_init.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
daemon_init.o: /usr/include/time.h /usr/include/endian.h
daemon_init.o: /usr/include/bits/endian.h /usr/include/sys/select.h
daemon_init.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
daemon_init.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
daemon_init.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
daemon_init.o: /usr/include/netinet/in.h /usr/include/stdint.h
daemon_init.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
daemon_init.o: /usr/include/limits.h
daemon_init.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
daemon_init.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
daemon_init.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
daemon_init.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
daemon_init.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
daemon_init.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
daemon_init.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
daemon_init.o: /usr/include/bits/uio.h /usr/include/netdb.h
daemon_init.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
daemon_init.o: /usr/include/bits/netdb.h /usr/include/stdlib.h
daemon_init.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
daemon_init.o: /usr/include/xlocale.h /usr/include/alloca.h
daemon_init.o: /usr/include/regex.h /usr/include/string.h
daemon_init.o: /usr/include/ctype.h /usr/include/fcntl.h
daemon_init.o: /usr/include/bits/fcntl.h /usr/include/signal.h
daemon_init.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
daemon_init.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
daemon_init.o: /usr/include/bits/sigstack.h /usr/include/ucontext.h
daemon_init.o: /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h
daemon_init.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
daemon_init.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
daemon_init.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
daemon_init.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
daemon_init.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
daemon_init.o: /usr/include/getopt.h /usr/include/sys/time.h
daemon_init.o: /usr/include/errno.h /usr/include/bits/errno.h
daemon_init.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
daemon_init.o: /usr/include/arpa/telnet.h wrappers.h exit_error.h
daemon_init.o: daemon_init.h
device.o: powerman.h /usr/include/stdio.h /usr/include/features.h
device.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
device.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
device.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
device.o: /usr/include/bits/sched.h /usr/include/libio.h
device.o: /usr/include/_G_config.h /usr/include/wchar.h
device.o: /usr/include/bits/wchar.h /usr/include/gconv.h
device.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
device.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
device.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
device.o: /usr/include/time.h /usr/include/endian.h
device.o: /usr/include/bits/endian.h /usr/include/sys/select.h
device.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
device.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
device.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
device.o: /usr/include/netinet/in.h /usr/include/stdint.h
device.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
device.o: /usr/include/limits.h
device.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
device.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
device.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
device.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
device.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
device.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
device.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
device.o: /usr/include/bits/uio.h /usr/include/netdb.h
device.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
device.o: /usr/include/bits/netdb.h /usr/include/stdlib.h
device.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
device.o: /usr/include/xlocale.h /usr/include/alloca.h /usr/include/regex.h
device.o: /usr/include/string.h /usr/include/ctype.h /usr/include/fcntl.h
device.o: /usr/include/bits/fcntl.h /usr/include/signal.h
device.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
device.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
device.o: /usr/include/bits/sigstack.h /usr/include/ucontext.h
device.o: /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h
device.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
device.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
device.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
device.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
device.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
device.o: /usr/include/getopt.h /usr/include/sys/time.h /usr/include/errno.h
device.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
device.o: /usr/include/asm/errno.h /usr/include/arpa/telnet.h list.h config.h
device.o: device.h main.h action.h exit_error.h wrappers.h pm_string.h
device.o: buffer.h log.h
exit_error.o: /usr/include/string.h /usr/include/features.h
exit_error.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
exit_error.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
exit_error.o: /usr/include/xlocale.h /usr/include/assert.h powerman.h
exit_error.o: /usr/include/stdio.h /usr/include/bits/types.h
exit_error.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
exit_error.o: /usr/include/libio.h /usr/include/_G_config.h
exit_error.o: /usr/include/wchar.h /usr/include/bits/wchar.h
exit_error.o: /usr/include/gconv.h
exit_error.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
exit_error.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
exit_error.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
exit_error.o: /usr/include/time.h /usr/include/endian.h
exit_error.o: /usr/include/bits/endian.h /usr/include/sys/select.h
exit_error.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
exit_error.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
exit_error.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
exit_error.o: /usr/include/netinet/in.h /usr/include/stdint.h
exit_error.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
exit_error.o: /usr/include/limits.h
exit_error.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
exit_error.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
exit_error.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
exit_error.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
exit_error.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
exit_error.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
exit_error.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
exit_error.o: /usr/include/bits/uio.h /usr/include/netdb.h
exit_error.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
exit_error.o: /usr/include/bits/netdb.h /usr/include/stdlib.h
exit_error.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
exit_error.o: /usr/include/alloca.h /usr/include/regex.h /usr/include/ctype.h
exit_error.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
exit_error.o: /usr/include/signal.h /usr/include/bits/signum.h
exit_error.o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
exit_error.o: /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h
exit_error.o: /usr/include/ucontext.h /usr/include/sys/ucontext.h
exit_error.o: /usr/include/bits/sigthread.h /usr/include/sys/ioctl.h
exit_error.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
exit_error.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
exit_error.o: /usr/include/sys/ttydefaults.h /usr/include/unistd.h
exit_error.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
exit_error.o: /usr/include/bits/confname.h /usr/include/getopt.h
exit_error.o: /usr/include/sys/time.h /usr/include/errno.h
exit_error.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
exit_error.o: /usr/include/asm/errno.h /usr/include/arpa/telnet.h list.h
exit_error.o: exit_error.h
list.o: /usr/include/assert.h /usr/include/features.h
list.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
list.o: /usr/include/stdlib.h
list.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
list.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
list.o: /usr/include/endian.h /usr/include/bits/endian.h
list.o: /usr/include/xlocale.h /usr/include/sys/types.h
list.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
list.o: /usr/include/bits/sched.h /usr/include/time.h
list.o: /usr/include/sys/select.h /usr/include/bits/select.h
list.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
list.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
list.o: /usr/include/string.h list.h
listener.o: /usr/include/arpa/inet.h /usr/include/features.h
listener.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
listener.o: /usr/include/netinet/in.h /usr/include/stdint.h
listener.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
listener.o: /usr/include/bits/types.h
listener.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
listener.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
listener.o: /usr/include/bits/socket.h /usr/include/limits.h
listener.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
listener.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
listener.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
listener.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
listener.o: /usr/include/sys/types.h /usr/include/time.h
listener.o: /usr/include/endian.h /usr/include/bits/endian.h
listener.o: /usr/include/sys/select.h /usr/include/bits/select.h
listener.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
listener.o: /usr/include/sys/sysmacros.h /usr/include/bits/sockaddr.h
listener.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
listener.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
listener.o: /usr/include/syslog.h /usr/include/sys/syslog.h
listener.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
listener.o: /usr/include/tcpd.h powerman.h /usr/include/stdio.h
listener.o: /usr/include/libio.h /usr/include/_G_config.h
listener.o: /usr/include/wchar.h /usr/include/gconv.h /usr/include/sys/stat.h
listener.o: /usr/include/bits/stat.h /usr/include/sys/socket.h
listener.o: /usr/include/sys/uio.h /usr/include/bits/uio.h
listener.o: /usr/include/netdb.h /usr/include/rpc/netdb.h
listener.o: /usr/include/bits/siginfo.h /usr/include/bits/netdb.h
listener.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
listener.o: /usr/include/bits/waitstatus.h /usr/include/xlocale.h
listener.o: /usr/include/alloca.h /usr/include/regex.h /usr/include/string.h
listener.o: /usr/include/ctype.h /usr/include/fcntl.h
listener.o: /usr/include/bits/fcntl.h /usr/include/signal.h
listener.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
listener.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
listener.o: /usr/include/bits/sigstack.h /usr/include/ucontext.h
listener.o: /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h
listener.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
listener.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
listener.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
listener.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
listener.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
listener.o: /usr/include/getopt.h /usr/include/sys/time.h
listener.o: /usr/include/errno.h /usr/include/bits/errno.h
listener.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
listener.o: /usr/include/arpa/telnet.h daemon_init.h wrappers.h list.h
listener.o: config.h main.h server.h buffer.h exit_error.h log.h pm_string.h
listener.o: listener.h
log.o: /usr/include/assert.h /usr/include/features.h /usr/include/sys/cdefs.h
log.o: /usr/include/gnu/stubs.h powerman.h /usr/include/stdio.h
log.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
log.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
log.o: /usr/include/bits/sched.h /usr/include/libio.h
log.o: /usr/include/_G_config.h /usr/include/wchar.h
log.o: /usr/include/bits/wchar.h /usr/include/gconv.h
log.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
log.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
log.o: /usr/include/sys/syslog.h /usr/include/sys/types.h /usr/include/time.h
log.o: /usr/include/endian.h /usr/include/bits/endian.h
log.o: /usr/include/sys/select.h /usr/include/bits/select.h
log.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
log.o: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
log.o: /usr/include/bits/stat.h /usr/include/netinet/in.h
log.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
log.o: /usr/include/bits/socket.h /usr/include/limits.h
log.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
log.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
log.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
log.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
log.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
log.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
log.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
log.o: /usr/include/bits/uio.h /usr/include/netdb.h /usr/include/rpc/netdb.h
log.o: /usr/include/bits/siginfo.h /usr/include/bits/netdb.h
log.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
log.o: /usr/include/bits/waitstatus.h /usr/include/xlocale.h
log.o: /usr/include/alloca.h /usr/include/regex.h /usr/include/string.h
log.o: /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h
log.o: /usr/include/signal.h /usr/include/bits/signum.h
log.o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
log.o: /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h
log.o: /usr/include/ucontext.h /usr/include/sys/ucontext.h
log.o: /usr/include/bits/sigthread.h /usr/include/sys/ioctl.h
log.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
log.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
log.o: /usr/include/sys/ttydefaults.h /usr/include/unistd.h
log.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
log.o: /usr/include/bits/confname.h /usr/include/getopt.h
log.o: /usr/include/sys/time.h /usr/include/errno.h /usr/include/bits/errno.h
log.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
log.o: /usr/include/arpa/telnet.h wrappers.h exit_error.h pm_string.h
log.o: buffer.h log.h
main.o: powerman.h /usr/include/stdio.h /usr/include/features.h
main.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
main.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
main.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
main.o: /usr/include/bits/sched.h /usr/include/libio.h
main.o: /usr/include/_G_config.h /usr/include/wchar.h
main.o: /usr/include/bits/wchar.h /usr/include/gconv.h
main.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
main.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
main.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
main.o: /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h
main.o: /usr/include/sys/select.h /usr/include/bits/select.h
main.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
main.o: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
main.o: /usr/include/bits/stat.h /usr/include/netinet/in.h
main.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
main.o: /usr/include/bits/socket.h /usr/include/limits.h
main.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
main.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
main.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
main.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
main.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
main.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
main.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
main.o: /usr/include/bits/uio.h /usr/include/netdb.h /usr/include/rpc/netdb.h
main.o: /usr/include/bits/siginfo.h /usr/include/bits/netdb.h
main.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
main.o: /usr/include/bits/waitstatus.h /usr/include/xlocale.h
main.o: /usr/include/alloca.h /usr/include/regex.h /usr/include/string.h
main.o: /usr/include/ctype.h /usr/include/fcntl.h /usr/include/bits/fcntl.h
main.o: /usr/include/signal.h /usr/include/bits/signum.h
main.o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
main.o: /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h
main.o: /usr/include/ucontext.h /usr/include/sys/ucontext.h
main.o: /usr/include/bits/sigthread.h /usr/include/sys/ioctl.h
main.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
main.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
main.o: /usr/include/sys/ttydefaults.h /usr/include/unistd.h
main.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
main.o: /usr/include/bits/confname.h /usr/include/getopt.h
main.o: /usr/include/sys/time.h /usr/include/errno.h
main.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
main.o: /usr/include/asm/errno.h /usr/include/arpa/telnet.h list.h config.h
main.o: device.h main.h action.h daemon_init.h listener.h server.h
main.o: exit_error.h wrappers.h log.h
pm_string.o: powerman.h /usr/include/stdio.h /usr/include/features.h
pm_string.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
pm_string.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
pm_string.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
pm_string.o: /usr/include/bits/sched.h /usr/include/libio.h
pm_string.o: /usr/include/_G_config.h /usr/include/wchar.h
pm_string.o: /usr/include/bits/wchar.h /usr/include/gconv.h
pm_string.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
pm_string.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
pm_string.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
pm_string.o: /usr/include/time.h /usr/include/endian.h
pm_string.o: /usr/include/bits/endian.h /usr/include/sys/select.h
pm_string.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
pm_string.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
pm_string.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
pm_string.o: /usr/include/netinet/in.h /usr/include/stdint.h
pm_string.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
pm_string.o: /usr/include/limits.h
pm_string.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
pm_string.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
pm_string.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
pm_string.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
pm_string.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
pm_string.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
pm_string.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
pm_string.o: /usr/include/bits/uio.h /usr/include/netdb.h
pm_string.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
pm_string.o: /usr/include/bits/netdb.h /usr/include/stdlib.h
pm_string.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
pm_string.o: /usr/include/xlocale.h /usr/include/alloca.h
pm_string.o: /usr/include/regex.h /usr/include/string.h /usr/include/ctype.h
pm_string.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
pm_string.o: /usr/include/signal.h /usr/include/bits/signum.h
pm_string.o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
pm_string.o: /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h
pm_string.o: /usr/include/ucontext.h /usr/include/sys/ucontext.h
pm_string.o: /usr/include/bits/sigthread.h /usr/include/sys/ioctl.h
pm_string.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
pm_string.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
pm_string.o: /usr/include/sys/ttydefaults.h /usr/include/unistd.h
pm_string.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
pm_string.o: /usr/include/bits/confname.h /usr/include/getopt.h
pm_string.o: /usr/include/sys/time.h /usr/include/errno.h
pm_string.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
pm_string.o: /usr/include/asm/errno.h /usr/include/arpa/telnet.h buffer.h
pm_string.o: wrappers.h exit_error.h pm_string.h
powerman.o: /usr/include/stdio.h /usr/include/features.h
powerman.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
powerman.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
powerman.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
powerman.o: /usr/include/bits/sched.h /usr/include/libio.h
powerman.o: /usr/include/_G_config.h /usr/include/wchar.h
powerman.o: /usr/include/bits/wchar.h /usr/include/gconv.h
powerman.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
powerman.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
powerman.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
powerman.o: /usr/include/time.h /usr/include/endian.h
powerman.o: /usr/include/bits/endian.h /usr/include/sys/select.h
powerman.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
powerman.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
powerman.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
powerman.o: /usr/include/netinet/in.h /usr/include/stdint.h
powerman.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
powerman.o: /usr/include/limits.h
powerman.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
powerman.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
powerman.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
powerman.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
powerman.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
powerman.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
powerman.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
powerman.o: /usr/include/bits/uio.h /usr/include/netdb.h
powerman.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
powerman.o: /usr/include/bits/netdb.h /usr/include/stdlib.h
powerman.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
powerman.o: /usr/include/xlocale.h /usr/include/alloca.h /usr/include/regex.h
powerman.o: /usr/include/string.h /usr/include/ctype.h /usr/include/fcntl.h
powerman.o: /usr/include/bits/fcntl.h /usr/include/signal.h
powerman.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
powerman.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
powerman.o: /usr/include/bits/sigstack.h /usr/include/ucontext.h
powerman.o: /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h
powerman.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
powerman.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
powerman.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
powerman.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
powerman.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
powerman.o: /usr/include/getopt.h /usr/include/sys/time.h
powerman.o: /usr/include/errno.h /usr/include/bits/errno.h
powerman.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
powerman.o: /usr/include/arpa/telnet.h
server.o: powerman.h /usr/include/stdio.h /usr/include/features.h
server.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
server.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
server.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
server.o: /usr/include/bits/sched.h /usr/include/libio.h
server.o: /usr/include/_G_config.h /usr/include/wchar.h
server.o: /usr/include/bits/wchar.h /usr/include/gconv.h
server.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
server.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
server.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
server.o: /usr/include/time.h /usr/include/endian.h
server.o: /usr/include/bits/endian.h /usr/include/sys/select.h
server.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
server.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
server.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
server.o: /usr/include/netinet/in.h /usr/include/stdint.h
server.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
server.o: /usr/include/limits.h
server.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
server.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
server.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
server.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
server.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
server.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
server.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
server.o: /usr/include/bits/uio.h /usr/include/netdb.h
server.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
server.o: /usr/include/bits/netdb.h /usr/include/stdlib.h
server.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
server.o: /usr/include/xlocale.h /usr/include/alloca.h /usr/include/regex.h
server.o: /usr/include/string.h /usr/include/ctype.h /usr/include/fcntl.h
server.o: /usr/include/bits/fcntl.h /usr/include/signal.h
server.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
server.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
server.o: /usr/include/bits/sigstack.h /usr/include/ucontext.h
server.o: /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h
server.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
server.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
server.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
server.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
server.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
server.o: /usr/include/getopt.h /usr/include/sys/time.h /usr/include/errno.h
server.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
server.o: /usr/include/asm/errno.h /usr/include/arpa/telnet.h list.h config.h
server.o: main.h action.h exit_error.h wrappers.h pm_string.h buffer.h log.h
server.o: server.h
wrappers.o: /usr/include/assert.h /usr/include/features.h
wrappers.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h powerman.h
wrappers.o: /usr/include/stdio.h
wrappers.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
wrappers.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
wrappers.o: /usr/include/bits/sched.h /usr/include/libio.h
wrappers.o: /usr/include/_G_config.h /usr/include/wchar.h
wrappers.o: /usr/include/bits/wchar.h /usr/include/gconv.h
wrappers.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
wrappers.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
wrappers.o: /usr/include/sys/syslog.h /usr/include/sys/types.h
wrappers.o: /usr/include/time.h /usr/include/endian.h
wrappers.o: /usr/include/bits/endian.h /usr/include/sys/select.h
wrappers.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
wrappers.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
wrappers.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
wrappers.o: /usr/include/netinet/in.h /usr/include/stdint.h
wrappers.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
wrappers.o: /usr/include/limits.h
wrappers.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
wrappers.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
wrappers.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
wrappers.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
wrappers.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
wrappers.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
wrappers.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
wrappers.o: /usr/include/bits/uio.h /usr/include/netdb.h
wrappers.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
wrappers.o: /usr/include/bits/netdb.h /usr/include/stdlib.h
wrappers.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
wrappers.o: /usr/include/xlocale.h /usr/include/alloca.h /usr/include/regex.h
wrappers.o: /usr/include/string.h /usr/include/ctype.h /usr/include/fcntl.h
wrappers.o: /usr/include/bits/fcntl.h /usr/include/signal.h
wrappers.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
wrappers.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
wrappers.o: /usr/include/bits/sigstack.h /usr/include/ucontext.h
wrappers.o: /usr/include/sys/ucontext.h /usr/include/bits/sigthread.h
wrappers.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
wrappers.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
wrappers.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
wrappers.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
wrappers.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
wrappers.o: /usr/include/getopt.h /usr/include/sys/time.h
wrappers.o: /usr/include/errno.h /usr/include/bits/errno.h
wrappers.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
wrappers.o: /usr/include/arpa/telnet.h exit_error.h buffer.h wrappers.h
