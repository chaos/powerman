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

clean :
	rm -f *~ *.o
	rm -f vicebox svicebox vpcd
