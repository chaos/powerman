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
#include "string.h"
#include "util.h"
#include "action.h"
#include "hostlist.h"

#define LISTEN_BACKLOG    5
#define PROMPT            "PowerMan> "

/* prototypes */
static Action *_process_input(Client *c);
static Action *_parse_input(Client *c, String expect);
static void _destroy_client(Client * client);

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


static void _command_help(Client *c)
{
    c->write_status = CLI_WRITING;
    buf_printf(c->to,
        "------------- Command Reference -------------\r\n"
        "query-hard <nodes>  - query hard power status\r\n"
        "query-soft <nodes>  - query soft power status\r\n"
        "query-nodes         - query available node list\r\n"
        "on <nodes>          - hard power on\r\n"
        "off <nodes>         - hard power off\r\n"
        "cycle <nodes>       - hard power cycle\r\n"
        "reset <nodes>       - soft power reset\r\n"
        "help                - display help\r\n"
        "quit                - logout\r\n"); 
}


static void _command_unknown(Client *c, String expect)
{
    c->write_status = CLI_WRITING;
    buf_printf(c->to, "Unknown command\r\n" PROMPT);
}


static void _hostlist_error(Client *c)
{
    c->write_status = CLI_WRITING;
    buf_printf(c->to, "Error: ");

    switch (errno) {
        case ERANGE:
            buf_printf(c->to, "Too many hosts in range\r\n");
            break;
        case EINVAL:
            buf_printf(c->to, "Invalid hostlist range\r\n");
            break;
        default:
            buf_printf(c->to, "Unknown hostlist error\r\n");
    }

    buf_printf(c->to, PROMPT);
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
    Action *act = NULL;

    CHECK_MAGIC(c);

    n = buf_read(c->from);
    if ((n < 0) && (errno == EWOULDBLOCK))
        return;

    /* EOF close or wait for writes to finish */
    if (n <= 0) {
        c->read_status = CLI_DONE;
        return;
    }

    do {
        /* 
         *   If there isn't a full command left in the from buffer then this 
         * returns NULL. Otherwise it will return a newly created Action
         * act, extract the next command from the buffer, set the
         * Action fields client, seq, com, num and target for later
         * processing, and enqueue the Action.  
         */
        act = _process_input(c);
        if (act != NULL)
            act_add(act);

    } while (act != NULL);
}


/*
 * Grab a line in the "from" buffer and try to parse it as a command.
 * If it's unrecognizable return an error, if it's just a \n\r ignore it,
 * but if it's a real command pass it back to the action queue.
 */
