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

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#include "powerman.h"
#include "list.h"
#include "parse_util.h"
#include "device.h"
#include "error.h"
#include "wrappers.h"
#include "cbuf.h"
#include "hostlist.h"
#include "debug.h"
#include "client_proto.h"

#define MIN_DEV_BUF     1024
#define MAX_DEV_BUF     1024*1024

/*
 * Actions are appended to a per device list in dev->acts
 */
#define ACT_MAGIC 0xb00bb000
typedef struct {
    int magic;
    int com;                    /* one of the PM_* above */
    ListIterator itr;           /* next place in the script sequence */
    Stmt *cur;                  /* current place in the script sequence */
    char *target;               /* native device represenation of target plug(s) */
    ActionCB complete_fun;      /* callback for action completion */
    VerbosePrintf vpf_fun;      /* callback for device telemetry */
    int client_id;              /* client id so completion can find client */
    ActError errnum;            /* errno for action */
    struct timeval time_stamp;  /* time stamp for timeouts */
    struct timeval delay_start; /* time stamp for delay completion */
    ArgList *arglist;           /* argument for query actions (list of Arg's) */
} Action;

#define _act_started(act)   ((act)->cur != NULL)


static bool _reconnect(Device * dev);
static bool _finish_connect(Device * dev, struct timeval *timeout);
static bool _time_to_reconnect(Device * dev, struct timeval *timeout);
static void _disconnect(Device * dev);
static bool _process_expect(Device * dev);
static bool _process_send(Device * dev);
static bool _process_delay(Device * dev, struct timeval *timeout);
static void _set_argval_state(ArgList * arglist, char *node,
                              ArgState state);
static void _set_argval(ArgList * arglist, char *node, char *val);
static void _match_subexpressions(Device * dev, Action * act,
                                  char *expect);
static int _match_name(Device * dev, void *key);
static void _handle_read(Device * dev);
static void _handle_write(Device * dev);
static void _process_action(Device * dev, struct timeval *timeout);
static bool _timeout(struct timeval *timestamp, struct timeval *timeout,
                     struct timeval *timeleft);
static int _enqueue_actions(Device * dev, int com, hostlist_t hl,
                            ActionCB complete_fun, VerbosePrintf vpf_fun,
                            int client_id, ArgList * arglist);
static Action *_create_action(Device * dev, int com, char *target,
                              ActionCB complete_fun, VerbosePrintf vpf_fun,
                              int client_id, ArgList * arglist);
static int _enqueue_targetted_actions(Device * dev, int com, hostlist_t hl,
                                      ActionCB complete_fun, 
                                      VerbosePrintf vpf_fun,
                                      int client_id, ArgList * arglist);
static unsigned char *_findregex(regex_t * re, unsigned char *str,
                                 int len);
static char *_getregex_buf(cbuf_t b, regex_t * re);
static bool _command_needs_device(Device * dev, hostlist_t hl);
static void _process_ping(Device * dev, struct timeval *timeout);

static List dev_devices = NULL;


static void _dbg_actions(Device * dev)
{
    char tmpstr[1024];
    Action *act;
    ListIterator itr = list_iterator_create(dev->acts);

    tmpstr[0] = '\0';
    while ((act = list_next(itr))) {
        snprintf(tmpstr + strlen(tmpstr), sizeof(tmpstr) - strlen(tmpstr),
                 "%d,", act->com);
    }
    tmpstr[strlen(tmpstr) - 1] = '\0';  /* zap trailing comma */
    dbg(DBG_ACTION, "%s: %s", dev->name, tmpstr);
}

/* 
 * Find regular expression in string.
 *  re (IN)   regular expression
 *  str (IN)  where to look for regular expression
 *  len (IN)  number of chars in str to search
 *  RETURN    pointer to char following the last char of the match,
 *            or NULL if no match
 */
static unsigned char *_findregex(regex_t * re, unsigned char *str, int len)
{
    int n;
    size_t nmatch = MAX_MATCH_POS + 1;
    regmatch_t pmatch[MAX_MATCH_POS + 1];
    int eflags = 0;

    /* FIXME: len not observed before assertion! */
    n = Regexec(re, str, nmatch, pmatch, eflags);
    if (n != REG_NOERROR)
        return NULL;
    if (pmatch[0].rm_so == -1 || pmatch[0].rm_eo == -1)
        return NULL;
    assert(pmatch[0].rm_eo <= len);
    return (str + pmatch[0].rm_eo);
}

/*
 * Apply regular expression to the contents of a cbuf_t.
 * If there is a match, return (and consume) from the beginning
 * of the buffer to the last character of the match.
 * NOTE: embedded \0 chars are converted to \377 because libc regex 
 * functions would treat these as string terminators.  As a result, 
 * \0 chars cannot be matched explicitly.
 *  b (IN)  buffer to apply regex to
 *  re (IN) regular expression
 *  RETURN  String match (caller must free) or NULL if no match
 */
