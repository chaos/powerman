CFLAGS= -g -O2 -Wall -I../src
VOBJS = vicebox.o wrappers.o error.o

all: vpcd vicebox svicebox

vpcd: vpcd.o
	$(CC) -o $@ $< -lpthread

svicebox : svicebox.c
	$(CC) $(CFLAGS) -o $@ $<

vicebox : $(VOBJS)
	$(CC) $(CFLAGS) -o $@ $(VOBJS)

wrappers.o : ../src/wrappers.c
	$(CC) $(CFLAGS) -c -o $@ $<

error.o : ../src/error.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean :
	rm -f *~ *.o
	rm -f vicebox svicebox vpcd
