####################################################################
#   $Id$
####################################################################

PROJECT=	powerman
SHELL=		/bin/sh
CC=		gcc
MAKE=		/usr/bin/make
LEX=		/usr/bin/flex
YACC=		/usr/bin/bison

VERSTR=		$(shell if test -n "$(VERSION)"; then \
		  if test -n "$(RELEASE)"; then \
                    echo $(PROJECT)-$(VERSION)-$(RELEASE); \
                  else \
                    echo $(PROJECT)-$(VERSION); fi; \
		else echo development; fi)

DEFS=		-DHAVE_CONFIG_H -DPOWERMAN_VERSION=\"$(VERSTR)\"
#DEFS+=		-DNDEBUG
CFLAGS= 	-g -Wall $(DEFS)

LIBS=		-lfl -lwrap -lnsl

COMMON_OBJS = wrappers.o error.o hostlist.o debug.o

SERVER_UTIL = $(COMMON_OBJS) daemon.o cbuf.o

SERVER_OBJS = $(SERVER_UTIL) list.o parse_util.o device.o \
	      powermand.o client.o parse_lex.o parse_tab.o \
              device_telnet.o device_serial.o device_tcp.o

CLIENT_OBJS = $(COMMON_OBJS) powerman.o


all:  powerman powermand

powermand: $(SERVER_OBJS)
	$(CC) -o $@ $(SERVER_OBJS) $(LIBS)

powerman: $(CLIENT_OBJS)
	$(CC) -o $@ $(CLIENT_OBJS) $(LIBS)

parse_lex.c: parse.lex
	$(LEX) -oparse_lex.c parse.lex

parse_lex.o: parse_lex.c parse_tab.h 
	$(CC) $(CFLAGS) -c -o parse_lex.o parse_lex.c

# Fix for annoying "yyval used uninitialized" warning
parse_tab.c parse_tab.h: parse.y
	$(YACC) -d -oparse_tab.c parse.y
	sed s/YYSTYPE\ yyval/YYSTYPE\ yyval\ =\ NULL/ parse_tab.c >foo.c
	mv foo.c parse_tab.c

parse_tab.o: parse_tab.c parse_tab.h powerman.h
	$(CC) $(CFLAGS) -c -o parse_tab.o parse_tab.c

tags: *.[ch]
	-ctags -e *.[chy] *.lex 2>/dev/null

depend:
	-makedepend -- $(CFLAGS) *.[ch] 2>/dev/null

clean:
	-rm -f *~ *.o .\#* parse_tab.c parse_tab.h parse_lex.c
	-rm -f powerman powermand

realclean: clean
	-rm -f cscope*.out tags TAGS

# DO NOT DELETE THIS LINE -- make depend depends on it.