static char *_getregex_buf(cbuf_t b, regex_t * re)
{
    unsigned char *str = Malloc(MAX_DEV_BUF + 1);
    unsigned char *match_end;
    int bytes_peeked = cbuf_peek(b, str, MAX_DEV_BUF);
    int i, dropped;

    if (bytes_peeked <= 0) {            /* FIXME: any -1 handling needed? */
        if (bytes_peeked < 0)
            err(TRUE, "_getregex_buf: cbuf_peek returned %d", bytes_peeked);
        Free(str);
        return NULL;
    }
    assert(bytes_peeked <= MAX_DEV_BUF);
    for (i = 0; i < bytes_peeked; i++) {/* convert embedded \0 to \377 */
        if (str[i] == '\0') {
            str[i] = '\377';
        }
    }
    str[bytes_peeked] = '\0';           /* null terminate result */
    match_end = _findregex(re, str, bytes_peeked);
    if (match_end == NULL) {
        Free(str);
        return NULL;
    }
    assert(match_end - str <= strlen(str));
    *match_end = '\0';
                                        /* match: consume that much buffer */
    dropped = cbuf_drop(b, match_end - str);
    if (dropped != match_end - str)
        err((dropped < 0), "_getregex_buf: cbuf_drop returned %d", dropped);

    return str;
}

static Action *_create_action(Device * dev, int com, char *target,
                              ActionCB complete_fun, VerbosePrintf vpf_fun,
                              int client_id, ArgList * arglist)
{
    Action *act;

    dbg(DBG_ACTION, "_create_action: %d", com);
    act = (Action *) Malloc(sizeof(Action));
    act->magic = ACT_MAGIC;
    act->com = com;
    act->complete_fun = complete_fun;
    act->vpf_fun = vpf_fun;
    act->client_id = client_id;
    act->itr = list_iterator_create(dev->scripts[act->com]);
    act->cur = NULL;
    act->target = target ? Strdup(target) : NULL;
    act->errnum = ACT_ESUCCESS;
    act->arglist = arglist ? dev_link_arglist(arglist) : NULL;
    timerclear(&act->time_stamp);

    return act;
}

static void _destroy_action(Action * act)
{
    assert(act->magic == ACT_MAGIC);
    act->magic = 0;
    dbg(DBG_ACTION, "_destroy_action: %d", act->com);
    if (act->target != NULL)
        Free(act->target);
    act->target = NULL;
    if (act->itr != NULL)
        list_iterator_destroy(act->itr);
    act->itr = NULL;
    act->cur = NULL;
    if (act->arglist)
        dev_unlink_arglist(act->arglist);
    Free(act);
}

/* initialize this module */
void dev_init(void)
{
    dev_devices = list_create((ListDelF) dev_destroy);
}

/* tear down this module */
void dev_fini(void)
{
    list_destroy(dev_devices);
}

/* add a device to the device list (called from config file parser) */
void dev_add(Device * dev)
{
    list_append(dev_devices, dev);
}

/* 
 * Client needs access to device list to process "devices" query.
 */
List dev_getdevices(void)
{
    return dev_devices;
}

/*
 * Test whether timeout has occurred
 *  time_stamp (IN) 
 *  timeout (IN)
 *  timeleft (OUT)  if timeout has not occurred, put time left here
 *  RETURN          TRUE if (time_stamp + timeout > now)
 */
static bool _timeout(struct timeval *time_stamp, struct timeval *timeout,
                     struct timeval *timeleft)
{
    struct timeval now;
    struct timeval limit;
    bool result = FALSE;
    /* limit = time_stamp + timeout */
    timeradd(time_stamp, timeout, &limit);

    Gettimeofday(&now, NULL);

    if (timercmp(&now, &limit, >))      /* if now > limit */
        result = TRUE;

    if (result == FALSE)
        timersub(&limit, &now, timeleft);       /* timeleft = limit - now */
    else
        timerclear(timeleft);

    return result;
}

/* 
 * If tv is less than timeout, or timeout is zero, set timeout = tv.
 */
void _update_timeout(struct timeval *timeout, struct timeval *tv)
{
    if (timercmp(tv, timeout, <) || !timerisset(timeout))
        *timeout = *tv;
}

/* 
 * Return TRUE if OK to attempt reconnect.  If FALSE, put the time left 
 * in timeout if it is less than timeout or if timeout is zero.
 */
bool _time_to_reconnect(Device * dev, struct timeval *timeout)
{
    static int rtab[] = { 1, 2, 4, 8, 15, 30, 60 };
    int max_rtab_index = sizeof(rtab) / sizeof(int) - 1;
    int rix = dev->retry_count - 1;
    struct timeval timeleft, retry;
    bool reconnect = TRUE;

    if (dev->retry_count > 0) {

        timerclear(&retry);
        retry.tv_sec = rtab[rix > max_rtab_index ? max_rtab_index : rix];

        if (!_timeout(&dev->last_retry, &retry, &timeleft))
            reconnect = FALSE;
        if (timeout && !reconnect)
            _update_timeout(timeout, &timeleft);
    }
    return reconnect;
}


/*
 *   The dev struct is initialized with all the needed host info.
 * This is my copying of Stevens' nonblocking connect.  After we 
 * have a file descriptor we create buffers for sending and 
 * receiving.  At the bottom, in the unlikely event the the 
 * connect completes immediately we launch the log in script.
 */
