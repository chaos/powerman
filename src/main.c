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

/* all the declarations */
#include "powerman.h"
#include "action.h"
#include "config.h"
#include "daemon_init.h"
#include "device.h"
#include "listener.h"
#include "main.h"
#include "server.h"

/* prototypes */
int main(int argc, char **argv);
static void process_command_line(Globals *g, int argc, char **argv);
static void signal_daemon(int signum);
static void usage(char *prog);
static void do_select_loop(Globals *g);
static int find_max_fd(Globals *g, fd_set *rset, fd_set *wset);
#ifndef NDUMP
static void dump_Globals(Globals *g);
static void dump_Server_Status(Server_Status status);
#endif
static Globals *make_Globals();
static void sig_hup_handler(int signum);
static void exit_handler(int signum);

Globals *cheat;  /* This is synonymous with the Globals *g declared in */
                 /* main().  I call attention to global access as      */
                 /* "cheat".  This only happens in exit_error() and in */
                 /* the parser.    */

const char *powerman_license = \
    "Copyright (C) 2001-2002 The Regents of the University of California.\n"  \
    "Produced at Lawrence Livermore National Laboratory.\n"                   \
    "Written by Andrew Uselton <uselton2@llnl.gov>.\n"                           \
    "http://www.llnl.gov/linux/powerman/\n"                                     \
    "UCRL-CODE-2002-008\n\n"                                                  \
    "PowerMan is free software; you can redistribute it and/or modify it\n"     \
    "under the terms of the GNU General Public License as published by\n"     \
    "the Free Software Foundation.\n";

const struct option long_options[] =
{                             /* c:dhkLrV */
		{"config_file",    required_argument, 0, 'c'},
		{"dont_daemonize", no_argument,       0, 'd'},
		{"help",           no_argument,       0, 'h'},
		{"kill",           no_argument,       0, 'k'},
		{"license",        no_argument,       0, 'L'},
		{"reread_config",  no_argument,       0, 'r'},
		{"version",        no_argument,       0, 'V'},
		{0, 0, 0, 0}
};

const struct option *longopts = long_options;


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

	if( geteuid() != 0 ) 
	{
		printf("Must be root\n");
		exit(0);
	}

	make_log();

	cheat = g = make_Globals();
	
	process_command_line(g, argc, argv);

	Signal(SIGHUP, sig_hup_handler);
	Signal(SIGTERM, exit_handler);

	if( g->daemonize )
		daemon_init();

	read_Config_file(g);

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
void
process_command_line(Globals *g, int argc, char **argv)
{
	int c;
	int signum = 0;
	int len;
	int longindex;

	opterr = 0;
	while ((c = getopt_long(argc, argv, "c:dhkLrV", longopts, &longindex)) != -1) {
		switch(c) {
		case 'c':
			if( g->config_file != NULL ) 
				Free(g->config_file, strlen(g->config_file));
			len = strlen(optarg);
			g->config_file = Malloc(len + 1);
			strncpy(g->config_file, optarg, len);
			g->config_file[len] = '\0';
			break;
		case 'd':
			g->daemonize = FALSE;
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'k':
			signum = SIGTERM;
			break;
		case 'L':
			printf("%s", powerman_license);
			exit(0);
		case 'r':
			signum = SIGHUP;
			break;
		case 'V':
			printf("%s-%s\n", PROJECT, VERSION);
			exit(0);
		default:
			printf("Unrecognized command line option (try -h).\n");
			exit(0);
			break;
		}
	}
	
	if (signum) 
	{
/* 
 *   For now only "-k" "TERM" works.  A "-r" "HUP" option is planned 
 * that will reread the config file and reinitialize the internal state.
 */
		signal_daemon(signum);
		exit(0);
	}

	return;
}

/*
 *   Both SIGTERM and SIGHUP may be sent to the process id listed in
 * the location PID_FILE_NAME (/var/run/powerman/powerman.pid), but only
 * SIGTERM actually works.  If the pid file is missing the program
 * exits with an error, and no process is signalled.  If the pid file
 * is found it is deleted whether the process is found or not.
 *
 * FIXME: I'd prefer to have /var/run/powerman/powerman.<pid> and 
 * allow for multiple daemons to coexist and be manageable.  "-k"
 * would presumably sequence through sending "TERM" to all of them.
 * There can be multiple powemand instances but currently "-k" will
 * only TERM the last one started.
 */
