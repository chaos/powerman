CFLAGS= -g -Wall -I../src
VICEOBJS = vicebox.o wrappers.o error.o debug.o

all: vpcd vicebox svicebox

vpcd: vpcd.o
	$(CC) -o $@ $< -lpthread

vicebox : $(VICEOBJS)
	$(CC) $(CFLAGS) -o $@ $(VICEOBJS)
wrappers.o : ../src/wrappers.c
	$(CC) $(CFLAGS) -c -o $@ $<
error.o : ../src/error.c
	$(CC) $(CFLAGS) -c -o $@ $<
debug.o : ../src/debug.c
	$(CC) $(CFLAGS) -c -o $@ $<

svicebox : svicebox.c
	$(CC) $(CFLAGS) -o $@ $<

test64.conf: mkconf.pl		# 4 ptys x 16 plugs each
	./mkconf.pl 64 >$@
run64: test64.conf
	../src/powermand -f -c $<

test2048.conf: mkconf.pl	# 128 ptys x 16 plugs each
	./mkconf.pl 2048 >$@
run2048: test2048.conf
	../src/powermand -f -c $<

test4000.conf: mkconf.pl	# 250 ptys x 16 plugs each
	./mkconf.pl 4000 >$@
run4000: test4000.conf
	../src/powermand -f -c $<


clean :
	rm -f *~ *.o
	rm -f vicebox svicebox vpcd
	rm -f *.conf
