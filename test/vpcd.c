/*
 * $Id$
 * $Source$
 *
 * Virtual power controller daemon - configuration defined in "vpc.dev".
 * This daemon spawns multiple threads to implement NUM_THREADS devices
 * listening on consecutive ports starting at BASE_PORT.  Activity
 * is logged to stderr.  This is a vehicle for testing powerman against
 * multiple power control devices. 
 */

#define _GNU_SOURCE  /* for dprintf */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#define NUM_THREADS	8
#define BASE_PORT	8080
#define NUM_PLUGS	8

static struct {
	int plug[NUM_PLUGS];
} dev[NUM_THREADS];

/* 
 * Get a line from file descriptor (minus \r or \n).  Result will always
 * be null-terminated.  If buffer size is exhausted, may return partial line. 
 */
static int _dgets(char *buf, int size, int fd)
{
    char *p = buf;
    int sizeleft = size;
    char c;

    while (sizeleft > 1) { /* leave room for terminating null */
	if (read(fd, &c, 1) <= 0)
	    return -1;
	if (c == '\n')
	    break;
	if (c == '\r')
	    continue;
	*p++ = c;
	sizeleft--;
    }
    *p = '\0';
    return strlen(buf);
}

/*
 * Lptest-like spewage to test buffering/select stuff.
 * This seems to wedge telnet if more than a few hundred lines are sent.
 */
#define SPEW \
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]"
static void _spew(int fd, int linenum)
{
    char buf[80];
    memcpy(buf, SPEW+linenum, strlen(SPEW)-linenum);
    memcpy(buf+strlen(SPEW)-linenum, SPEW, linenum);
    buf[strlen(SPEW)] = '\0';
    dprintf(fd, "%s\n", buf);
}

/*
 * Prompt for a command, parse it, execute it, <repeat>
 */
static void _prompt_loop(int num, int fd)
{
    int seq;
    int i;
    int n1;

    for (seq = 0; ; seq++) {
	char buf[128];

	dprintf(fd, "%d vpc> ", seq);		/* prompt */
	if (_dgets(buf, sizeof(buf), fd) < 0) {
	    printf("%d: lost connection\n", num);
	    break;
	}
	if (strlen(buf) == 0)			/* empty command */
	    continue;
	if (strcmp(buf, "logoff") == 0) {	/* logoff */
	    printf("%d: logoff\n", num);
	    dprintf(fd, "%d OK\n", seq);
	    break;
	}
	if (strcmp(buf, "login") == 0) {	/* logon */
	    printf("%d: logon\n", num);
	    goto ok;
	    break;
	}
	if (strcmp(buf, "stat") == 0) {		/* stat */
	    for (i = 0; i < NUM_PLUGS; i++)
		dprintf(fd, "plug %d: %s\n", i, dev[num].plug[i] ? "ON":"OFF");
	    printf("%d: stat\n", num);
	    goto ok;
	}
	if (sscanf(buf, "spew %d", &n1) == 1) {	/* spew <linecount> */
	    if (n1 <= 0) {
		dprintf(fd, "%d BADVAL: %d\n", seq, n1);
		continue;
	    }
	    for (i = 0; i < n1; i++)
		_spew(fd, i);
	    printf("%d: spew\n", num);
	    goto ok;
	}
	if (sscanf(buf, "on %d", &n1) == 1) {	/* on <plugnum> */
	    if (n1 < 0 || n1 >= NUM_PLUGS) {
		dprintf(fd, "%d BADVAL: %d\n", seq, n1);
		continue;
	    }
	    dev[num].plug[n1] = 1;
	    printf("%d: on %d\n", num, n1);
#if 0
	    printf("XXX: delaying response to on command 10 seconds\n");
	    sleep(10); /* try to trigger expect timeout */
#endif
#if 1
	    printf("XXX: generating UNKONWN response to on command\n");
#else
	    goto ok;
#endif
	}
	if (sscanf(buf, "off %d", &n1) == 1) {	/* off <plugnum> */
	    if (n1 < 0 || n1 >= NUM_PLUGS) {
		dprintf(fd, "%d BADVAL: %d\n", seq, n1);
		continue;
	    }
	    dev[num].plug[n1] = 0;
	    printf("%d: off %d\n", num, n1);
	    goto ok;
	}
	if (strcmp(buf, "on *") == 0) {		/* on * */
	    for (i = 0; i < NUM_PLUGS; i++)
		dev[num].plug[i] = 1;
	    printf("%d: on *\n", num);
	    goto ok;
	}
	if (strcmp(buf, "off *") == 0) {	/* off * */
	    for (i = 0; i < NUM_PLUGS; i++)
		dev[num].plug[i] = 0;
	    printf("%d: off *\n", num);
	    goto ok;
	}
	dprintf(fd, "%d UNKNOWN: %s\n", seq, buf);
	continue;
ok:
	dprintf(fd, "%d OK\n", seq);
    }
}

/*
 * Begin listening on the specified port.
 */
static int _vpc_listen(int port)
{
    struct sockaddr_in saddr;
    int sopt = 1;
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
	perror("socket");
	exit(1);
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sopt, sizeof(sopt)) < 0) {
	perror("setsockopt");
	exit(1);
    }
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd, &saddr, sizeof(struct sockaddr_in)) < 0) {
	printf("port %d:", port);
	perror("bind");
	exit(1);
    }
    if (listen(fd, 10) < 0) {
	perror("listen");
	exit(1);
    }
    return fd;
}

/*
 * A virtual power controller thread.
 * Accept one connection, process it to completion, repeat...
 */
static void *_vpc_thread(void *arg)
{
    int my_threadnum = (int)arg;
    int my_port = BASE_PORT + my_threadnum;
    int listen_fd;

    printf("%d: starting on port %d\n", my_threadnum, my_port);
    listen_fd = _vpc_listen(my_port);

    while (1) {
	    char tmpstr[128];
	    struct sockaddr_in saddr;
	    int saddr_size = sizeof(struct sockaddr_in);
	    int fd;

	    if ((fd = accept(listen_fd, &saddr, &saddr_size)) < 0) {
		perror("accept");
		exit(1);
	    }
	    if (!inet_ntop(AF_INET, &saddr.sin_addr, tmpstr, sizeof(tmpstr))) {
		perror("inet_ntop");
		exit(1);
	    }
	    printf("%d: accept from %s\n", my_threadnum, tmpstr);
	    _prompt_loop(my_threadnum, fd);
	    close(fd);
    }
    return NULL;
}

/*
 * Start NUM_THREADS power controllers on consecutive ports starting at
 * BASE_PORT.  Pause waiting for a signal.  Does not daemonize and logs to
 * stdout.
 */
int
main(int argc, char *argv[])
{
    pthread_attr_t vpc_attr[NUM_THREADS];
    pthread_t vpc_thd[NUM_THREADS];
    int i;

    memset(dev, 0, sizeof(dev));

    for (i = 0; i < NUM_THREADS; i++) {
	pthread_attr_init(&vpc_attr[i]);
	pthread_attr_setdetachstate(&vpc_attr[i], PTHREAD_CREATE_DETACHED);
	pthread_create(&vpc_thd[i], &vpc_attr[i], _vpc_thread, (void *)i);
    }
    if (pause() < 0) {
	perror("pause");
	exit(1);
    }
    exit(0);
}

/*
 * vi:softtabstop=4
 */
