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
#include <stdarg.h>

#include <arpa/inet.h>
#include <tcpd.h>
#include <stdio.h>
#include <fcntl.h>

#include "powerman.h"
#include "wrappers.h"
#include "list.h"
#include "parse_util.h"
#include "client.h"
#include "cbuf.h"
#include "error.h"
#include "hostlist.h"
#include "client_proto.h"
#include "debug.h"
#include "device.h"

#define LISTEN_BACKLOG    5

#define MIN_CLIENT_BUF     1024
#define MAX_CLIENT_BUF     1024*1024

typedef struct {
    int com;                    /* script index */
    hostlist_t hl;              /* target nodes */
    int pending;                /* count of pending device actions */
    bool error;                 /* cumulative error flag for actions */
    ArgList *arglist;           /* argument for query commands */
} Command;

#define CLI_MAGIC    0xdadadada
typedef enum { CLI_IDLE, CLI_READING, CLI_WRITING, CLI_DONE } Client_Status;
typedef struct {
    int magic;
    Client_Status read_status;
    Client_Status write_status;
    int fd;                     /* file desriptor for  the socket */
    char *ip;                   /* IP address of the client's host */
    unsigned short int port;    /* Port of client connection */
    char *host;                 /* host name of client host */
    cbuf_t to;                  /* out buffer */
    cbuf_t from;                /* in buffer */
    Command *cmd;               /* command (there can be only one) */
    int client_id;              /* client identifier */
    bool telemetry;             /* client wants telemetry debugging info */
    bool exprange;              /* client wants host ranges expanded */
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
static void _create_client(void);
static void _act_finish(int client_id, ActError acterr, const char *fmt, ...);
static void _telemetry_printf(int client_id, const char *fmt, ...);

/* tcp wrappers support */
extern int hosts_ctl(char *daemon, char *client_name, char *client_addr,
                     char *client_user);
int allow_severity = LOG_INFO;  /* logging level for accepted reqs */
int deny_severity = LOG_WARNING;        /* logging level for rejected reqs */

static int listen_fd = NO_FD;   /* powermand listen socket */
static List cli_clients = NULL; /* list of clients */

static int cli_id_seq = 1;      /* range 1...INT_MAX */
#define _next_cli_id() \
    (cli_id_seq < INT_MAX ? cli_id_seq++ : (cli_id_seq = 1, INT_MAX))

#define _internal_error_response(c) \
    _client_printf(c, CP_ERR_INTERNAL, __FILE__, __LINE__)

/*
 * Allocate a string and sprintf to it, resizing until sprintf succedes.
 * Then copy the string to c->out buffer and free it.
 */
#define CHUNKSIZE 80
static void _client_printf(Client *c, const char *fmt, ...)
{
    va_list ap;
    int len, size = 0;
    char *str = NULL;
    int written, dropped;

    /* build tmp string */
    do {
        str = (size == 0) ? Malloc(CHUNKSIZE) : Realloc(str, size+CHUNKSIZE);
        size += CHUNKSIZE;

        va_start(ap, fmt);
        len = vsnprintf(str, size, fmt, ap);
        va_end(ap);
    } while (len == -1 || len >= size);
    assert(len == strlen(str));

    /* Write to the client buffer */
    c->write_status = CLI_WRITING;
    written = cbuf_write(c->to, str, len, &dropped);
    if (written < 0)
        err(TRUE, "_client_printf: cbuf_write returned %d", written);
    else if (dropped > 0)
        err(FALSE, "_client_printf: cbuf_write dropped %d chars", dropped);

    /* Free the tmp string */
    Free(str);
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
    }
    if (!valid) {
        char hosts[CP_LINEMAX];

        if (hostlist_ranged_string(badhl, sizeof(hosts), hosts) == -1)
            _client_printf(c, CP_ERR_NOSUCHNODES, str);
        else
            _client_printf(c, CP_ERR_NOSUCHNODES, hosts);
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

    if (c->exprange) {
        hostlist_iterator_t itr;
        char *node;
    
        if ((itr = hostlist_iterator_create(nodes)) == NULL) {
            _internal_error_response(c);
            return;
        }
        while ((node = hostlist_next(itr))) {
            _client_printf(c, CP_INFO_XNODES, node);
        }
        hostlist_iterator_destroy(itr);
        
    } else {
        char hosts[CP_LINEMAX];

        if (hostlist_ranged_string(nodes, sizeof(hosts), hosts) == -1) {
            _internal_error_response(c);
            return;
        }
        _client_printf(c, CP_INFO_NODES, hosts);
    }

    _client_printf(c, CP_RSP_QRY_COMPLETE);
}

/* 
 * Helper for _client_query_device_reply() .
 * Create a hostlist string for the plugs attached to the specified device.
 * Return TRUE if str contains a valid hostlist string.
 */
static bool _make_pluglist(Device * dev, char *str, int len)
{
    hostlist_t hl = hostlist_create(NULL);
    ListIterator itr;
    bool res = FALSE;
    Plug *plug;

    if (hl != NULL) {
        itr = list_iterator_create(dev->plugs);
        while ((plug = list_next(itr)))
            hostlist_push(hl, plug->node);
        list_iterator_destroy(itr);

        if (hostlist_ranged_string(hl, len, str) != -1)
            res = TRUE;
        hostlist_destroy(hl);
    }
    return res;
}

/*
 * Helper for _client_query_device_reply.
 * Return TRUE if hostlist string in 'arg' matches any plugs on device.
 */
static bool _device_matches_targets(Device *dev, char *arg)
{
    hostlist_t targ = hostlist_create(arg);
    ListIterator itr;
    Plug *plug;
    bool res = FALSE;

    if (targ != NULL) {
        itr = list_iterator_create(dev->plugs);
        while ((plug = list_next(itr))) {
            if (hostlist_find(targ, plug->node) != -1) {
                res = TRUE;
                break;
            }
        }
        list_iterator_destroy(itr);
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
            char nodelist[CP_LINEMAX];
            int con = dev->stat_successful_connects;

            if (arg && !_device_matches_targets(dev, arg))
                continue;

            if (_make_pluglist(dev, nodelist, sizeof(nodelist))) {
                _client_printf(c, CP_INFO_DEVICE, 
                        dev->name,
                        con > 0 ? con - 1 : 0, 
                        dev->stat_successful_actions,
                        dev->specname,
                        nodelist);
            }
        }
        list_iterator_destroy(itr);
    }
    _client_printf(c, CP_RSP_QRY_COMPLETE);

}

