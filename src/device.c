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
#include "hprintf.h"

/* ExecCtx's are the state for the execution of a block of statements.
 * They are stacked on the Action (new ExecCtx pushed when executing an
 * inner block).
 */
typedef struct {
    Plug *target;               /* target->name used for send "%s" (NULL=all) */
    List block;                 /* List of stmts */
    ListIterator itr;           /* next stmt in block */
    Stmt *cur;                  /* current stmt */
    ListIterator plugitr;       /* used by foreach */
    bool processing;            /* flag used by stmts, ifon/ifoff */
} ExecCtx;

/* Actions are queued on a device and executed one at a time.  Each action
 * represents a request to run a particular script on a device, for a set of
 * plugs.  Actions can be enqueued by the client or internally (e.g. login).
 */
#define ACT_MAGIC 0xb00bb000
#define MAX_LEVELS 2
typedef struct {
    int magic;
    int com;                    /* one of the PM_* above */
    List exec;                  /* stack of ExecCtxs (outer block is first) */
    ActionCB complete_fun;      /* callback for action completion */
    VerbosePrintf vpf_fun;      /* callback for device telemetry */
    int client_id;              /* client id so completion can find client */
    ActError errnum;            /* errno for action */
    struct timeval time_stamp;  /* time stamp for timeouts */
    struct timeval delay_start; /* time stamp for delay completion */
    ArgList *arglist;           /* argument for query actions (list of Arg's) */
} Action;


static bool _process_stmt(Device *dev, Action *act, ExecCtx *e, 
        struct timeval *timeout);
static bool _process_ifonoff(Device *dev, Action *act, ExecCtx *e);
static bool _process_foreach(Device *dev, Action *act, ExecCtx *e);
static bool _process_setplugstate(Device * dev, Action *act, ExecCtx *e);
static bool _process_expect(Device * dev, Action *act, ExecCtx *e);
static bool _process_send(Device * dev, Action *act, ExecCtx *e);
static bool _process_delay(Device * dev, Action *act, ExecCtx *e, 
        struct timeval *timeout);
static void _set_argval(ArgList * arglist, char *node, char *val, List interps);
static InterpState _get_argval(ArgList *arglist, char *node);
static int _match_name(Device * dev, void *key);
static void _handle_read(Device * dev, struct timeval *timeout);
static void _handle_write(Device * dev, struct timeval *timeout);
static void _process_action(Device * dev, struct timeval *timeout);
static bool _timeout(struct timeval *timestamp, struct timeval *timeout,
                     struct timeval *timeleft);
static int _get_all_script(Device * dev, int com);
static int _enqueue_actions(Device * dev, int com, hostlist_t hl,
                            ActionCB complete_fun, VerbosePrintf vpf_fun,
                            int client_id, ArgList * arglist);
static Action *_create_action(Device * dev, int com, Plug *target,
                              ActionCB complete_fun, VerbosePrintf vpf_fun,
                              int client_id, ArgList * arglist);
static int _enqueue_targetted_actions(Device * dev, int com, hostlist_t hl,
                                      ActionCB complete_fun, 
                                      VerbosePrintf vpf_fun,
                                      int client_id, ArgList * arglist);
static unsigned char *_findregex(regex_t * re, unsigned char *str,
                                 int len, size_t nm, regmatch_t pm[]);
static char *_getregex_buf(cbuf_t b, regex_t * re, size_t nm, regmatch_t pm[]);
static bool _command_needs_device(Device * dev, hostlist_t hl);
static void _process_ping(Device * dev, struct timeval *timeout);
static void _login(Device *dev);
static void _disconnect(Device * dev);
static bool _connect(Device * dev);
static bool _reconnect(Device * dev, struct timeval *timeout);
static bool _time_to_reconnect(Device * dev, struct timeval *timeout);

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
 *  nm (IN)   number of elem in pm array (can be 0)
 *  pm (OUT)  filled with subexpression matches (can be NULL)
 *  RETURN    pointer to char following the last char of the match,
 *            or if pm = NULL, the first char of str;  NULL if no match
 */