static bool _reconnect(Device * dev)
{
    struct addrinfo hints, *addrinfo;
    int sock_opt;
    int fd_settings;

    assert(dev->magic == DEV_MAGIC);
    assert(dev->type == TCP_DEV);
    assert(dev->connect_status == DEV_NOT_CONNECTED);
    assert(dev->fd == -1);

    Gettimeofday(&dev->last_retry, NULL);
    dev->retry_count++;

    memset(&hints, 0, sizeof(struct addrinfo));
    addrinfo = &hints;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    switch (dev->type) {
    case TCP_DEV:
        Getaddrinfo(dev->devu.tcpd.host, dev->devu.tcpd.service,
                    &hints, &addrinfo);
        break;
    default:
        err_exit(FALSE, "unknown device type %d", dev->type);
    }

    dev->fd = Socket(addrinfo->ai_family, addrinfo->ai_socktype,
                     addrinfo->ai_protocol);

    dbg(DBG_DEVICE, "_reconnect: %s on fd %d", dev->name, dev->fd);

    /* set up and initiate a non-blocking connect */

    sock_opt = 1;
    Setsockopt(dev->fd, SOL_SOCKET, SO_REUSEADDR,
               &(sock_opt), sizeof(sock_opt));
    fd_settings = Fcntl(dev->fd, F_GETFL, 0);
    Fcntl(dev->fd, F_SETFL, fd_settings | O_NONBLOCK);

    /* Connect - 0 = connected, -1 implies EINPROGRESS */
    dev->connect_status = DEV_CONNECTING;
    if (Connect(dev->fd, addrinfo->ai_addr, addrinfo->ai_addrlen) >= 0)
        _finish_connect(dev, NULL);
    freeaddrinfo(addrinfo);

    {
        Action *act = list_peek(dev->acts);
        if (act)
            dbg(DBG_ACTION, "_reconnect: next action on queue: %d",
                act->com);
        else
            dbg(DBG_ACTION, "_reconnect: no action on queue");
    }

    return (dev->connect_status == DEV_CONNECTED);
}

/* helper for dev_check_actions/dev_enqueue_actions */
static bool _command_needs_device(Device * dev, hostlist_t hl)
{
    bool needed = FALSE;
    ListIterator itr;
    Plug *plug;

    itr = list_iterator_create(dev->plugs);
    while ((plug = list_next(itr))) {
        if (plug->node != NULL && hostlist_find(hl, plug->node) != -1) {
            needed = TRUE;
            break;
        }
    }
    list_iterator_destroy(itr);
    return needed;
}

/*
 * Return true if all devices targetted by hostlist implement the
 * specified action.
 */
bool dev_check_actions(int com, hostlist_t hl)
{
    Device *dev;
    ListIterator itr;
    bool valid = TRUE;

    if (hl != NULL) {
        itr = list_iterator_create(dev_devices);
        while ((dev = list_next(itr))) {
            if (_command_needs_device(dev, hl)) {
                if (dev->scripts[com] == NULL) {  /* unimplemented */
                    valid = FALSE;
                    break;
                }
            }
        }
        list_iterator_destroy(itr);
    }
    return valid;
}

/*
 * Translate a command from a client into actions for devices.
 * Return an action count so the client be notified when all the
 * actions "check in".
 */
int dev_enqueue_actions(int com, hostlist_t hl, ActionCB complete_fun, 
        VerbosePrintf vpf_fun, int client_id, ArgList * arglist)
{
    Device *dev;
    ListIterator itr;
    int total = 0;

    itr = list_iterator_create(dev_devices);
    while ((dev = list_next(itr))) {
        int count;

        if (!dev->scripts[com])               /* unimplemented script */
            continue;
        if (hl && !_command_needs_device(dev, hl))  /* uninvolved device */
            continue;
        count = _enqueue_actions(dev, com, hl, complete_fun, vpf_fun, 
                client_id, arglist);
        if (count > 0 && !(dev->connect_status & DEV_CONNECTED))
            dev->retry_count = 0;   /* expedite retries on this device */
        total += count;
    }
    list_iterator_destroy(itr);

    return total;
}

static int _enqueue_actions(Device * dev, int com, hostlist_t hl,
                            ActionCB complete_fun, VerbosePrintf vpf_fun,
                            int client_id, ArgList * arglist)
{
    Action *act;
    int count = 0;

    switch (com) {
    case PM_LOG_IN:
        /* reset script of preempted action so it starts over */
        if (!list_is_empty(dev->acts)) {
            act = list_peek(dev->acts);
            list_iterator_reset(act->itr);
            dbg(DBG_ACTION, "resetting iterator for non-login action");
        }
        act = _create_action(dev, com, NULL, complete_fun, vpf_fun,
                client_id, arglist);
        list_prepend(dev->acts, act);
        count++;
        break;
    case PM_POWER_ON:
    case PM_POWER_OFF:
    case PM_BEACON_ON:
    case PM_BEACON_OFF:
    case PM_POWER_CYCLE:
    case PM_RESET:
        if (hl) {
            count += _enqueue_targetted_actions(dev, com, hl, complete_fun,
                                                vpf_fun, client_id, arglist);
            break;
        }
    /*FALLTHROUGH*/ case PM_STATUS_PLUGS:
    case PM_STATUS_NODES:
    case PM_STATUS_TEMP:
    case PM_STATUS_BEACON:
    case PM_LOG_OUT:
    case PM_PING:
        act = _create_action(dev, com, NULL, complete_fun, vpf_fun, client_id, 
                arglist);
        list_append(dev->acts, act);
        count++;
        break;
    default:
        assert(FALSE);
    }

    return count;
}