/* helper for _client_query_status_reply */
static int _argval_ranged_string(ArgList * arglist, char *str, int len,
                                 InterpState state)
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
        hostlist_sort(hl);
        if (hostlist_ranged_string(hl, len, str) != -1)
            res = 0;
        hostlist_destroy(hl);
    }
    return res;
}

/* 
 * Reply to client request for plug/soft status.
 */
static void _client_query_status_reply(Client * c, bool error)
{
    ArgList *arglist = c->cmd->arglist;

    assert(c->cmd != NULL);

    if (c->exprange) {
        ListIterator itr;
        Arg *arg;

        itr = list_iterator_create(arglist->argv);
        while ((arg = list_next(itr))) {
            _client_printf(c, CP_INFO_XSTATUS, arg->node, 
                    arg->state == ST_ON ? "on" : 
                    arg->state == ST_OFF ? "off" : "unknown");
        }
        list_iterator_destroy(itr);

    } else {
        char on[CP_LINEMAX], off[CP_LINEMAX], unknown[CP_LINEMAX];
        int n;

        n  = _argval_ranged_string(arglist, on,      CP_LINEMAX, ST_ON);
        n |= _argval_ranged_string(arglist, off,     CP_LINEMAX, ST_OFF);
        n |= _argval_ranged_string(arglist, unknown, CP_LINEMAX, ST_UNKNOWN);
        if (n != 0) {
            _internal_error_response(c);
            return;
        }
        _client_printf(c, CP_INFO_STATUS, on, off, unknown);
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
    ListIterator itr;
    Arg *arg;
    hostlist_t hl = hostlist_create(NULL);
    char tmpstr[CP_LINEMAX];

    assert(c->cmd != NULL);
    itr = list_iterator_create(c->cmd->arglist->argv);
    while ((arg = list_next(itr))) {
        if (arg->val)
            _client_printf(c, CP_INFO_XSTATUS, arg->node, arg->val);
        else
            hostlist_push(hl, arg->node);
    }
    list_iterator_destroy(itr);
    if (!hostlist_is_empty(hl)) {
        if (hostlist_ranged_string(hl, sizeof(tmpstr), tmpstr) == -1) {
            _internal_error_response(c);
            return;
        }
        _client_printf(c, CP_INFO_XSTATUS, tmpstr, "unknown");
    }
    if (error)
        _client_printf(c, CP_ERR_QRY_COMPLETE);
    else
        _client_printf(c, CP_RSP_QRY_COMPLETE);
}

/*
 * Create Command.
 * On error, return an error to the client and NULL to the caller.
 */
static Command *_create_command(Client * c, int com, char *arg1)
{
    Command *cmd = (Command *) Malloc(sizeof(Command));

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
     * actions, because we want to allow 'setstatus' in any context.
     */
    if (cmd) {
        cmd->arglist = dev_create_arglist(cmd->hl);
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
        _client_printf(c, CP_RSP_QUIT);                 /* quit */
        _handle_write(c);
        c->read_status = CLI_DONE;
        c->write_status = CLI_IDLE;
        return;                                         /* no prompt */
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
    } else if (sscanf(str, CP_SOFT, arg1) == 1) {       /* soft [hostlist] */
        cmd = _create_command(c, PM_STATUS_NODES, arg1);
    } else if (!strncasecmp(str, CP_SOFT_ALL, strlen(CP_SOFT_ALL))) {
        cmd = _create_command(c, PM_STATUS_NODES, NULL);
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
    if (cmd == NULL)
        _client_printf(c, CP_PROMPT);
}

/*
 * Callback for device debugging printfs (sent to client if --telemetry)
 */
#define TRUNC_MSG "[truncated]"
static void _telemetry_printf(int client_id, const char *fmt, ...)
{
    va_list ap;
    Client *c;

    if ((c = _find_client(client_id))) {
        char buf[CP_LINEMAX];
        int len = CP_LINEMAX - sizeof(CP_INFO_TELEMETRY) - sizeof(TRUNC_MSG);
        int n;

        va_start(ap, fmt);
        n = vsnprintf(buf, len, fmt, ap);
        va_end(ap);

        if (n < 0 || n >= len)
            strcat(buf, TRUNC_MSG);

        _client_printf(c, CP_INFO_TELEMETRY, buf);
    }
}

/*
 * Callback for device action completion.
 */
static void _act_finish(int client_id, ActError acterr, const char *fmt, ...)
{
    va_list ap;
    Client *c;

    /* if client has gone away do nothing */
    if (!(c = _find_client(client_id)))
        return;
    assert(c->magic == CLI_MAGIC);
    assert(c->cmd != NULL);

    /* handle errors immediately */
    if (acterr != ACT_ESUCCESS) {
        char tmpstr[CP_LINEMAX];

        va_start(ap, fmt);
        vsnprintf(tmpstr, CP_LINEMAX, fmt, ap);     /* ignore truncation */
        va_end(ap);
        _client_printf(c, CP_INFO_ACTERROR, tmpstr);

        c->cmd->error = TRUE;       /* when done say "completed with errors" */
    }

    /* all actions have called back - return response to client */
    if (--c->cmd->pending == 0) {
        switch (c->cmd->com) {
        case PM_STATUS_PLUGS:      /* status */
        case PM_STATUS_NODES:      /* soft */
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
        Close(c->fd);
        c->fd = NO_FD;
        dbg(DBG_CLIENT, "_destroy_client: closing fd %d", c->fd);
    }
    if (c->to)
        cbuf_destroy(c->to);
    if (c->from)
        cbuf_destroy(c->from);
    if (c->cmd)
        _destroy_command(c->cmd);
    if (c->ip)
        Free(c->ip);
    if (c->host)
        Free(c->host);
    Free(c);
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
    Setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt,
               sizeof(sock_opt));
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
    Client *c;
    struct sockaddr_in saddr;
    int saddr_size = sizeof(struct sockaddr_in);
    int fd_settings;
    struct hostent *hent;
    char buf[64];

    /* create client data structure */
    c = (Client *) Malloc(sizeof(Client));
    c->magic = CLI_MAGIC;
    c->read_status = CLI_READING;
    c->write_status = CLI_IDLE;
    c->to = NULL;
    c->from = NULL;
    c->cmd = NULL;
    c->client_id = _next_cli_id();
    c->telemetry = FALSE;
    c->exprange = FALSE;

    c->fd = Accept(listen_fd, &saddr, &saddr_size);
    /* client died after it initiated connect and before we could accept */
    if (c->fd < 0) {
        _destroy_client(c);
        err(TRUE, "_create_client: accept");
        return;
    }

    /* get c->ip */
    if (inet_ntop(AF_INET, &saddr.sin_addr, buf, sizeof(buf)) == NULL) {
        _destroy_client(c);
        err(TRUE, "_create_client: inet_ntop");
        return;
    }
    c->ip = Strdup(buf);
    c->port = ntohs(saddr.sin_port);

    /* get c->host */
    if ((hent = gethostbyaddr((const char *) &saddr.sin_addr,
                              sizeof(struct in_addr), AF_INET)) == NULL) {
        err(FALSE, "_create_client: gethostbyaddr failed");
    } else {
        c->host = Strdup(hent->h_name);
    }

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

    /* create I/O buffers */
    c->to = cbuf_create(MIN_CLIENT_BUF, MAX_CLIENT_BUF);
    c->from = cbuf_create(MIN_CLIENT_BUF, MAX_CLIENT_BUF);

    /* mark fd as non-blocking */
    fd_settings = Fcntl(c->fd, F_GETFL, 0);
    Fcntl(c->fd, F_SETFL, fd_settings | O_NONBLOCK);

    /* append to the list of clients */
    list_append(cli_clients, c);

    dbg(DBG_CLIENT, "connect %s:%d fd %d",
        c->host ? c->host : c->ip,
        c->port, c->fd);

    /* prompt the client */
    _client_printf(c, CP_VERSION, POWERMAN_VERSION);
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
        err(TRUE, "client read error");
        c->read_status = CLI_DONE;
        return;
    }
    if (n == 0) {
        err(FALSE, "client read returned EOF");
        c->read_status = CLI_DONE;
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

    assert(c->magic == CLI_MAGIC);
    n = cbuf_read_to_fd(c->to, c->fd, -1);
    if (n < 0) {
        err(TRUE, "write error on client");
        return;
    }
    if (n == 0) { /* XXX is this even possible? */
        err(FALSE, "write sent no data to client");
        return; 
    }
    if (cbuf_is_empty(c->to))
        c->write_status = CLI_IDLE;
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
 * Prep rset/wset/maxfd for the main select call.
 */
void cli_pre_select(fd_set * rset, fd_set * wset, int *maxfd)
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
void cli_post_select(fd_set * rset, fd_set * wset)
{
    ListIterator itr;
    Client *c;

    if (FD_ISSET(listen_fd, rset))
        _create_client();

    itr = list_iterator_create(cli_clients);
    while ((c = list_next(itr))) {
        if (c->fd < 0)
            continue;

        if (FD_ISSET(c->fd, rset))
            _handle_read(c);

        if (FD_ISSET(c->fd, wset))
            _handle_write(c);

        _handle_input(c);

        if ((c->read_status == CLI_DONE) &&
            (c->write_status == CLI_IDLE))
            list_delete(itr);
    }
    list_iterator_destroy(itr);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
