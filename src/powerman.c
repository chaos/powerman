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

/* Review: link poweron and poweroffto invoke powerman -1/-0 (check argv[0]) */
/* Review: support -F - to read targets from stdin */
/* Review: issue warning if reset attempted on node that is powered off. */
/* Review: does --verify make any sense with --reset? */

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
#include "string.h"
#include "buffer.h"
#include "config.h"
#include "device.h"
#include "error.h"
#include "wrappers.h"
#include "hostlist.h"

#define OPT_STRING "cd:F:h10Llnp:qrvVQxzt"
static const struct option long_options[] = {
    {"on", no_argument, 0, '1'},
    {"off", no_argument, 0, '0'},
    {"cycle", no_argument, 0, 'c'},
    {"reset", no_argument, 0, 'r'},
    {"file", required_argument, 0, 'F'},
    {"help", no_argument, 0, 'h'},
    {"list", no_argument, 0, 'l'},
    {"queryon", no_argument, 0, 'q'},
    {"queryoff", no_argument, 0, 'Q'},
    {"verify", no_argument, 0, 'v'},
    {"version", no_argument, 0, 'V'},
    {"regex", no_argument, 0, 'x'},
    {"report", no_argument, 0, 'z'},
    {"noexec", no_argument, 0, 'n'},
    {"host", required_argument, 0, 'd'},
    {"port", required_argument, 0, 'p'},
    {"license", no_argument, 0, 'L'},
    {"telemetry", no_argument, 0, 't'},
    {0, 0, 0, 0}
};

static const struct option *longopts = long_options;

static bool debug_telemetry = FALSE;	/* show protocol interactions */

typedef enum {
    CMD_ERROR, CMD_QUERY_ON, CMD_QUERY_OFF, CMD_QUERY_ALL, CMD_POWER_ON,
    CMD_POWER_OFF, CMD_POWER_CYCLE, CMD_RESET, CMD_LISTTARG,
} command_t;

/* 
 * An instance of Config is built by process_command_line().
 */
typedef struct {
    char *host;		/* server hostname */
    char *service;		/* server port name */
    int port;			/* server port number */
    int fd;			/* socket open to server */
    command_t com;		/* comment to send to server */
    bool regex;			/* target list should be processed as regex */
    bool verify;		/* after requesting state change, verify it */
    bool noexec;		/* do not request server to do commands */
    hostlist_t targ;		/* target hostname list */
    List reply;			/* list-o-char *'s: reply from server */
} Config;

static void dump_Conf(Config * conf);
static Config *process_command_line(int argc, char **argv);
static void usage(char *prog);
static void read_Targets(Config * conf, char *name);
static void connect_to_server(Config * conf);
static List request_State(Config * conf);
static List get_Names(int fd);
static void get_State(int fd, List cluster);
static List dialog(int fd, const char *str);
static void process_targets(Config * conf);
static char *glob2regex(char *glob);
static List send_server(Config * conf, char *str);
static void publish_reply(Config * conf, List reply);
static void print_readable(Config * conf, List cluster);
static void print_list(Config * conf, List cluster, State_Val state);
static void print_Targets(List targ);
static Config *make_Config();
static void free_Config(Config * conf);
static void append_Nodes(List cluster, List targ);
static void update_Nodes_soft_state(List cluster, List reply);
static void update_Nodes_hard_state(List cluster, List reply);
static Node *xmake_Node(const char *name);
static int cmp_Nodes(Node * node1, Node * node2);
static void xfree_Node(Node * node);
static bool is_prompt(char *targ);

const char *powerman_license =
    "Copyright (C) 2001-2002 The Regents of the University of California.\n"
    "Produced at Lawrence Livermore National Laboratory.\n"
    "Written by Andrew Uselton <uselton2@llnl.gov>.\n"
    "http://www.llnl.gov/linux/powerman/\n"
    "UCRL-CODE-2002-008\n\n"
    "PowerMan is free software; you can redistribute it and/or modify it\n"
    "under the terms of the GNU General Public License as published by\n"
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