/* return "all" version of script if defined else -1 */
static int _get_all_script(Device * dev, int com)
{
    int new = -1;

    switch (com) {
    case PM_POWER_ON:
        if (dev->scripts[PM_POWER_ON_ALL])
            new = PM_POWER_ON_ALL;
        break;
    case PM_POWER_OFF:
        if (dev->scripts[PM_POWER_OFF_ALL])
            new = PM_POWER_OFF_ALL;
        break;
    case PM_POWER_CYCLE:
        if (dev->scripts[PM_POWER_CYCLE_ALL])
            new = PM_POWER_CYCLE_ALL;
        break;
    case PM_RESET:
        if (dev->scripts[PM_RESET_ALL])
            new = PM_RESET_ALL;
        break;
    default:
        break;
    }
    return new;
}


static int _enqueue_targetted_actions(Device * dev, int com, hostlist_t hl,
                                      ActionCB complete_fun, 
                                      VerbosePrintf vpf_fun, 
                                      int client_id, ArgList * arglist)
{
    List new_acts = list_create((ListDelF) _destroy_action);
    bool all = TRUE;
    Plug *plug;
    ListIterator itr;
    int count = 0;
    Action *act;

    itr = list_iterator_create(dev->plugs);
    while ((plug = list_next(itr))) {
        /* antisocial to gratuitously turn on/off unused plug */
        if (plug->node == NULL) {
            all = FALSE;
            continue;
        }
        /* check if node name for plug matches the target */
        if (hostlist_find(hl, plug->node) == -1) {
            all = FALSE;
            continue;
        }
        /* match! */
        act = _create_action(dev, com, plug->name, complete_fun, vpf_fun,
                    client_id, arglist);
        list_append(new_acts, act);
    }

    if (all) {
        int ncom;
        /* _ALL script available - use that */
        if ((ncom = _get_all_script(dev, com)) != -1) {
            act = _create_action(dev, ncom, NULL, complete_fun, vpf_fun,
                    client_id, arglist);
            list_append(dev->acts, act);
            count++;
        } else if (dev->all != NULL) {  /* normal script, "*" plug */
            act =
                _create_action(dev, com, dev->all, complete_fun, vpf_fun,
                        client_id, arglist);
            list_append(dev->acts, act);
            count++;
        }
    }

    /* "all" wasn't appropriate or wasn't defined so do one action per plug */
    if (count == 0) {
        while ((act = list_pop(new_acts))) {
            list_append(dev->acts, act);
            count++;
        }
    }

    list_iterator_destroy(itr);
    list_destroy(new_acts);

    return count;
}


/*
 *   We've supposedly reconnected, so check if we really are.
 * If not go into reconnect mode.  If this is a succeeding
 * reconnect, log that fact.  When all is well initiate the
 * logon script.
 */
static bool _finish_connect(Device * dev, struct timeval *timeout)
{
    int rc;
    int error = 0;
    int len = sizeof(err);

    assert(dev->connect_status == DEV_CONNECTING);
    rc = getsockopt(dev->fd, SOL_SOCKET, SO_ERROR, &error, &len);
    /*
     *  If an error occurred, Berkeley-derived implementations
     *    return 0 with the pending error in 'err'.  But Solaris
     *    returns -1 with the pending error in 'errno'.  -dun
     */
    if (rc < 0)
        error = errno;
    if (error) {
        err(FALSE, "_finish_connect %s: %s", dev->name, strerror(error));
        _disconnect(dev);
        if (_time_to_reconnect(dev, timeout))
            _reconnect(dev);
    } else {
        err(FALSE, "_finish_connect: %s connected", dev->name);
        dev->connect_status = DEV_CONNECTED;
        dev->stat_successful_connects++;
        dev->retry_count = 0;
        _enqueue_actions(dev, PM_LOG_IN, NULL, NULL, NULL, 0, NULL);
    }

    return (dev->connect_status == DEV_CONNECTED);
}

/*
 * Select says device is ready for reading.
 */
static void _handle_read(Device * dev)
{
    int n;
    int dropped;

    assert(dev->magic == DEV_MAGIC);
    n = cbuf_write_from_fd(dev->from, dev->fd, -1, &dropped);
    if (n < 0) {
        err(TRUE, "read error on %s", dev->name);
        _disconnect(dev);
        _reconnect(dev);
        return;
    }
    if (dropped > 0) {
        err(FALSE, "buffer space for %s exhausted", dev->name);
        _disconnect(dev);
        _reconnect(dev);
        return;
    }
    if (n == 0) {
        err(FALSE, "read returned EOF on %s", dev->name);
        _disconnect(dev);
        _reconnect(dev);
        return;
    }
}

/*
 * Select says device is ready for writing.
 */
static void _handle_write(Device * dev)
{
    int n;

    assert(dev->magic == DEV_MAGIC);
    n = cbuf_read_to_fd(dev->to, dev->fd, -1);
    if (n < 0) {
        err(TRUE, "write error on %s", dev->name);
        _disconnect(dev);
        _reconnect(dev);
    }
    if (n == 0) {
        err(FALSE, "write sent no data on %s", dev->name);
        _disconnect(dev);
        _reconnect(dev);
        /* XXX: is this even possible? */
    }
}

