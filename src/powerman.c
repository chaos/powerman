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

#include <string.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "powerman.h"
#include "list.h"
#include "pm_string.h"
#include "buffer.h"
#include "config.h"
#include "device.h"
#include "exit_error.h"
#include "powermand.h"
#include "wrappers.h"
#include "hostlist.h"

#define OPT_STRING "cd:F:h10Llnp:qrvVQxzt"
static const struct option long_options[] =
{
		{"on",      no_argument,       0, '1'},
		{"off",     no_argument,       0, '0'},
		{"cycle",   no_argument,       0, 'c'},
		{"reset",   no_argument,       0, 'r'},
		{"file",    required_argument, 0, 'F'},
		{"help",    no_argument,       0, 'h'},
		{"list",    no_argument,       0, 'l'},
		{"queryon", no_argument,       0, 'q'},
		{"queryoff",no_argument,       0, 'Q'},
		{"verify",  no_argument,       0, 'v'},
		{"version", no_argument,       0, 'V'},
		{"regex",   no_argument,       0, 'x'},
		{"report",  no_argument,       0, 'z'},
		{"noexec",  no_argument,       0, 'n'},
		{"host",    required_argument, 0, 'd'},
		{"port",    required_argument, 0, 'p'},
		{"license", no_argument,       0, 'L'},
		{"telemetry", no_argument,     0, 't'},
		{0, 0, 0, 0}
};

static const struct option *longopts = long_options;

static bool debug_telemetry = FALSE; /* show protocol interactions */

typedef enum {
	CMD_ERROR, CMD_QUERY_ON, CMD_QUERY_OFF, CMD_QUERY_ALL, CMD_POWER_ON, 
	CMD_POWER_OFF, CMD_POWER_CYCLE, CMD_RESET, CMD_LISTTARG,
} command_t;

/* 
 * An instance of Config is built by process_command_line().
 */
typedef struct {
	String host;		/* server hostname */
	String service;		/* server port name */
	int port;		/* server port number */
	int fd;			/* socket open to server */
	command_t com;		/* comment to send to server */
	bool regex;		/* target list should be processed as regex */
	bool verify;		/* after requesting state change, verify it */
	bool noexec;		/* do not request server to do commands */
	hostlist_t targ;	/* target hostname list */
	List reply;		/* list-o-Strings: reply from server */
} Config;

static void dump_Conf(Config *conf);
static Config *process_command_line(int argc, char ** argv);
static void usage(char *prog);
static void read_Targets(Config *conf, char *name);
static void connect_to_server(Config *conf);
static List request_State(Config *conf);
static List get_Names(int fd);
static void get_State(int fd, List cluster);
static List dialog(int fd, const char *str);
static void process_targets(Config *conf);
static String glob2regex(String glob);
static List send_server(Config *conf, String str);
static void publish_reply(Config *conf, List reply);
static void print_readable(Config *conf, List cluster);
static void print_list(Config *conf, List cluster, State_Val state);
static void print_Targets(List targ);
static Config *make_Config();
static void free_Config(Config *conf);
static void append_Nodes(List cluster, List targ);
static void update_Nodes_soft_state(List cluster, List reply);
static void update_Nodes_hard_state(List cluster, List reply);
static Node *xmake_Node(const char *name);
static int cmp_Nodes(void *node1, void *node2);
static void xfree_Node(void *node);
static bool is_prompt(String targ);

const char *powerman_license = \
    "Copyright (C) 2001-2002 The Regents of the University of California.\n"  \
    "Produced at Lawrence Livermore National Laboratory.\n"                   \
    "Written by Andrew Uselton <uselton2@llnl.gov>.\n"                           \
    "http://www.llnl.gov/linux/powerman/\n"                                     \
    "UCRL-CODE-2002-008\n\n"                                                  \
    "PowerMan is free software; you can redistribute it and/or modify it\n"     \
    "under the terms of the GNU General Public License as published by\n"     \
    "the Free Software Foundation.\n";

#define PROMPT '>'

#define AUTHENTICATE_FMT 	"powerman\r\n"
#define GET_NAMES_FMT 		"names %s\r\n"
#define GET_HARD_UPDATE_FMT 	"update plugs\r\n"
#define GET_SOFT_UPDATE_FMT 	"update nodes\r\n"
#define DO_POWER_ON_FMT 	"on %s\r\n"
#define DO_POWER_OFF_FMT 	"off %s\r\n"
#define DO_POWER_CYCLE_FMT 	"cycle %s\r\n"
#define DO_RESET_FMT 		"reset %s\r\n"

