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
              wrappers.o 

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

all :  powermand powerman tags

powermand : $(SERVER_OBJS)
	$(CC) $(COPTS) -o ${srcbin}/powermand $(SERVER_OBJS) $(INC) $(LIB)

powerman.h : $(COMMON_HEADERS)


action.o : action.c powerman.h $(MODULE_HEADERS)
	$(CC) $(COPTS) -c -o $@ $< $(INC)

buffer.o : buffer.c powerman.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

config.o : config.c powerman.h $(MODULE_HEADERS)
	$(CC) $(COPTS) -c -o $@ $< $(INC)

daemon_init.o : daemon_init.c powerman.h $(MODULE_HEADERS)
	$(CC) $(COPTS) -c -o $@ $< $(INC)

device.o : device.c powerman.h $(MODULE_HEADERS)
	$(CC) $(COPTS) -c -o $@ $< $(INC)

exit_error.o : exit_error.c powerman.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

list.o : list.c list.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

listener.o : listener.c powerman.h $(MODULE_HEADERS)
	$(CC) $(COPTS) -c -o $@ $< $(INC)

log.o : log.c powerman.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

main.o : main.c powerman.h $(MODULE_HEADERS)
	$(CC) $(COPTS) -c -o $@ $< $(INC)

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

pm_string.o : pm_string.c powerman.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

server.o : server.c powerman.h $(MODULE_HEADERS)
	$(CC) $(COPTS) -c -o $@ $< $(INC)

wrappers.o : wrappers.c powerman.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

powerman : $(CLIENT_OBJS)
	$(CC) $(COPTS) -o ${srcbin}/powerman $(CLIENT_OBJS) $(INC) $(LIB)

client.o : client.c powerman.h client.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

tags: *.[ch]
	-ctags -e *.[chy] *.lex 2>/dev/null

clean :
	rm -f *~ *.o .#* parse_* TAGS





