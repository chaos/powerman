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
COPTS = -g -Wall
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

COMMON_OBJS = list.o pm_string.o wrappers.o exit_error.o hostlist.o

SERVER_UTIL = $(COMMON_OBJS) daemon_init.o log.o util.o buffer.o pidlock.o

SERVER_OBJS = $(SERVER_UTIL) action.o config.o device.o listener.o  \
	      powermand.o server.o parse_lex.o parse_tab.o

CLIENT_OBJS = $(COMMON_OBJS) powerman.o


all :  powerman powermand tags depend

powermand : $(SERVER_OBJS)
	$(mkinstalldirs)	${srcbin}
	$(CC) $(COPTS) -o $@ $(SERVER_OBJS) $(INC) $(LIB)

powerman : $(CLIENT_OBJS)
	$(mkinstalldirs)	${srcbin}
	$(CC) $(COPTS) -o $@ $(CLIENT_OBJS) $(INC) $(LIB)

.c.o:
	$(CC) $(COPTS) -c $<

parse_lex.c : parse.lex
	$(LEX) -oparse_lex.c parse.lex

parse_lex.o : parse_lex.c parse_tab.h 
	$(CC) $(COPTS) -c -o parse_lex.o parse_lex.c $(INC)


# Fix for annoying "yyval used uninitialized" warning
parse_tab.c parse_tab.h : parse.y
	$(YACC) -d -oparse_tab.c parse.y
	sed s/YYSTYPE\ yyval/YYSTYPE\ yyval\ =\ NULL/ parse_tab.c >foo.c
	mv foo.c parse_tab.c

parse_tab.o : parse_tab.c parse_tab.h powerman.h
	$(CC) $(COPTS) -c -o parse_tab.o parse_tab.c $(INC)

tags: *.[ch]
	-ctags -e *.[chy] *.lex 2>/dev/null

depend:
	-makedepend -- $(COPTS) $(DEFS) *.[ch] 2>/dev/null

clean :
	rm -f *~ *.o .#* parse_* TAGS
	rm -f powerman powermand


# DO NOT DELETE THIS LINE -- make depend depends on it.

