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
#include <sys/stat.h>
#include <syslog.h>
#include <stdio.h>
#include <assert.h>

#include "powerman.h"
#include "list.h"
#include "config.h"
#include "device.h"
#include "powermand.h"
#include "action.h"
#include "daemon.h"
#include "listener.h"
#include "client.h"
#include "error.h"
#include "wrappers.h"
#include "util.h"

/* prototypes */
static void usage(char *prog);
static void do_select_loop(Globals * g);
static int find_max_fd(Globals * g, fd_set * rset, fd_set * wset);
static Globals *make_Globals();
static void noop_handler(int signum);
static void exit_handler(int signum);

/* This is synonymous with the Globals *g declared in main().
 * I call attention to global access as "cheat".  This only
 * happens in the parser.
 */
Globals *cheat;


static const char *powerman_license =
    "Copyright (C) 2001-2002 The Regents of the University of California.\n"
    "Produced at Lawrence Livermore National Laboratory.\n"
    "Written by Andrew Uselton <uselton2@llnl.gov>.\n"
    "http://www.llnl.gov/linux/powerman/\n"
    "UCRL-CODE-2002-008\n\n"
    "PowerMan is free software; you can redistribute it and/or modify it\n"
    "under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation.\n";

#define OPT_STRING "c:fhLVt"
static const struct option long_options[] = {
    {"config_file", required_argument, 0, 'c'},
    {"foreground", no_argument, 0, 'f'},
    {"help", no_argument, 0, 'h'},
    {"license", no_argument, 0, 'L'},
    {"version", no_argument, 0, 'V'},
    {"telemetry", no_argument, 0, 't'},
    {0, 0, 0, 0}
};

void _validate_config_file(char *filename)
{
    struct stat stbuf;

    if (stat(filename, &stbuf) < 0)
	err_exit(TRUE, "%s", filename);
    if ((stbuf.st_mode & S_IFMT) != S_IFREG)
	err_exit(FALSE, "%s is not a regular file\n", filename);
}

/* initialize all the power control devices */
void _initialize_devs(List devs, bool debug_telemetry)
{
    Device *dev;
    ListIterator itr = list_iterator_create(devs);

    while ((dev = list_next(itr)))
	dev_init(dev, debug_telemetry);
    list_iterator_destroy(itr);
}


static const struct option *longopts = long_options;


int main(int argc, char **argv)
{
    int c;
    int lindex;
    Globals *g;
    char *config_filename = NULL;
    bool debug_telemetry = FALSE;
    bool daemonize = TRUE;

    /* initialize error module */
    err_init(argv[0]);

    /* FIXME: initialize a big blob of globals */
    cheat = g = make_Globals();

    /* parse command line options */
    opterr = 0;
    while ((c = getopt_long(argc, argv, OPT_STRING, longopts, &lindex)) != -1) {
	switch (c) {
	case 'c':		/* --config_file */
	    if (config_filename != NULL)
		usage(argv[0]);
	    config_filename = Strdup(optarg);
	    break;
	case 'f':		/* --foreground */
	    daemonize = FALSE;
	    break;
	case 'h':		/* --help */
	    usage(argv[0]);
	    exit(0);
	case 'L':		/* --license */
	    printf("%s", powerman_license);
	    exit(0);
	case 't':		/* --telemetry */
	    debug_telemetry = TRUE;
	    break;
	case 'V':
	    printf("%s-%s\n", PROJECT, VERSION);
	    exit(0);
	default:
	    err_exit(FALSE, "unrecognized cmd line option (see --help).");
	    exit(0);
	    break;
	}
    }

    if (debug_telemetry && daemonize == TRUE)
	err_exit(FALSE, "--telemetry cannot be used in daemon mode");

    if (geteuid() != 0)
	err_exit(FALSE, "must be root");

    if (config_filename == NULL)
	config_filename = DFLT_CONFIG_FILE;
    _validate_config_file(config_filename);

    Signal(SIGHUP, noop_handler);
    Signal(SIGTERM, exit_handler);

    /* build the client protocol */
    g->client_prot = conf_init_client_protocol();

    /* call yacc/lex parser */
    parse_config_file(config_filename);

    /* drop controlling tty, open syslog, etc */
    if (daemonize)
	daemon_init();

    /* initialize all the power control devs */
    _initialize_devs(g->devs, debug_telemetry);

    /* initialize listener */
    listen_init(g->listener);

    /* We now have a socket at g->listener->fd running in listen mode */
    /* and a file descriptor for communicating with each device */
    do_select_loop(g);
    return 0;
}