static void _disconnect(Device * dev)
{
    Action *act;

    assert(dev->connect_status == DEV_CONNECTING
           || dev->connect_status == DEV_CONNECTED);

    dbg(DBG_DEVICE, "_disconnect: %s on fd %d", dev->name, dev->fd);

    /* close socket if open */
    if (dev->fd >= 0) {
        Close(dev->fd);
        dev->fd = NO_FD;
    }

    /* flush buffers */
    cbuf_flush(dev->from);
    cbuf_flush(dev->to);

    /* update state */
    dev->connect_status = DEV_NOT_CONNECTED;
    dev->script_status = 0;

    /* delete PM_LOG_IN action queued for this device, if any */
    if (((act = list_peek(dev->acts)) != NULL) && act->com == PM_LOG_IN)
        _destroy_action(list_dequeue(dev->acts));
}

static void _act_completion(Action *act, Device *dev)
{
    assert(act->complete_fun != NULL);

    switch (act->errnum) {
    case ACT_ECONNECTTIMEOUT:
        act->complete_fun(act->client_id, act->errnum, 
                "%s: connect timeout", dev->name);
        break;
    case ACT_ELOGINTIMEOUT:
        act->complete_fun(act->client_id, act->errnum, 
                "%s: login timeout", dev->name);
        break;
    case ACT_EEXPFAIL:
        act->complete_fun(act->client_id, act->errnum, 
                "%s: action timed out waiting for expected response", dev->name);
        break;
    case ACT_EABORT:
        act->complete_fun(act->client_id, act->errnum, 
                "%s: action aborted due to previous action timeout", dev->name);
        break;
    case ACT_ESUCCESS:
        act->complete_fun(act->client_id, act->errnum, NULL);
        break;
    }
}

/*
 * Process the script for the current action for this device.
 * Update timeout and return if one of the script elements stalls.
 * Start the next action if we complete this one.
 */
static void _process_action(Device * dev, struct timeval *timeout)
{
    bool stalled = FALSE;

    while (!stalled) {
        Action *act = list_peek(dev->acts);
        struct timeval timeleft;

        if (act == NULL)        /* no actions in queue */
            break;
        dbg(DBG_ACTION, "_process_action: processing action %d", act->com);
        _dbg_actions(dev);

        /* Start the action if it is a new one.
         * act->cur points to the current script statement.
         * act->time_stamp will cause action timeout if elapsed time for
         * entire action exceeds dev->timeout.
         */
        assert(act->itr != NULL);
        if (!_act_started(act)) {
            act->cur = list_next(act->itr);     /* point to first script elem */
            Gettimeofday(&act->time_stamp, NULL);
        }
        assert(act->cur != NULL);

        /* process actions 
         */
        if (_timeout(&act->time_stamp, &dev->timeout, &timeleft)) {
            if (!(dev->connect_status == DEV_CONNECTED)) 
                act->errnum = ACT_ECONNECTTIMEOUT;
            else if (!(dev->script_status & DEV_LOGGED_IN))
                act->errnum = ACT_ELOGINTIMEOUT;
            else
                act->errnum = ACT_EEXPFAIL;

            if (act->vpf_fun) {
                unsigned char mem[MAX_DEV_BUF];
                int len = cbuf_peek(dev->from, mem, MAX_DEV_BUF);
                char *memstr = dbg_memstr(mem, len);
    
                if (!(dev->connect_status == DEV_CONNECTED)) 
                    act->vpf_fun(act->client_id, "connect(%s): timeout",
                            dev->name);
                else
                    act->vpf_fun(act->client_id, "recv(%s): '%s'",
                            dev->name, memstr);
                Free(memstr);
            }
        } else if (!(dev->connect_status == DEV_CONNECTED)) {
            stalled = TRUE;                             /* not connnected */
        } else if (act->cur->type == STMT_EXPECT) {
            stalled = !_process_expect(dev);            /* do expect */
        } else if (act->cur->type == STMT_SEND) {
            stalled = !_process_send(dev);              /* do send */
        } else {
            assert(act->cur->type == STMT_DELAY);
            stalled = !_process_delay(dev, timeout);    /* do delay */
        }

        if (stalled) {          /* if stalled, set timeout for select */
            _update_timeout(timeout, &timeleft);
        } else {
            ActError res = act->errnum;

            assert(act->itr != NULL);
            /* completed action - notify client and dequeue action */
            if (res != ACT_ESUCCESS || !(act->cur = list_next(act->itr))) {
                if (act->com == PM_LOG_IN) {
                    if (res == ACT_ESUCCESS)
                        dev->script_status |= DEV_LOGGED_IN;
                }
                if (act->complete_fun)                  /* notify client */
                    _act_completion(act, dev);
                if (res == ACT_ESUCCESS)
                    dev->stat_successful_actions++;
                _destroy_action(list_dequeue(dev->acts));
            }

            /* if one action failed, abort the rest in the device queue 
             * in preparation for reconnect.
             */
            if (res != ACT_ESUCCESS) {
                Action *act;

                while ((act = list_dequeue(dev->acts)) != NULL) { 
                    act->errnum = (res == ACT_EEXPFAIL ? ACT_EABORT : res);
                    if (act->complete_fun)
                        _act_completion(act, dev);
                    _destroy_action(act);
                }
            }

            /* reconnect/login if expect timed out */
            if (res != ACT_ESUCCESS && (dev->connect_status & DEV_CONNECTED)) {
                dbg(DBG_DEVICE,
                    "_process_action: disconnecting due to error");
                _disconnect(dev);
                _reconnect(dev);
                break;
            }
        }
    }
}

