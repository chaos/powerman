/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
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

/* This is the component of the server that deals with clients.
 * The client itself is contained in powerman.c.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#if HAVE_TCP_WRAPPERS
#include <tcpd.h>
#endif
#include <stdio.h>
#include <fcntl.h>

#include "xtypes.h"
#include "xmalloc.h"
#include "xpoll.h"
#include "xregex.h"
#include "hostlist.h"
#include "list.h"
#include "parse_util.h"
#include "client.h"
#include "cbuf.h"
#include "error.h"
#include "client_proto.h"
#include "debug.h"
#include "pluglist.h"
#include "hprintf.h"
#include "arglist.h"
#include "device_private.h"
#include "xpty.h"
#include "powerman.h"

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;                  /* socklen_t is uint32_t in Posix.1g */
#endif /* !HAVE_SOCKLEN_T */

#define LISTEN_BACKLOG    5

#define MIN_CLIENT_BUF     1024
#define MAX_CLIENT_BUF     1024*1024

typedef struct {
    int com;                    /* script index */
    hostlist_t hl;              /* target nodes */
    int pending;                /* count of pending device actions */
    bool error;                 /* cumulative error flag for actions */
    ArgList arglist;            /* argument for query commands */
} Command;

#define CLI_MAGIC    0xdadadada
typedef struct {
    int magic;
    int fd;                     /* file desriptor for the socket */
    int ofd;                    /* separate output file descriptor (if used) */
    char *ip;                   /* IP address of the client's host */
    unsigned short int port;    /* Port of client connection */
    char *host;                 /* host name of client host */
    cbuf_t to;                  /* out buffer */
    cbuf_t from;                /* in buffer */
    Command *cmd;               /* command (there can be only one) */
    int client_id;              /* client identifier */
    bool telemetry;             /* client wants telemetry debugging info */
    bool exprange;              /* client wants host ranges expanded */
    bool client_quit;           /* set true after client quit command */
} Client;

/* prototypes for internal functions */
static Command *_create_command(Client * c, int com, char *arg1);
static void _destroy_command(Command * cmd);
static int _match_client(Client * c, void *key);
static Client *_find_client(int client_id);
static hostlist_t _hostlist_create_validated(Client * c, char *str);
static void _client_query_nodes_reply(Client * c);
static void _client_query_device_reply(Client * c, char *arg);
static void _client_query_status_reply(Client * c, bool error);
static void _client_query_status_reply_nointerp(Client * c, bool error);
static void _handle_read(Client * c);
static void _handle_write(Client * c);
static void _handle_input(Client *c);
static char *_strip_whitespace(char *str);
static void _parse_input(Client * c, char *input);
static void _destroy_client(Client * c);
static void _create_client_socket(int fd);
static void _create_client_stdio(void);
static void _act_finish(int client_id, ActError acterr, const char *fmt, ...);
static void _telemetry_printf(int client_id, const char *fmt, ...);
#if HAVE_TCP_WRAPPERS
/* tcp wrappers support */
extern int hosts_ctl(char *daemon, char *client_name, char *client_addr,
                     char *client_user);
int allow_severity = LOG_INFO;  /* logging level for accepted reqs */
int deny_severity = LOG_WARNING;/* logging level for rejected reqs */
#endif

static int *listen_fds;         /* powermand listen sockets */
static int listen_fds_len = 0;  /* count of above sockets */
static List cli_clients = NULL; /* list of clients */
static bool one_client = FALSE; /* terminate after first client */
static bool server_done = FALSE;/* true when stdio client exits */

static int cli_id_seq = 1;      /* range 1...INT_MAX */
#define _next_cli_id() \
    (cli_id_seq < INT_MAX ? cli_id_seq++ : (cli_id_seq = 1, INT_MAX))

#define _internal_error_response(c) \
    _client_printf(c, CP_ERR_INTERNAL, __FILE__, __LINE__)

#include "hostlist.h"

/*
 * Wrapped hostlist_ranged_string() with internal buffer allocation,
 * which caller must xfree().
 */
#define CHUNKSIZE 80
static char *_xhostlist_ranged_string(hostlist_t hl)
{
    int size = 0;
    char *str = NULL;

    do {
        str = (size == 0) ? xmalloc(CHUNKSIZE) : xrealloc(str, size+CHUNKSIZE);
        size += CHUNKSIZE;
    } while (hostlist_ranged_string(hl, size, str) == -1);

    return str;
}

