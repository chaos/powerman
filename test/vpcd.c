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
#define _GNU_SOURCE
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define NUM_THREADS	8
#define BASE_PORT	8080
#define NUM_PLUGS	8

static struct {
	int		plug[NUM_PLUGS];
	pthread_attr_t	attr;
	pthread_t	thd;
	int		logged_in;
} dev[NUM_THREADS];

static int opt_drop_command = 0;
static int opt_bad_response = 0;
static int opt_off_rpc = 0;
static int opt_hung_rpc = 0;
static int opt_soft_off = 0;

static int errcount = 0;

static void _noop_handler(int signum)
{
    printf("vpcd: received signal %d\n", signum);
}

/* 
 * Get a line from file descriptor (minus \r or \n).  Result will always
 * be null-terminated.  If buffer size is exhausted, may return partial line. 
 * Return length of line, -1 on failure, -2 on EOF.
 */
static int _dgets(char *buf, int size, int fd)
{
    char *p = buf;
    int sizeleft = size;
    char c;
    int res;

    while (sizeleft > 1) { /* leave room for terminating null */
	if ((res = read(fd, &c, 1)) <= 0)
	    return (res == 0 ? -2 : res);
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
    int res;

    for (seq = 0; ; seq++) {
	char buf[128];

	if (num == 0 && opt_hung_rpc) {
	    printf("vpcd: vpcd%d is hung\n", num);
	    pause();
	}

	dprintf(fd, "%d vpc> ", seq);		/* prompt */
	res = _dgets(buf, sizeof(buf), fd);
	if (res == -2) {
	    printf("%d: read returned EOF\n", num);
	    break;
	}
	if (res < 0) {
	    printf("%d: %s\n", num, strerror(errno));
	    break;
	}
	if (strlen(buf) == 0)			/* empty command */
	    continue;
	if (strcmp(buf, "logoff") == 0) {	/* logoff */
	    printf("%d: logoff\n", num);
	    dprintf(fd, "%d OK\n", seq);
	    dev[num].logged_in = 0;
	    break;
	}
	if (strcmp(buf, "login") == 0) {	/* logon */
	    printf("%d: logon\n", num);
	    dev[num].logged_in = 1;
	    goto ok;
	}
	if (!dev[num].logged_in) {
	    dprintf(fd, "%d Please login\n", seq);
	    goto noresp;
	}

	if (strcmp(buf, "stat") == 0) {		/* stat */
	    for (i = 0; i < NUM_PLUGS; i++)
		dprintf(fd, "plug %d: %s\n", i, 
				dev[num].plug[i] ? "ON":"OFF");
	    printf("%d: stat\n", num);
	    goto ok;
	}
	if (strcmp(buf, "stat_soft") == 0) {	/* stat_soft */
	    for (i = 0; i < NUM_PLUGS; i++) {
		if (opt_soft_off && num == 0 && i == 0) {
		    printf("vpcd: stat_soft forced OFF for vpcd0 plug 0\n");
		    dprintf(fd, "plug %d: %s\n", i, "OFF");
		} else
		    dprintf(fd, "plug %d: %s\n", i, 
				    dev[num].plug[i] ? "ON":"OFF");
	    }
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
	    printf("%d: on %d\n", num, n1);
	    if (opt_drop_command && errcount++ == 0) {
		printf("vpcd: dropping OK response to 'on' command\n");
		goto noresp;
	    }
	    if (opt_bad_response && errcount++ == 0) {
		printf("vpcd: responding to 'on' with UNKONWN instead of OK\n");
		goto unknown;
	    }
	    dev[num].plug[n1] = 1;
	    goto ok;
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
unknown:
	dprintf(fd, "%d UNKNOWN: %s\n", seq, buf);
	continue;
ok:
	dprintf(fd, "%d OK\n", seq);
noresp:
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

#define OPT_STR "dbhos"
static const struct option long_options[] = {
    {"drop_command", no_argument, 0, 'd'},
    {"bad_response", no_argument, 0, 'b'},
    {"hung_rpc", no_argument, 0, 'h'},
    {"off_rpc", no_argument, 0, 'o'},
    {"soft_off", no_argument, 0, 's'},
    {0, 0, 0, 0}
};
static const struct option *longopts = long_options;

static void _usage(void)
{
    fprintf(stderr, "Usage: vpcd [one option]\n"
"--drop_command       drop response to first \"on\" command\n"
"--bad_response       respond to first \"on\" command with UNKNOWN\n"
"--hung_rpc           vpc0 is unresponsive after connect\n"
"--off_rpc            vpc0 is not started\n"
"--soft_off           vpc0 plug 0 soft status forced to OFF\n");
    exit(1);	    
}

/*
 * Start NUM_THREADS power controllers on consecutive ports starting at
 * BASE_PORT.  Pause waiting for a signal.  Does not daemonize and logs to
 * stdout.
 */
int
main(int argc, char *argv[])
{
    int c;
    int longindex;
    int i;
    int optcount = 0;

    opterr = 0;
    while ((c = getopt_long(argc, argv, OPT_STR, longopts, &longindex)) != -1) {
	switch (c) {
	    case 'd':	/* --drop_command (drop first "on" command) */
		opt_drop_command++;
		optcount++;
		break;
	    case 'b':	/* --bad_response (one bad response to "on") */
		opt_bad_response++;
		optcount++;
		break;
	    case 'h':	/* --hung_rpc (vpc0 nonresponsive) */
		opt_hung_rpc++;
		optcount++;
		break;
	    case 'o':	/* --off_rpc (vpc0 "turned off") */
		opt_off_rpc++;
		optcount++;
		break;
	    case 's':	/* --soft_off (vpc0 plug 0 soft state forced to off) */
		opt_soft_off++;
		optcount++;
		break;
	    default:
		_usage();
	}
    }
    if (optcount > 1)
	_usage();

    if (signal(SIGPIPE, _noop_handler) == SIG_ERR) {
	perror("signal");
	exit(1);
    }

    memset(dev, 0, sizeof(dev));

    for (i = 0; i < NUM_THREADS; i++) {
	if (opt_off_rpc && i == 0) {
	    printf("vpcd: not starting vpcd%d\n", i);
	    continue;
	}
	pthread_attr_init(&dev[i].attr);
	pthread_attr_setdetachstate(&dev[i].attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&dev[i].thd, &dev[i].attr, _vpc_thread, (void *)i);
    }
    if (pause() < 0) {
	perror("pause");
	exit(1);
    }
    printf("exiting \n");
    exit(0);
}

/*
 * vi:softtabstop=4
 */
