/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include "list.h"
#include "hostlist.h"
#include "parse_util.h"
#include "xmalloc.h"
#include "xpoll.h"
#include "xsignal.h"
#include "pluglist.h"
#include "device.h"
#include "daemon.h"
#include "client.h"
#include "error.h"
#include "debug.h"
#include "hprintf.h"
#include "powerman.h"

/* prototypes */
static void _usage(char *prog);
static void _version(void);
static void _noop_handler(int signum);
static void _exit_handler(int signum);
static void _select_loop(void);

#define OPTIONS "c:fhd:VsY"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {"conf",            required_argument,  0, 'c'},
    {"foreground",      no_argument,        0, 'f'},
    {"help",            no_argument,        0, 'h'},
    {"debug",           required_argument,  0, 'd'},
    {"version",         no_argument,        0, 'V'},
    {"stdio",           no_argument,        0, 's'},
    {"short-circuit-delay", no_argument,    0, 'Y'},
    {0, 0, 0, 0}
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int main(int argc, char **argv)
{
    int c;
    char *config_filename = NULL;
    bool daemonize = true;
    bool use_stdio = false;
    bool short_circuit_delay = false;

    /* parse command line options */
    err_init(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
        case 'Y': /* --short-circuit-delay */
            short_circuit_delay = true;
            break;
        case 'c': /* --conf */
            if (!config_filename)
                config_filename = xstrdup(optarg);
            break;
        case 'f': /* --foreground */
            daemonize = false;
            break;
        case 'd': /* --debug */
            {
                unsigned long val = strtol(optarg, NULL, 0);

                if ((val == LONG_MAX || val == LONG_MIN)
                    && errno == ERANGE)
                    err_exit(true, "strtol on debug mask");
                dbg_setmask(val);
            }
            break;
        case 'V': /* --version */
            _version();
            /*NOTREACHED*/
            break;
        case 's': /* --stdio */
            use_stdio = true;
            daemonize = false;
            break;
        case 'h': /* --help */
        default:
            _usage(argv[0]);
            /*NOTREACHED*/
            break;
        }
    }

    if (!config_filename)
        config_filename = hsprintf("%s/%s/%s", X_SYSCONFDIR,
                                   "powerman", "powerman.conf");

    dev_init(short_circuit_delay);
    cli_init();

    conf_init(config_filename);
    xfree(config_filename);

    xsignal(SIGHUP, _noop_handler);
    xsignal(SIGTERM, _exit_handler);
    xsignal(SIGINT, _exit_handler);
    xsignal(SIGPIPE, SIG_IGN);

    cli_start(use_stdio);

    if (daemonize) {
        char *rundir = hsprintf("%s/powerman", X_RUNSTATEDIR);
        char *pidfile = hsprintf("%s/powermand.pid", rundir);
        int *fds, len;

        cli_listen_fds(&fds, &len);
        daemon_init(fds, len, rundir, pidfile, DAEMON_NAME);
        xfree(rundir);
        xfree(pidfile);
        err_notty();
        dbg_notty();
    }

    /* We now have a socket at listener fd running in listen mode */
    /* and a file descriptor for communicating with each device */
    _select_loop();
    return 0;
}

static void _usage(char *prog)
{
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("  -c,--conf=PATH            Specify config file path\n");
    printf("  -f,--foreground           Don't daemonize\n");
    printf("  -s,--stdio                Talk to client on stdin/stdout\n");
    printf("  -Y,--short-circuit-delay  Change all device delays to zero\n");
    printf("  -d,--debug=MASK           Enable debug logging\n");
    printf("  -V,--version              Report powerman version\n");
    printf("  -h,--help                 Display help\n");
    exit(0);
}

static void _version(void)
{
    printf("%s\n", VERSION);
    exit(0);
}

static void _select_loop(void)
{
    struct timeval tmout;
    xpollfd_t pfd = xpollfd_create();

    timerclear(&tmout);

    /* start non-blocking connections to all the devices - finish them inside
     * the poll loop.
     */
    dev_initial_connect();

    while (1) {
        xpollfd_zero(pfd);

        cli_pre_poll(pfd);
        dev_pre_poll(pfd);

        xpoll(pfd, timerisset(&tmout) ? &tmout : NULL);
        timerclear(&tmout);

        /*
         * Process activity on client and device fd's.
         * If a device requires a timeout, for example to reconnect or
         * to process a scripted delay, tmout is updated.
         */
        cli_post_poll(pfd);
        dev_post_poll(pfd, &tmout);

        if (cli_server_done())
            break;
    }
    xpollfd_destroy(pfd);
}

static void _noop_handler(int signum)
{
    /* do nothing */
}

static void _exit_handler(int signum)
{
    cli_fini();
    dev_fini();
    conf_fini();
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
