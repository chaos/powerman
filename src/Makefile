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
prefix=	        /usr
etcdir=         /etc
exec_prefix=    ${prefix}
bindir=         ${exec_prefix}/bin
sbindir=        ${exec_prefix}/sbin
libdir=         ${exec_prefix}/lib
packagedir=     ${etcdir}/${PACKAGE}

COMMON_OBJS = list.o pm_string.o wrappers.o exit_error.o hostlist.o

SERVER_UTIL = $(COMMON_OBJS) daemon_init.o util.o buffer.o pidlock.o

SERVER_OBJS = $(SERVER_UTIL) action.o config.o device.o listener.o  \
	      powermand.o server.o parse_lex.o parse_tab.o

CLIENT_OBJS = $(COMMON_OBJS) powerman.o


all :  powerman powermand tags depend

powermand : $(SERVER_OBJS)
	$(CC) $(COPTS) -o $@ $(SERVER_OBJS) $(INC) $(LIB)

powerman : $(CLIENT_OBJS)
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

clean:
	rm -f *~ *.o .\#* parse_* cscope*.out tags TAGS

realclean: clean
	rm -f powerman powermand

# DO NOT DELETE THIS LINE -- make depend depends on it.