static void usage(char *prog)
{
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("  -c --config_file FILE\tSpecify configuration [%s]\n",
	   DFLT_CONFIG_FILE);
    printf("  -f --foreground\tDon't daemonize\n");
    printf("  -h --help\t\tDisplay this help\n");
    printf("  -L --license\t\tDisplay license information\n");
    printf("  -V --version\t\tDisplay version information\n");
}


/*
 *   By the time we get here the config file has been successfully 
 * parsed and the Globals data structure is set up.  This loop does 
 * not terminate except via the program being killed.  The loop
 * pauses for g->timout_interval or until there is activity on 
 * a descriptor.  That activity could be a new client, a log entry 
 * ready to write, or I/O to a client or a device.  Each possiblity 
 * is examined and handled in turn.   Once an hour a timestamp is 
 * sent to the log.  Once per g->cluster->update_interval the daemon
 * intiates both an "update nodes" and an "update plugs" query for
 * every node in the cluster.  Each time through the loop the 
 * scripts managing the I/O to the clients and the devices are 
 * examined and nudged along if they can make progress.  If all the
 * devices have completed their assigned tasks the cluster becomes 
 * "quiecent" and a new action may be initiated from the list 
 * queued up by the clients.  
 */
static void do_select_loop(Globals * g)
{
    int maxfd = 0;
    struct timeval tv;
    Client *client;
    ListIterator cli_i;
    Device *dev;
    ListIterator dev_i;
    int n;
    fd_set rset;
    fd_set wset;
    bool over_time;		/* for update actions */
    bool active_devs;		/* active_devs == FALSE => Quiescent */
    bool activity;		/* activity == TRUE => scripts need service */
    Action *act;

    CHECK_MAGIC(g);

    cli_i = list_iterator_create(g->clients);
    dev_i = list_iterator_create(g->devs);

    while (1) {
	over_time = FALSE;
	active_devs = FALSE;

	/* 
	 * set up for and call select
	 * find_max_fd() has a side effect of setting wset and rset based on
	 * "status" values in the devices, and the "read" and "write" values
	 * in the clients, log, and listener.
	 */
	maxfd = find_max_fd(g, &rset, &wset);
        /* 
         * some "select" implementations are reputed to alter tv so set it
         * anew with each iteration.  timeout_interval may be set in the
         * config file.
         */
	tv = g->timeout_interval;
	n = Select(maxfd + 1, &rset, &wset, NULL, &tv);

	/* New connection? */
	if (FD_ISSET(g->listener->fd, &rset))
	    listen_handler(g);

	/* Client reading and writing?  */
	list_iterator_reset(cli_i);
	while ((client = list_next(cli_i))) {
	    if (client->fd < 0)
		continue;

	    if (FD_ISSET(client->fd, &rset))
		cli_handle_read(g->client_prot, g->cluster,
				   g->acts, client);
	    if (FD_ISSET(client->fd, &wset))
		cli_handle_write(client);
	    /* Is this connection done? */
	    if ((client->read_status == CLI_DONE) &&
		(client->write_status == CLI_IDLE))
		list_delete(cli_i);
	}

	/* update_interval may be set in the config file.
	 * Any activity will suppress updates for that
	 * period, not just other updates.
	 */
	over_time = util_overdue(&(g->cluster->time_stamp),
			    &(g->cluster->update_interval));

	/* Device reading and writing? */
	list_iterator_reset(dev_i);
	while ((dev = list_next(dev_i))) {
	    /* we only initiate device recover once per
	     * update period.  Otherwise we can get
	     * swamped with reconnect messages.
	     */
	    if (over_time && (dev->status == DEV_NOT_CONNECTED))
		dev_nb_connect(dev);

	    if (dev->fd < 0)
		continue;

	    activity = FALSE;

	    /* Any active device is sufficient to
	     * suppress starting the next action
	     */
	    if (dev->status & (DEV_SENDING | DEV_EXPECTING))
		active_devs = TRUE;

	    /* The first activity is always the signal
	     * of a newly connected device.  We'll have
	     * run the log in script to get back into
	     * business as usual.
	     */
	    if ((dev->status == DEV_CONNECTING)) {
		if (FD_ISSET(dev->fd, &rset) || FD_ISSET(dev->fd, &wset))
		    dev_connect(dev);
		continue;
	    }
	    if (FD_ISSET(dev->fd, &rset)) {
		dev_handle_read(dev);
		activity = TRUE;
	    }
	    if (FD_ISSET(dev->fd, &wset)) {
		dev_handle_write(dev);
		/*
		 * We may want to pace the commands
		 * sent to the cluster.  interDev
		 * may be set in the config file.
		 */
		Delay(&(g->interDev));
		activity = TRUE;
	    }
	    /*
	     * Since some I/O took place we need to see
	     * if the scripts need to be nudged along
	     */
	    if (activity)
		dev_process_script(dev);
	    /*
	     * dev->timeout may be set in the config file.
	     */
	    if (dev_stalled(dev))
		dev_recover(dev);
	}
	/* queue up an update action */
	if (over_time)
	    act_update(g->cluster, g->acts);
	/* launch the nexxt action in the queue */
	if ((!active_devs) &
	    ((act = act_find(g->acts, g->clients)) != NULL)) {
	    /* A previous action may need a reply sent
	     * back to a client.
	     */
	    if (g->status == Occupied)
		act_finish(g, act);
	    /*
	     * Double check.  If there really was an
	     * action in the queue, launch it.
	     */
	    if ((act = act_find(g->acts, g->clients)) != NULL)
		act_initiate(g, act);
	}
    }
}

