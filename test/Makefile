CFLAGS= -g -Wall -I../src
VICEOBJS = vicebox.o wrappers.o error.o debug.o

all: vpcd

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

# tcp device
# XXX note vpcd uses ~2 threads per device - adjust ulimit -n accordingly
test64t.conf: mkconf_thd.pl	# 8 threads x 16 plugs each 
	./mkconf_thd.pl 64 >$@
run64t: test64t.conf
	../src/powermand -c $<	# XXX will leave powermand running (kill it!)
	./vpcd -n 8

test4096t.conf: mkconf_thd.pl	# 256 threads x 16 plugs each 
	./mkconf_thd.pl 4096 >$@
run4096t: test4096t.conf
	../src/powermand -c $<	# XXX will leave powermand running (kill it!)
	./vpcd -n 256

test8192t.conf: mkconf_thd.pl	# 512 threads x 16 plugs each 
	./mkconf_thd.pl 8192 >$@
run8192t: test8192t.conf
	#ulimit -n 4096
	../src/powermand -c $<	# XXX will leave powermand running (kill it!)
	./vpcd -n 512

# XXX powerman uses select with fdsets limited to 1024
test16384t.conf: mkconf_thd.pl	# 1024 threads x 16 plugs each 
	./mkconf_thd.pl 16384 >$@
run16384t: test16384t.conf
	#ulimit -n 4096
	../src/powermand -c $<	# XXX will leave powermand running (kill it!)
	./vpcd -n 1024

# "pipe" device
test64.conf: mkconf.pl		# 4 ptys x 16 plugs each
	./mkconf.pl 64 >$@
run64: test64.conf
	../src/powermand -f -c $<

test2048.conf: mkconf.pl	# 128 ptys x 16 plugs each
	./mkconf.pl 2048 >$@
run2048: test2048.conf
	../src/powermand -f -c $<

test4096.conf: mkconf.pl	# 256 ptys x 16 plugs each
	./mkconf.pl 4096 >$@
run4096: test4096.conf
	../src/powermand -f -c $<

test8192.conf: mkconf.pl	# 512 ptys x 16 plugs each
	./mkconf.pl 8192 >$@
run8192: test8192.conf
	../src/powermand -f -c $<

test16384.conf: mkconf.pl	# 1024 ptys x 16 plugs each
	./mkconf.pl 16384 >$@
run16384: test16384.conf
	../src/powermand -f -c $<

clean :
	rm -f *~ *.o
	rm -f vicebox svicebox vpcd
	rm -f *.conf
