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
/*
 *   Keep in mind that you may want to install the port numbers in the 
 * /etc/services file:
 * powerman	10101/tcp		# powerman client protocol
 * vicebox	11000/tcp		# vicebox raw TCP protocol 
 * icebox	1010/tcp		# icebox raw TCP protocol 
 */

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
#include "config.h"
#include "device.h"
#include "daemon.h"
#include "client.h"
#include "error.h"
#include "wrappers.h"
#include "debug.h"

/* prototypes */
static void _usage(char *prog);
static void _noop_handler(int signum);
static void _exit_handler(int signum);
static void _select_loop(void);

static const char *powerman_license =
    "Copyright (C) 2001-2002 The Regents of the University of California.\n"
    "Produced at Lawrence Livermore National Laboratory.\n"
    "Written by Andrew Uselton <uselton2@llnl.gov>.\n"
    "http://www.llnl.gov/linux/powerman/\n"
    "UCRL-CODE-2002-008\n\n"
    "PowerMan is free software; you can redistribute it and/or modify it\n"
    "under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation.\n";

#define OPT_STRING "c:fhLVd:"
static const struct option long_options[] = {
    {"config_file", required_argument, 0, 'c'},
    {"foreground", no_argument, 0, 'f'},
    {"help", no_argument, 0, 'h'},
    {"license", no_argument, 0, 'L'},
    {"version", no_argument, 0, 'V'},
    {"debug", required_argument, 0, 'd'},
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
    /*act_init();*/
    dev_init();	 
    cli_init();

    /* parse command line options */
    opterr = 0;
    while ((c = getopt_long(argc, argv, OPT_STRING, longopts, &lindex)) != -1) {
	switch (c) {
	case 'c':		/* --config_file */
	    if (config_filename != NULL)
		_usage(argv[0]);
	    config_filename = Strdup(optarg);
	    break;
	case 'f':		/* --foreground */
	    daemonize = FALSE;
	    break;
	case 'h':		/* --help */
	    _usage(argv[0]);
	    exit(0);
	case 'L':		/* --license */
	    printf("%s", powerman_license);
	    exit(0);
	case 'd':		/* --debug */
	    {
		unsigned long val = strtol(optarg, NULL, 0);

		if ((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE)
		    err_exit(TRUE, "strtol on debug mask");
		dbg_setmask(val);
	    }
	    break;
#if 0
	case 'V':
	    printf("%s-%s\n", PROJECT, VERSION);
	    exit(0);
#endif
	default:
	    err_exit(FALSE, "unrecognized cmd line option (see --help).");
	    exit(0);
	    break;
	}
    }

    if (geteuid() != 0)
	err_exit(FALSE, "must be root");

    Signal(SIGHUP, _noop_handler);
    Signal(SIGTERM, _exit_handler);
    Signal(SIGINT, _exit_handler);

    /* parses config file */
    conf_init(config_filename ? config_filename : DFLT_CONFIG_FILE);

    if (config_filename != NULL)
      Free(config_filename);

    if (daemonize)
	daemon_init();

    /* initialize listener */
    cli_listen();

    /* We now have a socket at listener fd running in listen mode */
    /* and a file descriptor for communicating with each device */
    _select_loop();
    /*NOTREACHED*/
    return 0;
}

static void _usage(char *prog)
{
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("  -c --config_file FILE\tSpecify configuration [%s]\n",
	   DFLT_CONFIG_FILE);
    printf("  -f --foreground\tDon't daemonize\n");
    printf("  -h --help\t\tDisplay this help\n");
    printf("  -L --license\t\tDisplay license information\n");
    printf("  -V --version\t\tDisplay version information\n");
}

static bool _tvzero(struct timeval *tv)
{
    return ((tv->tv_sec == 0 && tv->tv_usec == 0) ? TRUE : FALSE);
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
    int maxfd;
    struct timeval tmout;
    fd_set rset;
    fd_set wset;
    char rsetstr[1024], wsetstr[1024], tmoutstr[32];

    /* start non-blocking connections to all the devices */
    dev_initial_connect();

    timerclear(&tmout);

    while (1) {
	dbg(DBG_SELECT, "---------------------");
	/* set maxfd and read/write fd sets for select call */
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	maxfd = 0;
	cli_pre_select(&rset, &wset, &maxfd);
	dev_pre_select(&rset, &wset, &maxfd);

	dbg(DBG_SELECT, "select rset=[%s] wset=[%s] tmout=%s",
		    dbg_fdsetstr(&rset, maxfd+1, rsetstr, sizeof(rsetstr)),
		    dbg_fdsetstr(&wset, maxfd+1, wsetstr, sizeof(wsetstr)),
		    _tvzero(&tmout) ? "NULL" : dbg_tvstr(&tmout, tmoutstr, 
						sizeof(tmoutstr)));

        /* Note: some selects modify timeval arg */
	Select(maxfd + 1, &rset, &wset, NULL, _tvzero(&tmout) ? NULL : &tmout);
	timerclear(&tmout);

	dbg(DBG_SELECT, "ready rset=[%s] wset=[%s]", 
			dbg_fdsetstr(&rset, maxfd+1, rsetstr, sizeof(rsetstr)),
		    dbg_fdsetstr(&wset, maxfd+1, wsetstr, sizeof(wsetstr)));

	/* 
	 * Process activity on client and device fd's.
	 * If a device requires a timeout, for example to reconnect or
	 * to process a scripted delay, tmout is updated.
	 */
	cli_post_select(&rset, &wset);
	dev_post_select(&rset, &wset, &tmout);
    }
    /*NOTREACHED*/
}

static void _noop_handler(int signum)
{
    /* do nothing */
}

static void _exit_handler(int signum)
{
    cli_fini();
    /*act_fini();*/
    dev_fini();
    conf_fini();
    err_exit(FALSE, "exiting on signal %d", signum);
}

/* out of memory handler for list.[ch] routines */
void *out_of_memory(void)
{
    err_exit(FALSE, "list routines out of memory");
    /*NOTREACHED*/
    return NULL;
}

/*
 * vi:softtabstop=4
 */
