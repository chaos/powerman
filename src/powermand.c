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
#include "daemon_init.h"
#include "listener.h"
#include "server.h"
#include "exit_error.h"
#include "wrappers.h"

/* prototypes */
static void process_command_line(Globals *g, int argc, char **argv);
static void usage(char *prog);
static void do_select_loop(Globals *g);
static int find_max_fd(Globals *g, fd_set *rset, fd_set *wset);
#ifndef NDUMP
static void dump_Globals(void);
static void dump_Server_Status(Server_Status status);
#endif
static Globals *make_Globals();
static void sig_hup_handler(int signum);
static void exit_handler(int signum);
static void read_Config_file(Globals *g);

Globals *cheat;  /* This is synonymous with the Globals *g declared in */
                 /* main().  I call attention to global access as      */
                 /* "cheat".  This only happens in exit_error() and in */
                 /* the parser.    */

static const char *powerman_license = \
    "Copyright (C) 2001-2002 The Regents of the University of California.\n"  \
    "Produced at Lawrence Livermore National Laboratory.\n"                   \
    "Written by Andrew Uselton <uselton2@llnl.gov>.\n"                           \
    "http://www.llnl.gov/linux/powerman/\n"                                     \
    "UCRL-CODE-2002-008\n\n"                                                  \
    "PowerMan is free software; you can redistribute it and/or modify it\n"     \
    "under the terms of the GNU General Public License as published by\n"     \
    "the Free Software Foundation.\n";

#define OPT_STRING "c:fhLVt"
static const struct option long_options[] =
{
		{"config_file",    required_argument, 0, 'c'},
		{"foreground",     no_argument,       0, 'f'},
		{"help",           no_argument,       0, 'h'},
		{"license",        no_argument,       0, 'L'},
		{"version",        no_argument,       0, 'V'},
		{"telemetry",      no_argument,       0, 't'},
		{0, 0, 0, 0}
};

static const struct option *longopts = long_options;

static bool debug_telemetry = FALSE;


/*
 * There is no shutdown/restart signal handling, nor does
 *         exitting clean any mallocs up.  Once it enters the
 *         select loop it stays there until you call the "kill"
 *         option.
 */ 
int
main(int argc, char **argv)
{
/* 
 *   Globals has the the timeout intervals, a structure for the 
 * client protocol, and pointers for the listener, the list of 
 * devices, the list of clients, the list of pending actions, 
 * and the cluster state info.
 */
	Globals *g;
	Device *dev;
	ListIterator dev_i;

	init_error(argv[0]);
	cheat = g = make_Globals();
	
	process_command_line(g, argc, argv);

	if( geteuid() != 0 ) exit_msg("Must be root");

	Signal(SIGHUP, sig_hup_handler);
	Signal(SIGTERM, exit_handler);

	read_Config_file(g);

	if( g->daemonize )
		daemon_init(); 	/* closes all open fd's, opens syslog */
				/*   and tells exit_error to use syslog */

	dev_i = list_iterator_create(g->devs);
	while( (dev = (Device *)list_next(dev_i)) )
		init_Device(dev);
	list_iterator_destroy(dev_i);

	init_Listener(g->listener);

/* We now have a socket at g->listener->fd running in listen mode */
/* and a file descriptor for communicating with each device */
	do_select_loop(g);
	return 0;
}

/* 
 *   The primary task here is to get the configuration file name.
 * There is no internally defined default so the program must always
 * be run with a "-c <file>" command line parameter or the "kill" 
 * option "-k".  The other options are self explanitory and less
 * important.
 */