int main(int argc, char **argv)
{
    Config *conf;

    err_init(argv[0]);
    conf = process_command_line(argc, argv);

    /*if( geteuid() != 0 ) err_exit(FALSE, "Must be root"); */

    connect_to_server(conf);

    if (conf->com == CMD_QUERY_ON || conf->com == CMD_QUERY_OFF
	|| conf->com == CMD_QUERY_ALL) {
	List cluster = request_State(conf);

	/* what do we do with nodes in the ST_UNKNOWN state ? */
	if (conf->com == CMD_QUERY_ON)
	    print_list(conf, cluster, ST_ON);
	else if (conf->com == CMD_QUERY_OFF)
	    print_list(conf, cluster, ST_OFF);
	else if (conf->com == CMD_QUERY_ALL)
	    print_readable(conf, cluster);

	list_destroy(cluster);
    } else {
	process_targets(conf);
	if (conf->verify) {
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
static Config *process_command_line(int argc, char **argv)
{
    int c;
    Config *conf;
    int longindex;
    enum { NO_TARGS, REQ_TARGS, OPT_TARGS } targs_req = NO_TARGS;

    conf = make_Config();
    opterr = 0;
    while ((c =
	    getopt_long(argc, argv, OPT_STRING, longopts,
			&longindex)) != -1) {
	switch (c) {
	    /* 
	     * These just do something quick and exit.
	     */
	case 'V':		/* --version */
	    printf("%s-%s\n", PROJECT, VERSION);
	    exit(0);
	case 'L':		/* --license */
	    printf("%s", powerman_license);
	    exit(0);
	case 'h':		/* --help */
	    usage(argv[0]);
	    exit(0);
	    /* 
	     * The main commands.
	     */
	case 'c':		/* --cycle */
	    if (conf->com != CMD_ERROR)
		err_exit(FALSE, EXACTLY_ONE_MSG);
	    conf->com = CMD_POWER_CYCLE;
	    targs_req = REQ_TARGS;
	    break;
	case '1':		/* --on */
	    if (conf->com != CMD_ERROR)
		err_exit(FALSE, EXACTLY_ONE_MSG);
	    conf->com = CMD_POWER_ON;
	    targs_req = REQ_TARGS;
	    break;
	case '0':		/* --off */
	    if (conf->com != CMD_ERROR)
		err_exit(FALSE, EXACTLY_ONE_MSG);
	    conf->com = CMD_POWER_OFF;
	    targs_req = REQ_TARGS;
	    break;
	case 'r':		/* --reset */
	    if (conf->com != CMD_ERROR)
		err_exit(FALSE, EXACTLY_ONE_MSG);
	    conf->com = CMD_RESET;
	    targs_req = REQ_TARGS;
	    break;
	case 'q':		/* --queryon */
	    if (conf->com != CMD_ERROR)
		err_exit(FALSE, EXACTLY_ONE_MSG);
	    conf->com = CMD_QUERY_ON;
	    targs_req = NO_TARGS;
	    break;
	case 'Q':		/* --queryoff */
	    if (conf->com != CMD_ERROR)
		err_exit(FALSE, EXACTLY_ONE_MSG);
	    conf->com = CMD_QUERY_OFF;
	    targs_req = NO_TARGS;
	    break;
	case 'l':		/* --list */
	    if (conf->com != CMD_ERROR)
		err_exit(FALSE, EXACTLY_ONE_MSG);
	    conf->com = CMD_LISTTARG;
	    targs_req = OPT_TARGS;
	    break;
	case 'z':		/* --report */
	    if (conf->com != CMD_ERROR)
		err_exit(FALSE, EXACTLY_ONE_MSG);
	    conf->com = CMD_QUERY_ALL;
	    targs_req = NO_TARGS;
	    break;
	    /* 
	     * Modifiers.
	     */
	case 't':		/* --telemetry */
	    debug_telemetry = TRUE;
	    break;
	case 'd':		/* --host */
	    if (conf->host != NULL)
		Free(conf->host);
	    conf->host = Strdup(optarg);
	    break;
	case 'F':		/* --file */
	    read_Targets(conf, optarg);
	    break;
	case 'p':		/* --port */
	    /* Is optarg a legal service value integer? */
	    /* If not, can you find the named service?  */
	    /* If not, err_exit()                     */
	    if (conf->service != NULL)
		Free(conf->service);
	    conf->service = Strdup(optarg);
	    break;
	case 'n':		/* --noexec */
	    conf->noexec = TRUE;
	    break;
	case 'v':		/* --verify */
	    conf->verify = TRUE;
	    break;
	case 'x':		/* --regex */
	    conf->regex = TRUE;
	    break;
	default:
	    err_exit(FALSE, "unrecognized command line option (see --help)");
	 /*NOTREACHED*/}
    }
    if (conf->com == CMD_ERROR)
	err_exit(FALSE, EXACTLY_ONE_MSG);

    /* build hostlist of targets */
    while (optind < argc) {
	if (conf->regex)
	    hostlist_push_host(conf->targ, argv[optind]);
	else
	    hostlist_push(conf->targ, argv[optind]);
	optind++;
    }

    if (hostlist_is_empty(conf->targ) && targs_req == REQ_TARGS)
	err_exit(FALSE, "command requires target(s)");
    if (!hostlist_is_empty(conf->targ) && targs_req == NO_TARGS)
	err_exit(FALSE, "command may not be specified with targets");

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
static void usage(char *prog)
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

static void dump_Conf(Config * conf)
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

/* Review:  add comment here on format of file */
/* 
 *   This should be changed to allow for a '-' in place of the 
 * file name, which would then cause it to read from stdin.
 */
static void read_Targets(Config * conf, char *name)
{
    FILE *fp;
    unsigned char buf[MAX_BUF];
    int i = 0;
    unsigned int c;

    if ((fp = fopen(name, "r")) == NULL) {
	err_exit(TRUE, "%s", name);
    }
    while ((c = getc(fp)) != EOF) {
	/* Review: use isspace() */
	if ((c != ' ') && (c != '\r') && (c != '\n') && (c != '\t')) {
	    buf[i] = c;
	    i++;
	    if (i == MAX_BUF)
		i = 0;
	} else {
	    if (i > 0) {
		buf[i] = '\0';
		hostlist_push(conf->targ, buf);
	    }
	    i = 0;
	}
    }
    /* Review: check for error on fclose() */
    fclose(fp);
}

/*
 * Set up connection to server.
 */
static void connect_to_server(Config * conf)
{
    struct addrinfo hints, *addrinfo;
    int n;
    List reply;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    assert(conf->host != NULL);
    assert(conf->service != NULL);
    Getaddrinfo(conf->host, conf->service, &hints, &addrinfo);

    conf->fd = Socket(addrinfo->ai_family, addrinfo->ai_socktype,
		      addrinfo->ai_protocol);

    n = Connect(conf->fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
    freeaddrinfo(addrinfo);

    /* send nothing (hence NULL arg), read 'password> ' */
    reply = dialog(conf->fd, NULL);
    list_destroy(reply);
    /* send password, read '0 PowerMan> ' */
    reply = dialog(conf->fd, AUTHENTICATE_FMT);
    list_destroy(reply);

}

/*
 * Request the list of nodes known to the server and their power state.
 * Results are returned as a list-o-nodes that must be freed by the caller.
 */
static List request_State(Config * conf)
{
    List cluster;

    cluster = get_Names(conf->fd);
    get_State(conf->fd, cluster);
    list_sort(cluster, (ListCmpF) cmp_Nodes);
    return cluster;
}

/*
 * Request list of Nodes known to the server.
 * Results are returned as a list-o-Nodes thatmust be freed by the caller.
 */
static List get_Names(int fd)
{
    List cluster;		/* list-o-nodes */
    List reply;			/* list-o-strings */
    char buf[80];
    int res;

    cluster = list_create((ListDelF) xfree_Node);
    res = snprintf(buf, sizeof(buf), GET_NAMES_FMT, ".*");
    assert(res != -1 && res <= sizeof(buf));
    reply = dialog(fd, buf);
    /* 
     * Build Nodes list from list of hostnames
     * Nodes have additional properties like p_state, n_state 
     */
    append_Nodes(cluster, reply);
    list_destroy(reply);

    return cluster;
}

/* 
 * Request power state for all nodes known to powerman.
 * Fills in the p_state and n_state fields of each Node in the 
 * 'cluster' list-o-Nodes.
 */
static void get_State(int fd, List cluster)
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

/* Review: revisit this documentation */
/*
 * dialog() returns a circular list without a dummy header record
 * and will exit with an error if it failed to read after "limit"
 * tries.
 *
 * List source
 */
static List dialog(int fd, const char *str)
{
    unsigned char buf[MAX_BUF];
    int in = 0;
    int n;
    int i;
    int j;
    int len;
    /* Review: should be moved to a #define at top of module with comment */
    int limit = 100;
    bool found = FALSE;
    List reply = list_create((ListDelF) Free);
    /* Review: check for NULL reply or use out_of_memory() */
    char *targ;

    if (debug_telemetry) {
	if (str)
	    printf("C: '%s'\n", str);
    }
    memset(buf, 0, MAX_BUF);
    if (str != NULL) {
	len = strlen(str);
	n = Write(fd, (char *) str, len);
	if (n != len)
	    err_exit(FALSE, "Rncomplete write of %s", str);
    }
    /* Review: redundant to have found initialized here and above. */
    found = FALSE;
    /* Review: comment purpose of this block */
    /* Review: localize scope of some vars used only in these loops  */
    while (!found && (limit > 0)) {
	n = Read(fd, buf + in, MAX_BUF);
	for (i = in; i < in + n; i++) {
	    /* Review: break after found = TRUE */
	    if (buf[i] == PROMPT)
		found = TRUE;
	}
	in += n;
	limit--;
    }
    /* Review: bail out here if !found  
     * (then can remove if(found) block below) */
    /* Review: comment purpose of this block */
    if (found) {
	len = in;
	i = 0;
	j = 0;
	while (i < len + 1) {
	    if ((buf[i] == '\r') || (buf[i] == '\n') || (buf[i] == '\0')) {
		buf[i] = '\0';
		if (j < i) {
		    targ = Strdup(buf + j);
		    list_append(reply, targ);
		}
		j = i + 1;
	    }
	    i++;
	}
    }
    if (list_is_empty(reply))
	err_exit(FALSE, "command \"%s\" failed.", str);

    if (debug_telemetry) {
	char *tmpstr;

	ListIterator itr = list_iterator_create(reply);
	while ((tmpstr = list_next(itr)))
	    printf("S: '%s'\n", tmpstr);
	list_iterator_destroy(itr);
    }

    return reply;
}

/*
 * Iterate through conf->targ nodes executing command for each node.
 * XXX this requires a separate handshake for every node specified;
 * consider making this more efficient.
 */
static void process_targets(Config * conf)
{
    char *host;
    hostlist_iterator_t itr;

    if ((itr = hostlist_iterator_create(conf->targ)) == NULL)
	err_exit(TRUE, "hostlist_iterator_create");
    while ((host = hostlist_next(itr))) {
	char *targ = Strdup(host);
	char *str;
	List reply;

	if (conf->regex == TRUE)
	    str = targ;
	else {
	    str = glob2regex(targ);
	    Free(targ);
	}
	reply = send_server(conf, str);
	publish_reply(conf, reply);
	Free(str);
	list_destroy(reply);
    }
    hostlist_iterator_destroy(itr);
}


static char *glob2regex(char *glob)
{
    unsigned char buf[MAX_BUF];
    int i = 0;
    char *regex;

    /* Review: check that glob really is a legal glob here. 
     * look into fnmatch() */
    /* Review: check for overruns of buf */
    /* Review: check for escape of . or * etc.  ("\." "\*" "\?") */
    while (*glob != '\0') {
	if (isalnum(*glob) || (*glob == '-') || (*glob == '_')) {
	    buf[i] = *glob;
	    i++;
	} else if ((*glob == '*') || (*glob == '?')) {
	    buf[i] = '.';
	    i++;
	    buf[i] = *glob;
	    i++;
	} else
	    err_exit(FALSE, "%c is not a legal character for a host name", *glob);
	glob++;
    }
    buf[i] = '\0';
    regex = Strdup(buf);
    return regex;
}

/* 
 * List source
 */
static List send_server(Config * conf, char *str)
{
    List reply;
    char buf[MAX_BUF];
    int len = sizeof(buf);
    int res;

    switch (conf->com) {
    case CMD_LISTTARG:
	res = snprintf(buf, len, GET_NAMES_FMT, str);
	break;
    case CMD_POWER_ON:
	res = snprintf(buf, len, DO_POWER_ON_FMT, str);
	break;
    case CMD_POWER_OFF:
	res = snprintf(buf, len, DO_POWER_OFF_FMT, str);
	break;
    case CMD_POWER_CYCLE:
	res = snprintf(buf, len, DO_POWER_CYCLE_FMT, str);
	break;
    case CMD_RESET:
	res = snprintf(buf, len, DO_RESET_FMT, str);
	break;
    default:
	assert(FALSE);
    }
    assert(res != -1 && res <= len);
    reply = dialog(conf->fd, buf);
    return reply;
}

static void publish_reply(Config * conf, List reply)
{
    switch (conf->com) {
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

static void warn_softstate(char *host)
{
#if 0				/* shuddup for now */
    fprintf(stderr, "warning: %s: plug is on but node may not be\n", host);
#endif
}

/*
 * Produce a readable report showing who's on and who's off (--report)
 */
static void print_readable(Config * conf, List cluster)
{
    hostlist_t on = hostlist_create(NULL);
    hostlist_t off = hostlist_create(NULL);
    hostlist_t unk = hostlist_create(NULL);
    Node *node;
    ListIterator itr;
    char tmpstr[1024];

    if (!on || !off || !unk)
	err_exit(TRUE, "hostlist_create");

    /* Sort nodes into three hostlist bins */
    itr = list_iterator_create(cluster);
    while ((node = list_next(itr))) {
	switch (node->p_state) {
	case ST_ON:
	    hostlist_push_host(on, node->name);
	    if (node->n_state != ST_ON)
		warn_softstate(node->name);
	    break;
	case ST_OFF:
	    hostlist_push_host(off, node->name);
	    break;
	case ST_UNKNOWN:
	    hostlist_push_host(unk, node->name);
	    break;
	}
    }
    list_iterator_destroy(itr);

    /* Report the status */
    if (!hostlist_is_empty(on)) {
	hostlist_sort(on);
	hostlist_ranged_string(on, sizeof(tmpstr), tmpstr);
	printf("on\t%s\n", tmpstr);
    }
    if (!hostlist_is_empty(off)) {
	hostlist_sort(off);
	hostlist_ranged_string(off, sizeof(tmpstr), tmpstr);
	printf("off\t%s\n", tmpstr);
    }
    if (!hostlist_is_empty(unk)) {
	hostlist_sort(unk);
	hostlist_ranged_string(unk, sizeof(tmpstr), tmpstr);
	printf("unknown\t%s\n", tmpstr);
    }

    hostlist_destroy(on);
    hostlist_destroy(off);
    hostlist_destroy(unk);
}

/* Review: consider more descriptive name and add comment */
static void print_list(Config * conf, List cluster, State_Val state)
{
    Node *node;
    ListIterator itr;

    itr = list_iterator_create(cluster);
    while ((node = list_next(itr))) {
	if (node->p_state == state)
	    printf("%s\n", node->name);
	if (node->p_state == ST_ON && node->n_state == ST_OFF)
	    warn_softstate(node->name);
    }
    list_iterator_destroy(itr);
}

static void print_Targets(List targ)
{
    char *t;
    ListIterator itr;

    assert(targ != NULL);
    itr = list_iterator_create(targ);
    while ((t = list_next(itr)) && !is_prompt(t)) {
	printf("%s\n", t);
    }
    list_iterator_destroy(itr);
}


/*
 * Config source
 */
static Config *make_Config(void)
{
    Config *conf;
    char *env;
    char *port;
    int p;

    conf = (Config *) Malloc(sizeof(Config));
    conf->noexec = FALSE;
    conf->host = NULL;
    conf->service = NULL;
    if ((env = getenv("POWERMAN_HOST"))) {
	if ((port = strchr(env, ':'))) {
	    *port++ = '\0';
	    if ((p = atoi(port)) > 0) {
		conf->service = Strdup(port);
		conf->port = p;
	    }
	}
	if (*env) {
	    conf->host = Strdup(env);
	}
    }
    if (conf->host == NULL)
	conf->host = Strdup("localhost");
    if ((port = getenv("POWERMAN_PORT")) && (*env)) {
	if ((p = atoi(port)) > 0) {
	    conf->service = Strdup(port);
	    conf->port = p;
	}
    }
    if (conf->service == NULL) {
	conf->service = Strdup("10101");
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
static void free_Config(Config * conf)
{
    Free(conf->host);
    Free(conf->service);
    hostlist_destroy(conf->targ);
    if (conf->reply != NULL)
	list_destroy(conf->reply);
    Free(conf);
}

static void append_Nodes(List cluster, List reply)
{
    Node *node;
    ListIterator itr;
    char *targ;

    assert(reply != NULL);
    itr = list_iterator_create(reply);
    while ((targ = list_next(itr)) && !is_prompt(targ)) {
	node = xmake_Node(targ);
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
static void update_Nodes_soft_state(List cluster, List reply)
{
    Node *node;
    ListIterator node_i;
    int i = 0;
    char *targ;

    assert(reply != NULL);
    targ = list_pop(reply);	/* reply list is always one char * */
    assert(targ != NULL);
    assert(strlen(targ) != 0);

    node_i = list_iterator_create(cluster);
    node = list_next(node_i);
    for (i = 0; i < strlen(targ); i++) {
	assert(node != NULL);
	switch (targ[i]) {
	case '0':
	    node->n_state = ST_OFF;
	    break;
	case '1':
	    node->n_state = ST_ON;
	    break;
	case '?':
	    node->n_state = ST_UNKNOWN;
	    break;
	default:
	    err_exit(FALSE, "Illegal soft state value.");
	}
	node = list_next(node_i);
    }
    list_iterator_destroy(node_i);
}

/* 
 * Process 'reply', obtained in response to a GET_SOFT_UDPATE_FMT request.
 * Reply will consist of a string of ones and zeroes indicating soft power 
 * state for 'cluster' node list.  Update the 'n_state' field for each node
 * in the 'cluster' list.
 */
static void update_Nodes_hard_state(List cluster, List reply)
{
    Node *node;
    ListIterator node_i;
    int i = 0;
    char *targ;

    assert(reply != NULL);
    targ = list_pop(reply);	/* reply list is always one char * */
    assert(targ != NULL);
    assert(strlen(targ) != 0);

    node_i = list_iterator_create(cluster);
    node = list_next(node_i);
    for (i = 0; i < strlen(targ); i++) {
	assert(node != NULL);
	switch (targ[i]) {
	case '0':
	    node->p_state = ST_OFF;
	    break;
	case '1':
	    node->p_state = ST_ON;
	    break;
	case '?':
	    node->p_state = ST_UNKNOWN;
	    break;
	default:
	    err_exit(FALSE, "illegal hard state value.");
	}
	node = list_next(node_i);
    }
    list_iterator_destroy(node_i);
}

/*
 * Node source
 */
static Node *xmake_Node(const char *name)
{
    Node *node;

    node = (Node *) Malloc(sizeof(Node));
    node->name = Strdup(name);
    node->n_state = ST_UNKNOWN;
    node->p_state = ST_UNKNOWN;
    return node;
}

static int cmp_Nodes(Node * node1, Node * node2)
{
    assert(node1 != NULL);
    assert(node1->name != NULL);
    assert(node2 != NULL);
    assert(node2->name != NULL);

    return strcmp(node1->name, node2->name);
}

static void xfree_Node(Node * node)
{
    Free(node->name);
    Free(node);
}

static bool is_prompt(char *targ)
{
    int i = 0;
    bool result = FALSE;

    assert(targ != NULL);

    for (i = 0; i < strlen(targ); i++) {
	if (targ[i] == PROMPT)
	    result = TRUE;
	i++;
    }
    return result;
}

/*
 * vi:softtabstop=4
 */
