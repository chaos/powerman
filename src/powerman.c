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

#define OPT_STRING "cd:F:h10Llnp:qrsvVQxz"
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
		{"soft",    no_argument,       0, 's'},
		{"verify",  no_argument,       0, 'v'},
		{"version", no_argument,       0, 'V'},
		{"regex",   no_argument,       0, 'x'},
		{"report",  no_argument,       0, 'z'},
		{"noexec",  no_argument,       0, 'n'},
		{"host",    required_argument, 0, 'd'},
		{"port",    required_argument, 0, 'p'},
		{"license", no_argument,       0, 'L'},
		{0, 0, 0, 0}
};

static const struct option *longopts = long_options;

typedef enum {
	CMD_ERROR, CMD_QUERY_ON, CMD_QUERY_OFF, CMD_QUERY_ALL, CMD_POWER_ON, 
	CMD_POWER_OFF, CMD_POWER_CYCLE, CMD_RESET, CMD_LISTTARG,
} _command_t;

/* config is the result of argument processing */
typedef struct {
	String host;
	String service;
	int port;
	int fd;
	_command_t com;
	bool regex;
	bool soft;
	bool verify;
	List targ;
	List reply;
	hostlist_t ntarg;
	bool noexec;
} Config;

static void dump_Conf(Config *conf);
static Config *process_command_line(int argc, char ** argv);
static void usage(char *prog);
static void read_Targets(Config *conf, char *name);
static List connect_to_server(Config *conf);
static List get_Names(int fd);
static void get_State(int fd, List cluster);
static List dialog(int fd, const char *str);
static void process_targets(Config *conf, List cluster);
static String glob2regex(String glob);
static List send_server(Config *conf, String str);
static void publish_reply(Config *conf, List cluster, List reply);
static void print_readable(Config *conf, List cluster);
static List get_next_subrange(List nodes);
static void subrange_report(List nodes, bool soft, State_Val state, char *state_string);
static void print_list(Config *conf, List cluster, List targ, State_Val state);
static void print_Targets(List targ);
static Config *make_Config();
static void free_Config(Config *conf);
static void append_Nodes(List cluster, List targ);
static List transfer_Nodes(List cluster);
static void join_Nodes(List list1, List list2);
static void update_Nodes_soft_state(List cluster, List reply);
static void update_Nodes_hard_state(List cluster, List reply);
static Node *xmake_Node(const char *name);
static int cmp_Nodes(void *node1, void *node2);
static int xmatch_Node(void *node, void *key);
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
	List cluster;

	init_error(argv[0], NULL);
	conf = process_command_line(argc, argv);

	/*if( geteuid() != 0 ) exit_msg("Must be root");*/

	cluster = connect_to_server(conf);
	if( list_is_empty(cluster) ) 
		exit_msg("No cluster members found");
	process_targets(conf, cluster);
	free_Config(conf);
	list_destroy(cluster);
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
	String targ;
	Config *conf;
	int longindex;
	bool targs_required = FALSE;

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
			targs_required = TRUE;
			break;
		case '1':	/* --on */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_POWER_ON;
			targs_required = TRUE;
			break;
		case '0':	/* --off */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_POWER_OFF;
			targs_required = TRUE;
			break;
		case 'q':	/* --queryon */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_QUERY_ON;
			break;
		case 'Q':	/* --queryoff */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_QUERY_OFF;
			break;
		case 'r':	/* --reset */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_RESET;
			targs_required = TRUE;
			break;
		case 'l':	/* --list */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_LISTTARG;
			break;
		case 'z':	/* --report */
			if (conf->com != CMD_ERROR)
				exit_msg(EXACTLY_ONE_MSG);
			conf->com = CMD_QUERY_ALL;
			break;
		/* 
		 * Modifiers.
		 */
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
		case 's':	/* --soft */
			conf->soft = TRUE;
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

	/*
	 * Prior code pushed args directly onto conf->targ.
	 * This code builds conf->ntarg with Mark's hostlist_t package to
	 * get support for Quadrics style host ranges.  
	 */
	while (optind < argc)
	{
		if (conf->regex)
			hostlist_push_host(conf->ntarg, argv[optind]);
		else
			hostlist_push(conf->ntarg, argv[optind]);
		optind++;
	}
	if (hostlist_count(conf->ntarg) > 0) 
	{
		char *host;
		hostlist_iterator_t iter;

		iter = hostlist_iterator_create(conf->ntarg);
		while ((host = hostlist_next(iter)) != NULL) {
			targ = make_String(host);
			list_append(conf->targ, (void *)targ);
			free(host);
		}
		hostlist_iterator_destroy(iter);
	}

	if (list_is_empty(conf->targ)) {
		if (targs_required)
			exit_msg("command requires target(s)");
		else {
			targ = make_String("*");
			list_append(conf->targ, (void *)targ);
		}
	}
	/* If --noexec, just say what we would have done but don't contact
	 * the server.
	 */
	if (conf->noexec) 
	{
		dump_Conf(conf);
	}
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
  -s --soft      Use soft power status   -v --verify    Verify state change\n\
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
			conf->com == CMD_LISTTARG ? "List targets" : "Error");

	hostlist_ranged_string(conf->ntarg, sizeof(tmpstr), tmpstr);
	printf("Targets: %s\n", tmpstr);
	/*print_Targets(conf->targ);*/
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
	String targ;

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
				targ = make_String(buf);
				list_append(conf->targ, (void *)targ);
			}
			i = 0;
		}
	}
	fclose(fp);
}