static void
process_command_line(Globals *g, int argc, char **argv)
{
	int c;
	int longindex;

	opterr = 0;
	while ((c = getopt_long(argc, argv, OPT_STRING, longopts, &longindex)) != -1) {
		switch(c) {
		case 'c':	/* --config_file */
			if( g->config_file != NULL ) 
				usage(argv[0]);
			g->config_file = Strdup(optarg);
			break;
		case 'f':	/* --foreground */
			g->daemonize = FALSE;
			break;
		case 'h':	/* --help */
			usage(argv[0]);
			exit(0);
		case 'L':	/* --license */
			printf("%s", powerman_license);
			exit(0);
		case 't':	/* --telemetry */
			debug_telemetry = TRUE;
			break;	
		case 'V':
			printf("%s-%s\n", PROJECT, VERSION);
			exit(0);
		default:
			exit_msg("Unrecognized cmd line option (see --help).");
			exit(0);
			break;
		}
	}

	if (debug_telemetry && g->daemonize == TRUE)
		exit_msg("--telemetry cannot be used in daemon mode");

	if (! g->config_file)
		g->config_file = Strdup(DFLT_CONFIG_FILE);	
}


static void 
usage(char *prog)
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
static void
do_select_loop(Globals *g)
{
	int maxfd = 0;
	struct timeval tv;
	Client *client;
	ListIterator c_iter;
	Device  *dev;
	ListIterator dev_i;
	int n;
	fd_set rset;
	fd_set wset;
	bool over_time;   /* for update actions */
	bool active_devs; /* active_devs == FALSE => Quiescent */
	bool activity;    /* activity == TRUE => scripts need service */
	Action *act;

	CHECK_MAGIC(g);

	c_iter = list_iterator_create(g->clients);
	dev_i  = list_iterator_create(g->devs);

	while ( 1 ) 
	{
		over_time = FALSE;
		active_devs  = FALSE;

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
		if ( FD_ISSET(g->listener->fd, &rset) )
			handle_Listener(g);

                /* Client reading and writing?  */
		list_iterator_reset(c_iter);
		while( ((void *)client) = list_next(c_iter) )
		{
			if (FD_ISSET(client->fd, &rset))
				handle_Client_read(g->client_prot, g->cluster, 
						   g->acts, client);
			if (FD_ISSET(client->fd, &wset))
				handle_Client_write(client);
                        /* Is this connection done? */
			if ( (client->read_status == CLI_DONE) && 
			     (client->write_status == CLI_IDLE) )
				list_delete(c_iter);
		}
		
		/* update_interval may be set in the config file. */
                /* Any activity will suppress updates for that    */
		/* period, not just other updates.                */
		over_time = overdue(&(g->cluster->time_stamp), &(g->cluster->update_interval));
		
                /* Device reading and writing? */
		list_iterator_reset(dev_i);
		while( (dev = (Device *)list_next(dev_i)) )
		{
		        activity = FALSE;
			/* Any active device is sufficient to  */
			/* suppress starting the next action   */
			if( dev->status & (DEV_SENDING | DEV_EXPECTING) )
			     active_devs = TRUE;

			/* The first activity is always the signal  */
			/* of a newly connected device.  We'll have */
			/* run the log in script to get back into   */
			/* business as usual.                       */
			if( (dev->status & DEV_CONNECTED) == 0 )
			  {
			    if( (FD_ISSET(dev->fd, &rset) || 
				 FD_ISSET(dev->fd, &wset)) 
				&&
				(!dev->error || over_time) )
			      do_Device_connect(dev);
			    continue;
			  }
			if(FD_ISSET(dev->fd, &rset) )
			  {
				handle_Device_read(dev, debug_telemetry);
				activity = TRUE;
			  }
			if(FD_ISSET(dev->fd, &wset))
			  {
				handle_Device_write(dev, debug_telemetry);
				/* We may want to pace the commands */
				/* sent to the cluster.  interDev   */
				/* may be set in the config file.   */
				Delay( &(g->interDev) );
				activity = TRUE;
			  }
			/* Since some I/O took place we need to see  */
			/* if the scripts need to be nudged along    */
			if(activity) process_script(dev);
			/* dev->timeout may be set in the config file. */
			if( stalled_Device(dev) )
				recover_Device(dev);
			/* we only initiate device recover once per */
			/* update period.  Otherwise we can get     */
			/* swamped with reconnect messages.         */
			if( over_time && dev->error )
				initiate_nonblocking_connect(dev);
		}
		/* queue up an update action */
		if( over_time )
			update_Action(g->cluster, g->acts);
		/* launch the nexxt action in the queue */
		if ( (!active_devs) & 
		     ((act = find_Action(g->acts, g->clients)) != NULL) )
		{
			/* A previous action may need a reply sent */
			/* back to a client.                       */
			if (g->status == Occupied)
				finish_Action(g, act);
			/* Double check.  If there really was an */
			/* action in the queue, launch it.       */
			if ( (act = find_Action(g->acts, g->clients)) != NULL ) 
				do_Action(g, act);
		}
	}
}


