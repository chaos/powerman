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

#include "powerman.h"
#include "config.h"
#include "client.h"

#define ERROR     (-1)
#define WHOSON      0
#define WHOSOFF     1
#define POWER_ON    2
#define POWER_OFF   3
#define POWER_CYCLE 4
#define RESET       5
#define NAMES       6

const struct option long_options[] =
{                             /* cd:F:hHlLnp:qrsvVwxz */
		{"cycle",   no_argument,       0, 'c'},
		{"host",    required_argument, 0, 'd'},
		{"file",    required_argument, 0, 'F'},
		{"help",    no_argument,       0, 'h'},
		{"on",      no_argument,       0, 'H'},
		{"off",     no_argument,       0, 'l'},
		{"license", no_argument,       0, 'L'},
		{"list",    no_argument,       0, 'n'},
		{"port",    required_argument, 0, 'p'},
		{"query",   no_argument,       0, 'q'},
		{"reset",   no_argument,       0, 'r'},
		{"soft",    no_argument,       0, 's'},
		{"verify",  no_argument,       0, 'v'},
		{"version", no_argument,       0, 'V'},
		{"whosoff", no_argument,       0, 'w'},
		{"regex",   no_argument,       0, 'x'},
		{"report",  no_argument,       0, 'z'},
		{0, 0, 0, 0}
};

const struct option *longopts = long_options;



typedef struct config_struct {
	String *host;
	String *service;
	int fd;
	int com;
	bool regex;
	bool soft;
	bool readable;
	bool verify;
	List targ;
	List reply;
}Config;

static Config *process_command_line(int argc, char ** argv);
static void usage(char *prog);
static void read_Targets(Config *conf, char *name);
static List connect_to_server(Config *conf);
static List get_Names(int fd);
static void get_State(int fd, List cluster);
static List dialog(int fd, const char *str);
static void process_targets(Config *conf, List cluster);
static String *glob2regex(String *glob);
static List send_server(Config *conf, String *str);
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
extern Node *make_Node(const char *name);
extern int cmp_Nodes(void *node1, void *node2);
extern int match_Node(void *node, void *key);
extern void free_Node(void *node);
static bool is_prompt(String *targ);
extern void exit_error(const char *fmt, ...);

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

const char *com_strings[] =
{
	"powerman\r\n",
	"names %s\r\n",
	"update plugs\r\n",
	"update nodes\r\n",
	"on %s\r\n",
	"off %s\r\n",
	"cycle %s\r\n",
	"reset %s\r\n",
	"error\r\n"
};

#define AUTHENTICATE    com_strings[0]
#define GET_NAMES       com_strings[1]
#define GET_HARD_UPDATE com_strings[2]
#define GET_SOFT_UPDATE com_strings[3]
#define DO_POWER_ON     com_strings[4]
#define DO_POWER_OFF    com_strings[5]
#define DO_POWER_CYCLE  com_strings[6]
#define DO_RESET        com_strings[7]

int 
main(int argc, char **argv)
{
	Config *conf;
	List cluster;

	if( geteuid() != 0 ) exit_error("Must be root\n");

	conf = process_command_line(argc, argv);
	cluster = connect_to_server(conf);
	errno = 0;
	if( list_is_empty(cluster) ) 
		exit_error("No cluster members found");
	process_targets(conf, cluster);
	free_Config(conf);
	list_destroy(cluster);
	return 0;
}

/*
 * Config struct source
 */
