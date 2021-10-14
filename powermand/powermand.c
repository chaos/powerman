/*****************************************************************************
 *  Copyright (C) 2001 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  UCRL-CODE-2002-008.
 *
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see http://github.com/chaos/powerman/
 *
 *  PowerMan is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  PowerMan is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with PowerMan; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

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
#include <stdarg.h>

#include "xtypes.h"
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

#define OPTIONS "c:fhd:VsY1"
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
    {"one-client",      no_argument,        0, '1'},
    {0, 0, 0, 0}
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int main(int argc, char **argv)
{
    int c;
    char *config_filename = NULL;
    bool daemonize = TRUE;
    bool use_stdio = FALSE;
    bool short_circuit_delay = FALSE;
    bool one_client = FALSE;

    /* parse command line options */
    err_init(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
        case 'Y': /* --short-circuit-delay */
            short_circuit_delay = TRUE;
            break;
        case 'c': /* --conf */
            if (!config_filename)
                config_filename = xstrdup(optarg);
            break;
        case 'f': /* --foreground */
            daemonize = FALSE;
            break;
        case 'd': /* --debug */
            {
                unsigned long val = strtol(optarg, NULL, 0);

                if ((val == LONG_MAX || val == LONG_MIN)
                    && errno == ERANGE)
                    err_exit(TRUE, "strtol on debug mask");
                dbg_setmask(val);
            }
            break;
        case 'V': /* --version */
            _version();
            /*NOTREACHED*/
            break;
        case 's': /* --stdio */
            use_stdio = TRUE;
            one_client = TRUE;
            break;
        case '1': /* --one-client */
            one_client = TRUE;
            break;
        case 'h': /* --help */
        default:
            _usage(argv[0]);
            /*NOTREACHED*/
            break;
        }
    }

    if (use_stdio && daemonize)
        err_exit(FALSE, "--stdio should only be used with --foreground");

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

    cli_start(use_stdio, one_client);

    if (daemonize) {
        char *rundir = hsprintf("%s/run/powerman", X_LOCALSTATEDIR);
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
    printf("  -c --conf <filename>   Specify config file path\n");
    printf("  -d --debug <mask>      Set debug mask [0]\n");
    printf("  -f --foreground        Don't daemonize\n");
    printf("  -V --version           Report powerman version\n");
    printf("  -s --stdio             Talk to client on stdin/stdout\n");
    printf("  -1 --one-client        Terminate when client disconnects\n");
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
    err_exit(FALSE, "exiting on signal %d", signum);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