static unsigned char *_findregex(regex_t * re, unsigned char *str, int len,
        size_t nm, regmatch_t pm[])
{
    int n;
    int eflags = 0;
    char *res = NULL;

    n = Regexec(re, str, nm, pm, eflags);
    if (n != REG_NOMATCH) {
        if (nm > 0) {
            assert(pm[0].rm_eo <= len);
            res = str + pm[0].rm_eo;
        } else
            res = str;
    }
    
    return res;
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
 *  nm (IN)   number of elem in pm array (can be 0)
 *  pm (OUT)  filled with subexpression matches (can be NULL)
 *  RETURN  String match (caller must free) or NULL if no match
 */
static char *_getregex_buf(cbuf_t b, regex_t * re, size_t nm, regmatch_t pm[])
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
    match_end = _findregex(re, str, bytes_peeked, nm, pm);
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

static ExecCtx *_create_exec_ctx(Device *dev, List block, Plug *target)
{
    ExecCtx *new = (ExecCtx *)Malloc(sizeof(ExecCtx));

    new->itr = list_iterator_create(block);
    new->cur = list_next(new->itr); 
    new->block = block;
    new->target = target;
    new->plugitr = NULL;
    new->processing = FALSE;

    return new;
}

static void _destroy_exec_ctx(ExecCtx *e)
{
    if (e->itr != NULL)
        list_iterator_destroy(e->itr);
    e->itr = NULL;
    e->cur = NULL;
    e->target = NULL;
    Free(e);
}

static void _rewind_action(Action *act)
{
    ExecCtx *e;

    /* get back to the context for the outer block */
    while ((e = list_pop(act->exec))) {
        if (list_is_empty(act->exec)) {
            list_push(act->exec, e);
            break;
        }
        _destroy_exec_ctx(e);
    }
    /* reset outer block iterator and current pointer */
    if (e) {
        list_iterator_reset(e->itr);
        e->cur = list_next(e->itr); 
    }
}

static Action *_create_action(Device * dev, int com, Plug *target,
                              ActionCB complete_fun, VerbosePrintf vpf_fun,
                              int client_id, ArgList * arglist)
{
    Action *act;
    ExecCtx *e;

    dbg(DBG_ACTION, "_create_action: %d", com);
    act = (Action *) Malloc(sizeof(Action));
    act->magic = ACT_MAGIC;
    act->com = com;
    act->complete_fun = complete_fun;
    act->vpf_fun = vpf_fun;
    act->client_id = client_id;

    act->exec = list_create((ListDelF)_destroy_exec_ctx);
    e = _create_exec_ctx(dev, dev->scripts[act->com], target);
    list_push(act->exec, e);

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
    if (act->exec)
        list_destroy(act->exec);
    act->exec = NULL;
    if (act->arglist)
        dev_unlink_arglist(act->arglist);
    act->arglist = NULL;
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
 * Helper for _reconnect().
 * Return TRUE if OK to attempt reconnect.  If FALSE, put the time left 
 * in timeout if it is less than timeout or if timeout is zero.
 */
static bool _time_to_reconnect(Device * dev, struct timeval *timeout)
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

static bool _connect(Device * dev)
{
    bool connected;

    assert(dev->connect != NULL);

    Gettimeofday(&dev->last_retry, NULL);
    dev->retry_count++;

    connected = dev->connect(dev);

    if (connected)
        _login(dev);

    return connected;
}

static bool _reconnect(Device *dev, struct timeval *timeout)
{
    bool connected = FALSE;

    if (dev->connect_state != DEV_NOT_CONNECTED)
        _disconnect(dev);

    if (_time_to_reconnect(dev, timeout))
        connected = _connect(dev);

    return connected;
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

    assert(hl != NULL);

    itr = list_iterator_create(dev_devices);
    while ((dev = list_next(itr))) {
        if (_command_needs_device(dev, hl)) {
            if (!dev->scripts[com] && _get_all_script(dev, com) == -1)  {
                valid = FALSE;
                break;
            }
        }
    }
    list_iterator_destroy(itr);
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

        if (!dev->scripts[com] && _get_all_script(dev, com) == -1)
            continue;                               /* unimplemented script */
        if (hl && !_command_needs_device(dev, hl))
            continue;                               /* uninvolved device */
        count = _enqueue_actions(dev, com, hl, complete_fun, vpf_fun, 
                client_id, arglist);
        if (count > 0 && dev->connect_state != DEV_CONNECTED)
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
            _rewind_action(act);
            dbg(DBG_ACTION, "resetting iterator for non-login action");
        }
        act = _create_action(dev, com, NULL, complete_fun, vpf_fun,
                client_id, arglist);
        list_prepend(dev->acts, act);
        count++;
        break;
    case PM_LOG_OUT:
    case PM_PING:
        act = _create_action(dev, com, NULL, complete_fun, vpf_fun, client_id, 
                arglist);
        list_append(dev->acts, act);
        count++;
        break;

    case PM_POWER_ON:
    case PM_POWER_OFF:
    case PM_BEACON_ON:
    case PM_BEACON_OFF:
    case PM_POWER_CYCLE:
    case PM_RESET:
    case PM_STATUS_PLUGS:
    case PM_STATUS_NODES:
    case PM_STATUS_TEMP:
    case PM_STATUS_BEACON:
        count += _enqueue_targetted_actions(dev, com, hl, complete_fun,
                                                vpf_fun, client_id, arglist);
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
    case PM_STATUS_PLUGS:
        if (dev->scripts[PM_STATUS_PLUGS_ALL])
            new = PM_STATUS_PLUGS_ALL;
        break;
    case PM_STATUS_NODES:
        if (dev->scripts[PM_STATUS_NODES_ALL])
            new = PM_STATUS_NODES_ALL;
        break;
    case PM_STATUS_TEMP:
        if (dev->scripts[PM_STATUS_TEMP_ALL])
            new = PM_STATUS_TEMP_ALL;
        break;
    case PM_STATUS_BEACON:
        if (dev->scripts[PM_STATUS_BEACON_ALL])
            new = PM_STATUS_BEACON_ALL;
        break;
    default:
        break;
    }
    return new;
}