/*
 *   This not only generates the needed maxfd for the select loop.
 * It sets the read and write fd_sets as well.  log write is always on,
 * listener read is always on, each client maintains READING and WRITING
 * status in its data structure, device read is always on, and device
 * write stsus is also maintained in the device structure.  
 */
static int find_max_fd(Globals * g, fd_set * rs, fd_set * ws)
{
    Client *client;
    Device *dev;
    ListIterator itr;
    int maxfd = 0;

    CHECK_MAGIC(g);

    /*
     * Build the FD_SET's: rs, ws
     */
    FD_ZERO(rs);
    FD_ZERO(ws);
    if (g->listener->read) {
	assert(g->listener->fd >= 0);
	FD_SET(g->listener->fd, rs);
	maxfd = MAX(maxfd, g->listener->fd);
    }
    itr = list_iterator_create(g->clients);
    while ((client = list_next(itr))) {
	if (client->fd < 0)
	    continue;

	if (client->read_status == CLI_READING) {
	    FD_SET(client->fd, rs);
	    maxfd = MAX(maxfd, client->fd);
	}
	if (client->write_status == CLI_WRITING) {
	    FD_SET(client->fd, ws);
	    maxfd = MAX(maxfd, client->fd);
	}
    }
    list_iterator_destroy(itr);

    itr = list_iterator_create(g->devs);
    while ((dev = list_next(itr))) {
	if (dev->fd < 0)
	    continue;

	/* To handle telnet I'm having it always ready to read.
	 */
	FD_SET(dev->fd, rs);
	maxfd = MAX(maxfd, dev->fd);

	/* The descriptor becomes writable when a non-blocking
	 * connect (ie, DEV_CONNECTING) completes.
	 */
	if ((dev->status & DEV_CONNECTING) || (dev->status & DEV_SENDING)) {
	    FD_SET(dev->fd, ws);
	}
    }
    list_iterator_destroy(itr);

    return maxfd;
}

static void noop_handler(int signum)
{
}

static void exit_handler(int signum)
{
    err_exit(FALSE, "exiting on signal %d", signum);
}

static Globals *make_Globals()
{
    Globals *g;

    g = (Globals *) Malloc(sizeof(Globals));
    INIT_MAGIC(g);

    g->timeout_interval.tv_sec = TIMEOUT_SECONDS;
    g->timeout_interval.tv_usec = 0;
    g->interDev.tv_sec = 0;
    g->interDev.tv_usec = INTER_DEV_USECONDS;
    g->TCP_wrappers = FALSE;
    g->listener = listen_create();
    g->clients = list_create((ListDelF) cli_destroy);
    g->status = Quiescent;
    g->acts = list_create((ListDelF) act_destroy);
    g->client_prot = NULL;
    g->specs = list_create((ListDelF) conf_spec_destroy);
    g->devs = list_create((ListDelF) dev_destroy);
    g->cluster = conf_cluster_create();
    return g;
}

#if 0
static void free_Globals(Globals * g)
{
    int i;

    list_destroy(g->specs);
    conf_cluster_destroy(g->cluster);
    list_destroy(g->acts);
    listen_destroy(g->listener);
    list_iterator_create(g->clients);
    list_destroy(g->clients);
    for (i = 0; i < g->client_prot->num_scripts; i++)
	list_destroy(g->client_prot->scripts[i]);
    Free(g->client_prot);
    list_destroy(g->devs);
    Free(g);
}
#endif


/*
 * vi:softtabstop=4
 */
