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
#include <syslog.h>
#include <ctype.h>

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
#include "action.h"
#include "hostlist.h"
#include "client_proto.h"

#define LISTEN_BACKLOG    5

/* prototypes for internal functions */
static void _hostlist_error(Client *c);
static int _match_nodename(Node* node, void *key);
static bool _node_exists(char *name);
static hostlist_t _hostlist_create_validated(Client *c, char *str);
static void _client_query_nodes_reply(Client *c);
static void _client_query_status_reply(Client *c);
static void _handle_client_read(Client * c);
static void _handle_client_write(Client * c);
static char *_strip_whitespace(char *str);
static Action *_parse_input(Client *c, char *input);
static void _destroy_client(Client * client);
static int _match_client(Client * client, void *key);
static void _create_client(void);

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

/* module data */
static int listen_fd = NO_FD;
static List cli_clients = NULL;


/* Initialize module */
void cli_init(void)
{
    /* create cli_clients list */
    cli_clients = list_create((ListDelF) _destroy_client);
}


/* Finalize module */
void cli_fini(void)
{
    /* destroy clients */
    list_destroy(cli_clients);
}

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

static int _match_nodename(Node* node, void *key)
{
    return (strcmp((char *)key, node->name) == 0);
}

static bool _node_exists(char *name)
{
    List nodes = conf_getnodes();
    Node *node;

    node = list_find_first(nodes, (ListFindF) _match_nodename, name);
    return (node == NULL ? FALSE : TRUE);
}

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
	if (!_node_exists(host)) {
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

/* list nodes */
static void _client_query_nodes_reply(Client *c)
{
    List nodes = conf_getnodes();
    hostlist_t hl;
    char hosts[CP_LINEMAX]; /* FIXME: doesn't include prefix to response */
    ListIterator itr;
    Node *node;

    if ((hl = hostlist_create(NULL)) == NULL) {
	_hostlist_error(c);
	return;
    }

    itr = list_iterator_create(nodes);
    while ((node = list_next(itr)))
	hostlist_push_host(hl, node->name);

    if (hostlist_ranged_string(hl, sizeof(hosts), hosts) == -1) 
	_client_msg(c, CP_ERR_INTERNAL);
    else
	_client_msg(c, CP_RSP_NODES, hosts);

    hostlist_destroy(hl);
    list_iterator_destroy(itr);
}

/* list on/off/unknown status nodes */
static void _client_query_status_reply(Client *c)
{
    List nodes = conf_getnodes();
    hostlist_t hl_on = hostlist_create(NULL);
    hostlist_t hl_off = hostlist_create(NULL);
    hostlist_t hl_unk = hostlist_create(NULL);
    char on[CP_LINEMAX];    /* FIXME: doesn't include prefix to response */
    char off[CP_LINEMAX];   /* FIXME: doesn't include prefix to response */
    char unk[CP_LINEMAX];   /* FIXME: doesn't include prefix to response */
    ListIterator itr = list_iterator_create(nodes);
    Node *node;

    if (hl_on == NULL || hl_off == NULL || hl_unk == NULL) {
	_hostlist_error(c);
	goto done;
    }

    while ((node = list_next(itr))) {
	switch (node->p_state) {
	    case ST_ON:
		hostlist_push_host(hl_on, node->name);
		break;
	    case ST_OFF:
		hostlist_push_host(hl_off, node->name);
		break;
	    default:
		hostlist_push_host(hl_unk, node->name);
		break;
	}
    }
    if (hostlist_ranged_string(hl_on, sizeof(on), on) == -1) {
	_client_msg(c, CP_ERR_INTERNAL);
	goto done;
    }
    if (hostlist_ranged_string(hl_off, sizeof(off), off) == -1) {
	_client_msg(c, CP_ERR_INTERNAL);
	goto done;
    }
    if (hostlist_ranged_string(hl_unk, sizeof(unk), unk) == -1) {
	_client_msg(c, CP_ERR_INTERNAL);
	goto done;
    }
    _client_msg(c, CP_RSP_STATUS, on, off, unk);

done:
    list_iterator_destroy(itr);
    if (hl_on)
	hostlist_destroy(hl_on);
    if (hl_off)
	hostlist_destroy(hl_off);
    if (hl_unk)
	hostlist_destroy(hl_unk);
}

/*
 *   Select has indicated that there is material read to
 * be read on the fd associated with the Client c. 
 *   If there was in fact stuff to read then that stuff is put
 * in the "from" buffer for the client.  If a (or several) 
 * recognizable command(s) is (are) present then it is interpreted 
 * and put in a Server Action struct which is queued to
 * be processed when the server is Quiescent.  If the 
 * material is recognizably wrong then an error is returned 
 * to the client.  If there isn't a complete command (no '\n')
 * then the "from" buf keeps the data for later completion.
 * If there was nothing to read then it may be time to 
 * close the connection to the client.   
 */
static void _handle_client_read(Client * c)
{
    int n;
    char buf[CP_LINEMAX];

    CHECK_MAGIC(c);

    n = buf_read(c->from);
    if ((n < 0) && (errno == EWOULDBLOCK))
        return;

    /* EOF close or wait for writes to finish */
    if (n <= 0) {
        c->read_status = CLI_DONE;
        return;
    }

    while (buf_getline(c->from, buf, sizeof(buf)) > 0) {
	Action *act = _parse_input(c, buf);

	if (act != NULL) {
	    act->client = c;
	    act->seq = c->seq++;
	    act_add(act);
	}
    }
}

/*
 *   Select has notified that it is willing to accept some 
 * write data.
 *   If the client is wanting to close the connection 
 * and the write buffer is clear then close the 
 * connection.
 */
static void _handle_client_write(Client * c)
{
    int n;

    CHECK_MAGIC(c);

    n = buf_write(c->to);
    if (n < 0)
        return;	/* EWOULDBLOCK */

    if (buf_isempty(c->to))
        c->write_status = CLI_IDLE;
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
 * Parse a line of input.  If the command requires more work, an Action
 * is returned that should be enqueued.  If the command is completed here, 
 * a NULL is returned.
 */
static Action *_parse_input(Client *c, char *input)
{
    char *str = _strip_whitespace(input);
    Action *act = NULL;
    char arg1[CP_LINEMAX];
    int args = 0;

    memset(arg1, 0, CP_LINEMAX);

    if (strlen(str) >= CP_LINEMAX) {
	_client_msg(c, CP_ERR_TOOLONG);			/* error: too long */
    } else if (!strncasecmp(str, CP_HELP, strlen(CP_HELP))) {
	_client_msg(c, CP_RSP_HELP);			/* help */
    } else if (!strncasecmp(str, CP_QUERY_STATUS, strlen(CP_QUERY_STATUS))) {
	act = act_create(PM_UPDATE_PLUGS);		/* query-status */
    } else if (!strncasecmp(str, CP_QUERY_NODES, strlen(CP_QUERY_NODES))) {
	_client_query_nodes_reply(c);
    } else if (!strncasecmp(str, CP_QUIT, strlen(CP_QUIT))) {
        act = act_create(PM_LOG_OUT);			/* quit */
    } else if (sscanf(str, CP_ON, arg1) == 1) {
        act = act_create(PM_POWER_ON);			/* on */
	args = 1;
    } else if (sscanf(str, CP_OFF, arg1) == 1) {
        act = act_create(PM_POWER_OFF);			/* off */
	args = 1;
    } else if (sscanf(str, CP_CYCLE, arg1) == 1) {
        act = act_create(PM_POWER_CYCLE);		/* cycle */
	args = 1;
    } else if (sscanf(str, CP_RESET, arg1) == 1) {
        act = act_create(PM_RESET);			/* reset */
	args = 1;
    } else {
	_client_msg(c, CP_ERR_UNKNOWN);			/* error: unknown */
    }

    /* convert argument to hostlist and tack onto action */
    assert(args == 1 || args == 0);
    if (act != NULL && args == 1) {
	act->target = Strdup(arg1);
	if ((act->hl = _hostlist_create_validated(c, arg1)) == NULL) {
	    act_destroy(act); 
	    act = NULL;
	}
    }

    /* reissue prompt if we didn't queue up an action */
    if (act == NULL)
	_client_prompt(c);
    return act; 
}

/*
 * Return the results of an action to the client.
 * FIXME: results should be a part of the action.
 */
void cli_reply(Action * act)
{
    Client *c = act->client;

    CHECK_MAGIC(act);
    CHECK_MAGIC(c);
    assert(c->fd != NO_FD);

    switch (act->com) {
        case PM_LOG_OUT:	/* quit */
	    _client_msg(c, CP_RSP_QUIT);
            _handle_client_write(c);
            c->read_status = CLI_DONE;
            c->write_status = CLI_IDLE;
            break;
        case PM_UPDATE_PLUGS:  /* query-status */
	    _client_query_status_reply(c);
            break;
        case PM_POWER_ON:	/* on */
        case PM_POWER_OFF:	/* off */
        case PM_POWER_CYCLE:	/* cycle */
        case PM_RESET:		/* reset */
	    /* FIXME: always returns success! */
	    _client_msg(c, CP_RSP_SUCCESS);
            break;
        case PM_ERROR:		/* shouldn't happen */
        case PM_NAMES:
        case PM_CHECK_LOGIN:
        case PM_UPDATE_NODES:
        default:
            assert(FALSE);
	    _client_msg(c, CP_ERR_INTERNAL);
            break;
    }

    _client_prompt(c);
}

static void _destroy_client(Client * client)
{
    CHECK_MAGIC(client);

    if (client->fd != NO_FD) {
        Close(client->fd);
        client->fd = NO_FD;
        syslog(LOG_DEBUG, "client on descriptor %d signs off",
            ((Client *) client)->fd);
    }

    if (client->to != NULL)
        buf_destroy(client->to);

    if (client->from != NULL)
        buf_destroy(client->from);

    Free(client);
}

static int _match_client(Client * client, void *key)
{
    return (client == key);
}

bool cli_exists(Client *cli)
{
    Client *client;

    client = list_find_first(cli_clients, (ListFindF) _match_client, cli);
    return (client == NULL ? FALSE : TRUE);
}


/*
 *  This is a conventional implementation of the code in Stevens.
 * The Listener already exists and on entry and on completion the
 * descriptor is waiting on new connections.
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
}


/*
 * Read activity has been detected on the listener socket.  A connection
 * request has been received.  The new client is commemorated with a
 * Client data structure, it gets buffers for sending and receiving, 
 * and is vetted through TCP wrappers.
 */
static void _create_client(void)
{
    Client *client;
    struct sockaddr_in saddr;
    int saddr_size = sizeof(struct sockaddr_in);
    int fd_settings;
    bool accepted_client = TRUE;
    char buf[MAX_BUF];
    struct hostent *hent;
    char *ip;
    char *host;
    char *fqdn;
    char *p;

    /* create client data structure */
    client = (Client *) Malloc(sizeof(Client));
    INIT_MAGIC(client);
    client->loggedin = TRUE;  /* FIXME: not currently used */
    client->read_status = CLI_READING;
    client->write_status = CLI_IDLE;
    client->seq = 0;

    client->fd = Accept(listen_fd, &saddr, &saddr_size);
    /* client died after it initiated connect and before we could accept */
    if (client->fd < 0) {
        _destroy_client(client);
        syslog(LOG_ERR, "Client aborted connection attempt");
        return;
    }

    /* Got the new client, now look at TCP wrappers */
    /* get client->ip */
    if (inet_ntop(AF_INET, &saddr.sin_addr, buf, MAX_BUF) == NULL) {
        Close(client->fd);
        _destroy_client(client);
        syslog(LOG_ERR, "Unable to convert network address into string");
        return;
    }

    client->to = buf_create(client->fd, MAX_BUF, NULL, NULL);
    client->from = buf_create(client->fd, MAX_BUF, NULL, NULL);
    client->port = ntohs(saddr.sin_port);

    p = buf;
    while ((p - buf < MAX_BUF) && (*p != '/'))
        p++;

    if (*p == '/')
        *p = '\0';

    client->ip = Strdup(buf);
    ip = client->ip;
    fqdn = ip;
    host = STRING_UNKNOWN;

    /* get client->host */
    if ((hent = gethostbyaddr((const char *) &saddr.sin_addr,
		    sizeof(struct in_addr), AF_INET)) == NULL) {
        syslog(LOG_ERR, "Unable to get host name from network address");
    } else {
        client->host = Strdup(hent->h_name);
        host = client->host;
        fqdn = host;
    }

    if (conf_get_use_tcp_wrappers() == TRUE) {
        accepted_client = hosts_ctl(DAEMON_NAME, host, ip, STRING_UNKNOWN);

        if (accepted_client == FALSE) {
            Close(client->fd);
            _destroy_client(client);
            syslog(LOG_ERR, "Client rejected: <%s, %d>", fqdn,
            client->port);
            return;
        }
    }

    /*
     * We'll need to add the new fd to the list, mark it non-blocking,
     * and initiate the PM_LOG_IN sequence.
     */
    fd_settings = Fcntl(client->fd, F_GETFL, 0);
    Fcntl(client->fd, F_SETFL, fd_settings | O_NONBLOCK);

    /* append to the list of clients */
    list_append(cli_clients, client);

    syslog(LOG_DEBUG, "New connection: <%s, %d> on descriptor %d",
        fqdn, client->port, client->fd);
    _client_msg(client, CP_VERSION);
    _client_prompt(client);
}


/* handle any client activity (new connection or read/write) */
void cli_process_select(fd_set *rset, fd_set *wset, bool over_time)
{
    ListIterator itr;
    Client *client;
    
    /* New connection?  Instantiate a new client object. */
    if (FD_ISSET(listen_fd, rset))
        _create_client();

    itr = list_iterator_create(cli_clients);
    /* Client reading and writing?  */
    while ((client = list_next(itr))) {
        if (client->fd < 0)
            continue;

        if (FD_ISSET(client->fd, rset))
            _handle_client_read(client);

        if (FD_ISSET(client->fd, wset))
            _handle_client_write(client);

        /* Is this connection done? */
        if ((client->read_status == CLI_DONE) &&
            (client->write_status == CLI_IDLE))
            list_delete(itr);
    }

    list_iterator_destroy(itr);
}


void cli_prepfor_select(fd_set *rset, fd_set *wset, int *maxfd)
{
    ListIterator itr;
    Client *client;

    if (listen_fd != NO_FD) {
        assert(listen_fd >= 0);
        FD_SET(listen_fd, rset);
        *maxfd = MAX(*maxfd, listen_fd);
    }

    itr = list_iterator_create(cli_clients);
    while ((client = list_next(itr))) {
        if (client->fd < 0)
            continue;

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
}

/*
 * vi:softtabstop=4
 */
