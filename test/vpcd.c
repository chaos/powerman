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
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <libgen.h>

#include "hprintf.h"

#define NUM_PLUGS   16
static int plug[NUM_PLUGS];
static int logged_in = 0;

static char *prog;

static void _noop_handler(int signum)
{
    fprintf(stderr, "%s: received signal %d\n", prog, signum);
}

/*
 * Lptest-like spewage to test buffering/select stuff.
 * This seems to wedge telnet if more than a few hundred lines are sent.
 */
#define SPEW \
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]"
static void _spew(int linenum)
{
    char buf[80];

    memcpy(buf, SPEW + linenum, strlen(SPEW) - linenum);
    memcpy(buf + strlen(SPEW) - linenum, SPEW, linenum);
    buf[strlen(SPEW)] = '\0';
    printf("%s\n", buf);
}

static void _zap_trailing_whitespace(char *s)
{
    while (isspace(s[strlen(s) - 1]))
        s[strlen(s) - 1] = '\0';
}

/*
 * Prompt for a command, parse it, execute it, <repeat>
 */
static void _prompt_loop(void)
{
    int seq, i, n1, res;
    char buf[128];

    for (seq = 0;; seq++) {
        printf("%d vpc> ", seq);   /* prompt */
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;
        _zap_trailing_whitespace(buf);
        if (strlen(buf) == 0)   /* empty command */
            continue;
        if (strcmp(buf, "logoff") == 0) {       /* logoff */
            printf("%d OK\n", seq);
            logged_in = 0;
            break;
        }
        if (strcmp(buf, "login") == 0) {        /* logon */
            logged_in = 1;
            goto ok;
        }
        if (!logged_in) {
            printf("%d Please login\n", seq);
            continue;
        }
        if (strcmp(buf, "stat") == 0) {         /* stat */
            for (i = 0; i < NUM_PLUGS; i++)
                printf("plug %d: %s\n", i, plug[i] ? "ON" : "OFF");
            goto ok;
        }
        if (strcmp(buf, "stat_soft") == 0) {    /* stat_soft */
            for (i = 0; i < NUM_PLUGS; i++)
                printf("plug %d: %s\n", i, plug[i] ? "ON" : "OFF");
            goto ok;
        }
        if (sscanf(buf, "spew %d", &n1) == 1) { /* spew <linecount> */
            if (n1 <= 0) {
                printf("%d BADVAL: %d\n", seq, n1);
                continue;
            }
            for (i = 0; i < n1; i++)
                _spew(i);
            goto ok;
        }
        if (sscanf(buf, "on %d", &n1) == 1) {   /* on <plugnum> */
            if (n1 < 0 || n1 >= NUM_PLUGS) {
                printf("%d BADVAL: %d\n", seq, n1);
                continue;
            }
            plug[n1] = 1;
            goto ok;
        }
        if (sscanf(buf, "off %d", &n1) == 1) {  /* off <plugnum> */
            if (n1 < 0 || n1 >= NUM_PLUGS) {
                printf("%d BADVAL: %d\n", seq, n1);
                continue;
            }
            plug[n1] = 0;
            goto ok;
        }
        if (sscanf(buf, "toggle %d", &n1) == 1) {  /* toggle <plugnum> */
            if (n1 < 0 || n1 >= NUM_PLUGS) {
                printf("%d BADVAL: %d\n", seq, n1);
                continue;
            }
            plug[n1] = plug[n1] == 0 ? 1 : 0;
            goto ok;
        }
        if (strcmp(buf, "on *") == 0) { /* on * */
            for (i = 0; i < NUM_PLUGS; i++)
                plug[i] = 1;
            goto ok;
        }
        if (strcmp(buf, "off *") == 0) {        /* off * */
            for (i = 0; i < NUM_PLUGS; i++)
                plug[i] = 0;
            goto ok;
        }
        if (strcmp(buf, "toggle *") == 0) {
            for (i = 0; i < NUM_PLUGS; i++)
                plug[i] = plug[i] == 0 ? 1 : 0;
            goto ok;
        }
        printf("%d UNKNOWN: %s\n", seq, buf);
        continue;
      ok:
        printf("%d OK\n", seq);
    }
}

/*
 * Start 'num_threads' power controllers on consecutive ports starting at
 * BASE_PORT.  Pause waiting for a signal.  Does not daemonize and logs to
 * stdout.
 */
int main(int argc, char *argv[])
{
    int i;

    prog = basename(argv[0]);

    if (signal(SIGPIPE, _noop_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    for (i = 0; i < NUM_PLUGS; i++)
        plug[i] = 0;
    _prompt_loop();
        
    fprintf(stderr, "exiting \n");
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
