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
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <syslog.h>

#include <arpa/inet.h>
#include <tcpd.h>
#include <stdio.h>
#include <fcntl.h>

#include "powerman.h"
#include "wrappers.h"
#include "list.h"
#include "config.h"
#include "client.h"
#include "buffer.h"
#include "error.h"
#include "hostlist.h"
#include "client_proto.h"
#include "debug.h"
#include "device.h"

#define LISTEN_BACKLOG    5

/* prototypes for internal functions */
static Command *_create_command(Client *c, int com, char *arg1);
static void _destroy_command(Command *cmd);
static void _hostlist_error(Client *c);
static int _match_client(Client * client, void *key);
static Client *_find_client(int client_id);
static hostlist_t _hostlist_create_validated(Client *c, char *str);
static void _client_query_nodes_reply(Client *c);
static void _client_query_devices_reply(Client *c);
static void _client_query_reply(Client *c);
static void _client_query_raw_reply(Client *c);
static void _handle_client_read(Client * c);
static void _handle_client_write(Client * c);
static char *_strip_whitespace(char *str);
static void _parse_input(Client *c, char *input);
static void _destroy_client(Client * client);
static void _create_client(void);
static void _act_finish(int client_id, char *errfmt, char *errarg);

#define _client_msg(c,args...) do {  \
    (c)->write_status = CLI_WRITING; \
    buf_printf((c)->to, ##args); \
} while(0)
#define _client_prompt(c)   _client_msg((c), CP_PROMPT)

/* tcp wrappers support */
extern int hosts_ctl(char *daemon, char *client_name, char *client_addr, 
		char *client_user);
int allow_severity = LOG_INFO;		/* logging level for accepted reqs */
int deny_severity = LOG_WARNING;	/* logging level for rejected reqs */

static int listen_fd = NO_FD;		/* powermand listen socket */
static List cli_clients = NULL;		/* list of clients */

static int cli_id_seq = 1;		/* range 1...INT_MAX */
#define _next_cli_id() \
    (cli_id_seq < INT_MAX ? cli_id_seq++ : (cli_id_seq = 1, INT_MAX))

/*
 * Initialize module.
 */
void cli_init(void)
{
    /* create cli_clients list */
    cli_clients = list_create((ListDelF) _destroy_client);
}

/* 
 * Finalize module.
 */
void cli_fini(void)
{
    /* destroy clients */
    list_destroy(cli_clients);
}

/* 
 * Issue appropriate error message to client given a hostlist-generated errno.
 */
static void _hostlist_error(Client *c)
{
    switch (errno) {
        case ERANGE:
	    _client_msg(c, CP_ERR_HLRANGE);
            break;
        case EINVAL:
	    _client_msg(c, CP_ERR_HLINVAL);
            break;
        default:
	    _client_msg(c, CP_ERR_HLUNK);
	    break;
    }
}

/* 
 * Build a hostlist_t from a string, validating each node name against 
 * powerman configuration.  If any bogus nodes are found, issue error 
 * message to client and return NULL.
 */
static hostlist_t _hostlist_create_validated(Client *c, char *str)
{
    hostlist_t hl = NULL;
    hostlist_t badhl = NULL;
    hostlist_iterator_t itr = NULL;
    char *host;
    bool valid = TRUE;

    if ((hl = hostlist_create(str)) == NULL) {
	_hostlist_error(c);
	return NULL;
    }
    if ((badhl = hostlist_create(NULL)) == NULL) {
	_client_msg(c, CP_ERR_INTERNAL);
	return NULL;
    }
    if ((itr = hostlist_iterator_create(hl)) == NULL) {
	_client_msg(c, CP_ERR_INTERNAL);
	hostlist_destroy(hl);
	return NULL;
    }
    while ((host = hostlist_next(itr)) != NULL) {
	if (!conf_node_exists(host)) {
	    valid = FALSE;
	    hostlist_push_host(badhl, host);
	}
    }
    if (!valid) {
	char hosts[CP_LINEMAX];

	if (hostlist_ranged_string(badhl, sizeof(hosts), hosts) == -1)
	    _client_msg(c, CP_ERR_NOSUCHNODES, str);
	else
	    _client_msg(c, CP_ERR_NOSUCHNODES, hosts);
	hostlist_iterator_destroy(itr);
	hostlist_destroy(hl);
	hostlist_destroy(badhl);
	return NULL;
    }
    hostlist_iterator_destroy(itr);
    hostlist_destroy(badhl);
    return hl;
}

