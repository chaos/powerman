CFLAGS = -Wall

all: vpcd

vpcd: vpcd.o
	$(CC) -o $@ $< -lpthread

clean: 
	rm -f vpcd a.out core *.o
