####################################################################
#   $Id$
####################################################################

PACKAGE=	powerman
SHELL=		/bin/sh
CC=		gcc

DEFS=		# -DNDEBUG
#CFLAGS= 	-O -Wall
#LDFLAGS=	-s
CFLAGS= 	-g -Wall
LDFLAGS=	

LIBS=		-lfl -lwrap -lnsl
COPTS=		$(CFLAGS) $(CPPFLAGS) $(DEFS)
INSTALL=	/usr/bin/install -c
mkinstalldirs=	$(SHELL) $(top_srcdir)/aux/mkinstalldirs

MAKE=		/usr/bin/make
LEX=		/usr/bin/flex
YACC=		/usr/bin/bison

top_srcdir=     ..
prefix=	        /usr
etcdir=         /etc
exec_prefix=    ${prefix}
bindir=         ${exec_prefix}/bin
sbindir=        ${exec_prefix}/sbin
libdir=         ${exec_prefix}/lib
packagedir=     ${etcdir}/${PACKAGE}

COMMON_OBJS = wrappers.o error.o hostlist.o debug.o

SERVER_UTIL = $(COMMON_OBJS) daemon.o cbuf.o

SERVER_OBJS = $(SERVER_UTIL) list.o config.o device.o \
	      powermand.o client.o parse_lex.o parse_tab.o

CLIENT_OBJS = $(COMMON_OBJS) powerman.o


all:  powerman powermand

powermand: $(SERVER_OBJS)
	$(CC) $(COPTS) -o $@ $(SERVER_OBJS) $(LIBS)

powerman: $(CLIENT_OBJS)
	$(CC) $(COPTS) -o $@ $(CLIENT_OBJS) $(LIBS)

.c.o:
	$(CC) $(COPTS) -c $<

parse_lex.c: parse.lex
	$(LEX) -oparse_lex.c parse.lex

parse_lex.o: parse_lex.c parse_tab.h 
	$(CC) $(COPTS) -c -o parse_lex.o parse_lex.c

# Fix for annoying "yyval used uninitialized" warning
parse_tab.c parse_tab.h: parse.y
	$(YACC) -d -oparse_tab.c parse.y
	sed s/YYSTYPE\ yyval/YYSTYPE\ yyval\ =\ NULL/ parse_tab.c >foo.c
	mv foo.c parse_tab.c

parse_tab.o: parse_tab.c parse_tab.h powerman.h
	$(CC) $(COPTS) -c -o parse_tab.o parse_tab.c

list.o: list.c
	$(CC) $(COPTS) -DWITH_OOMF -o $@ -c $<

tags: *.[ch]
	-ctags -e *.[chy] *.lex 2>/dev/null

depend:
	-makedepend -- $(COPTS) $(DEFS) *.[ch] 2>/dev/null

clean:
	-rm -f *~ *.o .\#* parse_*
	-rm -f powerman powermand

realclean: clean
	-rm -f cscope*.out tags TAGS

# DO NOT DELETE THIS LINE -- make depend depends on it.