/*
 *   This not only generates the needed maxfd for the select loop.
 * It sets the read and write fd_sets as well.  log write is always on,
 * listener read is always on, each client maintains READIN and WRITING
 * status in its data structure, device read is always on, and device
 * write stsus is also maintained in the device structure.  
 */ 
static int
find_max_fd(Globals *g, fd_set *rs, fd_set *ws)
{
	Device *dev;
	Client *client;
	ListIterator c_iter;
	ListIterator dev_i;
	int maxfd = 0; 

	CHECK_MAGIC(g);

	/*
	 * Build the FD_SET's: rs, ws
	 */
	FD_ZERO(rs);
	FD_ZERO(ws);
	if (g->listener->read)  {
		FD_SET(g->listener->fd, rs);
		maxfd = MAX(maxfd, g->listener->fd);
	}
	c_iter = list_iterator_create(g->clients);
	while( (((Client *)client) = list_next(c_iter)) )
	{
		if (client->read_status == CLI_READING)
			FD_SET(client->fd, rs);
		if (client->write_status == CLI_WRITING) 
			FD_SET(client->fd, ws);
		maxfd = MAX(maxfd, client->fd);
	}
	list_iterator_destroy(c_iter);

	dev_i = list_iterator_create(g->devs);
	while( (dev = (Device *)list_next(dev_i)) )
	{
	  /* To handle telnet I'm having it always ready to read */
	        FD_SET(dev->fd, rs);
		if ( ((dev->status & DEV_CONNECTED) == 0) ||
		     (dev->status & DEV_SENDING) ) 
			FD_SET(dev->fd, ws);
		maxfd = MAX(maxfd, dev->fd);
	}
	list_iterator_destroy(dev_i);

	return maxfd;
}

/*
 *   Eventually this will initiate a re read of the config file and
 * a reinitialization of the internal data structures.
 */
static void
sig_hup_handler(int signum)
{
}

/*
 *   Send a death poem on the way out.  I used to have this send
 * and exit_error(0), but there must have been some sort of hidden
 * race condition, because it would fail to find the syslog_on
 * bool telling it to send the stuff to the syslog.  It would try 
 * to go to the stderr instead, which didn't exist, or worse, was
 * some client or device connection.  The message would disapear 
 * without a trace.  
 */
static void
exit_handler(int signum)
{
	syslog(LOG_NOTICE, "exiting on signal %d", signum);
	exit(0);
}


/*
 *   This function has two jobs.  Get the config file based on the
 * name on the command line, and wrap the call to the parser 
 * (parse.lex and parse.y).  
 */ 
static void
read_Config_file(Globals *g)
{
	struct stat stbuf;

	CHECK_MAGIC(g);

/* Did we really get a config file? */
	if (g->config_file == NULL)
		exit_msg("no config file specified");
	if (stat(g->config_file, &stbuf) < 0)
		exit_error("%s", g->config_file);
	if( (stbuf.st_mode & S_IFMT) != S_IFREG )
		exit_msg("%s is not a regular file\n", g->config_file);
	g->cf = fopen(g->config_file, "r");
	if( g->cf == NULL ) 
		exit_error("fopen %s", g->config_file); 

	g->client_prot = init_Client_Protocol();

	parse_config_file();
	
	fclose(g->cf);
}