/* 
 * Reply to client request for list of nodes in powerman configuration.
 */
static void _client_query_nodes_reply(Client *c)
{
    hostlist_t nodes = conf_getnodes();
    char hosts[CP_LINEMAX];

    if (hostlist_ranged_string(nodes, sizeof(hosts), hosts) == -1) 
	_client_msg(c, CP_ERR_INTERNAL);
    else
	_client_msg(c, CP_RSP_NODES, hosts);
}

/* helper for _client_query_devices_reply() */
static bool _make_pluglist(Device *dev, char *str, int len)
{
    hostlist_t hl = hostlist_create(NULL);
    ListIterator itr;
    bool res = FALSE;
    Plug *plug;

    if (hl != NULL) {
	itr = list_iterator_create(dev->plugs);
	while ((plug = list_next(itr))) {
	    hostlist_push(hl, plug->node);
	}
	list_iterator_destroy(itr);
	if (hostlist_ranged_string(hl, len, str) != -1) 
	    res = TRUE;
	hostlist_destroy(hl);
    }
    return res;
}

/* 
 * Reply to client request for list of devices in powerman configuration.
 */
static void _client_query_devices_reply(Client *c)
{
    List devs = dev_getdevices();
    Device *dev;
    ListIterator itr;

    if (devs) {

	itr = list_iterator_create(devs);
	while ((dev = list_next(itr))) {
	    char nodelist[CP_LINEMAX];
	    int con = dev->stat_successful_connects;

	    if (_make_pluglist(dev, nodelist, sizeof(nodelist))) {
		_client_msg(c, CP_RSP_DEVICES, dev->name, nodelist, 
			    con > 0 ? con - 1 : 0,
			    dev->stat_successful_actions);
	    }
	}
	list_iterator_destroy(itr);
    }
    _client_msg(c, CP_RSP_QUERY_COMPLETE);
}

/* helper for _client_query_status_reply */
static int _argval_ranged_string(ArgList *arglist, char *str, int len, 
		ArgState state)
{
    hostlist_t hl = hostlist_create(NULL);
    ListIterator itr;
    Arg *arg;
    int res = -1;

    if (hl != NULL) {
	itr = list_iterator_create(arglist->argv);
	while ((arg = list_next(itr))) {
	    if (arg->state == state)
		hostlist_push(hl, arg->node);
	}
	list_iterator_destroy(itr);
	if (hostlist_ranged_string(hl, len, str) != -1) 
	    res = 0;
	hostlist_destroy(hl);
    } 
    return res;
}

/* 
 * Reply to client request for plug/soft status.
 */
static void _client_query_reply(Client *c)
{
    char on[CP_LINEMAX], off[CP_LINEMAX], unknown[CP_LINEMAX];
    ArgList *arglist = c->cmd->arglist;
    int n;

    assert(c->cmd != NULL);
    n  = _argval_ranged_string(arglist, on,      CP_LINEMAX, ST_ON);
    n |= _argval_ranged_string(arglist, off,     CP_LINEMAX, ST_OFF);
    n |= _argval_ranged_string(arglist, unknown, CP_LINEMAX, ST_UNKNOWN);
    if (n != 0)
	_client_msg(c, CP_ERR_INTERNAL);
    else
	_client_msg(c, CP_RSP_STATUS, on, off, unknown);
}

/* 
 * Reply to client request for temperature/beacon status.
 */
static void _client_query_raw_reply(Client *c)
{
    ListIterator itr;
    Arg *arg;
    hostlist_t hl = hostlist_create(NULL);
    char str[1024];

    assert(c->cmd != NULL);
    itr = list_iterator_create(c->cmd->arglist->argv);
    while ((arg = list_next(itr))) {
	if (arg->val)
	    _client_msg(c, CP_RSP_RAW, arg->node, arg->val);
	else
	    hostlist_push(hl, arg->node);
    }
    list_iterator_destroy(itr);
    if (!hostlist_is_empty(hl)) {
	if (hostlist_ranged_string(hl, sizeof(str), str) == -1) 
	    _client_msg(c, CP_ERR_INTERNAL);
	else
	    _client_msg(c, CP_RSP_RAW, str, "unknown");
    }
    _client_msg(c, CP_RSP_QUERY_COMPLETE);
}