/*
 * printf-like function which writes to the output cbuf.
 */
static void _client_printf(Client *c, const char *fmt, ...)
{
    char *str = NULL;
    int written, dropped;
    va_list ap;

    va_start(ap, fmt);
    str = hvsprintf(fmt, ap);
    va_end(ap);

    /* Write to the client buffer */
    written = cbuf_write(c->to, str, strlen(str), &dropped);
    if (written < 0)
        err(TRUE, "_client_printf: cbuf_write returned %d", written);
    else if (dropped > 0)
        err(FALSE, "_client_printf: cbuf_write dropped %d chars", dropped);

    /* Free the tmp string */
    xfree(str);
}

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
 * Build a hostlist_t from a string, validating each node name against 
 * powerman configuration.  If any bogus nodes are found, issue error 
 * response to client and return NULL.
 */
static hostlist_t _hostlist_create_validated(Client * c, char *str)
{
    hostlist_t hl = NULL;
    hostlist_t badhl = NULL;
    hostlist_iterator_t itr = NULL;
    char *host;
    bool valid = TRUE;

    if ((hl = hostlist_create(str)) == NULL) {
        /* Note: report detailed error since 'str' comes from the user */
        if (errno == ERANGE || errno == EINVAL)
            _client_printf(c, CP_ERR_HOSTLIST, "invalid range");
        else
            _internal_error_response(c);
        return NULL;
    }
    conf_exp_aliases(hl);       
    if ((badhl = hostlist_create(NULL)) == NULL) {
        /* Note: other hostlist failures not user-induced so OK to be vague */
        _internal_error_response(c);
        return NULL;
    }
    if ((itr = hostlist_iterator_create(hl)) == NULL) {
        _internal_error_response(c);
        hostlist_destroy(hl);
        return NULL;
    }
    while ((host = hostlist_next(itr)) != NULL) {
        if (!conf_node_exists(host)) {
            valid = FALSE;
            hostlist_push_host(badhl, host);
        }
        free(host); /* hostlist_next strdups returned string */
    }
    if (!valid) {
        char *hosts;

        hosts = _xhostlist_ranged_string(badhl);
        _client_printf(c, CP_ERR_NOSUCHNODES, hosts);
        xfree (hosts);
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
static void _client_query_nodes_reply(Client * c)
{
    hostlist_t nodes = conf_getnodes();

    hostlist_sort(nodes);

    if (c->exprange) {
        hostlist_iterator_t itr;
        char *node;
    
        if ((itr = hostlist_iterator_create(nodes)) == NULL) {
            _internal_error_response(c);
            return;
        }
        while ((node = hostlist_next(itr))) {
            _client_printf(c, CP_INFO_XNODES, node);
            free(node); /* hostlist_next strdups returned string */
        }
        hostlist_iterator_destroy(itr);
        
    } else {
        char *hosts = _xhostlist_ranged_string(nodes);

        _client_printf(c, CP_INFO_NODES, hosts);
        xfree (hosts);
    }

    _client_printf(c, CP_RSP_QRY_COMPLETE);
}

/* 
 * Helper for _client_query_device_reply() .
 * Create a hostlist string for the nodes attached to the specified device.
 * Caller must xfree().
 */
static char *_make_pluglist_str(Device * dev)
{
    hostlist_t hl = hostlist_create(NULL);
    PlugListIterator itr;
    Plug *plug;
    char *str = NULL;

    if (hl) {
        itr = pluglist_iterator_create(dev->plugs);
        while ((plug = pluglist_next(itr))) {
            assert(plug->name != NULL);
            hostlist_push(hl, plug->node);
        }
        pluglist_iterator_destroy(itr);

        hostlist_sort(hl);
        str = _xhostlist_ranged_string(hl);
        hostlist_destroy(hl);
    }
    return str;
}

/*
 * Helper for _client_query_device_reply.
 * Return TRUE if hostlist string in 'arg' matches any plugs on device.
 */
static bool _device_matches_targets(Device *dev, char *arg)
{
    hostlist_t targ = hostlist_create(arg);
    PlugListIterator itr;
    Plug *plug;
    bool res = FALSE;

    if (targ != NULL) {
        itr = pluglist_iterator_create(dev->plugs);
        while ((plug = pluglist_next(itr))) {
            if (hostlist_find(targ, plug->node) != -1) {
                res = TRUE;
                break;
            }
        }
        pluglist_iterator_destroy(itr);
        hostlist_destroy(targ);
    }
    return res;
}

/* 
 * Reply to client request for list of devices in powerman configuration.
 */
static void _client_query_device_reply(Client * c, char *arg)
{
    List devs = dev_getdevices();
    Device *dev;
    ListIterator itr;

    if (devs) {

        itr = list_iterator_create(devs);
        while ((dev = list_next(itr))) {
            char *nodelist;
            int con = dev->stat_successful_connects;

            if (arg && !_device_matches_targets(dev, arg))
                continue;

            if ((nodelist = _make_pluglist_str(dev))) {
                _client_printf(c, CP_INFO_DEVICE, 
                        dev->name,
                        dev->connect_state == DEV_CONNECTED ? "connected"
                          : dev->connect_state == DEV_CONNECTING ? "connecting"
                          : "disconnected",
                        con > 0 ? con - 1 : 0, 
                        dev->stat_successful_actions,
                        dev->specname,
                        nodelist);
                xfree (nodelist);
            }
        }
        list_iterator_destroy(itr);
    }
    _client_printf(c, CP_RSP_QRY_COMPLETE);
}

/* 
 * Reply to client request for plug/soft status.
 */
static void _client_query_status_reply(Client * c, bool error)
{
    Arg *arg;
    ArgListIterator itr;

    assert(c->cmd != NULL);

    if (c->exprange) {
        itr = arglist_iterator_create(c->cmd->arglist);
        while ((arg = arglist_next(itr))) {
            _client_printf(c, CP_INFO_XSTATUS, arg->node, 
                    arg->state == ST_ON ? "on" 
                    : arg->state == ST_OFF ? "off" : "unknown");
        }
        arglist_iterator_destroy(itr);

    } else {
        char *on, *off, *unknown;
        hostlist_t hl_on, hl_off, hl_unknown;

        hl_on = hostlist_create(NULL);
        hl_off = hostlist_create(NULL);
        hl_unknown = hostlist_create(NULL);

        itr = arglist_iterator_create(c->cmd->arglist);
        while ((arg = arglist_next(itr))) {
            switch (arg->state) {
                case ST_UNKNOWN:
                    hostlist_push(hl_unknown, arg->node);
                    break;
                case ST_ON:
                    hostlist_push(hl_on, arg->node);
                    break;
                case ST_OFF:
                    hostlist_push(hl_off, arg->node);
                    break;
            }
        }
        arglist_iterator_destroy(itr);

        hostlist_sort(hl_unknown);
        hostlist_sort(hl_on);
        hostlist_sort(hl_off);

        unknown = _xhostlist_ranged_string(hl_unknown);
        on      = _xhostlist_ranged_string(hl_on);
        off     = _xhostlist_ranged_string(hl_off);

        hostlist_destroy(hl_unknown);
        hostlist_destroy(hl_on);
        hostlist_destroy(hl_off);

        _client_printf(c, CP_INFO_STATUS, on, off, unknown);

        xfree (unknown);
        xfree (on);
        xfree (off);
    }

    if (error)
        _client_printf(c, CP_ERR_QRY_COMPLETE);
    else
        _client_printf(c, CP_RSP_QRY_COMPLETE);
}

/* 
 * Reply to client request for temperature/beacon status.
 */
static void _client_query_status_reply_nointerp(Client * c, bool error)
{
    Arg *arg;
    ArgListIterator itr;
    hostlist_t hl = hostlist_create(NULL);
    char *tmpstr;

    assert(c->cmd != NULL);

    itr = arglist_iterator_create(c->cmd->arglist);
    while ((arg = arglist_next(itr))) {
        _client_printf(c, CP_INFO_XSTATUS, arg->node, arg->val);
        if (!arg->val)
            hostlist_push(hl, arg->node);
    }
    arglist_iterator_destroy(itr);

    if (!hostlist_is_empty(hl)) {
        hostlist_sort(hl);
        tmpstr = _xhostlist_ranged_string(hl);
        _client_printf(c, CP_INFO_XSTATUS, tmpstr, "unknown");
        xfree (tmpstr);
    }
    if (error)
        _client_printf(c, CP_ERR_QRY_COMPLETE);
    else
        _client_printf(c, CP_RSP_QRY_COMPLETE);
    hostlist_destroy(hl);
}

/*
 * Create Command.
 * On error, return an error to the client and NULL to the caller.
 */
static Command *_create_command(Client * c, int com, char *arg1)
{
    Command *cmd = (Command *) xmalloc(sizeof(Command));

    cmd->com = com;
    cmd->error = FALSE;
    cmd->pending = 0;
    cmd->hl = NULL;
    cmd->arglist = NULL;

    if (arg1) {
        /* Note: this can send CP_ERR_HOSTLIST to client */
        cmd->hl = _hostlist_create_validated(c, arg1);
        if (cmd->hl == NULL) {
            _destroy_command(cmd);
            cmd = NULL;
        }
    } else
        cmd->hl = hostlist_copy(conf_getnodes());

    if (cmd && !dev_check_actions(cmd->com, cmd->hl)) {
        _destroy_command(cmd);
        _client_printf(c, CP_ERR_UNIMPL);
        cmd = NULL;
    }
    /* NOTE 1: cmd->arglist has a reference count and can persist after we 
     * unlink from it in _destroy_command().  If client goes away prematurely,
     * actions that write to arglist will still have valid pointers.
     * NOTE 2: we create arglist for all actions, rather than just query
     * actions, because we want to allow 'setplugstate' in any context.
     */
    if (cmd) {
        cmd->arglist = arglist_create(cmd->hl);
        if (cmd->arglist == NULL) {
            _destroy_command(cmd);
            _internal_error_response(c);
            cmd = NULL;
        }
    }
    return cmd;
}

/*
 * Destroy a Command.
 */
static void _destroy_command(Command * cmd)
{
    if (cmd->hl)
        hostlist_destroy(cmd->hl);
    if (cmd->arglist)
        arglist_unlink(cmd->arglist);
    xfree(cmd);
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
static void _parse_input(Client * c, char *input)
{
    char *str = _strip_whitespace(input);
    char arg1[CP_LINEMAX];
    Command *cmd = NULL;

    memset(arg1, 0, CP_LINEMAX);

    /* NOTE: sscanf is safe because 'str' is guaranteed to be < CP_LINEMAX */

    if (strlen(str) >= CP_LINEMAX) {
        _client_printf(c, CP_ERR_TOOLONG);              /* error: too long */
    } else if (c->cmd != NULL) {
        _client_printf(c, CP_ERR_CLIBUSY);              /* error: busy */
        return;                                         /* no prompt */
    } else if (!strncasecmp(str, CP_HELP, strlen(CP_HELP))) {
        _client_printf(c, CP_INFO_HELP);                /* help */
        _client_printf(c, CP_RSP_QRY_COMPLETE);
    } else if (!strncasecmp(str, CP_NODES, strlen(CP_NODES))) {
        _client_query_nodes_reply(c);                   /* nodes */
    } else if (!strncasecmp(str, CP_TELEMETRY, strlen(CP_TELEMETRY))) {
        c->telemetry = !c->telemetry;                   /* telemetry */
        _client_printf(c, CP_RSP_TELEMETRY, c->telemetry ? "ON" : "OFF");
    } else if (!strncasecmp(str, CP_EXPRANGE, strlen(CP_EXPRANGE))) {
        c->exprange = !c->exprange;                     /* exprange */
        _client_printf(c, CP_RSP_EXPRANGE, c->exprange ? "ON" : "OFF");
    } else if (!strncasecmp(str, CP_QUIT, strlen(CP_QUIT))) {
        c->client_quit = TRUE;
        _client_printf(c, CP_RSP_QUIT);                 /* quit */
        _handle_write(c);
    } else if (sscanf(str, CP_ON, arg1) == 1) {         /* on hostlist */
        cmd = _create_command(c, PM_POWER_ON, arg1);
    } else if (sscanf(str, CP_OFF, arg1) == 1) {        /* off hostlist */
        cmd = _create_command(c, PM_POWER_OFF, arg1);
    } else if (sscanf(str, CP_CYCLE, arg1) == 1) {      /* cycle hostlist */
        cmd = _create_command(c, PM_POWER_CYCLE, arg1);
    } else if (sscanf(str, CP_RESET, arg1) == 1) {      /* reset hostlist */
        cmd = _create_command(c, PM_RESET, arg1);
    } else if (sscanf(str, CP_BEACON_ON, arg1) == 1) {  /* beacon_on hostlist */
        cmd = _create_command(c, PM_BEACON_ON, arg1);
    } else if (sscanf(str, CP_BEACON_OFF, arg1) == 1) { /* beacon_off hostlist*/
        cmd = _create_command(c, PM_BEACON_OFF, arg1);
    } else if (sscanf(str, CP_STATUS, arg1) == 1) {     /* status [hostlist] */
        cmd = _create_command(c, PM_STATUS_PLUGS, arg1);
    } else if (!strncasecmp(str, CP_STATUS_ALL, strlen(CP_STATUS_ALL))) {
        cmd = _create_command(c, PM_STATUS_PLUGS, NULL);
    } else if (sscanf(str, CP_TEMP, arg1) == 1) {       /* temp [hostlist] */
        cmd = _create_command(c, PM_STATUS_TEMP, arg1);
    } else if (!strncasecmp(str, CP_TEMP_ALL, strlen(CP_TEMP_ALL))) {
        cmd = _create_command(c, PM_STATUS_TEMP, NULL);
    } else if (sscanf(str, CP_BEACON, arg1) == 1) {     /* beacon [hostlist] */
        cmd = _create_command(c, PM_STATUS_BEACON, arg1);
    } else if (!strncasecmp(str, CP_BEACON_ALL, strlen(CP_BEACON_ALL))) {
        cmd = _create_command(c, PM_STATUS_BEACON, NULL);
    } else if (sscanf(str, CP_DEVICE, arg1) == 1) {     /* device [hostlist] */
        _client_query_device_reply(c, arg1);              
    } else if (!strncasecmp(str, CP_DEVICE_ALL, strlen(CP_DEVICE_ALL))) {
        _client_query_device_reply(c, NULL);              
    } else {                                            /* error: unknown */
        _client_printf(c, CP_ERR_UNKNOWN);
    }

    /* enqueue device actions and tie up the client if necessary */
    if (cmd) {
        assert(cmd->hl != NULL);
        dbg(DBG_CLIENT, "_parse_input: enqueuing actions");
        cmd->pending = dev_enqueue_actions(cmd->com, cmd->hl, _act_finish, 
                c->telemetry ? _telemetry_printf : NULL, 
                c->client_id, cmd->arglist);
        if (cmd->pending == 0) {
            _client_printf(c, CP_ERR_UNIMPL);
            _destroy_command(cmd);
            cmd = NULL;
        }
        assert(c->cmd == NULL);
        c->cmd = cmd;
    }

    /* reissue prompt if we didn't queue up any device actions */
    if (cmd == NULL && !c->client_quit)
        _client_printf(c, CP_PROMPT);
}

/*
 * Callback for device debugging printfs (sent to client if --telemetry)
 */
static void _telemetry_printf(int client_id, const char *fmt, ...)
{
    va_list ap;
    Client *c;
    char *str;

    if ((c = _find_client(client_id))) {
        va_start(ap, fmt);
        str = hvsprintf(fmt, ap);
        va_end(ap);
        _client_printf(c, CP_INFO_TELEMETRY, str);
        xfree(str);
    }
}

/*
 * Callback for device action completion.
 */
static void _act_finish(int client_id, ActError acterr, const char *fmt, ...)
{
    va_list ap;
    Client *c;
    char *str;

    /* if client has gone away do nothing */
    if (!(c = _find_client(client_id)))
        return;
    assert(c->magic == CLI_MAGIC);
    assert(c->cmd != NULL);

    /* handle errors immediately */
    if (acterr != ACT_ESUCCESS) {
        va_start(ap, fmt);
        str = hvsprintf(fmt, ap);
        va_end(ap);
        _client_printf(c, CP_INFO_ACTERROR, str);
        xfree(str);

        c->cmd->error = TRUE;       /* when done say "completed with errors" */
    }

    /* all actions have called back - return response to client */
    if (--c->cmd->pending == 0) {
        switch (c->cmd->com) {
        case PM_STATUS_PLUGS:      /* status */
        case PM_STATUS_BEACON:     /* beacon */
            _client_query_status_reply(c, c->cmd->error);
            break;
        case PM_STATUS_TEMP:       /* temp */
            _client_query_status_reply_nointerp(c, c->cmd->error);
            break;
        case PM_POWER_ON:          /* on */
        case PM_POWER_OFF:         /* off */
        case PM_BEACON_ON:         /* flash */
        case PM_BEACON_OFF:        /* unflash */
        case PM_POWER_CYCLE:       /* cycle */
        case PM_RESET:             /* reset */
            if (c->cmd->error)
                _client_printf(c, CP_ERR_COM_COMPLETE);
            else
                _client_printf(c, CP_RSP_COM_COMPLETE);
            break;
        default:
            assert(FALSE);
            _internal_error_response(c);
            break;
        }

        /* clean up and re-prompt */
        _destroy_command(c->cmd);
        c->cmd = NULL;
        _client_printf(c, CP_PROMPT);
    }
}

/* 
 * Destroy a client.
 */
static void _destroy_client(Client *c)
{
    assert(c->magic == CLI_MAGIC);

    if (c->fd != NO_FD) {
        dbg(DBG_CLIENT, "_destroy_client: closing fd %d", c->fd);
        if (close(c->fd) < 0)
            err(TRUE, "close fd %d", c->fd);
        c->fd = NO_FD;
    }
    if (c->ofd != NO_FD) {
        dbg(DBG_CLIENT, "_destroy_client: closing fd %d", c->ofd);
        if (close(c->ofd) < 0)
            err(TRUE, "close fd %d", c->ofd);
        c->ofd = NO_FD;
    }
    if (c->to)
        cbuf_destroy(c->to);
    if (c->from)
        cbuf_destroy(c->from);
    if (c->cmd)
        _destroy_command(c->cmd);
    if (c->ip)
        xfree(c->ip);
    if (c->host)
        xfree(c->host);
    xfree(c);
    if (one_client)
        server_done = TRUE;
}

/* helper for _find_client */
static int _match_client(Client *c, void *key)
{
    return (c->client_id == *(int *) key);
}

/*
 * Test if a client still exists (by sequence).
 */
static Client *_find_client(int seq)
{
    return list_find_first(cli_clients, (ListFindF) _match_client, &seq);
}

/*
 * Begin listening for clients on configured listen addresses.
 * This function leaves listen_fds[] (of size listen_fds_len) initialized
 * with each element either NO_FD or an open fd we are listening on.
 */
static void _listen_client(void)
{
    int fd, error, i, opt, count;
    struct addrinfo hints, *res, *r;
    char *addr, *host, *port;
    char *what = "nothing";
    int saved_errno = 0;
    List addrs; 
    ListIterator itr;

    i = 0;
    count = 0;
    saved_errno = 0;

    addrs = conf_get_listen();
    itr = list_iterator_create(addrs);
    while ((addr = list_next(itr))) {
        host = addr;
        if (!(port = strchr(addr, ':')))
            err_exit(FALSE, "error parsing listen address: %s", addr);
        *port++ = '\0';

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        /* N.B.: host=NULL, ai_hints=AI_PASSIVE says "any interface",
         * while host="0.0.0.0" is IPv4-specific INADDR_ANY.
         */
        if ((error = getaddrinfo(host, port, &hints, &res)))
            err_exit(FALSE, "getaddrinfo: %s", gai_strerror(error));
        if (res == NULL)
            err_exit(FALSE, "listen address has no addrinfo: %s", addr);

        /* allocate the listen_fds array */
        for (r = res; r != NULL; r = r->ai_next)
            listen_fds_len++;
        if (listen_fds == NULL)
            listen_fds = (int *)xmalloc(sizeof(int) * listen_fds_len);
        else
            listen_fds = (int *)xrealloc((char *)listen_fds,
                                         sizeof(int) * listen_fds_len); 

        /* bind sockets to addresses */
        for (r = res; r != NULL; r = r->ai_next, i++) {
            assert(i < listen_fds_len); 

            listen_fds[i] = NO_FD;
            if ((fd = socket(r->ai_family, r->ai_socktype, 0)) < 0) {
                saved_errno = errno;
                what = "socket";
                continue;
            }
            opt = 1;
            if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt,
                           sizeof(opt)) < 0) {
                saved_errno = errno;
                what = "setsockopt";
                close(fd);
                continue;
            }
            nonblock_set(fd);
            if (bind(fd, r->ai_addr, r->ai_addrlen) < 0) {
                saved_errno = errno;
                what = "bind";
                close(fd);
                continue;
            }
            if (listen(fd, LISTEN_BACKLOG) < 0) {
                saved_errno = errno;
                what = "listen";
                close(fd);
                continue;
            }
            listen_fds[i] = fd;
            count++;
        }

        freeaddrinfo(res);
    }
    list_iterator_destroy(itr);

    /* FIXME: dodgy error handling here:  As we go through the loop(s) we may
     * encounter errors that are interesting, however we only display the
     * most recent one, and then only if no bindable addresses were found.
     */

    if (count == 0) {
        errno = saved_errno;
        err_exit(TRUE, "%s", what);
    }
    dbg(DBG_CLIENT, "listening on %d sockets", count);
}

static void _create_client_socket(int fd)
{
    Client *c;
    struct sockaddr_storage addr;
    socklen_t addr_size = sizeof(addr);
    char hbuf[64], pbuf[6];
    int error;

    /* create client data structure */
    c = (Client *) xmalloc(sizeof(Client));
    c->magic = CLI_MAGIC;
    c->to = NULL;
    c->from = NULL;
    c->cmd = NULL;
    c->client_id = _next_cli_id();
    c->telemetry = FALSE;
    c->exprange = FALSE;
    c->ofd = NO_FD;
    c->client_quit = FALSE;

    c->fd = accept(fd, (struct sockaddr *)&addr, &addr_size);
    if (c->fd < 0){ 
        /* client died after it initiated connect and before we could accept 
         * Ref. Stevens, UNP p424 
         */
        if (errno == EWOULDBLOCK || errno == ECONNABORTED 
                                 || errno == EPROTO || errno == EINTR) {
            _destroy_client(c);
            err(TRUE, "_create_client: accept");
            return;
        }
        err_exit(TRUE, "accept");
    }

    if ((error = getnameinfo((struct sockaddr *)&addr, addr_size, 
                             hbuf, sizeof(hbuf), pbuf, sizeof(pbuf), 
                             NI_NUMERICHOST | NI_NUMERICSERV))) {
        _destroy_client(c);
        err(TRUE, "_create_client: getnameinfo: %s", gai_strerror(error));
        return;
    }
    c->ip   = xstrdup(hbuf);
    c->port = strtoul(pbuf, NULL, 10);

    if ((error = getnameinfo((struct sockaddr *)&addr, addr_size, 
                             hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD)))
        err(FALSE, "_create_client: getnameinfo: %s", gai_strerror(error));
    else
        c->host = xstrdup(hbuf);

#if HAVE_TCP_WRAPPERS
    /* get authorization from tcp wrappers */
    if (conf_get_use_tcp_wrappers()) {
        if (!hosts_ctl(DAEMON_NAME,
                       c->host ? c->host : STRING_UNKNOWN,
                       c->ip, STRING_UNKNOWN)) {
            err(FALSE, "_create_client: tcp wrappers denies %s:%d",
                c->host ? c->host : c->ip, c->port);
            _destroy_client(c);
            return;
        }
    }
#endif

    /* create I/O buffers */
    c->to = cbuf_create(MIN_CLIENT_BUF, MAX_CLIENT_BUF);
    c->from = cbuf_create(MIN_CLIENT_BUF, MAX_CLIENT_BUF);

    nonblock_set(c->fd);

    /* append to the list of clients */
    list_append(cli_clients, c);

    dbg(DBG_CLIENT, "connect %s:%d fd %d",
        c->host ? c->host : c->ip,
        c->port, c->fd);

    /* prompt the client */
    _client_printf(c, CP_VERSION, META_VERSION);
    _client_printf(c, CP_PROMPT);
}

static void _create_client_stdio(void)
{
    Client *c;

    /* create client data structure */
    c = (Client *) xmalloc(sizeof(Client));
    c->magic = CLI_MAGIC;
    c->cmd = NULL;
    c->client_id = _next_cli_id();
    c->telemetry = FALSE;
    c->exprange = FALSE;
    c->client_quit = FALSE;
    c->fd = STDIN_FILENO;
    c->ofd = STDOUT_FILENO;
    c->host = xstrdup("localhost");
    c->ip = xstrdup("127.0.0.1"); /* XXX lies */
    c->port = 0;
    c->to = cbuf_create(MIN_CLIENT_BUF, MAX_CLIENT_BUF);
    c->from = cbuf_create(MIN_CLIENT_BUF, MAX_CLIENT_BUF);

    nonblock_set(c->fd);
    nonblock_set(c->ofd);

    /* append to the list of clients */
    list_append(cli_clients, c);

    /* prompt the client */
    _client_printf(c, CP_VERSION, META_VERSION);
    _client_printf(c, CP_PROMPT);
}

/* 
 * select(2) read handler for the client 
 */
static void _handle_read(Client * c)
{
    int n;
    int dropped;

    assert(c->magic == CLI_MAGIC);
    n = cbuf_write_from_fd(c->from, c->fd, -1, &dropped);
    if (n < 0) {
        c->client_quit = TRUE;
        err(TRUE, "client read error");
        return;
    }
    if (n == 0) {
        c->client_quit = TRUE;
        err(FALSE, "client read returned EOF");
        return;
    }
    if (dropped != 0)
        err(FALSE, "dropped %d bytes of client input", dropped);
}

/* 
 * select(2) write handler for the client 
 */
static void _handle_write(Client * c)
{
    int n;
    int ofd = c->ofd != NO_FD ? c->ofd : c->fd;

    assert(c->magic == CLI_MAGIC);
    if (c->client_quit)
        nonblock_clr(ofd);
    n = cbuf_read_to_fd(c->to, ofd, -1);
    if (n < 0) {
        err(TRUE, "write error on client");
        c->client_quit = TRUE;
    }
}

static void _handle_input(Client *c)
{
    char buf[MAX_CLIENT_BUF];
    int len;

    while ((len = cbuf_read_line(c->from, buf, sizeof(buf), 1)) > 0)
        _parse_input(c, buf);
    if (len < 0)
        err(TRUE, "client cbuf_read_line returned %d", len);
}

/*
 * Prep rset/wset/maxfd for the main poll call.
 */
void cli_pre_poll(xpollfd_t pfd)
{
    ListIterator itr;
    Client *client;
    int i;

    for (i = 0; i < listen_fds_len; i++) {
        if (listen_fds[i] != NO_FD)
            assert(listen_fds[i] >= 0);
            xpollfd_set(pfd, listen_fds[i], XPOLLIN);
    }

    itr = list_iterator_create(cli_clients);
    while ((client = list_next(itr))) {
        if (client->fd < 0)
            continue;

        /* always set read set bits so select will unblock if the
         * connection is dropped.
         */
        xpollfd_set(pfd, client->fd, XPOLLIN);

        /* need to be in the write set if we are sending anything */
        if (!cbuf_is_empty(client->to)) {
            if (client->ofd != NO_FD)
                xpollfd_set(pfd, client->ofd, XPOLLOUT);
            else
                xpollfd_set(pfd, client->fd, XPOLLOUT);
        }
    }
    list_iterator_destroy(itr);
}

/* 
 * Handle any client activity (new connection or read/write).
 */
void cli_post_poll(xpollfd_t pfd)
{
    ListIterator itr;
    Client *c;
    int i;

    for (i = 0; i < listen_fds_len; i++) {
        if (listen_fds[i] != NO_FD)
            if ((xpollfd_revents(pfd, listen_fds[i]) & XPOLLIN))
                _create_client_socket(listen_fds[i]);
    }

    itr = list_iterator_create(cli_clients);
    while ((c = list_next(itr))) {
        if (c->fd != NO_FD) {
            short flags = xpollfd_revents(pfd, c->fd);

            if (flags & XPOLLERR)
                err(FALSE, "client poll: error");
            if (flags & XPOLLHUP)
                err(FALSE, "client poll: hangup");
            if (flags & XPOLLNVAL)
                err(FALSE, "client poll: fd not open");
            if (flags & (XPOLLERR | XPOLLHUP | XPOLLNVAL))
                goto client_dead;
            if ((flags & XPOLLIN))
                _handle_read(c);
            if ((flags & XPOLLOUT))
                _handle_write(c);
        }
        if (c->ofd != NO_FD) {
            short flags = xpollfd_revents(pfd, c->ofd);

            if (flags & XPOLLERR)
                err(FALSE, "client poll: error");
            if (flags & XPOLLHUP)
                err(FALSE, "client poll: hangup");
            if (flags & XPOLLNVAL)
                err(FALSE, "client poll: fd not open");
            if (flags & (XPOLLERR | XPOLLHUP | XPOLLNVAL))
                goto client_dead;
            if (c->fd == NO_FD)
                goto client_dead;
            if ((flags & XPOLLOUT))
                _handle_write(c);
        }

        _handle_input(c);

        if (c->client_quit)
            goto client_dead;
        continue;

client_dead:
        list_delete(itr);
    }
    list_iterator_destroy(itr);
}

/* hook so daemonization function can avoid closing our fd */
void cli_listen_fds(int **fdsp, int *lenp)
{
    *fdsp = listen_fds;
    *lenp = listen_fds_len;
}

bool cli_server_done(void)
{
    return server_done;
}

void cli_start(bool use_stdio, bool oc)
{
    if (use_stdio)
        _create_client_stdio();
    else
        _listen_client();
    one_client = oc;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