/* return TRUE if expect is finished */
static bool _process_expect(Device * dev)
{
    regex_t *re;
    Action *act = list_peek(dev->acts);
    char *expect;
    bool finished = FALSE;

    assert(act != NULL);
    assert(act->cur != NULL);
    assert(act->cur->type == STMT_EXPECT);

    re = &act->cur->u.expect.exp;

    if ((expect = _getregex_buf(dev->from, re))) {      /* match */
        char *memstr = dbg_memstr(expect, strlen(expect));

        if (act->vpf_fun)
            act->vpf_fun(act->client_id, "recv(%s): '%s'", dev->name, memstr);
        Free(memstr);

        /* process values of parenthesized subexpressions, if any */
        _match_subexpressions(dev, act, expect);
        Free(expect);
        finished = TRUE;
    } 
    return finished;
}

/*
 * Allocate a string and sprintf to it, resizing until sprintf succedes.
 * Result must be destroyed with Free().
 */
#define CHUNKSIZE 80
static char *_malloc_sprintf(const char *fmt, ...)
{
    va_list ap;
    char *str = NULL;
    int len, size = 0;

    do {
        str = (size == 0) ? Malloc(CHUNKSIZE) : Realloc(str, size + CHUNKSIZE);
        size += CHUNKSIZE;

        va_start(ap, fmt);
        len = vsnprintf(str, size, fmt, ap);
        va_end(ap);
    } while (len == -1 || len >= size);

    return str;
}

static bool _process_send(Device * dev)
{
    Action *act = list_peek(dev->acts);
    bool finished = FALSE;

    assert(act != NULL);
    assert(act->magic == ACT_MAGIC);
    assert(act->cur != NULL);
    assert(act->cur->type == STMT_SEND);

    /* first time through? */
    if (!(dev->script_status & DEV_SENDING)) {
        int dropped = 0;
        char *str = _malloc_sprintf(act->cur->u.send.fmt, act->target);
        int written = cbuf_write(dev->to, str, strlen(str), &dropped);

        if (written < 0)
            err(TRUE, "_process_send(%s): cbuf_write returned %d", 
                    dev->name, written);
        else if (dropped > 0)
            err(FALSE, "_process_send(%s): buffer overrun, %d dropped", 
                    dev->name, dropped);
        else {
            char *memstr = dbg_memstr(str, strlen(str));

            if (act->vpf_fun)
                act->vpf_fun(act->client_id, "send(%s): '%s'", 
                        dev->name, memstr);
            Free(memstr);
        }
        assert(written < 0 || (dropped == strlen(str) - written));
        dev->script_status |= DEV_SENDING;
      
        Free(str);
    }

    if (cbuf_is_empty(dev->to)) {           /* finished! */
        dev->script_status &= ~DEV_SENDING;
        finished = TRUE;
    } 

    return finished;
}

/* return TRUE if delay is finished */
static bool _process_delay(Device * dev, struct timeval *timeout)
{
    bool finished = FALSE;
    struct timeval delay, timeleft;
    Action *act = list_peek(dev->acts);

    assert(act != NULL);
    assert(act->cur != NULL);
    assert(act->cur->type == STMT_DELAY);

    delay = act->cur->u.delay.tv;

    /* first time */
    if (!(dev->script_status & DEV_DELAYING)) {
        if (act->vpf_fun)
            act->vpf_fun(act->client_id, "delay(%s): %ld.%-6.6ld", dev->name, 
                    delay.tv_sec, delay.tv_usec);
        dev->script_status |= DEV_DELAYING;
        Gettimeofday(&act->delay_start, NULL);
    }

    /* timeout expired? */
    if (_timeout(&act->delay_start, &delay, &timeleft)) {
        dev->script_status &= ~DEV_DELAYING;
        finished = TRUE;
    } else
        _update_timeout(timeout, &timeleft);

    return finished;
}

static char *_copy_pmatch(char *str, regmatch_t m)
{
    char *new;

    assert(m.rm_so < MAX_DEV_BUF && m.rm_so >= 0);
    assert(m.rm_eo < MAX_DEV_BUF && m.rm_eo >= 0);
    assert(m.rm_eo - m.rm_so > 0);

    new = Malloc(m.rm_eo - m.rm_so + 1);
    memcpy(new, str + m.rm_so, m.rm_eo - m.rm_so);
    new[m.rm_eo - m.rm_so] = '\0';
    /* XXX andrew zapped trailing spaces in old code - needed here? */
    return new;
}