int 
main(int argc, char **argv)
{
	Config *conf;

	init_error(argv[0], NULL);
	conf = process_command_line(argc, argv);

	/*if( geteuid() != 0 ) exit_msg("Must be root");*/

	connect_to_server(conf);

	if (conf->com == CMD_QUERY_ON || conf->com == CMD_QUERY_OFF
			|| conf->com == CMD_QUERY_ALL)
	{
		List cluster = request_State(conf);

		/* what do we do with nodes in the ST_UNKNOWN state ? */
		if (conf->com == CMD_QUERY_ON)
			print_list(conf, cluster, ON);
		else if (conf->com == CMD_QUERY_OFF)
			print_list(conf, cluster, OFF);
		else if (conf->com == CMD_QUERY_ALL)
			print_readable(conf, cluster);

		list_destroy(cluster);
	}
	else
	{
		process_targets(conf);
		if (conf->verify)
		{
			List cluster = request_State(conf);
			/* XXX verify conf->targ are all in the state 
			 * conf->com requested */
			print_readable(conf, cluster);
			list_destroy(cluster);
		}
	}
	free_Config(conf);
	return 0;
}

/*
 * Config struct source
 */
#define EXACTLY_ONE_MSG "Use exactly one command argument (see --help)"
static Config *
process_command_line(int argc, char **argv)
{
	int c;
	Config *conf;
	int longindex;
	enum { NO_TARGS, REQ_TARGS, OPT_TARGS } targs_req = NO_TARGS;

	conf = make_Config();
	opterr = 0;
	while ((c = getopt_long(argc, argv, OPT_STRING, longopts, &longindex)) != -1) 
	{
		switch(c) {
		/* 
		 * These just do something quick and exit.
		 */
		case 'V':	/* --version */
			printf("%s-%s\n", PROJECT, VERSION);
			exit(0);
		case 'L':	/* --license */
			printf("%s", powerman_license);
			exit(0);
		case 'h':	/* --help */
			usage(argv[0]);
			exit(0);
		/* 
		 * The main commands.
		 */
		case 'c':	/* --cycle */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_POWER_CYCLE;
			targs_req = REQ_TARGS;
			break;
		case '1':	/* --on */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_POWER_ON;
			targs_req = REQ_TARGS;
			break;
		case '0':	/* --off */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_POWER_OFF;
			targs_req = REQ_TARGS;
			break;
		case 'r':	/* --reset */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_RESET;
			targs_req = REQ_TARGS;
			break;
		case 'q':	/* --queryon */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_QUERY_ON;
			targs_req = NO_TARGS;
			break;
		case 'Q':	/* --queryoff */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_QUERY_OFF;
			targs_req = NO_TARGS;
			break;
		case 'l':	/* --list */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_LISTTARG;
			targs_req = OPT_TARGS;
			break;
		case 'z':	/* --report */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_QUERY_ALL;
			targs_req = NO_TARGS;
			break;
		/* 
		 * Modifiers.
		 */
		case 't':	/* --telemetry */
			debug_telemetry = TRUE;
			break;
		case 'd':	/* --host */
			if (conf->host != NULL)
				free_String( (void *)conf->host );
			conf->host = make_String(optarg);
			break;
		case 'F':	/* --file */
			read_Targets(conf, optarg);
			break;
		case 'p':	/* --port */
			/* Is optarg a legal service value integer? */
			/* If not, can you find the named service?  */
			/* If not, exit_error()                     */
			if (conf->service != NULL)
				free_String( (void *)conf->service );
			conf->service = make_String(optarg);
			break;
		case 'n':	/* --noexec */
			conf->noexec = TRUE;
			break;
		case 'v':	/* --verify */
			conf->verify = TRUE;
			break;
		case 'x':	/* --regex */
			conf->regex = TRUE;
			break;
		default:
			exit_msg("Unrecognized command line option (see --help)");
			/*NOTREACHED*/
		}
	}
	if (conf->com == CMD_ERROR)
		exit_msg(EXACTLY_ONE_MSG);

	/* build hostlist of targets */
	while (optind < argc)
	{
		if (conf->regex)
			hostlist_push_host(conf->targ, argv[optind]);
		else
			hostlist_push(conf->targ, argv[optind]);
		optind++;
	}

	if (hostlist_is_empty(conf->targ) && targs_req == REQ_TARGS)
		exit_msg("command requires target(s)");
	if (!hostlist_is_empty(conf->targ) && targs_req == NO_TARGS)
		exit_msg("command may not be specified with targets");

	/* missing target list means "all" */
	if (hostlist_is_empty(conf->targ) && targs_req == OPT_TARGS)
		hostlist_push_host(conf->targ, "*");

	/* --noexec: say what we would have done but don't contact server */
	if (conf->noexec) 
		dump_Conf(conf);

	return conf;
}