/*
 * Create Command.
 */
static Command *_create_command(Client *c, int com, char *arg1)
{
    Command *cmd = (Command *)Malloc(sizeof(Command));

    cmd->com = com;
    cmd->error = FALSE;
    cmd->pending = 0;
    cmd->hl = NULL;
    cmd->arglist = NULL;
    
    if (arg1) {
	cmd->hl = _hostlist_create_validated(c, arg1);
	if (cmd->hl == NULL) {
	    _destroy_command(cmd);
	    cmd = NULL;
	}
    }
    if (cmd && !dev_check_actions(cmd->com, cmd->hl)) {
	_client_msg(c, CP_ERR_UNIMPL);
	_destroy_command(cmd);
	cmd = NULL;
    }
    /* NOTE: cmd->arglist has a reference count and can persist after we 
     * unlink from it in _destroy_command().  If client goes away prematurely,
     * query actions still have valid pointers.
     */
    if (cmd && (cmd->com == PM_STATUS_PLUGS 
			    || cmd->com == PM_STATUS_NODES
			    || cmd->com == PM_STATUS_TEMP
			    || cmd->com == PM_STATUS_BEACON)) {
	if (cmd->hl)
	    cmd->arglist = dev_create_arglist(cmd->hl);
	else
	    cmd->arglist = dev_create_arglist(conf_getnodes());
    }
    return cmd;
}

/*
 * Destroy a Command.
 */
static void _destroy_command(Command *cmd)
{
    if (cmd->hl)
	hostlist_destroy(cmd->hl);
    if (cmd->arglist)
	dev_unlink_arglist(cmd->arglist);
    Free(cmd);
}

/* helper for _parse_input that deletes leading & trailing whitespace */
static char *_strip_whitespace(char *str)
{
    char *head = str;
    char *tail = str + strlen(str) - 1;

    while (*head && isspace(*head))
	head++;
    while (tail > head && isspace(*tail))
	*tail-- = '\0';
    return head;
}

/*
 * Parse a line of input and create a Command (and enqueue device actions)
 * if needed.
 */