static void _match_subexpressions(Device * dev, Action * act, char *expect)
{
    Interp *interp;
    ListIterator itr;
    size_t nmatch = MAX_MATCH_POS + 1;
    regmatch_t pmatch[MAX_MATCH_POS + 1];
    int eflags = 0;
    int n;

    if (act->com != PM_STATUS_PLUGS
        && act->com != PM_STATUS_NODES
        && act->com != PM_STATUS_TEMP && act->com != PM_STATUS_BEACON)
        return;
    if (act->cur->u.expect.map == NULL)
        return;

    /* match regex against expect - must succede, we checked before */
    n = Regexec(&act->cur->u.expect.exp, expect, nmatch, pmatch,
                eflags);
    assert(n == REG_NOERROR);
    assert(pmatch[0].rm_so != -1 && pmatch[0].rm_eo != -1);
    assert(pmatch[0].rm_so <= strlen(expect));

    itr = list_iterator_create(act->cur->u.expect.map);
    while ((interp = list_next(itr))) {
        char *str;

        if (interp->node == NULL)       /* unused plug? */
            continue;
        assert(interp->match_pos >= 0 && interp->match_pos <= MAX_MATCH_POS);

        str = _copy_pmatch(expect, pmatch[interp->match_pos]);

        if (_findregex(&dev->on_re, str, strlen(str)))
            _set_argval_state(act->arglist, interp->node, ST_ON);
        if (_findregex(&dev->off_re, str, strlen(str)))
            _set_argval_state(act->arglist, interp->node, ST_OFF);
        _set_argval(act->arglist, interp->node, str);

        Free(str);
    }
    list_iterator_destroy(itr);
}

Device *dev_create(const char *name)
{
    Device *dev;
    int i;

    dev = (Device *) Malloc(sizeof(Device));
    dev->magic = DEV_MAGIC;
    dev->name = Strdup(name);
    dev->type = NO_DEV;
    dev->connect_status = DEV_NOT_CONNECTED;
    dev->script_status = 0;
    dev->fd = NO_FD;
    dev->acts = list_create((ListDelF) _destroy_action);
    timerclear(&dev->timeout);
    timerclear(&dev->last_retry);
    timerclear(&dev->last_ping);
    timerclear(&dev->ping_period);
    dev->to = cbuf_create(MIN_DEV_BUF, MAX_DEV_BUF);
    cbuf_opt_set(dev->to, CBUF_OPT_OVERWRITE, CBUF_NO_DROP);
    dev->from = cbuf_create(MIN_DEV_BUF, MAX_DEV_BUF);
    cbuf_opt_set(dev->to, CBUF_OPT_OVERWRITE, CBUF_NO_DROP);
    for (i = 0; i < NUM_SCRIPTS; i++)
        dev->scripts[i] = NULL;
    dev->plugs = list_create((ListDelF) dev_plug_destroy);
    dev->retry_count = 0;
    dev->stat_successful_connects = 0;
    dev->stat_successful_actions = 0;
    return dev;
}

/* helper for dev_findbyname */
static int _match_name(Device * dev, void *key)
{
    return (strcmp(dev->name, (char *) key) == 0);
}

Device *dev_findbyname(char *name)
{
    return list_find_first(dev_devices, (ListFindF) _match_name, name);
}

void dev_destroy(Device * dev)
{
    int i;

    assert(dev->magic == DEV_MAGIC);
    dev->magic = 0;

    Free(dev->name);
    Free(dev->specname);
    if (dev->all)
        Free(dev->all);
    regfree(&(dev->on_re));
    regfree(&(dev->off_re));
    if (dev->type == TCP_DEV) {
        Free(dev->devu.tcpd.host);
        Free(dev->devu.tcpd.service);
    }
    list_destroy(dev->acts);
    list_destroy(dev->plugs);

    for (i = 0; i < NUM_SCRIPTS; i++)
        if (dev->scripts[i] != NULL)
            list_destroy(dev->scripts[i]);

    cbuf_destroy(dev->to);
    cbuf_destroy(dev->from);
    Free(dev);
}

Plug *dev_plug_create(const char *name)
{
    Plug *plug;

    assert(name != NULL);
    plug = (Plug *) Malloc(sizeof(Plug));
    plug->name = Strdup(name);
    plug->node = NULL;
    return plug;
}

/* used with list_find_first() in parser */
int dev_plug_match(Plug * plug, void *key)
{
    return (strcmp(((Plug *) plug)->name, (char *) key) == 0);
}

/* helper for dev_create */
void dev_plug_destroy(Plug * plug)
{
    Free(plug->name);
    Free(plug->node);
    Free(plug);
}

static void _destroy_arg(Arg * arg)
{
    assert(arg != NULL);
    assert(arg->node != NULL);
    Free(arg->node);
    if (arg->val)
        Free(arg->val);
    Free(arg);
}

/*
 * Create arglist.
 */
ArgList *dev_create_arglist(hostlist_t hl)
{
    ArgList *new = (ArgList *) Malloc(sizeof(ArgList));
    hostlist_iterator_t itr;
    Arg *arg;
    char *node;

    new->refcount = 1;
    new->argv = list_create((ListDelF) _destroy_arg);

    if ((itr = hostlist_iterator_create(hl)) == NULL) {
        dev_unlink_arglist(new);
        return NULL;
    }
    while ((node = hostlist_next(itr)) != NULL) {
        arg = (Arg *) Malloc(sizeof(Arg));
        arg->node = Strdup(node);
        arg->state = ST_UNKNOWN;
        arg->val = NULL;
        list_append(new->argv, arg);
    }
    hostlist_iterator_destroy(itr);

    return new;
}