void
signal_daemon(int signum)
{
	FILE *fp;
	pid_t pid;
	int n;

	errno = 0;
	fp = fopen(PID_FILE_NAME, "r");
	if( fp == NULL ) exit_error("Failed to open pid file");
	n = fscanf(fp, "%d", &pid);
	unlink(PID_FILE_NAME);
	if( n != 1 ) exit_error("Failed to find pid in pidfile"); 
	n = kill(pid, signum);
	if( n < 0 ) exit_error("Failed to send signal %d to pid %d", signum, pid);
	errno = 0;
	return;
}

void usage(char *prog)
{
    printf("\nUsage: %s [OPTIONS]\n", prog);
    printf("\n");
    printf("  -c FILE --config_file FILE\tSpecify configuration [/etc/powerman.conf].\n");
    printf("  -d --dont_daemonize\t\tDon't daemonize (debug).\n");
    printf("  -h --help\t\t\tDisplay this help.\n");
    printf("  -k --kill\t\t\tKill daemon.\n");
    printf("  -L --license\t\t\tDisplay license information.\n");
    printf("  -r --reread_config\t\tReread configuration.\n");
    printf("  -V --version\t\t\tDisplay version information.\n");
    printf("\n");
    return;
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
void
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
	struct timeval now;
	struct timeval hourly;

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

		/* time to time stamp the log?     */
		/* it does so on the hour, but     */
		/* make sure it does it only once. */
		gettimeofday(&now, NULL);
		if ( ((now.tv_sec % 3600) == 0) &&
		     (now.tv_sec != hourly.tv_sec) )
		{
			hourly = now;
			log_it(0, "Hourly time stamp %s: ", 
			       ctime(&(hourly.tv_sec)));
		}

                /* Log write?  */
		if ( FD_ISSET(log->fd, &wset) )
			handle_log();

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
				handle_Device_read(dev);
				activity = TRUE;
			  }
			if(FD_ISSET(dev->fd, &wset))
			  {
				handle_Device_write(dev);
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
 * This is just time_stamp + timeout > now?
 */
bool
overdue(struct timeval *time_stamp, struct timeval *timeout)
{
	struct timeval now;
	struct timeval limit;
	bool result = FALSE;

	limit.tv_usec = time_stamp->tv_usec + timeout->tv_usec;
	limit.tv_sec  = time_stamp->tv_sec + timeout->tv_sec;
	if(limit.tv_usec > 1000000)
	{
		limit.tv_sec ++;
		limit.tv_usec -= 1000000;
	}
	gettimeofday(&now, NULL);
	if( (now.tv_sec > limit.tv_sec) ||
	    ((now.tv_sec == limit.tv_sec) && (now.tv_usec > limit.tv_usec)) )
		result = TRUE;
	return result;
}

/*
 *   This not only generates the needed maxfd for the select loop.
 * It sets the read and write fd_sets as well.  log write is always on,
 * listener read is always on, each client maintains READIN and WRITING
 * status in its data structure, device read is always on, and device
 * write stsus is also maintained in the device structure.  
 */ 
int
find_max_fd(Globals *g, fd_set *rs, fd_set *ws)
{
	Device *dev;
	Client *client;
	ListIterator c_iter;
	ListIterator dev_i;
	int maxfd;

	CHECK_MAGIC(g);

	FD_ZERO(rs);
	FD_ZERO(ws);

	if (log->write) FD_SET(log->fd, ws);
	if (g->listener->read) FD_SET(g->listener->fd, rs);
	maxfd = MAX(g->listener->fd, log->fd);

	c_iter = list_iterator_create(g->clients);
	while( (((Client *)client) = list_next(c_iter)) )
	{
		if (client->read_status == CLI_READING)  
			FD_SET(client->fd, rs);
		if (client->write_status == CLI_WRITING) 
			FD_SET(client->fd, ws);
		if( FD_ISSET(client->fd, rs) ||
		    FD_ISSET(client->fd, ws) )
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
void
sig_hup_handler(int signum)
{
	return;
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
void
exit_handler(int signum)
{
	syslog(LOG_NOTICE, "%s: %m", "done");
	exit(0);
}

/*
 *  During parsing struct timeval values in the config file will be
 * entered as decimals.  The lexer picks out the string and this 
 * function translates that arbitrary precision string into the 
 * seconds, micro-seconds form of a struct timeval.  Right now
 * it insists there be an integer,  decimal, integer.  I could 
 * improve the thing by allowing a variety of formats: 
 * 10 10.10 0.10 .10 
 */
void
set_tv(struct timeval *tv, char *s)
{
	int len;
	char decimal[MAX_BUF];
	int sec;
	int usec;
	int n;

	ASSERT( tv != NULL );
	n = sscanf(s, "%d.%s", &sec, decimal);
	if( (n != 2) || (sec < 0) ) 
		exit_error("Couldn't get the seconds and useconds from %s", s);
	len = strlen(decimal);
	n = sscanf(decimal, "%d", &usec);
	if( (n != 1) || (usec < 0) ) 
		exit_error("Couldn't get the useconds from %s", decimal);
	while( len > 6 )
	{
		usec /= 10;
		len--;
	}
	while( len < 6 )
	{
		usec *= 10;
		len++;
	}
	tv->tv_sec  = sec;
	tv->tv_usec = usec;
}

/*
 * constructor 
 *
 * produces:  Globals 
 */
Globals *
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
	g->cluster                  = NULL;
	g->client_prot              = NULL;
	g->specs                    = list_create(free_Spec);
	g->devs                     = list_create(free_Device);
	g->config_file              = NULL;
	g->cf                       = NULL;
	return g;
}

#ifndef NDUMP

/* 
 *   Display a debug listing of everything in the Globals struct.
 */
void
dump_Globals(Globals *g)
{
	ListIterator    act_iter;
	Action         *act;
	ListIterator    c_iter;
	Client         *client;
	ListIterator    d_iter;
	Device         *dev;
	int i;
	Node *node;
	Spec *spec;

	fprintf(stderr, "Config file: %s\n", g->config_file);
	fprintf(stderr, "Specifications:\n");
	while(g->specs != g->specs->next)
	{
		spec = g->specs->next;
		dump_Spec(spec);
	}
	fprintf(stderr, "Globals: %0x\n", (unsigned int)g);
	fprintf(stderr, "\ttimout interval: %d.%06d sec\n", 
		(int)g->timeout_interval.tv_sec, 
		(int)g->timeout_interval.tv_usec);
	dump_Cluster(g->cluster);
	dump_Server_Status(g->status);
	act_iter = list_iterator_create(g->acts);
	while( (act = (Action *)list_next(act_iter)) )
		dump_Action(g->act);
	list_iterator_destroy(act_iter);
	dump_Listener(g->listener);
	c_iter = list_iterator_create(g->clients);
	while( (client = (Client *)list_next(c_iter)) )
		dump_Client(client);
	list_iterator_destroy(c_iter);
	fprintf(stderr, "\tClient Scripts:\n");
	forcount(i, g->client_prot->num_scripts)
		dump_Script(g->client_prot->scripts[i], i);
	d_iter = list_iterator_create(g->devs);
	while( (dev = (Device *)list_next(dev_iter)) )
		dump_Device(dev);
	list_iterator_destroy(dev_iter);
	Report_Memory();
	fprintf(stderr, "That's all.\n");
}

/*
 * Helper functin for dump_Globals 
 */
void
dump_Server_Status(Server_Status status)
{
	fprintf(stderr, "\tServer Status: %d: ", status);
	if (status == Quiescent) fprintf(stderr, "Quiescent\n");
	else if (status == Occupied) fprintf(stderr, "Occupied\n");
	else fprintf(stderr, "Unknown\n");
}



#endif

/*
 *  destructor
 *
 * destroys:  Globals
 */
void
free_Globals(Globals *g)
{
	int i;

	Free(g->config_file, strlen(g->config_file));
	list_destroy(g->specs);
	free_Cluster(g->cluster);
	list_destroy(g->acts);
	free_Listener(g->listener);
	list_iterator_create(g->clients);
	list_destroy(g->clients);
	forcount(i, g->client_prot->num_scripts)
		list_destroy(g->client_prot->scripts[i]);
	Free(g->client_prot, g->client_prot->num_scripts*sizeof(List *));
	list_destroy(g->devs);
	Free(g, sizeof(Globals));
	free_Log();
}