action.o: /usr/include/string.h /usr/include/features.h
action.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
action.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
action.o: /usr/include/stdlib.h /usr/include/stdio.h
action.o: /usr/include/bits/types.h /usr/include/libio.h
action.o: /usr/include/_G_config.h /usr/include/wchar.h
action.o: /usr/include/bits/wchar.h /usr/include/gconv.h
action.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
action.o: /usr/include/bits/stdio_lim.h /usr/include/assert.h powerman.h
action.o: pm_string.h wrappers.h /usr/include/sys/time.h /usr/include/time.h
action.o: /usr/include/bits/time.h /usr/include/sys/select.h
action.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
action.o: /usr/include/regex.h /usr/include/netdb.h /usr/include/netinet/in.h
action.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
action.o: /usr/include/bits/socket.h /usr/include/limits.h
action.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
action.o: /usr/include/sys/types.h /usr/include/bits/sockaddr.h
action.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
action.o: /usr/include/bits/in.h /usr/include/endian.h
action.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
action.o: /usr/include/bits/netdb.h exit_error.h buffer.h log.h config.h
action.o: list.h device.h powermand.h action.h server.h
buffer.o: /usr/include/sys/time.h /usr/include/features.h
buffer.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
buffer.o: /usr/include/bits/types.h
buffer.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
buffer.o: /usr/include/time.h /usr/include/bits/time.h
buffer.o: /usr/include/sys/select.h /usr/include/bits/select.h
buffer.o: /usr/include/bits/sigset.h /usr/include/string.h
buffer.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
buffer.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
buffer.o: /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h
buffer.o: /usr/include/bits/stdio_lim.h /usr/include/assert.h powerman.h
buffer.o: exit_error.h wrappers.h /usr/include/regex.h /usr/include/netdb.h
buffer.o: /usr/include/netinet/in.h /usr/include/stdint.h
buffer.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
buffer.o: /usr/include/limits.h
buffer.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
buffer.o: /usr/include/sys/types.h /usr/include/bits/sockaddr.h
buffer.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
buffer.o: /usr/include/bits/in.h /usr/include/endian.h
buffer.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
buffer.o: /usr/include/bits/netdb.h buffer.h log.h
buffer.o: /usr/include/regex.h
config.o: /usr/include/sys/time.h /usr/include/features.h
config.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
config.o: /usr/include/bits/types.h
config.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
config.o: /usr/include/time.h /usr/include/bits/time.h
config.o: /usr/include/sys/select.h /usr/include/bits/select.h
config.o: /usr/include/bits/sigset.h /usr/include/string.h
config.o: /usr/include/assert.h powerman.h exit_error.h list.h config.h
config.o: /usr/include/regex.h /usr/include/sys/types.h pm_string.h device.h
config.o: buffer.h powermand.h /usr/include/stdio.h /usr/include/libio.h
config.o: /usr/include/_G_config.h /usr/include/wchar.h
config.o: /usr/include/bits/wchar.h /usr/include/gconv.h
config.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
config.o: /usr/include/bits/stdio_lim.h action.h wrappers.h
config.o: /usr/include/netdb.h /usr/include/netinet/in.h
config.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
config.o: /usr/include/bits/socket.h /usr/include/limits.h
config.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
config.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
config.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
config.o: /usr/include/endian.h /usr/include/bits/endian.h
config.o: /usr/include/bits/byteswap.h /usr/include/bits/netdb.h
config.o: /usr/include/regex.h /usr/include/sys/time.h
config.o: /usr/include/features.h /usr/include/sys/cdefs.h
config.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
config.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
config.o: /usr/include/time.h /usr/include/bits/time.h
config.o: /usr/include/sys/select.h /usr/include/bits/select.h
config.o: /usr/include/bits/sigset.h /usr/include/sys/types.h list.h
config.o: pm_string.h
daemon_init.o: /usr/include/syslog.h /usr/include/sys/syslog.h
daemon_init.o: /usr/include/features.h /usr/include/sys/cdefs.h
daemon_init.o: /usr/include/gnu/stubs.h
daemon_init.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
daemon_init.o: /usr/include/limits.h
daemon_init.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
daemon_init.o: /usr/include/errno.h /usr/include/bits/errno.h
daemon_init.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
daemon_init.o: /usr/include/signal.h /usr/include/bits/sigset.h
daemon_init.o: /usr/include/bits/types.h
daemon_init.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
daemon_init.o: /usr/include/bits/signum.h /usr/include/unistd.h
daemon_init.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
daemon_init.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
daemon_init.o: /usr/include/sys/types.h /usr/include/time.h
daemon_init.o: /usr/include/string.h /usr/include/stdlib.h
daemon_init.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
daemon_init.o: /usr/include/stdio.h /usr/include/libio.h
daemon_init.o: /usr/include/_G_config.h /usr/include/wchar.h
daemon_init.o: /usr/include/bits/wchar.h /usr/include/gconv.h
daemon_init.o: /usr/include/bits/stdio_lim.h /usr/include/assert.h powerman.h
daemon_init.o: wrappers.h /usr/include/sys/time.h /usr/include/bits/time.h
daemon_init.o: /usr/include/sys/select.h /usr/include/bits/select.h
daemon_init.o: /usr/include/regex.h /usr/include/netdb.h
daemon_init.o: /usr/include/netinet/in.h /usr/include/stdint.h
daemon_init.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
daemon_init.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
daemon_init.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
daemon_init.o: /usr/include/endian.h /usr/include/bits/endian.h
daemon_init.o: /usr/include/bits/byteswap.h /usr/include/bits/netdb.h
daemon_init.o: exit_error.h daemon_init.h
device.o: /usr/include/errno.h /usr/include/features.h
device.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
device.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
device.o: /usr/include/asm/errno.h /usr/include/sys/time.h
device.o: /usr/include/bits/types.h
device.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
device.o: /usr/include/time.h /usr/include/bits/time.h
device.o: /usr/include/sys/select.h /usr/include/bits/select.h
device.o: /usr/include/bits/sigset.h /usr/include/fcntl.h
device.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
device.o: /usr/include/ctype.h /usr/include/endian.h
device.o: /usr/include/bits/endian.h /usr/include/string.h
device.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
device.o: /usr/include/bits/uio.h /usr/include/bits/socket.h
device.o: /usr/include/limits.h
device.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
device.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
device.o: /usr/include/asm/sockios.h /usr/include/syslog.h
device.o: /usr/include/sys/syslog.h
device.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
device.o: /usr/include/assert.h powerman.h list.h config.h
device.o: /usr/include/regex.h pm_string.h device.h buffer.h powermand.h
device.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
device.o: /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h
device.o: /usr/include/bits/stdio_lim.h action.h exit_error.h wrappers.h
device.o: /usr/include/netdb.h /usr/include/netinet/in.h
device.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
device.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
device.o: /usr/include/bits/netdb.h log.h util.h
device.o: /usr/include/regex.h buffer.h
exit_error.o: /usr/include/string.h /usr/include/features.h
exit_error.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
exit_error.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
exit_error.o: /usr/include/assert.h /usr/include/errno.h
exit_error.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
exit_error.o: /usr/include/asm/errno.h /usr/include/stdlib.h
exit_error.o: /usr/include/syslog.h /usr/include/sys/syslog.h
exit_error.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
exit_error.o: /usr/include/stdio.h /usr/include/bits/types.h
exit_error.o: /usr/include/libio.h /usr/include/_G_config.h
exit_error.o: /usr/include/wchar.h /usr/include/bits/wchar.h
exit_error.o: /usr/include/gconv.h /usr/include/bits/stdio_lim.h powerman.h
exit_error.o: list.h exit_error.h
exit_error.o: powerman.h
hostlist.o: /usr/include/string.h /usr/include/features.h
hostlist.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
hostlist.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
hostlist.o: /usr/include/pthread.h /usr/include/sched.h
hostlist.o: /usr/include/bits/types.h /usr/include/time.h
hostlist.o: /usr/include/bits/sched.h /usr/include/signal.h
hostlist.o: /usr/include/bits/sigset.h /usr/include/bits/signum.h
hostlist.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/initspin.h
hostlist.o: /usr/include/bits/sigthread.h /usr/include/stdio.h
hostlist.o: /usr/include/libio.h /usr/include/_G_config.h
hostlist.o: /usr/include/wchar.h /usr/include/bits/wchar.h
hostlist.o: /usr/include/gconv.h
hostlist.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
hostlist.o: /usr/include/bits/stdio_lim.h /usr/include/stdlib.h
hostlist.o: /usr/include/assert.h /usr/include/errno.h
hostlist.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
hostlist.o: /usr/include/asm/errno.h /usr/include/ctype.h
hostlist.o: /usr/include/endian.h /usr/include/bits/endian.h
hostlist.o: /usr/include/sys/param.h /usr/include/limits.h
hostlist.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
hostlist.o: /usr/include/linux/limits.h /usr/include/linux/param.h
hostlist.o: /usr/include/asm/param.h /usr/include/sys/types.h
hostlist.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
hostlist.o: /usr/include/bits/confname.h hostlist.h exit_error.h powerman.h
list.o: /usr/include/assert.h /usr/include/features.h
list.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
list.o: /usr/include/stdlib.h
list.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
list.o: /usr/include/string.h list.h
listener.o: /usr/include/arpa/inet.h /usr/include/features.h
listener.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
listener.o: /usr/include/netinet/in.h /usr/include/stdint.h
listener.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
listener.o: /usr/include/bits/types.h
listener.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
listener.o: /usr/include/bits/socket.h /usr/include/limits.h
listener.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
listener.o: /usr/include/sys/types.h /usr/include/time.h
listener.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
listener.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
listener.o: /usr/include/endian.h /usr/include/bits/endian.h
listener.o: /usr/include/bits/byteswap.h /usr/include/syslog.h
listener.o: /usr/include/sys/syslog.h
listener.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
listener.o: /usr/include/tcpd.h /usr/include/fcntl.h
listener.o: /usr/include/bits/fcntl.h /usr/include/assert.h powerman.h
listener.o: wrappers.h /usr/include/sys/time.h /usr/include/bits/time.h
listener.o: /usr/include/sys/select.h /usr/include/bits/select.h
listener.o: /usr/include/bits/sigset.h /usr/include/regex.h
listener.o: /usr/include/netdb.h /usr/include/bits/netdb.h list.h config.h
listener.o: pm_string.h powermand.h /usr/include/stdio.h /usr/include/libio.h
listener.o: /usr/include/_G_config.h /usr/include/wchar.h
listener.o: /usr/include/gconv.h /usr/include/bits/stdio_lim.h server.h
listener.o: buffer.h exit_error.h log.h listener.h
log.o: /usr/include/assert.h /usr/include/features.h /usr/include/sys/cdefs.h
log.o: /usr/include/gnu/stubs.h /usr/include/fcntl.h
log.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
log.o: /usr/include/bits/types.h
log.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
log.o: /usr/include/time.h /usr/include/sys/stat.h /usr/include/bits/stat.h
log.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
log.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
log.o: /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h
log.o: /usr/include/bits/stdio_lim.h /usr/include/string.h powerman.h
log.o: wrappers.h /usr/include/sys/time.h /usr/include/bits/time.h
log.o: /usr/include/sys/select.h /usr/include/bits/select.h
log.o: /usr/include/bits/sigset.h /usr/include/regex.h /usr/include/netdb.h
log.o: /usr/include/netinet/in.h /usr/include/stdint.h
log.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
log.o: /usr/include/limits.h
log.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
log.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
log.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
log.o: /usr/include/endian.h /usr/include/bits/endian.h
log.o: /usr/include/bits/byteswap.h /usr/include/bits/netdb.h exit_error.h
log.o: pm_string.h buffer.h log.h
log.o: buffer.h /usr/include/regex.h
parse_lex.o: /usr/include/stdio.h /usr/include/features.h
parse_lex.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
parse_lex.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
parse_lex.o: /usr/include/bits/types.h /usr/include/libio.h
parse_lex.o: /usr/include/_G_config.h /usr/include/wchar.h
parse_lex.o: /usr/include/bits/wchar.h /usr/include/gconv.h
parse_lex.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
parse_lex.o: /usr/include/bits/stdio_lim.h /usr/include/unistd.h
parse_lex.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
parse_lex.o: /usr/include/string.h /usr/include/stdlib.h parse_tab.h
parse_lex.o: powerman.h buffer.h /usr/include/regex.h list.h wrappers.h
parse_lex.o: /usr/include/sys/time.h /usr/include/time.h
parse_lex.o: /usr/include/bits/time.h /usr/include/sys/select.h
parse_lex.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
parse_lex.o: /usr/include/netdb.h /usr/include/netinet/in.h
parse_lex.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
parse_lex.o: /usr/include/bits/socket.h /usr/include/limits.h
parse_lex.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
parse_lex.o: /usr/include/sys/types.h /usr/include/bits/sockaddr.h
parse_lex.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
parse_lex.o: /usr/include/bits/in.h /usr/include/endian.h
parse_lex.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
parse_lex.o: /usr/include/bits/netdb.h exit_error.h config.h pm_string.h
parse_lex.o: powermand.h
parse_tab.o: /usr/include/errno.h /usr/include/features.h
parse_tab.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
parse_tab.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
parse_tab.o: /usr/include/asm/errno.h /usr/include/string.h
parse_tab.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
parse_tab.o: /usr/include/stdlib.h /usr/include/assert.h powerman.h list.h
parse_tab.o: config.h /usr/include/regex.h /usr/include/sys/time.h
parse_tab.o: /usr/include/bits/types.h /usr/include/time.h
parse_tab.o: /usr/include/bits/time.h /usr/include/sys/select.h
parse_tab.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
parse_tab.o: /usr/include/sys/types.h pm_string.h device.h buffer.h
parse_tab.o: powermand.h /usr/include/stdio.h /usr/include/libio.h
parse_tab.o: /usr/include/_G_config.h /usr/include/wchar.h
parse_tab.o: /usr/include/bits/wchar.h /usr/include/gconv.h
parse_tab.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
parse_tab.o: /usr/include/bits/stdio_lim.h action.h server.h listener.h
parse_tab.o: exit_error.h log.h wrappers.h /usr/include/netdb.h
parse_tab.o: /usr/include/netinet/in.h /usr/include/stdint.h
parse_tab.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
parse_tab.o: /usr/include/limits.h
parse_tab.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
parse_tab.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
parse_tab.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
parse_tab.o: /usr/include/endian.h /usr/include/bits/endian.h
parse_tab.o: /usr/include/bits/byteswap.h /usr/include/bits/netdb.h
pidlock.o: /usr/include/fcntl.h /usr/include/features.h
pidlock.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
pidlock.o: /usr/include/bits/fcntl.h /usr/include/sys/types.h
pidlock.o: /usr/include/bits/types.h
pidlock.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
pidlock.o: /usr/include/time.h /usr/include/string.h exit_error.h powerman.h
pm_string.o: /usr/include/string.h /usr/include/features.h
pm_string.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
pm_string.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
pm_string.o: /usr/include/ctype.h /usr/include/bits/types.h
pm_string.o: /usr/include/endian.h /usr/include/bits/endian.h
pm_string.o: /usr/include/stdio.h /usr/include/libio.h
pm_string.o: /usr/include/_G_config.h /usr/include/wchar.h
pm_string.o: /usr/include/bits/wchar.h /usr/include/gconv.h
pm_string.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
pm_string.o: /usr/include/bits/stdio_lim.h /usr/include/assert.h powerman.h
pm_string.o: pm_string.h wrappers.h /usr/include/sys/time.h
pm_string.o: /usr/include/time.h /usr/include/bits/time.h
pm_string.o: /usr/include/sys/select.h /usr/include/bits/select.h
pm_string.o: /usr/include/bits/sigset.h /usr/include/regex.h
pm_string.o: /usr/include/netdb.h /usr/include/netinet/in.h
pm_string.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
pm_string.o: /usr/include/bits/socket.h /usr/include/limits.h
pm_string.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
pm_string.o: /usr/include/sys/types.h /usr/include/bits/sockaddr.h
pm_string.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
pm_string.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
pm_string.o: /usr/include/bits/netdb.h exit_error.h
powerman.o: /usr/include/string.h /usr/include/features.h
powerman.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
powerman.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
powerman.o: /usr/include/getopt.h /usr/include/unistd.h
powerman.o: /usr/include/bits/posix_opt.h /usr/include/bits/types.h
powerman.o: /usr/include/bits/confname.h /usr/include/ctype.h
powerman.o: /usr/include/endian.h /usr/include/bits/endian.h
powerman.o: /usr/include/stdlib.h /usr/include/stdio.h /usr/include/libio.h
powerman.o: /usr/include/_G_config.h /usr/include/wchar.h
powerman.o: /usr/include/bits/wchar.h /usr/include/gconv.h
powerman.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
powerman.o: /usr/include/bits/stdio_lim.h /usr/include/assert.h powerman.h
powerman.o: list.h pm_string.h buffer.h /usr/include/regex.h config.h
powerman.o: /usr/include/sys/time.h /usr/include/time.h
powerman.o: /usr/include/bits/time.h /usr/include/sys/select.h
powerman.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
powerman.o: /usr/include/sys/types.h device.h exit_error.h powermand.h
powerman.o: wrappers.h /usr/include/netdb.h /usr/include/netinet/in.h
powerman.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
powerman.o: /usr/include/bits/socket.h /usr/include/limits.h
powerman.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
powerman.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
powerman.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
powerman.o: /usr/include/bits/byteswap.h /usr/include/bits/netdb.h hostlist.h
powermand.o: /usr/include/errno.h /usr/include/features.h
powermand.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
powermand.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
powermand.o: /usr/include/asm/errno.h /usr/include/sys/time.h
powermand.o: /usr/include/bits/types.h
powermand.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
powermand.o: /usr/include/time.h /usr/include/bits/time.h
powermand.o: /usr/include/sys/select.h /usr/include/bits/select.h
powermand.o: /usr/include/bits/sigset.h /usr/include/getopt.h
powermand.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
powermand.o: /usr/include/bits/confname.h /usr/include/signal.h
powermand.o: /usr/include/bits/signum.h /usr/include/string.h
powermand.o: /usr/include/stdlib.h /usr/include/sys/stat.h
powermand.o: /usr/include/bits/stat.h /usr/include/syslog.h
powermand.o: /usr/include/sys/syslog.h
powermand.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
powermand.o: /usr/include/stdio.h /usr/include/libio.h
powermand.o: /usr/include/_G_config.h /usr/include/wchar.h
powermand.o: /usr/include/bits/wchar.h /usr/include/gconv.h
powermand.o: /usr/include/bits/stdio_lim.h /usr/include/assert.h powerman.h
powermand.o: list.h config.h /usr/include/regex.h /usr/include/sys/types.h
powermand.o: pm_string.h device.h buffer.h powermand.h action.h daemon_init.h
powermand.o: listener.h server.h exit_error.h wrappers.h /usr/include/netdb.h
powermand.o: /usr/include/netinet/in.h /usr/include/stdint.h
powermand.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
powermand.o: /usr/include/limits.h
powermand.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
powermand.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
powermand.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
powermand.o: /usr/include/endian.h /usr/include/bits/endian.h
powermand.o: /usr/include/bits/byteswap.h /usr/include/bits/netdb.h log.h
powermand.o: /usr/include/stdio.h /usr/include/features.h
powermand.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
powermand.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
powermand.o: /usr/include/bits/types.h /usr/include/libio.h
powermand.o: /usr/include/_G_config.h /usr/include/wchar.h
powermand.o: /usr/include/bits/wchar.h /usr/include/gconv.h
powermand.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
powermand.o: /usr/include/bits/stdio_lim.h
server.o: /usr/include/string.h /usr/include/features.h
server.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
server.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
server.o: /usr/include/errno.h /usr/include/bits/errno.h
server.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
server.o: /usr/include/assert.h powerman.h list.h config.h
server.o: /usr/include/regex.h /usr/include/sys/time.h
server.o: /usr/include/bits/types.h /usr/include/time.h
server.o: /usr/include/bits/time.h /usr/include/sys/select.h
server.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
server.o: /usr/include/sys/types.h pm_string.h powermand.h
server.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
server.o: /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h
server.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
server.o: /usr/include/bits/stdio_lim.h action.h exit_error.h wrappers.h
server.o: /usr/include/netdb.h /usr/include/netinet/in.h
server.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
server.o: /usr/include/bits/socket.h /usr/include/limits.h
server.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
server.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
server.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
server.o: /usr/include/endian.h /usr/include/bits/endian.h
server.o: /usr/include/bits/byteswap.h /usr/include/bits/netdb.h buffer.h
server.o: log.h server.h util.h
server.o: buffer.h /usr/include/regex.h
util.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
util.o: /usr/include/gnu/stubs.h
util.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
util.o: /usr/include/bits/types.h /usr/include/libio.h
util.o: /usr/include/_G_config.h /usr/include/wchar.h
util.o: /usr/include/bits/wchar.h /usr/include/gconv.h
util.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
util.o: /usr/include/bits/stdio_lim.h powerman.h buffer.h
util.o: /usr/include/regex.h pm_string.h
util.o: buffer.h /usr/include/regex.h pm_string.h
wrappers.o: /usr/include/string.h /usr/include/features.h
wrappers.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
wrappers.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
wrappers.o: /usr/include/errno.h /usr/include/bits/errno.h
wrappers.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
wrappers.o: /usr/include/assert.h /usr/include/unistd.h
wrappers.o: /usr/include/bits/posix_opt.h /usr/include/bits/types.h
wrappers.o: /usr/include/bits/confname.h /usr/include/signal.h
wrappers.o: /usr/include/bits/sigset.h /usr/include/bits/signum.h
wrappers.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
wrappers.o: /usr/include/sys/types.h /usr/include/time.h
wrappers.o: /usr/include/stdlib.h /usr/include/sys/socket.h
wrappers.o: /usr/include/sys/uio.h /usr/include/bits/uio.h
wrappers.o: /usr/include/bits/socket.h /usr/include/limits.h
wrappers.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
wrappers.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
wrappers.o: /usr/include/asm/sockios.h /usr/include/stdio.h
wrappers.o: /usr/include/libio.h /usr/include/_G_config.h
wrappers.o: /usr/include/wchar.h /usr/include/bits/wchar.h
wrappers.o: /usr/include/gconv.h
wrappers.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
wrappers.o: /usr/include/bits/stdio_lim.h powerman.h exit_error.h wrappers.h
wrappers.o: /usr/include/sys/time.h /usr/include/bits/time.h
wrappers.o: /usr/include/sys/select.h /usr/include/bits/select.h
wrappers.o: /usr/include/regex.h /usr/include/netdb.h
wrappers.o: /usr/include/netinet/in.h /usr/include/stdint.h
wrappers.o: /usr/include/bits/wordsize.h /usr/include/bits/in.h
wrappers.o: /usr/include/endian.h /usr/include/bits/endian.h
wrappers.o: /usr/include/bits/byteswap.h /usr/include/bits/netdb.h
wrappers.o: /usr/include/sys/time.h /usr/include/features.h
wrappers.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
wrappers.o: /usr/include/bits/types.h
wrappers.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
wrappers.o: /usr/include/time.h /usr/include/bits/time.h
wrappers.o: /usr/include/sys/select.h /usr/include/bits/select.h
wrappers.o: /usr/include/bits/sigset.h /usr/include/regex.h
wrappers.o: /usr/include/netdb.h /usr/include/netinet/in.h
wrappers.o: /usr/include/stdint.h /usr/include/bits/wchar.h
wrappers.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
wrappers.o: /usr/include/limits.h
wrappers.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
wrappers.o: /usr/include/sys/types.h /usr/include/bits/sockaddr.h
wrappers.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
wrappers.o: /usr/include/bits/in.h /usr/include/endian.h
wrappers.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
wrappers.o: /usr/include/bits/netdb.h
