CFLAGS= -g -Wall -I../src
VICEOBJS = vicebox.o wrappers.o error.o debug.o

all: vpcd vicebox svicebox test64.conf test1024.conf

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

test64.conf: mkconf.pl
	./mkconf.pl 64 >$@

test1024.conf: mkconf.pl
	./mkconf.pl 1024 >$@

run64: test64.conf
	../src/powermand -f -c $<

run1024: test1024.conf
	../src/powermand -f -c $<

clean :
	rm -f *~ *.o
	rm -f vicebox svicebox vpcd
	rm -f test64.conf test1024.conf