static bool _is_query_action(int com)
{
    switch (com) {
        case PM_STATUS_PLUGS:
        case PM_STATUS_PLUGS_ALL:
        case PM_STATUS_NODES:
        case PM_STATUS_NODES_ALL:
        case PM_STATUS_TEMP:
        case PM_STATUS_TEMP_ALL:
        case PM_STATUS_BEACON:
        case PM_STATUS_BEACON_ALL:
            return TRUE;
        default:
            return FALSE;
    }
    /*NOTREACHED*/
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

    assert(hl != NULL);

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
        /* append action to 'new_acts' */
        if (dev->scripts[com] != NULL) { /* maybe we only have _ALL... */
            act = _create_action(dev, com, plug, complete_fun, vpf_fun,
                        client_id, arglist);
            list_append(new_acts, act);
        }
    }
    list_iterator_destroy(itr);

    /* See if we can use a command targetted at all plugs 
     * (discard 'new_acts' later on if so)
     */
    if (all || _is_query_action(com)) {
        int ncom;
        /* _ALL script available - use that */
        if ((ncom = _get_all_script(dev, com)) != -1) {
            act = _create_action(dev, ncom, NULL, complete_fun, vpf_fun,
                    client_id, arglist);
            list_append(dev->acts, act);
            count++;
        }
    }

    /* _ALL wasn't appropriate or wasn't defined so do one action per plug */
    if (count == 0) {
        while ((act = list_pop(new_acts))) {
            list_append(dev->acts, act);
            count++;
        }
    }

    list_destroy(new_acts);

    return count;
}

