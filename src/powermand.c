/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton (uselton2@llnl.gov>
 *  UCRL-CODE-2002-008.
 *  
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see <http://www.llnl.gov/linux/powerman/>.
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

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "powerman.h"
#include "list.h"
#include "parse_util.h"
#include "wrappers.h"
#include "device.h"
#include "daemon.h"
#include "client.h"
#include "error.h"
#include "debug.h"

/* prototypes */
static void _usage(char *prog);
static void _version(void);
static void _noop_handler(int signum);
static void _exit_handler(int signum);
static void _select_loop(void);

#define OPT_STRING "c:fhd:V"
static const struct option long_options[] = {
    {"conf", required_argument, 0, 'c'},
    {"foreground", no_argument, 0, 'f'},
    {"help", no_argument, 0, 'h'},
    {"debug", required_argument, 0, 'd'},
    {"version", no_argument, 0, 'V'},
    {0, 0, 0, 0}
};

static const struct option *longopts = long_options;

int main(int argc, char **argv)
{
    int c;
    int lindex;
    char *config_filename = NULL;
    bool daemonize = TRUE;

    /* initialize various modules */
    err_init(argv[0]);
    dev_init();
    cli_init();

    /* parse command line options */
    while ((c = getopt_long(argc, argv, OPT_STRING, longopts,
                        &lindex)) != -1) {
        switch (c) {
        case 'c':               /* --conf */
            if (config_filename != NULL) {
                _usage(argv[0]);
                /*NOTREACHED*/
            }
            config_filename = Strdup(optarg);
            break;
        case 'f':               /* --foreground */
            daemonize = FALSE;
            break;
        case 'd':               /* --debug */
            {
                unsigned long val = strtol(optarg, NULL, 0);

                if ((val == LONG_MAX || val == LONG_MIN)
                    && errno == ERANGE)
                    err_exit(TRUE, "strtol on debug mask");
                dbg_setmask(val);
            }
            break;
        case 'V':               /* --version */
            _version();
            /*NOTREACHED*/
            break;
        case 'h':               /* --help */
        default:
            _usage(argv[0]);
            /*NOTREACHED*/
            break;
        }
    }

    /* need root for ability to chown ptys, etc */
    if (geteuid() != 0)
        err_exit(FALSE, "must be root");

    Signal(SIGHUP, _noop_handler);
    Signal(SIGTERM, _exit_handler);
    Signal(SIGINT, _exit_handler);
    Signal(SIGPIPE, SIG_IGN);

    /* parses config file */
    conf_init(config_filename ? config_filename : DFLT_CONFIG_FILE);

    if (config_filename != NULL)
        Free(config_filename);

    /* initialize listener */
    cli_listen();

    if (daemonize)
        daemon_init(); /* closes all fd's except client listen port */

    /* We now have a socket at listener fd running in listen mode */
    /* and a file descriptor for communicating with each device */
    _select_loop();
     /*NOTREACHED*/ 
    return 0;
}

static void _usage(char *prog)
{
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("  -c --conf <filename>   Specify config file path [%s]\n",
           DFLT_CONFIG_FILE);
    printf("  -d --debug <mask>      Set debug mask [0]\n");
    printf("  -f --foreground        Don't daemonize\n");
    printf("  -V --version           Report powerman version\n");
    exit(0);
}

static void _version(void)
{
    printf("%s\n", POWERMAN_VERSION);
    exit(0);
}

/*
 * This loop does not terminate except via the program being killed.  
 * The loop pauses for timeout_interval or until there is activity on 
 * a descriptor.  That activity could be a new client or I/O to a client 
 * or a device.  Once per update_interval we intiate both an "update nodes" 
 * and an "update plugs" query for every node in the cluster.  Each time 
 * through the loop the scripts managing the I/O to the clients and the 
 * devices are examined and nudged along if they can make progress.  If all 
 * the devices have completed their assigned tasks the cluster becomes 
 * "quiecent" and a new action may be initiated from the list 
 * queued up by the clients.  
 */
static void _select_loop(void)
{
    struct timeval tmout;
    Pollfd_t pfd = PollfdCreate();

    timerclear(&tmout);

    /* start non-blocking connections to all the devices */
    dev_initial_connect();

    while (1) {
        PollfdZero(pfd);
        cli_pre_poll(pfd);
        dev_pre_poll(pfd);

        Poll(pfd, timerisset(&tmout) ? &tmout : NULL);
        timerclear(&tmout);

        /* 
         * Process activity on client and device fd's.
         * If a device requires a timeout, for example to reconnect or
         * to process a scripted delay, tmout is updated.
         */
        cli_post_poll(pfd);
        dev_post_poll(pfd, &tmout);
    }
    /*NOTREACHED*/
    PollfdDestroy(pfd);
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
