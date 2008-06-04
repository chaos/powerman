/*
 * $Id$
 *
 * vpcd - virtual power controller daemon
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <libgen.h>

#include "hprintf.h"

#define NUM_PLUGS   16
static int plug[NUM_PLUGS];
static int logged_in = 0;

static int opt_foreground = 0;
static int opt_drop_command = 0;
static int opt_bad_response = 0;
static int opt_soft_off = 0;

static int errcount = 0;

static char *prog;

static void _noop_handler(int signum)
{
    fprintf(stderr, "%s: received signal %d\n", prog, signum);
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

    while (sizeleft > 1) {      /* leave room for terminating null */
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
    memcpy(buf, SPEW + linenum, strlen(SPEW) - linenum);
    memcpy(buf + strlen(SPEW) - linenum, SPEW, linenum);
    buf[strlen(SPEW)] = '\0';
    hfdprintf(fd, "%s\n", buf);
}

/*
 * Prompt for a command, parse it, execute it, <repeat>
 */
static void _prompt_loop(int rfd, int wfd)
{
    int seq;
    int i;
    int n1;
    int res;

    for (seq = 0;; seq++) {
        char buf[128];

        hfdprintf(wfd, "%d vpc> ", seq);   /* prompt */
        res = _dgets(buf, sizeof(buf), rfd);
        if (res == -2) {
            fprintf(stderr, "%s: read returned EOF\n", prog);
            break;
        }
        if (res < 0) {
            fprintf(stderr, "%s: %s\n", prog, strerror(errno));
            break;
        }
        if (strlen(buf) == 0)   /* empty command */
            continue;
        if (strcmp(buf, "logoff") == 0) {       /* logoff */
            fprintf(stderr, "%s: logoff\n", prog);
            hfdprintf(wfd, "%d OK\n", seq);
            logged_in = 0;
            break;
        }
        if (strcmp(buf, "login") == 0) {        /* logon */
            fprintf(stderr, "%s: logon\n", prog);
            logged_in = 1;
            goto ok;
        }
        if (!logged_in) {
            hfdprintf(wfd, "%d Please login\n", seq);
            continue;
        }

        if (strcmp(buf, "stat") == 0) { /* stat */
            for (i = 0; i < NUM_PLUGS; i++)
                hfdprintf(wfd, "plug %d: %s\n", i, plug[i] ? "ON" : "OFF");
            fprintf(stderr, "%s: stat\n", prog);
            goto ok;
        }
        if (strcmp(buf, "stat_soft") == 0) {    /* stat_soft */
            for (i = 0; i < NUM_PLUGS; i++) {
                if (opt_soft_off && i == 0) {
                    fprintf(stderr, "%s: stat_soft plug 0 forced OFF\n", prog);
                    hfdprintf(wfd, "plug %d: %s\n", i, "OFF");
                } else
                    hfdprintf(wfd, "plug %d: %s\n", i, plug[i] ? "ON" : "OFF");
            }
            fprintf(stderr, "%s: stat_soft\n", prog);
            goto ok;
        }
        if (sscanf(buf, "spew %d", &n1) == 1) { /* spew <linecount> */
            if (n1 <= 0) {
                hfdprintf(wfd, "%d BADVAL: %d\n", seq, n1);
                continue;
            }
            for (i = 0; i < n1; i++)
                _spew(wfd, i);
            fprintf(stderr, "%s: spew\n", prog);
            goto ok;
        }
        if (sscanf(buf, "on %d", &n1) == 1) {   /* on <plugnum> */
            if (n1 < 0 || n1 >= NUM_PLUGS) {
                hfdprintf(wfd, "%d BADVAL: %d\n", seq, n1);
                continue;
            }
            printf("%s: on %d\n", prog, n1);
            if (opt_drop_command && errcount++ == 0) {
                fprintf(stderr, "%s: on: forced no response\n", prog);
                continue;
            }
            if (opt_bad_response && errcount++ == 0) {
                fprintf(stderr, "%s: on: forced UNKONWN response\n", prog);
                goto unknown;
            }
            plug[n1] = 1;
            goto ok;
        }
        if (sscanf(buf, "off %d", &n1) == 1) {  /* off <plugnum> */
            if (n1 < 0 || n1 >= NUM_PLUGS) {
                hfdprintf(wfd, "%d BADVAL: %d\n", seq, n1);
                continue;
            }
            plug[n1] = 0;
            fprintf(stderr, "%s: off %d\n", prog, n1);
            goto ok;
        }
        if (sscanf(buf, "toggle %d", &n1) == 1) {  /* toggle <plugnum> */
            if (n1 < 0 || n1 >= NUM_PLUGS) {
                hfdprintf(wfd, "%d BADVAL: %d\n", seq, n1);
                continue;
            }
            plug[n1] = plug[n1] == 0 ? 1 : 0;
            fprintf(stderr, "%s: toggle %d\n", prog, n1);
            goto ok;
        }
        if (strcmp(buf, "on *") == 0) { /* on * */
            for (i = 0; i < NUM_PLUGS; i++)
                plug[i] = 1;
            fprintf(stderr, "%s: on *\n", prog);
            goto ok;
        }
        if (strcmp(buf, "off *") == 0) {        /* off * */
            for (i = 0; i < NUM_PLUGS; i++)
                plug[i] = 0;
            fprintf(stderr, "%s: off *\n", prog);
            goto ok;
        }
        if (strcmp(buf, "toggle *") == 0) {
            for (i = 0; i < NUM_PLUGS; i++)
                plug[i] = plug[i] == 0 ? 1 : 0;
            fprintf(stderr, "%s: toggle *\n", prog);
            goto ok;
        }
      unknown:
        hfdprintf(wfd, "%d UNKNOWN: %s\n", seq, buf);
        continue;
      ok:
        hfdprintf(wfd, "%d OK\n", seq);
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
    if (bind(fd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in)) < 0) {
        fprintf(stderr, "port %d:", port);
        perror("bind");
        exit(1);
    }
    if (listen(fd, 10) < 0) {
        perror("listen");
        exit(1);
    }
    return fd;
}

#define OPTIONS "dbs"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {"drop_command", no_argument, 0, 'd'},
    {"bad_response", no_argument, 0, 'b'},
    {"soft_off", no_argument, 0, 's'},
    {0, 0, 0, 0}
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

static void _usage(void)
{
    fprintf(stderr, "Usage: %s [-d|-b|-s]\n", prog);
    fprintf(stderr, " -d,--drop-command  drop first on command\n");
    fprintf(stderr, " -b,--bad-response  respond wierd to first on command\n");
    fprintf(stderr, " -s,--soft-off      plug 0(soft) stuck OFF\n");
    exit(1);
}

/*
 * Start 'num_threads' power controllers on consecutive ports starting at
 * BASE_PORT.  Pause waiting for a signal.  Does not daemonize and logs to
 * stdout.
 */
int main(int argc, char *argv[])
{
    int i, c, optcount = 0;

    prog = basename(argv[0]);

    opterr = 0;
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
        case 'd':  /* --drop-command */
            opt_drop_command++;
            optcount++;
            break;
        case 'b':  /* --bad-response */
            opt_bad_response++;
            optcount++;
            break;
        case 's':  /* --soft-off */
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

    for (i = 0; i < NUM_PLUGS; i++)
        plug[i] = 0;
    _prompt_loop(0, 1);
        
    fprintf(stderr, "exiting \n");
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