/*
 * List source
 */
static List
connect_to_server(Config *conf)
{
	struct addrinfo hints, *addrinfo;
	int sock_opt;
	int n;
	List cluster;
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

	reply = dialog(conf->fd, NULL);
	list_destroy(reply);
	reply = dialog(conf->fd, AUTHENTICATE_FMT);
	list_destroy(reply);

	cluster = get_Names(conf->fd);
	get_State(conf->fd, cluster);
	list_sort(cluster, cmp_Nodes);
	return cluster;
}

/*
 * List source
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
	return reply;
}

static void
process_targets(Config *conf, List cluster)
{
	String targ;
	List reply;
	ListIterator itr;
	String str;

	itr = list_iterator_create(conf->targ);
	while( (targ = (String)list_next(itr)) )
	{
		if( conf->regex == TRUE )
			str = targ;
		else
			str = glob2regex(targ);
		reply = send_server(conf, str);
		publish_reply(conf, cluster, reply);
	}
	list_iterator_destroy(itr);
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
	case CMD_QUERY_ON:
	case CMD_QUERY_OFF:
	case CMD_QUERY_ALL:
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
publish_reply(Config *conf, List cluster, List reply)
{
	State_Val state = OFF;

	switch(conf->com)
	{
	case CMD_QUERY_ON:
		state = ON;
	case CMD_QUERY_OFF:
		print_list(conf, cluster, reply, state);
		break;
	case CMD_QUERY_ALL:
		print_readable(conf, cluster);
	case CMD_POWER_ON:
	case CMD_POWER_OFF:
	case CMD_POWER_CYCLE:
	case CMD_RESET:
		if( conf->verify == TRUE )
		{
			get_State(conf->fd, cluster);
			print_readable(conf, cluster);
		}
		break;
	case CMD_LISTTARG       :
		print_Targets(reply);
		break;
	default :
		assert(FALSE);
		break;
	}
}

static void
print_readable(Config *conf, List cluster)
{
/*
 * This is probably more complicated than it needs to be 
 */

	List subrange;
	List list;

	list = transfer_Nodes(cluster);
	while( !list_is_empty(list) )
	{
		subrange = get_next_subrange(list);
		subrange_report(subrange, conf->soft, ST_UNKNOWN, "unknown");
		subrange_report(subrange, conf->soft, OFF, "off");
		subrange_report(subrange, conf->soft, ON, "on");
		join_Nodes(cluster, subrange);
	}
}

/*
 * List source
 */
static List
get_next_subrange(List nodes)
{
	List list;
	Node *node;
	Node *first_node;

	assert(nodes != NULL);

	first_node = (Node *)list_pop(nodes);
	assert(first_node != NULL);

	list = list_create(xfree_Node);
	list_append(list, (void *)first_node);
	node = (Node *)list_peek(nodes);
	while( node != NULL && prefix_match(first_node->name, node->name) )
	{
		node = (Node *)list_pop(nodes);
		list_append(list, (void *)node);
		node = (Node *)list_peek(nodes);
	}
	return list;
}