static void _parse_input(Client *c, char *input)
{
    char *str = _strip_whitespace(input);
    char arg1[CP_LINEMAX];
    Command *cmd = NULL;

    memset(arg1, 0, CP_LINEMAX);

    /* NOTE: sscanf is safe because 'str' is guaranteed to be < CP_LINEMAX */

    if (strlen(str) >= CP_LINEMAX) {
	_client_msg(c, CP_ERR_TOOLONG);			/* error: too long */
    } else if (c->cmd != NULL) {
	_client_msg(c, CP_ERR_CLIBUSY);			/* error: busy */
	return; /* no prompt */
    } else if (!strncasecmp(str, CP_HELP, strlen(CP_HELP))) {
	_client_msg(c, CP_RSP_HELP);			/* help */
    } else if (!strncasecmp(str, CP_NODES, strlen(CP_NODES))) {
	_client_query_nodes_reply(c);			/* nodes */
    } else if (!strncasecmp(str, CP_DEVICES, strlen(CP_DEVICES))) {
	_client_query_devices_reply(c);			/* devices */
    } else if (!strncasecmp(str, CP_QUIT, strlen(CP_QUIT))) {
	_client_msg(c, CP_RSP_QUIT);			/* quit */
	_handle_client_write(c);
	c->read_status = CLI_DONE;
	c->write_status = CLI_IDLE;
	return; /* no prompt */
    } else if (sscanf(str, CP_ON, arg1) == 1) {		/* on hostlist */
	cmd = _create_command(c, PM_POWER_ON, arg1);
    } else if (sscanf(str, CP_OFF, arg1) == 1) {	/* off hostlist */
	cmd = _create_command(c, PM_POWER_OFF, arg1);
    } else if (sscanf(str, CP_CYCLE, arg1) == 1) {	/* cycle hostlist */
	cmd = _create_command(c, PM_POWER_CYCLE, arg1);
    } else if (sscanf(str, CP_RESET, arg1) == 1) {	/* reset hostlist */
	cmd = _create_command(c, PM_RESET, arg1);
    } else if (sscanf(str, CP_BEACON_ON, arg1) == 1) {	/* beacon_on hostlist */
	cmd = _create_command(c, PM_BEACON_ON, arg1);
    } else if (sscanf(str, CP_BEACON_OFF, arg1) == 1) {	/* beacon_off hostlist*/
	cmd = _create_command(c, PM_BEACON_OFF, arg1);
    } else if (sscanf(str, CP_STATUS, arg1) == 1) {	/* status [hostlist] */
	cmd = _create_command(c, PM_STATUS_PLUGS, arg1);
    } else if (!strncasecmp(str, CP_STATUS_ALL, strlen(CP_STATUS_ALL))) {
	cmd = _create_command(c, PM_STATUS_PLUGS, NULL);
    } else if (sscanf(str, CP_SOFT, arg1) == 1) {	/* soft [hostlist] */
	cmd = _create_command(c, PM_STATUS_NODES, arg1);
    } else if (!strncasecmp(str, CP_SOFT_ALL, strlen(CP_SOFT_ALL))) {
	cmd = _create_command(c, PM_STATUS_NODES, NULL);
    } else if (sscanf(str, CP_TEMP, arg1) == 1) {	/* temp [hostlist] */
	cmd = _create_command(c, PM_STATUS_TEMP, arg1);
    } else if (!strncasecmp(str, CP_TEMP_ALL, strlen(CP_TEMP_ALL))) {
	cmd = _create_command(c, PM_STATUS_TEMP, NULL);
    } else if (sscanf(str, CP_BEACON, arg1) == 1) {	/* beacon [hostlist] */
	cmd = _create_command(c, PM_STATUS_BEACON, arg1);
    } else if (!strncasecmp(str, CP_BEACON_ALL, strlen(CP_BEACON_ALL))) {
	cmd = _create_command(c, PM_STATUS_BEACON, NULL);
    } else {						/* error: unknown */
	_client_msg(c, CP_ERR_UNKNOWN);
    }

    /* enqueue device actions and tie up the client if necessary */
    /* Note: cmd->hl may be NULL */
    if (cmd) {
	dbg(DBG_CLIENT, "_parse_input: enqueuing actions");
	cmd->pending = dev_enqueue_actions(cmd->com, cmd->hl, _act_finish, 
			c->client_id, cmd->arglist);
	if (cmd->pending == 0) {
	    _client_msg(c, CP_ERR_NOACTION);		
	    _destroy_command(cmd);
	    cmd = NULL;
	}
	assert(c->cmd == NULL);
	c->cmd = cmd;
    }

    /* reissue prompt if we didn't queue up any device actions */
    if (cmd == NULL)
	_client_prompt(c);
}

/*
 * Callback for device action completion.
 */
static void _act_finish(int client_id, char *errfmt, char *errarg)
{
    Client *c;

    /* if client has gone away do nothing */
    if (!(c = _find_client(client_id)))
	return;
    CHECK_MAGIC(c);
    assert(c->cmd != NULL);

    if (errfmt) {		    /* flag any errors for the final report */
	c->cmd->error = TRUE;
	_client_msg(c, errfmt, errarg);
    }

    if (--c->cmd->pending > 0)	    /* command is not complete */
	return;

    /*
     * All actions completed - return final status to the client.
     */
    switch (c->cmd->com) {
        case PM_STATUS_PLUGS:  /* query-status */
	    _client_query_reply(c);
            break;
	case PM_STATUS_NODES:
	    _client_query_reply(c);
	    break;
        case PM_STATUS_TEMP:
	    _client_query_raw_reply(c);
	    break;
	case PM_STATUS_BEACON:
	    /*_client_query_raw_reply(c);*/
	    _client_query_reply(c);
	    break;
        case PM_POWER_ON:	/* on */
        case PM_POWER_OFF:	/* off */
        case PM_BEACON_ON:	/* beacon_on */
        case PM_BEACON_OFF:	/* beacon_off */
        case PM_POWER_CYCLE:	/* cycle */
        case PM_RESET:		/* reset */
	    if (c->cmd->error)
		_client_msg(c, CP_ERR_COMPLETE);
	    else
		_client_msg(c, CP_RSP_COMPLETE);
            break;
        default:
            assert(FALSE);
	    _client_msg(c, CP_ERR_INTERNAL);
            break;
    }

    /* clean up and re-prompt */
    _destroy_command(c->cmd);
    c->cmd = NULL;
    _client_prompt(c);
}