/*
 * Display powerman usage and exit.
 */
static void 
usage(char * prog)
{
    printf("Usage: %s [COMMAND OPTS] [MODIFIER OPTS] [TARGETS]\n", prog);
    printf("COMMAND OPTIONS:\n\
  -c --cycle     Power cycle targets   -r --reset    Reset targets\n\
  -1 --on        Power on targets      -0 --off      Power off targets\n\
  -q --queryon   List on targets       -Q --queryoff List off targets\n\
  -z --report    List on/off/unknown   -l --list     List targets\n\
  -h --help      Display this help\n");
    printf("MODIFIER OPTIONS:\n\
  -v --verify    Verify state change\n\
  -x --regex     Use regex not glob      -F --file FILE Get targets from file\n\
  -d --host HOST Server name [localhost] -p --port PORT Server port\n\
  -V --version   Display version info    -L --license   Display license\n");
    printf("TARGETS:\n\
  Target hostname arguments - may contain glob [default] or regex [-x]\n");
}

static void
dump_Conf(Config *conf)
{
	char tmpstr[1024];

	printf("Command: %s\n", 
			conf->com == CMD_RESET ? "Reset targets" :
			conf->com == CMD_POWER_ON ? "Power on targets" :
			conf->com == CMD_POWER_OFF ? "Power off targets" :
			conf->com == CMD_POWER_CYCLE ? "Power cycle targets" :
			conf->com == CMD_QUERY_ON ? "List on targets" :
			conf->com == CMD_QUERY_OFF ? "List off targets" :
			conf->com == CMD_QUERY_ALL ? "List all targets" :
			conf->com == CMD_LISTTARG ? "List targets" : "Error");

	hostlist_ranged_string(conf->targ, sizeof(tmpstr), tmpstr);
	printf("Targets: %s\n", tmpstr);
	exit(0);
}

/* 
 *   This should be changed to allow for a '-' in place of the 
 * file name, which would then cause it to read from stdin.
 */
static void
read_Targets(Config *conf, char *name)
{
	FILE *fp;
	unsigned char buf[MAX_BUF];
	int i = 0;
	unsigned int c;

	if( (fp = fopen(name, "r")) == NULL)
	{
		exit_msg("Coudn't open %s for target names", name);
	}
	while( (c = getc(fp)) != EOF )
	{
		if ( (c != ' ') && (c != '\r') && (c != '\n') && (c != '\t') )
		{
			buf[i] = c;
			i++;
			if ( i == MAX_BUF ) i = 0;
		}
		else
		{
			if( i > 0 )
			{
				buf[i] = '\0';
				hostlist_push(conf->targ, buf);
			}
			i = 0;
		}
	}
	fclose(fp);
}

/*
 * Set up connection to server.
 */