static void _login(Device *dev)
{
    _enqueue_actions(dev, PM_LOG_IN, NULL, NULL, NULL, 0, NULL);
}

/*
 * Select says device is ready for reading.
 */
static void _handle_read(Device * dev, struct timeval *timeout)
{
    int n;
    int dropped;

    assert(dev->magic == DEV_MAGIC);
    n = cbuf_write_from_fd(dev->from, dev->fd, -1, &dropped);
    if (n < 0) {
        err(TRUE, "read error on %s", dev->name);
        goto err;
    }
    if (dropped > 0) {
        err(FALSE, "buffer space for %s exhausted", dev->name);
        goto err;
    }
    if (n == 0) {
        err(FALSE, "read returned EOF on %s", dev->name);
        goto err;
    }
    return;
err:
    _reconnect(dev, timeout);
}

/*
 * Select says device is ready for writing.
 */
static void _handle_write(Device * dev, struct timeval *timeout)
{
    int n;

    assert(dev->magic == DEV_MAGIC);
    n = cbuf_read_to_fd(dev->to, dev->fd, -1);
    if (n < 0) {
        err(TRUE, "write error on %s", dev->name);
        goto err;
    }
    if (n == 0) {
        err(FALSE, "write sent no data on %s", dev->name);
        goto err;
    }
    return;
err:
    _reconnect(dev, timeout);
}