/* 
 * Destroy a client.
 */
static void _destroy_client(Client * client)
{
    CHECK_MAGIC(client);

    if (client->fd != NO_FD) {
        Close(client->fd);
        client->fd = NO_FD;
        dbg(DBG_CLIENT, "_destroy_client: closing fd %d", client->fd);
    }
    if (client->to)
        buf_destroy(client->to);
    if (client->from)
        buf_destroy(client->from);
    if (client->cmd)
	_destroy_command(client->cmd);
    if (client->ip)
	Free(client->ip);
    if (client->host)
	Free(client->host);
    Free(client);
}

/* helper for _find_client */
static int _match_client(Client * client, void *key)
{
    return (client->client_id == *(int *)key);
}

/*
 * Test if a client still exists (by sequence).
 */
static Client *_find_client(int seq)
{
    return list_find_first(cli_clients, (ListFindF) _match_client, &seq);
}


/*
 * Begin listening for clients on powermand's socket.
 */
void cli_listen(void)
{
    struct sockaddr_in saddr;
    int saddr_size = sizeof(struct sockaddr_in);
    int sock_opt;
    int fd_settings;
    unsigned short listen_port;

    /* 
     * "All TCP servers should specify [the SO_REUSEADDR] socket option ..."
     *                                                  - Stevens, UNP p194 
     */
    listen_fd = Socket(PF_INET, SOCK_STREAM, 0);

    sock_opt = 1;
    Setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&sock_opt,sizeof(sock_opt));
    /* 
     *   A client could abort before a ready connection is accepted.  "The fix
     * for this problem is to:  1.  Always set a listening socket nonblocking
     * if we use select ..."                            - Stevens, UNP p424
     */
    fd_settings = Fcntl(listen_fd, F_GETFL, 0);
    Fcntl(listen_fd, F_SETFL, fd_settings | O_NONBLOCK);

    saddr.sin_family = AF_INET;
    listen_port = conf_get_listen_port();
    saddr.sin_port = htons(listen_port);
    saddr.sin_addr.s_addr = INADDR_ANY;
    Bind(listen_fd, &saddr, saddr_size);

    Listen(listen_fd, LISTEN_BACKLOG);

    dbg(DBG_CLIENT, "listening");
}


/*
 * Create a new client.
 */
static void _create_client(void)
{
    Client *client;
    struct sockaddr_in saddr;
    int saddr_size = sizeof(struct sockaddr_in);
    int fd_settings;
    struct hostent *hent;
    char buf[64];

    /* create client data structure */
    client = (Client *) Malloc(sizeof(Client));
    INIT_MAGIC(client);
    client->read_status = CLI_READING;
    client->write_status = CLI_IDLE;
    client->to = NULL;
    client->from = NULL;
    client->cmd = NULL;
    client->client_id = _next_cli_id();

    client->fd = Accept(listen_fd, &saddr, &saddr_size);
    /* client died after it initiated connect and before we could accept */
    if (client->fd < 0) {
        _destroy_client(client);
        err(TRUE, "_create_client: accept");
        return;
    }

    /* get client->ip */
    if (inet_ntop(AF_INET, &saddr.sin_addr, buf, sizeof(buf)) == NULL) {
        _destroy_client(client);
        err(TRUE, "_create_client: inet_ntop");
        return;
    }
    client->ip = Strdup(buf);
    client->port = ntohs(saddr.sin_port);

    /* get client->host */
    if ((hent = gethostbyaddr((const char *) &saddr.sin_addr,
		    sizeof(struct in_addr), AF_INET)) == NULL) {
        err(FALSE, "_create_client: gethostbyaddr failed");
    } else {
        client->host = Strdup(hent->h_name);
    }

    /* get authorization from tcp wrappers */
    if (conf_get_use_tcp_wrappers()) {
        if (!hosts_ctl(DAEMON_NAME, 
			client->host ? client->host : STRING_UNKNOWN, 
			client->ip, STRING_UNKNOWN)) {
            err(FALSE, "_create_client: tcp wrappers denies %s:%d", 
			    client->host ? client->host : client->ip,
			    client->port);
            _destroy_client(client);
            return;
        }
    }

    /* create I/O buffers */
    client->to = buf_create(client->fd, MAX_BUF, NULL, NULL);
    client->from = buf_create(client->fd, MAX_BUF, NULL, NULL);

    /* mark fd as non-blocking */
    fd_settings = Fcntl(client->fd, F_GETFL, 0);
    Fcntl(client->fd, F_SETFL, fd_settings | O_NONBLOCK);

    /* append to the list of clients */
    list_append(cli_clients, client);

    dbg(DBG_CLIENT, "connect %s:%d fd %d", 
		    client->host ? client->host : client->ip,
		    client->port, client->fd);

    /* prompt the client */
    _client_msg(client, CP_VERSION);
    _client_prompt(client);
}