Config *
process_command_line(int argc, char **argv)
{
	int c;
	int i;
	String *targ;
	Config *conf;
	int longindex;

	conf = make_Config();
	opterr = 0;
	while ((c = getopt_long(argc, argv, "cd:F:hHlLnp:qrsvVwxz", longopts, &longindex)) != -1) 
	{
		errno = 0;
		switch(c) {
		case 'c':
			if (conf->com != ERROR)
				exit_error("Use exactly one of -c, -H, -l, -n, -q, -r, and -w");
			conf->com = POWER_CYCLE;
			break;
		case 'd':
			if (conf->host != NULL)
				free_String( (void *)conf->host );
			conf->host = make_String(optarg);
			break;
		case 'F':
			read_Targets(conf, optarg);
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'H':
			if (conf->com != ERROR)
				exit_error("Use exactly one of -c, -H, -l, -n, -q, -r, and -w");
			conf->com = POWER_ON;
			break;
		case 'l':
			if (conf->com != ERROR)
				exit_error("Use exactly one of -c, -H, -l, -n, -q, -r, and -w");
			conf->com = POWER_OFF;
			break;
		case 'L':
			printf("%s", powerman_license);
			exit(0);
		case 'n':
			if (conf->com != ERROR)
				exit_error("Use exactly one of -c, -H, -l, -n, -q, -r, and -w");
			conf->com = NAMES;
			break;
		case 'p':
			/* Is optarg a legal service value integer? */
			/* If not, can you find the named service?  */
			/* If not, exit_error()                     */
			if (conf->service != NULL)
				free_String( (void *)conf->service );
			conf->service = make_String(optarg);
			break;
		case 'q':
			if (conf->com != ERROR)
				exit_error("Use exactly one of -c, -H, -l, -n, -q, -r, and -w");
			conf->com = WHOSON;
			break;
		case 'r':
			if (conf->com != ERROR)
				exit_error("Use exactly one of -c, -H, -l, -n, -q, -r, and -w");
			conf->com = RESET;
			break;
		case 's':
			conf->soft = TRUE;
			break;
		case 'v':
			conf->verify = TRUE;
			break;
		case 'V':
			printf("%s-%s\n", PROJECT, VERSION);
			exit(0);
		case 'w':
			if (conf->com != ERROR)
				exit_error("Use exactly one of -c, -H, -l, -n, -q, -r, and -w");
			conf->com = WHOSOFF;
			break;
		case 'x':
			conf->regex = TRUE;
			break;
		case 'z':
			conf->readable = TRUE;
			break;
		default:
			printf("Unrecognized command line option (try -h).\n");
			exit(0);
			break;
		}
	}
	if (conf->com == ERROR)
		exit_error("Use exactly one of -c, -H, -l, -n, -q, -r, and -w");
	i = optind;
	while (i < argc)
	{
		targ = make_String(argv[i]);
		list_append(conf->targ, (void *)targ);
		i++;
	}
	return conf;
}

void usage(char * prog)
{
    printf("\nUsage: %s [OPTIONS] [TARGETS]\n", prog);
    printf("OPTIONS:\n");
    printf("  -c --cycle     Power cycle the targets.\n");
    printf("  -d --host HOST Specify server host name [localhost].\n");
    printf("  -F --file FILE Get targets from FILE.\n");
    printf("  -h --help      Display this help.\n");
    printf("  -H --on        Power on the targets.\n");
    printf("  -l --off       Power off the targets.\n");
    printf("  -L --license   Display license information.\n");
    printf("  -n --list      List nodes that would be targets (but don't do anything).\n");
    printf("  -p --port PORT Specify serverport or service name [10101].\n");
    printf("  -q --query     Query which amongst targets is on.\n");
    printf("  -r --reset     Reset the targets.\n");
    printf("  -s --soft      Use soft power status.\n");
    printf("  -v --verify    Verify command (esp. after power control command).\n");
    printf("  -V --version   Display version information.\n");
    printf("  -w --whosoff   Query which amongst targets is off.\n");
    printf("  -x --regex     Interpret targets as RegEx expressions.\n");
    printf("  -z --report    Make human readable report of nodes states.\n");
    printf("Use exactly one of -c, -H, -l, -n, -q, -r, and -w.\n");
    printf("TARGETS:\n");
    printf("            After the last option you may list any number of\n");
    printf("            targets.  A target is a white-space-delimitted\n");
    printf("            string that is interpretted as a GLOB (default)\n");
    printf("            or a RegEx.  Any host whose name matches under\n");
    printf("            the corresponding rules will receive the given\n");
    printf("            action (query, on, off, ...).\n");
    printf("\n");
    return;
}