/*
 * constructor 
 *
 * produces:  Globals 
 */
static Globals *
make_Globals()
{
	Globals *g;

	g = (Globals *)Malloc(sizeof(Globals));
	INIT_MAGIC(g);

	g->timeout_interval.tv_sec  = TIMEOUT_SECONDS;
	g->timeout_interval.tv_usec = 0;
	g->interDev.tv_sec          = 0;
	g->interDev.tv_usec         = INTER_DEV_USECONDS;
	g->daemonize                = TRUE;
	g->TCP_wrappers             = FALSE;
	g->listener                 = make_Listener();
	g->clients                  = list_create(free_Client);
	g->status                   = Quiescent;
	g->cluster                  = NULL;
	g->acts                     = list_create(free_Action);
	g->client_prot              = NULL;
	g->specs                    = list_create(free_Spec);
	g->devs                     = list_create(free_Device);
	g->config_file              = NULL;
	g->cf                       = NULL;
	g->cluster = make_Cluster();
	return g;
}

#ifndef NDUMP

/* 
 *   Display a debug listing of everything in the Globals struct.
 */
static void
dump_Globals(void)
{
	ListIterator    act_iter;
	Action         *act;
	ListIterator    c_iter;
	Client         *client;
	ListIterator    d_iter;
	Device         *dev;
	int i;
	Globals *g = cheat;

	fprintf(stderr, "Config file: %s\n", g->config_file);
#if 0
	/* FIXME: doesn't compile and doesn't look like loop advances jg */
	fprintf(stderr, "Specifications:\n");
	while(g->specs != g->specs->next)
	{
		Spec *spec;

		spec = g->specs->next;
		dump_Spec(spec);
	}
#endif
	fprintf(stderr, "Globals: %0x\n", (unsigned int)g);
	fprintf(stderr, "\ttimout interval: %d.%06d sec\n", 
		(int)g->timeout_interval.tv_sec, 
		(int)g->timeout_interval.tv_usec);
	dump_Cluster(g->cluster);
	dump_Server_Status(g->status);
	act_iter = list_iterator_create(g->acts);
	while( (act = (Action *)list_next(act_iter)) )
		dump_Action((List)act);
	list_iterator_destroy(act_iter);
	dump_Listener(g->listener);
	c_iter = list_iterator_create(g->clients);
	while( (client = (Client *)list_next(c_iter)) )
		dump_Client(client);
	list_iterator_destroy(c_iter);
	fprintf(stderr, "\tClient Scripts:\n");
	for (i = 0; i < g->client_prot->num_scripts; i++)
		dump_Script(g->client_prot->scripts[i], i);
	d_iter = list_iterator_create(g->devs);
	while( (dev = (Device *)list_next(d_iter)) )
		dump_Device(dev);
	list_iterator_destroy(d_iter);
	Report_Memory();
	fprintf(stderr, "That's all.\n");
}

/*
 * Helper functin for dump_Globals 
 */
static void
dump_Server_Status(Server_Status status)
{
	fprintf(stderr, "\tServer Status: %d: ", status);
	if (status == Quiescent) fprintf(stderr, "Quiescent\n");
	else if (status == Occupied) fprintf(stderr, "Occupied\n");
	else fprintf(stderr, "Unknown\n");
}



#endif

#if 0
/*
 *  destructor
 *
 * destroys:  Globals
 */
static void
free_Globals(Globals *g)
{
	int i;

	Free(g->config_file);
	list_destroy(g->specs);
	free_Cluster(g->cluster);
	list_destroy(g->acts);
	free_Listener(g->listener);
	list_iterator_create(g->clients);
	list_destroy(g->clients);
	for (i = 0; i < g->client_prot->num_scripts; i++)
		list_destroy(g->client_prot->scripts[i]);
	Free(g->client_prot);
	list_destroy(g->devs);
	Free(g);
}
#endif