/*
 * Decrement the refcount for arglist and free when zero.
 */
void dev_unlink_arglist(ArgList * arglist)
{
    if (--arglist->refcount == 0) {
        list_destroy(arglist->argv);
        Free(arglist);
    }
}

/*
 * Increment the ref count for arglist.
 */
ArgList *dev_link_arglist(ArgList * arglist)
{
    arglist->refcount++;
    return arglist;
}

/* helper for _set_argval_onoff */
static int _arg_match(Arg * arg, void *key)
{
    /* redundant PS support: arg->node may not be unique so only match 
     * "virgin" entries.   Order doesn't particularly matter.
     */
    return (strcmp(arg->node, (char *) key) == 0 
            && (arg->state == ST_UNKNOWN || arg->val == NULL));
}

/*
 * Set the value of the argument with key = node.
 */
static void _set_argval_state(ArgList * arglist, char *node, ArgState state)
{
    Arg *arg;

    if ((arg = list_find_first(arglist->argv, (ListFindF) _arg_match, node)))
        arg->state = state;
}

/*
 * Set the value of the argument with key = node.
 */
static void _set_argval(ArgList * arglist, char *node, char *val)
{
    Arg *arg;

    if ((arg = list_find_first(arglist->argv, (ListFindF) _arg_match, node)))
        arg->val = Strdup(val);
}

static void _process_ping(Device * dev, struct timeval *timeout)
{
    struct timeval timeleft;

    if (dev->scripts[PM_PING] != NULL
        && timerisset(&dev->ping_period)) {
        if (_timeout(&dev->last_ping, &dev->ping_period, &timeleft)) {
            _enqueue_actions(dev, PM_PING, NULL, NULL, NULL, 0, NULL);
            Gettimeofday(&dev->last_ping, NULL);
            dbg(DBG_ACTION, "%s: enqeuuing ping", dev->name);
        } else
            _update_timeout(timeout, &timeleft);
    }
}


/*
 * Called prior to the select loop to initiate connects to all devices.
 */
void dev_initial_connect(void)
{
    Device *dev;
    ListIterator itr;

    itr = list_iterator_create(dev_devices);
    while ((dev = list_next(itr))) {
        assert(dev->connect_status == DEV_NOT_CONNECTED);
        _reconnect(dev);
    }
    list_iterator_destroy(itr);
}

/*
 * Called before select to ready fd_sets and maxfd.
 */
void dev_pre_select(fd_set * rset, fd_set * wset, int *maxfd)
{
    Device *dev;
    ListIterator itr;
    fd_set dev_fdset;
    char fdsetstr[1024];

    FD_ZERO(&dev_fdset);

    itr = list_iterator_create(dev_devices);
    while ((dev = list_next(itr))) {
        if (dev->fd < 0)
            continue;

        FD_SET(dev->fd, &dev_fdset);

        /* always set read set bits so select will unblock if the 
         * connection is dropped.
         */
        FD_SET(dev->fd, rset);
        *maxfd = MAX(*maxfd, dev->fd);

        /* need to be in the write set if we are sending scripted text */
        if (dev->connect_status == DEV_CONNECTED) {
            if ((dev->script_status & DEV_SENDING)) {
                FD_SET(dev->fd, wset);
                *maxfd = MAX(*maxfd, dev->fd);
            }
        }

        /* descriptor will become writable after a connect */
        if (dev->connect_status == DEV_CONNECTING) {
            FD_SET(dev->fd, wset);
            *maxfd = MAX(*maxfd, dev->fd);
        }

    }
    list_iterator_destroy(itr);

    dbg(DBG_DEVICE, "fds are [%s]", dbg_fdsetstr(&dev_fdset, *maxfd + 1,
                                                 fdsetstr,
                                                 sizeof(fdsetstr)));
}

/* 
 * Called after select to process ready file descriptors, timeouts, etc.
 */
void dev_post_select(fd_set * rset, fd_set * wset, struct timeval *timeout)
{
    Device *dev;
    ListIterator itr;

    itr = list_iterator_create(dev_devices);
    while ((dev = list_next(itr))) {

        /* reconnect if necessary */
        if (dev->connect_status == DEV_NOT_CONNECTED) {
            if (_time_to_reconnect(dev, timeout))
                _reconnect(dev);
            /* complete non-blocking connect if ready */
        } else if ((dev->connect_status == DEV_CONNECTING)) {
            assert(dev->fd != NO_FD);
            if (FD_ISSET(dev->fd, wset)) {
                _finish_connect(dev, timeout);
                if (dev->fd != NO_FD)
                    FD_CLR(dev->fd, wset);      /* avoid _handle_write error */
            }
        }

        /* if this device needs a ping, put it in the qeuue */
        if ((dev->connect_status & DEV_CONNECTED)) {
            _process_ping(dev, timeout);
        }

        /* read/write from/to buffer */
        if (dev->fd != NO_FD && (dev->connect_status & DEV_CONNECTED)) {
            if (FD_ISSET(dev->fd, rset))
                _handle_read(dev);      /* also handles ECONNRESET */
            if (FD_ISSET(dev->fd, wset))
                _handle_write(dev);
        }

        /* process actions */
        if (list_peek(dev->acts)) {
            _process_action(dev, timeout);
        }
    }
    list_iterator_destroy(itr);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