static void
connect_to_server(Config *conf)
{
	struct addrinfo hints, *addrinfo;
	int sock_opt;
	int n;
	List reply;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	Getaddrinfo(get_String(conf->host), get_String(conf->service), 
			    &hints, &addrinfo);

	conf->fd = Socket(addrinfo->ai_family, addrinfo->ai_socktype,
			      addrinfo->ai_protocol);
	sock_opt = 1;
	Setsockopt(conf->fd, SOL_SOCKET, SO_REUSEADDR,
		   &(sock_opt), sizeof(sock_opt));

	n = Connect(conf->fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
	freeaddrinfo(addrinfo);

	/* Connected, now "log in" */
	reply = dialog(conf->fd, NULL);
	list_destroy(reply);
	reply = dialog(conf->fd, AUTHENTICATE_FMT);
	list_destroy(reply);

}

/*
 * Request the list of nodes known to the server and their power state.
 * Results are returned as a list-o-nodes that must be freed by the caller.
 */
static List
request_State(Config *conf)
{
	List cluster;

	cluster = get_Names(conf->fd);
	get_State(conf->fd, cluster);
	list_sort(cluster, cmp_Nodes);
	return cluster;
}

/*
 * Request list of nodes known to the server.
 * Results are returned as a list-o-Nodes thatmust be freed by the caller.
 */
static List
get_Names(int fd)
{
	List cluster;
	List reply;
	char buf[MAX_BUF];

	cluster = list_create(xfree_Node);
/* get the names */
	sprintf(buf, GET_NAMES_FMT, ".*");
	reply = dialog(fd, buf);
	append_Nodes(cluster, reply); 
	list_destroy(reply);

	return cluster;
}

/* 
 * Request power state for all nodes known to powerman.
 * Fills in the p_state and n_state fields of each Node in the 
 * 'cluster' list-o-Nodes.
 */
static void
get_State(int fd, List cluster)
{
/* update the status */
	List reply;

	reply = dialog(fd, GET_SOFT_UPDATE_FMT);
	update_Nodes_soft_state(cluster, reply); 
	list_destroy(reply);

	reply = dialog(fd, GET_HARD_UPDATE_FMT);
	update_Nodes_hard_state(cluster, reply); 
	list_destroy(reply);
}

/*
 * dialog() returns a circular list without a dummy header record
 * and will exit with an error if it failed to read after "limit"
 * tries.
 *
 * List source
 */
static List
dialog(int fd, const char *str)
{
	unsigned char buf[MAX_BUF];
	int in = 0;
	int n;
	int i;
	int j;
	int len;
	int limit = 100;
	bool found = FALSE;
	List reply = list_create(free_String);
	String targ;

	if (debug_telemetry)
	{
		if (str)
			printf("C: '%s'\n", str);
	}
	memset(buf, 0, MAX_BUF);
	if( str != NULL )
	{
		len = strlen(str);
		n = Write(fd, (char *)str, len);
		if( n != len )
			exit_msg("Incomplete write of %s", str);
	}
	found = FALSE;
	while( !found && (limit > 0) )
	{
		n = Read(fd, buf + in, MAX_BUF);
		for( i = in; i < in + n; i++)
		{
			if ( buf[i] == PROMPT ) found = TRUE;
		}
		in += n;
		limit--;
	}
	if( found )
	{
		len = in;
		i = 0;
		j = 0;
		while( i < len + 1)
		{
			if( (buf[i] == '\r') || (buf[i] == '\n') || (buf[i] == '\0') )
			{
				buf[i] = '\0';
				if( j < i )
				{
					targ = make_String(buf + j);
					list_append(reply, (void *)targ);
				}
				j = i + 1;
			}
			i++;
		}
	}
	if( list_is_empty(reply) )
		exit_msg("Command \"%s\" failed.", str);

	if (debug_telemetry)
	{
		String tmpstr;

		ListIterator itr = list_iterator_create(reply);
		while( (tmpstr = (String)list_next(itr)) )
			printf("S: '%s'\n", get_String(tmpstr));
		list_iterator_destroy(itr);
	}

	return reply;
}

/*
 * Iterate through conf->targ nodes executing command for each node.
 * XXX this requires a separate handshake for every node specified;
 * consider making this more efficient.
 */
static void
process_targets(Config *conf)
{
	char *host;
	hostlist_iterator_t itr;

	if ((itr = hostlist_iterator_create(conf->targ)) == NULL)
		exit_error("hostlist_iterator_create");
	while( (host = hostlist_next(itr)) )
	{
		String targ = make_String(host);
		String str;
		List reply;

		if( conf->regex == TRUE )
			str = targ;
		else {
			str = glob2regex(targ);
			free_String(targ);
		}
		reply = send_server(conf, str);
		publish_reply(conf, reply);
		free_String(str);
		list_destroy(reply);
	}
	hostlist_iterator_destroy(itr);
}

/*
 * String source
 */
static String
glob2regex(String glob)
{
	unsigned char buf[MAX_BUF];
	int i = 0;
	String regex;
	unsigned char *g = get_String(glob);

	while(*g != '\0')
	{
		if( isalnum(*g) || (*g == '-') || (*g == '_') )
		{
			buf[i] = *g;
			i++;
		}
		else if ( (*g == '*') || (*g == '?') )
		{
			buf[i] = '.';
			i++;
			buf[i] = *g;
			i++;
		}
		else
			exit_msg("%c is not a legal character for a host name", *g);
		g++;
	}
	buf[i] = '\0';
	regex = make_String(buf);
	return regex;	
}

/* 
 * List source
 */
static List
send_server(Config *conf, String str)
{
	List reply;
	char buf[MAX_BUF];

	switch(conf->com)
	{
	case CMD_LISTTARG:
		sprintf(buf, GET_NAMES_FMT, get_String(str));
		break;
	case CMD_POWER_ON:
		sprintf(buf, DO_POWER_ON_FMT, get_String(str));
		break;
	case CMD_POWER_OFF:
		sprintf(buf, DO_POWER_OFF_FMT, get_String(str));
		break;
	case CMD_POWER_CYCLE:
		sprintf(buf, DO_POWER_CYCLE_FMT, get_String(str));
		break;
	case CMD_RESET:
		sprintf(buf, DO_RESET_FMT, get_String(str));
		break;
	default :
		assert(FALSE);
	}
	reply = dialog(conf->fd, buf) ;
	return reply;
}

static void 
publish_reply(Config *conf, List reply)
{
	switch(conf->com)
	{
		case CMD_POWER_ON:
		case CMD_POWER_OFF:
		case CMD_POWER_CYCLE:
		case CMD_RESET:
			break;
		case CMD_LISTTARG:
			print_Targets(reply);
			break;
		default:
			assert(FALSE);
			break;
	}
}

static void
warn_softstate(char *host)
{
	fprintf(stderr, "warning: %s: plug is on but node may not be\n", host);
}

/*
 * Produce a readable report showing who's on and who's off (--report)
 */
static void
print_readable(Config *conf, List cluster)
{
	hostlist_t on = hostlist_create(NULL);
	hostlist_t off = hostlist_create(NULL);
	hostlist_t unk = hostlist_create(NULL);
	Node *node;
	ListIterator itr;
	char tmpstr[1024];

	if (!on || !off || !unk)
		exit_error("hostlist_create");

	/* Sort nodes into three hostlist bins */
	itr = list_iterator_create(cluster);
	while( (node = (Node *)list_next(itr)))
	{
		switch (node->p_state) 
		{
			case ON:
				hostlist_push_host(on, get_String(node->name));
				if (node->n_state != ON)
					warn_softstate(get_String(node->name));
				break;
			case OFF:
				hostlist_push_host(off, get_String(node->name));
				break;
			case ST_UNKNOWN:
				hostlist_push_host(unk, get_String(node->name));
				break;
		}
	}
	list_iterator_destroy(itr);

	/* Report the status */
	if (! hostlist_is_empty(on)) {
		hostlist_ranged_string(on, sizeof(tmpstr), tmpstr);
		printf("on\t%s\n", tmpstr);
	}
	if (! hostlist_is_empty(off)) {
		hostlist_ranged_string(off, sizeof(tmpstr), tmpstr);
		printf("off\t%s\n", tmpstr);
	}
	if (! hostlist_is_empty(unk)) {
		hostlist_ranged_string(unk, sizeof(tmpstr), tmpstr);
		printf("unknown\t%s\n", tmpstr);
	}	

	hostlist_destroy(on);
	hostlist_destroy(off);
	hostlist_destroy(unk);
}

static void
print_list(Config *conf, List cluster, State_Val state)
{
	Node *node;
	ListIterator itr;

	itr = list_iterator_create(cluster);
	while( (node = (Node *)list_next(itr)))
	{
		if( node->p_state == state ) 
			printf("%s\n", get_String(node->name));
		if (node->p_state == ON && node->n_state == OFF)
			warn_softstate(get_String(node->name));
	}
	list_iterator_destroy(itr);
}

static void
print_Targets(List targ)
{
	String t;
	ListIterator itr;

	assert( targ != NULL );
	itr = list_iterator_create(targ);
	while( (t = (String )list_next(itr)) && !is_prompt(t) )
	{
		printf("%s\n", get_String(t));
	}
	list_iterator_destroy(itr);
}


/*
 * Config source
 */
static Config *
make_Config(void)
{
	Config *conf;
	char *env;
	char *port;
	int p;

	conf = (Config *)Malloc(sizeof(Config));
	conf->noexec = FALSE;
	conf->host = NULL;
	conf->service = NULL;
	if ((env = getenv("POWERMAN_HOST"))) 
	{
		if ((port = strchr(env, ':'))) 
		{
			*port++ = '\0';
			if ((p = atoi(port)) > 0)
			{
				conf->service = make_String(port);
				conf->port = p;
			}
		}
		if (*env) 
		{
			conf->host = make_String(env);
		}
	}
	if( conf->host == NULL )
		conf->host = make_String("localhost");
	if ((port = getenv("CONMAN_PORT")) && (*env)) 
	{
		if ((p = atoi(port)) > 0)
		{
			conf->service = make_String(port);
			conf->port = p;
		}
	}
	if(conf->service == NULL )
	{
		conf->service = make_String("10101");
		conf->port = 10101;
	}
	conf->com = CMD_ERROR;
	conf->regex = FALSE;
	conf->verify = FALSE;
	conf->targ = hostlist_create(NULL);
	conf->reply = NULL;
	return conf;
}

/*
 * Config sink
 */
static void
free_Config(Config *conf)
{
	free_String((void *)conf->host);
	free_String((void *)conf->service);
	hostlist_destroy(conf->targ);
	if( conf->reply != NULL)
		list_destroy(conf->reply);
	Free(conf);
}

static void 
append_Nodes(List cluster, List reply)
{
	Node *node;
	ListIterator itr;
	String targ;
	
	assert( reply != NULL );
	itr = list_iterator_create(reply);
	while( (targ = (String)list_next(itr)) && !is_prompt(targ) )
	{
		node = xmake_Node(get_String(targ));
		list_append(cluster, node);
	}
	list_iterator_destroy(itr);
}

/* 
 * Process 'reply', obtained in response to a GET_SOFT_UDPATE_FMT request.
 * Reply will consist of one string of ones and zeroes indicating soft power 
 * state for 'cluster' node list.  Update the 'n_state' field for each node
 * in the 'cluster' list.
 */
static void 
update_Nodes_soft_state(List cluster, List reply)
{
	Node *node;
	ListIterator node_i;
	int i = 0;
	String targ;

	assert( reply != NULL );
	targ = (String)list_pop(reply); /* reply list is always one String */
	assert( targ != NULL );
	assert( !empty_String(targ) );

	node_i = list_iterator_create(cluster);
	node = (Node *)list_next(node_i);
	for(i = 0; i < length_String(targ); i++)
	{
		assert( node != NULL );
		switch(byte_String(targ, i))
		{
		case '0' :
			node->n_state = OFF;
			break;
		case '1' :
			node->n_state = ON;
			break;
		case '?' :
			node->n_state = ST_UNKNOWN;
			break;
		default :
			exit_msg("Illegal soft state value.");
		}
		node = (Node *)list_next(node_i);
	}
	list_iterator_destroy(node_i);
}

/* 
 * Process 'reply', obtained in response to a GET_SOFT_UDPATE_FMT request.
 * Reply will consist of a string of ones and zeroes indicating soft power 
 * state for 'cluster' node list.  Update the 'n_state' field for each node
 * in the 'cluster' list.
 */
static void 
update_Nodes_hard_state(List cluster, List reply)
{
	Node *node;
	ListIterator node_i;
	int i = 0;
	String targ;

	assert( reply != NULL );
	targ = (String)list_pop(reply);	/* reply list is always one String */
	assert( targ != NULL );
	assert( !empty_String(targ) );

	node_i = list_iterator_create(cluster);
	node = (Node *)list_next(node_i);
	for (i = 0; i < length_String(targ); i++)
	{
		assert( node != NULL );
		switch(byte_String(targ, i))
		{
		case '0' :
			node->p_state = OFF;
			break;
		case '1' :
			node->p_state = ON;
			break;
		case '?' :
			node->p_state = ST_UNKNOWN;
			break;
		default :
			exit_msg("Illegal hard state value.");
		}
		node = (Node *)list_next(node_i);
	}
	list_iterator_destroy(node_i);
}

/*
 * Node source
 */
static Node *
xmake_Node(const char *name)
{
	Node *node;

	node = (Node *)Malloc(sizeof(Node));
	node->name = make_String(name);
	node->n_state = ST_UNKNOWN;
	node->p_state = ST_UNKNOWN;
	return node;
}

static int
cmp_Nodes(void *node1, void *node2)
{
	assert(((Node *)node1) != NULL);
	assert(((Node *)node1)->name != NULL);
	assert(((Node *)node2) != NULL);
	assert(((Node *)node2)->name != NULL);

	return strcmp(get_String(((Node *)node1)->name), 
			get_String(((Node *)node2)->name));
}

static void
xfree_Node(void *node)
{
	free_String( (void *)((Node *)node)->name);
	Free(node);
}

static bool
is_prompt(String targ)
{
	int i = 0;
	bool result = FALSE;
	
	assert(targ != NULL);

	for (i = 0; i < length_String(targ); i++)
	{
		if(byte_String(targ, i) == PROMPT) result = TRUE;
		i++;
	}
	return result;
}
