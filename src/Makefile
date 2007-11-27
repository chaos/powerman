####################################################################
#   $Id$
####################################################################

PROJECT=	powerman
VERSION=	development
CC=		gcc
LEX=		flex
YACC=		bison

DEFS=		-DHAVE_CONFIG_H -DPOWERMAN_VERSION=\"$(VERSION)\"
CFLAGS= 	-g -Wall $(DEFS) 
#CFLAGS+=	-pg
LDADD=		-lfl -lwrap -lnsl -lutil

COMMON_OBJS=	wrappers.o error.o hostlist.o debug.o
SERVER_OBJS=	$(COMMON_OBJS) daemon.o cbuf.o hprintf.o argv.o hash.o \
		list.o parse_util.o device.o powermand.o client.o \
		parse_lex.o parse_tab.o device_serial.o device_tcp.o \
		device_pipe.o arglist.o pluglist.o
CLIENT_OBJS= 	$(COMMON_OBJS) powerman.o
HTTP_OBJS=	$(COMMON_OBJS) httppower.o argv.o


all:  powerman powermand #httppower

powermand: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS) $(LDADD)

powerman: $(CLIENT_OBJS)
	$(CC) -o $@ $(CLIENT_OBJS) $(LDADD)

httppower: $(HTTP_OBJS)
	$(CC) -o $@ $(HTTP_OBJS) -lcurl -lreadline -ltermcap

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
	ctags -e *.[chy] *.lex 2>/dev/null

depend:
	makedepend -- $(CFLAGS) *.[ch] 2>/dev/null

clean:
	rm -f *~ *.o .\#* 
	rm -f powerman powermand httppower
	rm -f parse_tab.c parse_tab.h parse_lex.c
	rm -f gmon.out

realclean: clean
	-rm -f cscope*.out tags TAGS

# DO NOT DELETE THIS LINE -- make depend depends on it.