static Action *_process_input(Client * c)
{
    Action *act;
    String expect;

    expect = util_bufgetline(c->from);
    if (expect == NULL)
        return NULL;

    act = _parse_input(c, expect);
    if (act != NULL) {
        act->client = c;
        act->seq = c->seq++;
    } 

    str_destroy(expect);
    return act;
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


/*
 * Returning a NULL means the passed command was unknown, otherwise
 * and action will be allocated and passed back for queueing.
 */
static Action *_parse_input(Client *c, String expect)
{
    Action *act;
    char *str = str_get(expect);
    char cmd[4096], arg[4096];

    memset(cmd, 0, 4096);
    memset(arg, 0, 4096);

    /* Break the command in to <command> <arg> and discard any
     * leading or trailing white space.  Only one arg is accepted,
     * all others will be discarded.
     * FIXME: Buffer overruns are possible, fix it properly
     */
    if (sscanf(str, "%s %s", cmd, arg) < 1) {
        c->write_status = CLI_WRITING;
        buf_printf(c->to, PROMPT);
        return NULL;
    }

    act = act_create(PM_ERROR);

    if (!strncasecmp(cmd, "query-hard", 10))
        act->com = PM_UPDATE_PLUGS;
    else if (!strncasecmp(cmd, "query-soft", 10))
        act->com = PM_UPDATE_NODES;
    else if (!strncasecmp(cmd, "query-nodes", 11))
        act->com = PM_NAMES;
    else if (!strncasecmp(cmd, "on", 2))
        act->com = PM_POWER_ON;
    else if (!strncasecmp(cmd, "off", 3))
        act->com = PM_POWER_OFF;
    else if (!strncasecmp(cmd, "cycle", 5))
        act->com = PM_POWER_CYCLE;
    else if (!strncasecmp(cmd, "reset", 5))
        act->com = PM_RESET;
    else if (!strncasecmp(cmd, "quit", 4))
        act->com = PM_LOG_OUT;
    else if (!strncasecmp(cmd, "help", 4)) {
        _command_help(c);
        act->com = PM_CHECK_LOGIN;
    }

    if (act->com == PM_ERROR) {
        _command_unknown(c, expect);
        act_destroy(act); 
        return NULL;
    }

    if ((act->hl = hostlist_create(arg)) == NULL) {
        _hostlist_error(c);
        act_destroy(act); 
        return NULL;
    }

    act->target = str_create(arg);
    return act; 
}


/*
 * Based on the last command successfully parsed output the
 * appropriate information and the prompt.
 */
void cli_reply(Action * act)
{
    Client *client = act->client;
    List nodes = conf_getnodes();
    ListIterator itr;
    Node *node;
    hostlist_t hl;
    char hosts[4096];
    

    CHECK_MAGIC(act);
    CHECK_MAGIC(client);
    assert(client->fd != NO_FD);

    client->write_status = CLI_WRITING;
    itr = list_iterator_create(nodes);

    switch (act->com) {
        case PM_ERROR:
            buf_printf(client->to, "%s\r\n" PROMPT, "Error: Internal");
            break;
        case PM_LOG_OUT:
            buf_printf(client->to, "Goodbye\r\n");
            _handle_client_write(client);
            client->read_status = CLI_DONE;
            client->write_status = CLI_IDLE;
            break;
        case PM_UPDATE_PLUGS:  /* query-soft */
            while ((node = list_next(itr)))
                buf_printf(client->to, (node->p_state == ST_ON) ? "1" :
                    ((node->p_state == ST_OFF) ? "0" : "?"));
                
            buf_printf(client->to, "\r\n" PROMPT);
            break;
        case PM_UPDATE_NODES:  /* query-hard */
            while ((node = list_next(itr)))
                buf_printf(client->to, (node->n_state == ST_ON) ? "1" :
                    ((node->n_state == ST_OFF) ? "0" : "?"));

            buf_printf(client->to, "\r\n" PROMPT);
            break;
        case PM_CHECK_LOGIN:
        case PM_POWER_ON:
        case PM_POWER_OFF:
        case PM_POWER_CYCLE:
        case PM_RESET:
            buf_printf(client->to, PROMPT);
            break;
        case PM_NAMES:

            if ((hl = hostlist_create(NULL)) == NULL) {
                buf_printf(client->to, "%s\r\n" PROMPT,
                    "Error: Unable to create hostlist");
                break;
            }

            while ((node = list_next(itr)))
                hostlist_push_host(hl, str_get(node->name));

            /* FIXME: Deal with possible truncation issue */
            if (hostlist_ranged_string(hl, 4096, hosts) == -1)
                syslog(LOG_ERR, "%s", "Truncated hostlist returned to client");
                
            buf_printf(client->to, "%s\r\n" PROMPT, hosts);
            hostlist_destroy(hl);
            break;
        default:
            assert(FALSE);
    }

    list_iterator_destroy(itr);
}


/*
 *   This match utility is compatible with the list API's ListFindF
 * prototype for searching a list of Client * structs.  The match
 * criterion is an identity match on their addresses.  This comes 
 * into use in the act_find() when there is a chance an Action 
 * no longer has a client associated with it.
 */
static int _match_client(Client * client, void *key)
{
    return (client == key);
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

    client->ip = str_create(buf);
    ip = str_get(client->ip);
    fqdn = ip;
    host = STRING_UNKNOWN;

    /* get client->host */
    if ((hent = gethostbyaddr((const char *) &saddr.sin_addr,
		    sizeof(struct in_addr), AF_INET)) == NULL) {
        syslog(LOG_ERR, "Unable to get host name from network address");
    } else {
        client->host = str_create(hent->h_name);
        host = str_get(client->host);
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
    client->write_status = CLI_WRITING;
    buf_printf(client->to, "PowerMan v" VERSION "\r\n" PROMPT);
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