static void
subrange_report(List nodes, bool soft, State_Val state, char *state_string)
{
	Node *node;
	Node *next;
	Node *start;
	ListIterator node_i;
	int num = 0;
	char buf[MAX_BUF];
	bool first;

	assert(nodes != NULL);
	node_i = list_iterator_create(nodes);
	while( (node = (Node *)list_next(node_i)) )
	{
		assert(node != NULL);
		assert(node->name != NULL);
		assert( !empty_String(node->name) );

		if( soft )
		{
			if( node->n_state == state ) num++;
		}
		else
		{
			if( node->p_state == state ) num++;
		}
	}
	list_iterator_reset(node_i);
	if( num == 0 ) return;
	
	/* There's at least one so queue up on it */
	if( soft )
		while( (node = (Node *)list_next(node_i)) && 
		       (node->n_state != state) )
			;
	else
		while( (node = (Node *)list_next(node_i)) && 
		       node->p_state != state )
			;
	if( num == 1 )
	{
		printf("%s\t\t%s\n", state_string, get_String(node->name));
		list_iterator_destroy(node_i);
		return;
	}

	/* there are at least two, so use tux[...] notation */

	memset(buf, 0, MAX_BUF);
	strncpy(buf, get_String(node->name), prefix_String(node->name));
	printf("%s\t\t%s[%d", state_string, buf, index_String(node->name));
	first = TRUE;
	next = node;
	start = node;
	while( next != NULL )
	{
		if( soft )
			while( (next = (Node *)list_next(node_i)) && 
			       (next->n_state == state) ) node = next;
		else
			while( (next = (Node *)list_next(node_i)) && 
			       next->p_state == state ) node = next;
		if( node != start )
			printf("-%d", index_String(node->name));

		if( next == NULL ) continue;
		if( soft )
			while( (next = (Node *)list_next(node_i)) && 
			       (next->n_state != state) ) 
				;
		else
			while( (next = (Node *)list_next(node_i)) && 
			       next->p_state != state ) 
				;
		if( next == NULL ) continue;
		node = next;
		start = next;
		printf(",%d", index_String(node->name));

	}
	printf("]\n");
}

static void
print_list(Config *conf, List cluster, List targ, State_Val state)
{
	Node *node;
	ListIterator itr;
	String t;

	assert( targ != NULL );
	itr = list_iterator_create(targ);
	while( (t = (String )list_next(itr)) && !is_prompt(t) )
	{
		node = list_find_first(cluster, xmatch_Node, get_String(t));
		if( node == NULL ) exit_msg("No such node as \"%s\".", get_String(t));
		if( conf->soft == TRUE )
		{
			if( node->n_state == state ) printf("%s\n", get_String(node->name));
		}
		else
		{
			if( node->p_state == state ) printf("%s\n", get_String(node->name));
		}
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
	conf->targ = list_create(free_String);
	conf->ntarg = hostlist_create(NULL);
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
	list_destroy(conf->targ);
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
 * List source
 */
static List
transfer_Nodes(List cluster)
{
	Node *node;
	List nodes = list_create(xfree_Node);

	while( (node = (Node *)list_pop(cluster)) )
		list_append(nodes, (void *)node);
	return nodes;
}


static void
join_Nodes(List list1, List list2)
{
	Node *node;

	assert( list1 != NULL );
	assert( list2 != NULL );
	while( (node = (Node *)list_pop(list2)) )
	{
		list_append(list1, (void *)node);
	}
	list_destroy(list2);	
}

static void 
update_Nodes_soft_state(List cluster, List reply)
{
	Node *node;
	ListIterator node_i;
	int i = 0;
	String targ;

	assert( reply != NULL );
	targ = (String)list_pop(reply);
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

static void 
update_Nodes_hard_state(List cluster, List reply)
{
	Node *node;
	ListIterator node_i;
	int i = 0;
	String targ;

	assert( reply != NULL );
	targ = (String)list_pop(reply);
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

	return cmp_String(((Node *)node1)->name, 
			  ((Node *)node2)->name);
}

static int
xmatch_Node(void *node, void *key)
{
	if( match_String(((Node *)node)->name, (char *)key) )
		return TRUE;
	return FALSE;	
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