static void _disconnect(Device * dev)
{
    Action *act;

    assert(dev->disconnect != NULL);
    dev->disconnect(dev);

    /* flush buffers */
    cbuf_flush(dev->from);
    cbuf_flush(dev->to);

    /* update state */
    dev->connect_state = DEV_NOT_CONNECTED;
    dev->logged_in = FALSE;

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

static void _verify_plugassign(Device *dev)
{
    Plug *plug;
    ListIterator itr;

    itr = list_iterator_create(dev->plugs);
    while ((plug = list_next(itr))) {
        if (plug->name == NULL) {
            err(FALSE, "_verify_plugassign(%s): node %s: not found\n", 
                    dev->name, plug->node);
        }
    }
    list_iterator_destroy(itr);
}

/*
 * Process the script for the current action for this device.
 * Update timeout and return if one of the script elements stalls.
 * Start the next action if we complete this one.
 */
static void _process_action(Device * dev, struct timeval *timeout)
{
    bool stalled = FALSE;
    Action *act;

    while ((act = list_peek(dev->acts)) && !stalled) {
        struct timeval timeleft;
        ExecCtx *e = list_peek(act->exec);

        assert(e != NULL);

        dbg(DBG_ACTION, "_process_action: processing action %d", act->com);
        _dbg_actions(dev);

        /* initialize timeout (action is brand new) */
        if (!timerisset(&act->time_stamp))
            Gettimeofday(&act->time_stamp, NULL);

        /* timeout exceeded? */
        if (_timeout(&act->time_stamp, &dev->timeout, &timeleft)) {
            if (!(dev->connect_state == DEV_CONNECTED)) 
                act->errnum = ACT_ECONNECTTIMEOUT;
            else if (!dev->logged_in) {
                act->errnum = ACT_ELOGINTIMEOUT;
                /* Need a warning here for any unassigned plug names */
                _verify_plugassign(dev);
            } else
                act->errnum = ACT_EEXPFAIL;

            if (act->vpf_fun) {
                unsigned char mem[MAX_DEV_BUF];
                int len = cbuf_peek(dev->from, mem, MAX_DEV_BUF);
                char *memstr = dbg_memstr(mem, len);
    
                if (!(dev->connect_state == DEV_CONNECTED)) 
                    act->vpf_fun(act->client_id, "connect(%s): timeout",
                            dev->name);
                else
                    act->vpf_fun(act->client_id, "recv(%s): '%s'",
                            dev->name, memstr);
                Free(memstr);
            }

        /* not connected but timeout not yet exceeded */
        } else if (!(dev->connect_state == DEV_CONNECTED)) {
            stalled = TRUE;                             /* not connnected */

        /* connected - process statements */
        } else {
            /* If a statement has an inner block to execute, it pushes a new 
             * ExecCtx onto the action's exec stack and returns.  We notice 
             * the top of the exec stack has changed and try to exec the next 
             * stmt (now of the new context) and so on.  When the inner block 
             * is done, the stack is popped and the "current" stmt is back to
             * the one that initiated the inner block.
             */
            do {
                e = list_peek(act->exec); 
                stalled = !_process_stmt(dev, act, e, timeout);
            } while (e != list_peek(act->exec));
        }

        /* stalled - update timeout for select */
        if (stalled) {
            _update_timeout(timeout, &timeleft);

        /* most recently attempted stmt completed successfully */
        } else if (act->errnum == ACT_ESUCCESS) {
            e->cur = list_next(e->itr);     /* next stmt this block */
            if (!e->cur) {                  /* ...or new block */
                ExecCtx *e2 = list_pop(act->exec);

                assert(e2 == e);
                _destroy_exec_ctx(e2);
                e = list_peek(act->exec);
            }

            /* completed action successfully! */
            if (e == NULL) {
                if (act->com == PM_LOG_IN) {
                    dev->logged_in = TRUE;
                    /* Need a warning here for any unassigned plug names */
                    _verify_plugassign(dev);
                }
                if (act->complete_fun)
                    _act_completion(act, dev);
                _destroy_action(list_dequeue(dev->acts));
                dev->stat_successful_actions++;
            }

        /* most recently attempted stmt completed with error */
        } else {
            ActError res = act->errnum; /* save for ref after _destroy_action */

            if (act->complete_fun)
                _act_completion(act, dev);
            _destroy_action(list_dequeue(dev->acts));

            /* if one action failed, abort the rest in the device queue 
             * in preparation for reconnect.
             */
            while ((act = list_dequeue(dev->acts)) != NULL) { 
                act->errnum = (res == ACT_EEXPFAIL ? ACT_EABORT : res);
                if (act->complete_fun)
                    _act_completion(act, dev);
                _destroy_action(act);
            }

            /* reconnect/login if expect timed out */
            if ((dev->connect_state == DEV_CONNECTED)) {
                dbg(DBG_DEVICE, "_process_action: disconnecting due to error");
                _reconnect(dev, timeout);
                break;
            }
        }
    } /* while loop */
}

bool _process_stmt(Device *dev, Action *act, ExecCtx *e, 
        struct timeval *timeout)
{
    bool finished = 0;

    switch (e->cur->type)
    {
    case STMT_EXPECT:
        finished = _process_expect(dev, act, e);
        break;
    case STMT_SEND:
        finished = _process_send(dev, act, e);
        break;
    case STMT_SETPLUGSTATE:
        finished = _process_setplugstate(dev, act, e);
        break;
    case STMT_DELAY:
        finished = _process_delay(dev, act, e, timeout);
        break;
    case STMT_FOREACHPLUG:
    case STMT_FOREACHNODE:
        finished = _process_foreach(dev, act, e);
        break;
    case STMT_IFON:
    case STMT_IFOFF:
        finished = _process_ifonoff(dev, act, e);
        break;
    }
    return finished;
}

static bool _process_foreach(Device *dev, Action *act, ExecCtx *e)
{
    bool finished = TRUE;
    ExecCtx *new;
    Plug *plug;

    /* we store a plug iterator in the ExecCtx */
    if (e->plugitr == NULL)  
        e->plugitr = list_iterator_create(dev->plugs);

    /* Each time the inner block is executed, its argument will be
     * a new plug name.  Pick that up here.
     */
    if (e->cur->type == STMT_FOREACHPLUG) {
        plug = list_next(e->plugitr);
    } else if (e->cur->type == STMT_FOREACHNODE) {
        do {
            plug = list_next(e->plugitr);
        } while (plug && plug->node == NULL);
    } else {
        assert(0);
    }

    /* plug list not exhausted? start a new execution context for this block */
    if (plug != NULL) {
        new = _create_exec_ctx(dev, e->cur->u.foreach.stmts, plug);
        list_push(act->exec, new);
    } else {
        list_iterator_destroy(e->plugitr);
        e->plugitr = NULL;
    }

    /* we won't be called again if we don't push a new context */

    return finished;
}

static bool _process_ifonoff(Device *dev, Action *act, ExecCtx *e)
{
    bool finished = TRUE;

    if (e->processing) { /* if returning from subblock, we are done */
        e->processing = FALSE; 

    } else {
        InterpState state = _get_argval(act->arglist, 
                e->target ? e->target->node : NULL);
        bool condition = FALSE;
        ExecCtx *new;

        if (e->cur->type == STMT_IFON && state == ST_ON)
            condition = TRUE;
        else if (e->cur->type == STMT_IFOFF && state == ST_OFF)
            condition = TRUE;
        else if (state == ST_UNKNOWN) {
            act->errnum = ACT_EEXPFAIL; /* FIXME */
        }

        /* condition met? start a new execution context for this block */
        if (condition) {
            e->processing = TRUE;
            new = _create_exec_ctx(dev, e->cur->u.ifonoff.stmts, e->target);
            list_push(act->exec, new);
        } 
    } 

    return finished;
}


/* Make a copy of a device's cached subexpression match, referenced by
 * 'mp' match position, or NULL if no match.  Caller must free the result.
 */
static char *_copy_pmatch(Device *dev, int mp)
{
    char *new;
    regmatch_t m; 

    if (!dev->matchstr) /* no previous match */
        return NULL;
    if (mp < 0 || mp >= (sizeof(dev->pmatch) / sizeof(regmatch_t)))
        return NULL;    /* match position is out of range */

    m = dev->pmatch[mp];

    if (m.rm_so >= strlen(dev->matchstr) || m.rm_so < 0)
        return NULL;    /* start pointer is out of range */
                        /* NOTE: will be -1 if subexpression did not match */
    if (m.rm_eo > strlen(dev->matchstr) || m.rm_eo < 0)
        return NULL;    /* end pointer is out of range */
    if (m.rm_eo - m.rm_so <= 0)
        return NULL;    /* match is zero length */

    new = Malloc(m.rm_eo - m.rm_so + 1);
    memcpy(new, dev->matchstr + m.rm_so, m.rm_eo - m.rm_so);
    new[m.rm_eo - m.rm_so] = '\0';
    
    return new;
}

static char *_plug_to_node(Device *dev, char *plug_name)
{
    ListIterator itr;
    char *node = NULL;
    Plug *plug;

    itr = list_iterator_create(dev->plugs);
    while ((plug = list_next(itr))) {
        if (plug->name && strcmp(plug->name, plug_name) == 0)
            node = plug->node;
    }
    list_iterator_destroy(itr);
    return node;
}

static bool _process_setplugstate(Device *dev, Action *act, ExecCtx *e)
{
    bool finished = TRUE;
    char *plug_name = NULL;

    /* 
     * Usage: setplugstate [plug] status [interps]
     * plug can be literal plug name, or regex match, or omitted, 
     * (implying target plug name).
     */
    if (e->cur->u.setplugstate.plug_name)    /* literal */
        plug_name = Strdup(e->cur->u.setplugstate.plug_name);
    if (!plug_name)                         /* regex match */
        plug_name = _copy_pmatch(dev, e->cur->u.setplugstate.plug_mp);
    if (!plug_name && (e->target && e->target->name != NULL))
        plug_name = Strdup(e->target->name);/* use action target */
    /* if no plug name, do nothing */

    if (plug_name) {
        char *str = _copy_pmatch(dev, e->cur->u.setplugstate.stat_mp);
        char *node = _plug_to_node(dev, plug_name);

        if (str && node) {
            _set_argval(act->arglist, node, str, e->cur->u.setplugstate.interps);
            Free(str);
        } 
        /* if no match, do nothing */
        Free(plug_name); 
    }

    return finished;
}

/* return TRUE if expect is finished */
static bool _process_expect(Device *dev, Action *act, ExecCtx *e)
{
    regex_t *re;
    bool finished = FALSE;
    size_t nm;
    regmatch_t *pm;

    re = &e->cur->u.expect.exp;
    pm = dev->pmatch;
    nm = sizeof(dev->pmatch) / sizeof(regmatch_t);

    /* Free previously cached expect match string */
    if (dev->matchstr) {
        Free(dev->matchstr);
        dev->matchstr = NULL;
    }

    /* Match? */
    if ((dev->matchstr = _getregex_buf(dev->from, re, nm, pm))) {
        if (act->vpf_fun) {
            char *memstr = dbg_memstr(dev->matchstr, strlen(dev->matchstr));

            act->vpf_fun(act->client_id, "recv(%s): '%s'", dev->name, memstr);
            Free(memstr);
        }
        finished = TRUE;
    } 
    return finished;
}

static bool _process_send(Device *dev, Action *act, ExecCtx *e)
{
    bool finished = FALSE;

    /* first time through? */
    if (!e->processing) {
        int dropped = 0;
        char *str = hsprintf(e->cur->u.send.fmt, e->target ? 
                (e->target->name ? e->target->name : "[unresolved]") : 0);
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
        e->processing = TRUE;
      
        Free(str);
    }

    if (cbuf_is_empty(dev->to)) {           /* finished! */
        e->processing = FALSE;
        finished = TRUE;
    } 

    return finished;
}

/* return TRUE if delay is finished */
static bool _process_delay(Device *dev, Action *act, ExecCtx *e,
        struct timeval *timeout)
{
    bool finished = FALSE;
    struct timeval delay, timeleft;

    delay = e->cur->u.delay.tv;

    /* first time */
    if (!e->processing) {
        if (act->vpf_fun)
            act->vpf_fun(act->client_id, "delay(%s): %ld.%-6.6ld", dev->name, 
                    delay.tv_sec, delay.tv_usec);
        e->processing = TRUE;
        Gettimeofday(&act->delay_start, NULL);
    }

    /* timeout expired? */
    if (_timeout(&act->delay_start, &delay, &timeleft)) {
        e->processing = FALSE;
        finished = TRUE;
    } else
        _update_timeout(timeout, &timeleft);

    return finished;
}

Device *dev_create(const char *name)
{
    Device *dev;
    int i;

    dev = (Device *) Malloc(sizeof(Device));
    dev->magic = DEV_MAGIC;
    dev->name = Strdup(name);
    dev->connect_state = DEV_NOT_CONNECTED;
    dev->fd = NO_FD;
    dev->acts = list_create((ListDelF) _destroy_action);
    dev->matchstr = NULL;
    dev->data = NULL;

    timerclear(&dev->timeout);
    timerclear(&dev->last_retry);
    timerclear(&dev->last_ping);
    timerclear(&dev->ping_period);

    dev->to = cbuf_create(MIN_DEV_BUF, MAX_DEV_BUF);
    cbuf_opt_set(dev->to, CBUF_OPT_OVERWRITE, CBUF_NO_DROP);

    dev->from = cbuf_create(MIN_DEV_BUF, MAX_DEV_BUF);
    cbuf_opt_set(dev->from, CBUF_OPT_OVERWRITE, CBUF_NO_DROP);

    for (i = 0; i < NUM_SCRIPTS; i++)
        dev->scripts[i] = NULL;

    dev->plugs = list_create((ListDelF) dev_plug_destroy);
    dev->plugnames_hardwired = FALSE;
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
    if (dev->data) {
        assert(dev->destroy != NULL);
        dev->destroy(dev->data);
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

    plug = (Plug *) Malloc(sizeof(Plug));
    plug->name = Strdup(name);
    plug->node = NULL;
    return plug;
}

/* used with list_find_first() in parser */
int dev_plug_match_plugname(Plug * plug, void *key)
{
    return (plug->name == NULL ? 0 : (strcmp(plug->name, (char *) key) == 0));
}
int dev_plug_match_noname(Plug * plug, void *key)
{
    return ((plug->name == NULL && plug->node == NULL) ? 1 : 0);
}

/* helper for dev_create */
void dev_plug_destroy(Plug * plug)
{
    if (plug->name)
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

/* helper for _set_argval */
static int _arg_match(Arg * arg, void *key)
{
    return (key != NULL && strcmp(arg->node, (char *) key) == 0 
            /* && (arg->state == ST_UNKNOWN || arg->val == NULL) */);
}

static InterpState _get_argval(ArgList *arglist, char *node)
{
    Arg *arg;
    InterpState state = ST_UNKNOWN;

    if ((arg = list_find_first(arglist->argv, (ListFindF) _arg_match, node))) {
        state = arg->state;
    }
    return state;
}

/*
 * Set the value of the argument with key = node.
 */
static void _set_argval(ArgList *arglist, char *node, char *val, List interps)
{
    Arg *arg;

    if ((arg = list_find_first(arglist->argv, (ListFindF) _arg_match, node))) {
        ListIterator itr;
        Interp *i;

        arg->val = Strdup(val);

        /* check for a matching interpretation (will default to ST_UNKNOWN) */
        itr = list_iterator_create(interps);
        while ((i = list_next(itr))) {
            if (_findregex(i->re, val, strlen(val), 0, NULL)) {
                arg->state = i->state;
                break;
            }
        }
        list_iterator_destroy(itr);
    }
}

static void _process_ping(Device * dev, struct timeval *timeout)
{
    struct timeval timeleft;

    if (dev->scripts[PM_PING] != NULL && timerisset(&dev->ping_period)) {
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
        assert(dev->connect_state == DEV_NOT_CONNECTED);
        _connect(dev);
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

        /* need to be in the write set if we are sending anything */
        if (dev->connect_state == DEV_CONNECTED) {
            if (!cbuf_is_empty(dev->to)) {
                FD_SET(dev->fd, wset);
                *maxfd = MAX(*maxfd, dev->fd);
            }
        }

        /* descriptor will become writable after a connect */
        if (dev->connect_state == DEV_CONNECTING) {
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
        if (dev->connect_state == DEV_NOT_CONNECTED) {
            _reconnect(dev, timeout);
        /* complete non-blocking connect if ready */
        } else if ((dev->connect_state == DEV_CONNECTING)) {
            assert(dev->fd != NO_FD);
            if (FD_ISSET(dev->fd, wset)) {
                assert(dev->finish_connect != NULL);
                if (dev->finish_connect(dev)) {
                    _login(dev);
                } else {
                    _reconnect(dev, timeout);
                } 
                if (dev->fd != NO_FD)
                    FD_CLR(dev->fd, wset);      /* avoid _handle_write error */
            }
        }

        /* if this device needs a ping, put it in the qeuue */
        if ((dev->connect_state == DEV_CONNECTED)) {
            _process_ping(dev, timeout);
        }

        /* read/write from/to buffer */
        if (dev->connect_state == DEV_CONNECTED) {
            if (dev->fd != NO_FD && FD_ISSET(dev->fd, rset))
                _handle_read(dev, timeout); /* also handles ECONNRESET */
        }
        /* could have disconnected in _handle_read */
        if (dev->connect_state == DEV_CONNECTED) {
            if (dev->fd != NO_FD && FD_ISSET(dev->fd, wset))
                _handle_write(dev, timeout);
        }

        /* handle device-specific preprocessing such as telnet escapes */
        if (dev->connect_state == DEV_CONNECTED && dev->preprocess != NULL)
            dev->preprocess(dev);

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