/* 
 * select(2) read handler for the client 
 */
static void _handle_client_read(Client * c)
{
    int n;
    char buf[CP_LINEMAX];

    CHECK_MAGIC(c);

    n = buf_read(c->from);
    if ((n < 0) && (errno == EWOULDBLOCK)) {
	/* FIXME: should never happen?  need an error here */
        return;
    }

    /* EOF close or wait for writes to finish */
    if (n <= 0) {
        c->read_status = CLI_DONE;
        return;
    }

    while (buf_getline(c->from, buf, sizeof(buf)) > 0)
	_parse_input(c, buf);
}

/* 
 * select(2) write handler for the client 
 */
static void _handle_client_write(Client * c)
{
    int n;

    CHECK_MAGIC(c);

    n = buf_write(c->to);
    if (n < 0) {
	/* FIXME: should never happen?  need an error here */
        return;	/* EWOULDBLOCK */
    }

    if (buf_isempty(c->to))
        c->write_status = CLI_IDLE;
}

/*
 * Prep rset/wset/maxfd for the main select call.
 */
void cli_pre_select(fd_set *rset, fd_set *wset, int *maxfd)
{
    ListIterator itr;
    Client *client;
    fd_set cli_fdset;
    char fdsetstr[1024];

    FD_ZERO(&cli_fdset);

    if (listen_fd != NO_FD) {
        assert(listen_fd >= 0);
        FD_SET(listen_fd, rset);
        *maxfd = MAX(*maxfd, listen_fd);
    }

    itr = list_iterator_create(cli_clients);
    while ((client = list_next(itr))) {
        if (client->fd < 0)
            continue;

	FD_SET(client->fd, &cli_fdset);

        if (client->read_status == CLI_READING) {
            FD_SET(client->fd, rset);
            *maxfd = MAX(*maxfd, client->fd);
        }

        if (client->write_status == CLI_WRITING) {
            FD_SET(client->fd, wset);
            *maxfd = MAX(*maxfd, client->fd);
        }
    }
    list_iterator_destroy(itr);

    dbg(DBG_CLIENT, "fds are [%s] listen fd %d", 
		    dbg_fdsetstr(&cli_fdset, *maxfd + 1, 
			    fdsetstr, sizeof(fdsetstr)), listen_fd);
}

/* 
 * Handle any client activity (new connection or read/write).
 */
void cli_post_select(fd_set *rset, fd_set *wset)
{
    ListIterator itr;
    Client *client;
    
    if (FD_ISSET(listen_fd, rset))
        _create_client();

    itr = list_iterator_create(cli_clients);
    while ((client = list_next(itr))) {
        if (client->fd < 0)
            continue;

        if (FD_ISSET(client->fd, rset))
            _handle_client_read(client);

        if (FD_ISSET(client->fd, wset))
            _handle_client_write(client);

        if ((client->read_status == CLI_DONE) &&
            (client->write_status == CLI_IDLE))
            list_delete(itr);
    }
    list_iterator_destroy(itr);
}

/*
 * vi:softtabstop=4
 */