void
read_Targets(Config *conf, char *name)
{
	FILE *fp;
	unsigned char buf[MAX_BUF];
	int i = 0;
	unsigned int c;
	String *targ;

	errno = 0;
	if( (fp = fopen(name, "r")) == NULL)
	{
		exit_error("Coudn't open %s for target names", name);
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
List
connect_to_server(Config *conf)
{
	struct addrinfo hints, *addrinfo;
	int sock_opt;
	int n;
	List cluster;
	List reply;

	bzero(&hints, sizeof(struct addrinfo));
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
	reply = dialog(conf->fd, AUTHENTICATE);
	list_destroy(reply);

	cluster = get_Names(conf->fd);
	get_State(conf->fd, cluster);
	list_sort(cluster, cmp_Nodes);
	return cluster;
}

/*
 * List source
 */
List
get_Names(int fd)
{
	List cluster;
	List reply;
	char buf[MAX_BUF];

	cluster = list_create(free_Node);
/* get the names */
	sprintf(buf, GET_NAMES, ".*");
	reply = dialog(fd, buf);
	append_Nodes(cluster, reply); 
	list_destroy(reply);

	return cluster;
}

void
get_State(int fd, List cluster)
{
/* update the status */
	List reply;

	reply = dialog(fd, GET_SOFT_UPDATE);
	update_Nodes_soft_state(cluster, reply); 
	list_destroy(reply);

	reply = dialog(fd, GET_HARD_UPDATE);
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
List
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
	String *targ;

	bzero(buf, MAX_BUF);
	if( str != NULL )
	{
		len = strlen(str);
		errno = 0;
		n = Write(fd, (char *)str, len);
		if( n != len )
			exit_error("Incomplete write of %s.", str);
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
	errno = 0;
	if( list_is_empty(reply) )
		exit_error("Command \"%s\" failed.", str);
	return reply;
}

void
process_targets(Config *conf, List cluster)
{
	String *targ;
	List reply;
	ListIterator itr;
	String *str;

	itr = list_iterator_create(conf->targ);
	while( (targ = (String *)list_next(itr)) )
	{
		if( conf->regex == TRUE )
			str = targ;
		else
			str = glob2regex(targ);
		reply = send_server(conf, str);
		publish_reply(conf, cluster, reply);
	}
}

/*
 * String source
 */
String *
glob2regex(String *glob)
{
	unsigned char buf[MAX_BUF];
	int i = 0;
	String *regex;
	unsigned char *g = get_String(glob);

	errno = 0;
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
			exit_error("%c is not a legal character for a host name", *g);
		g++;
	}
	buf[i] = '\0';
	regex = make_String(buf);
	return regex;	
}

/* 
 * List source
 */
List
send_server(Config *conf, String *str)
{
	List reply;
	char buf[MAX_BUF];

	switch(conf->com)
	{
	case WHOSON      :
	case WHOSOFF     :
	case NAMES       :
		sprintf(buf, GET_NAMES, get_String(str));
		break;
	case POWER_ON    :
		sprintf(buf, DO_POWER_ON, get_String(str));
		break;
	case POWER_OFF   :
		sprintf(buf, DO_POWER_OFF, get_String(str));
		break;
	case POWER_CYCLE :
		sprintf(buf, DO_POWER_CYCLE, get_String(str));
		break;
	case RESET       :
		sprintf(buf, DO_RESET, get_String(str));
		break;
	default :
		ASSERT(FALSE);
	}
	reply = dialog(conf->fd, buf) ;
	return reply;
}

void 
publish_reply(Config *conf, List cluster, List reply)
{
	State_Val state = OFF;

	switch(conf->com)
	{
	case WHOSON      :
		state = ON;
	case WHOSOFF     :
		if( conf->readable == TRUE ) 
			print_readable(conf, cluster);
		else
			print_list(conf, cluster, reply, state);
		break;
	case POWER_ON    :
	case POWER_OFF   :
	case POWER_CYCLE :
	case RESET       :
		if( conf->verify == TRUE )
		{
			get_State(conf->fd, cluster);
			print_readable(conf, cluster);
		}
		break;
	case NAMES       :
		print_Targets(reply);
		break;
	default :
		ASSERT(FALSE);
		break;
	}
}

void
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
List
get_next_subrange(List nodes)
{
	List list;
	Node *node;
	Node *first_node;

	ASSERT(nodes != NULL);

	first_node = (Node *)list_pop(nodes);
	ASSERT(first_node != NULL);

	list = list_create(free_Node);
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

void
subrange_report(List nodes, bool soft, State_Val state, char *state_string)
{
	Node *node;
	Node *next;
	Node *start;
	ListIterator node_i;
	int num = 0;
	char buf[MAX_BUF];
	bool first;

	ASSERT(nodes != NULL);
	node_i = list_iterator_create(nodes);
	while( (node = (Node *)list_next(node_i)) )
	{
		ASSERT(node != NULL);
		ASSERT(node->name != NULL);
		ASSERT( !empty_String(node->name) );

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

	bzero(buf, MAX_BUF);
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

void
print_list(Config *conf, List cluster, List targ, State_Val state)
{
	Node *node;
	ListIterator itr;
	String *t;

	ASSERT( targ != NULL );
	itr = list_iterator_create(targ);
	while( (t = (String *)list_next(itr)) && !is_prompt(t) )
	{
		errno = 0;
		node = list_find_first(cluster, match_Node, get_String(t));
		if( node == NULL ) exit_error("No such node as \"%s\".", get_String(t));
		if( conf->soft == TRUE )
		{
			if( node->n_state == state ) printf("%s\n", get_String(node->name));
		}
		else
		{
			if( node->p_state == state ) printf("%s\n", get_String(node->name));
		}
	}
}

void
print_Targets(List targ)
{
	String *t;
	ListIterator itr;

	ASSERT( targ != NULL );
	itr = list_iterator_create(targ);
	while( (t = (String *)list_next(itr)) && !is_prompt(t) )
	{
		printf("%s\n", get_String(t));
	}
}

/*
 * Config source
 */
Config *
make_Config()
{
	Config *conf;

	conf = (Config *)Malloc(sizeof(Config));
	conf->host = make_String("localhost");
	conf->service = make_String("10101");
	conf->com = ERROR;
	conf->regex = FALSE;
	conf->readable = FALSE;
	conf->verify = FALSE;
	conf->targ = list_create(free_String);
	conf->reply = NULL;
	return conf;
}

/*
 * Config sink
 */
void
free_Config(Config *conf)
{
	free_String((void *)conf->host);
	free_String((void *)conf->service);
	list_destroy(conf->targ);
	if( conf->reply != NULL)
		list_destroy(conf->reply);
	Free(conf, sizeof(Config));
}

void 
append_Nodes(List cluster, List reply)
{
	Node *node;
	ListIterator itr;
	String *targ;
	
	ASSERT( reply != NULL );
	itr = list_iterator_create(reply);
	while( (targ = (String *)list_next(itr)) && !is_prompt(targ) )
	{
		node = make_Node(get_String(targ));
		list_append(cluster, node);
	}
}

/*
 * List source
 */
List
transfer_Nodes(List cluster)
{
	Node *node;
	List nodes = list_create(free_Node);

	while( (node = (Node *)list_pop(cluster)) )
		list_append(nodes, (void *)node);
	return nodes;
}


void
join_Nodes(List list1, List list2)
{
	Node *node;

	ASSERT( list1 != NULL );
	ASSERT( list2 != NULL );
	while( (node = (Node *)list_pop(list2)) )
	{
		list_append(list1, (void *)node);
	}
	list_destroy(list2);	
}

void 
update_Nodes_soft_state(List cluster, List reply)
{
	Node *node;
	ListIterator node_i;
	int i = 0;
	String *targ;

	ASSERT( reply != NULL );
	targ = (String *)list_pop(reply);
	ASSERT( targ != NULL );
	ASSERT( !empty_String(targ) );

	node_i = list_iterator_create(cluster);
	node = (Node *)list_next(node_i);
	errno = 0;
	forcount(i, length_String(targ))
	{
		ASSERT( node != NULL );
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
			exit_error("Illegal soft state value.");
		}
		node = (Node *)list_next(node_i);
	}
	list_iterator_destroy(node_i);
}

void 
update_Nodes_hard_state(List cluster, List reply)
{
	Node *node;
	ListIterator node_i;
	int i = 0;
	String *targ;

	ASSERT( reply != NULL );
	targ = (String *)list_pop(reply);
	ASSERT( targ != NULL );
	ASSERT( !empty_String(targ) );

	node_i = list_iterator_create(cluster);
	node = (Node *)list_next(node_i);
	errno = 0;
	forcount(i, length_String(targ))
	{
		ASSERT( node != NULL );
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
			exit_error("Illegal hard state value.");
		}
		node = (Node *)list_next(node_i);
	}
	list_iterator_destroy(node_i);
}

/*
 * Node source
 */
Node *
make_Node(const char *name)
{
	Node *node;

	node = (Node *)Malloc(sizeof(Node));
	node->name = make_String(name);
	node->n_state = ST_UNKNOWN;
	node->p_state = ST_UNKNOWN;
	return node;
}

int
cmp_Nodes(void *node1, void *node2)
{
	ASSERT(((Node *)node1) != NULL);
	ASSERT(((Node *)node1)->name != NULL);
	ASSERT(((Node *)node2) != NULL);
	ASSERT(((Node *)node2)->name != NULL);

	return cmp_String(((Node *)node1)->name, 
			  ((Node *)node2)->name);
}

int
match_Node(void *node, void *key)
{
	if( match_String(((Node *)node)->name, (char *)key) )
		return TRUE;
	return FALSE;	
}

void
free_Node(void *node)
{
	free_String( (void *)((Node *)node)->name);
	Free(node, sizeof(Node));
}

bool
is_prompt(String *targ)
{
	int i = 0;
	bool result = FALSE;
	
	ASSERT(targ != NULL);

	forcount(i, length_String(targ))
	{
		if(byte_String(targ, i) == PROMPT) result = TRUE;
		i++;
	}
	return result;
}

void
exit_error(const char *fmt, ...)
{
	va_list ap;
	char buf[MAX_BUF];
	int er;
	int len;

	er = errno;
	snprintf(buf, MAX_BUF, "PowerMan: ");
	len = strlen(buf);
	va_start(ap, fmt);
	vsnprintf(buf + len, MAX_BUF - len, fmt, ap);
	len = strlen(buf);
	snprintf(buf + len, MAX_BUF - len, ": %s\n", strerror(er));
	fflush(stdout);
	fputs(buf, stderr);
	fflush(stderr);
	va_end(ap);
	exit(1);
}

