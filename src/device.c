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
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#include "powerman.h"
#include "list.h"
#include "config.h"
#include "device.h"
#include "error.h"
#include "wrappers.h"
#include "buffer.h"
#include "hostlist.h"
#include "debug.h"
#include "client_proto.h"


/* bitwise values for dev->script_status */
#define DEV_LOGGED_IN	    0x01
#define DEV_SENDING	    0x02
#define DEV_EXPECTING	    0x04
#define DEV_DELAYING	    0x08

/*
 * Actions are appended to a per device list in dev->acts
 */
typedef struct {
    int          com;     /* one of the PM_* above */
    ListIterator itr;     /* next place in the script sequence */
    Script_El    *cur;    /* current place in the script sequence */
    char         *target; /* native device represenation of target plug(s) */
    ActionCB	 cb_fun;  /* callback for action completion */
    int		 client_id; /* client id so completion can find client */
    bool	 error;	  /* error flag for action */
    struct timeval  time_stamp; /* time stamp for timeouts */
    struct timeval  delay_start; /* time stamp for delay completion */
    ArgList      *arglist; /* argument for query actions (list of Arg's) */
    MAGIC;
} Action;

#define _act_started(act)   ((act)->cur != NULL)



static bool _reconnect(Device * dev);
static bool _finish_connect(Device * dev, struct timeval *timeout);
static bool _time_to_reconnect(Device *dev, struct timeval *timeout);
static void _disconnect(Device * dev);
static bool _process_expect(Device * dev);
static bool _process_send(Device * dev);
static bool _process_delay(Device * dev, struct timeval *timeout);
static void _set_argval_onoff(ArgList *arglist, char *node, char *val,
		ArgState state);
static void _match_subexpressions(Device *dev, Action *act, char *expect);
static int _match_name(Device * dev, void *key);
static void _handle_read(Device * dev);
static void _handle_write(Device * dev);
static void _process_action(Device * dev, struct timeval *timeout);
static bool _timeout(struct timeval * timestamp, struct timeval * timeout,
		struct timeval *timeleft);
static int _enqueue_actions(Device * dev, int com, hostlist_t hl, 
		ActionCB fun, int client_id, ArgList *arglist);
static Action *_create_action(Device *dev, int com, char *target,
		ActionCB fun, int client_id, ArgList *arglist);
static int _enqueue_targetted_actions(Device *dev, int com, hostlist_t hl,
		ActionCB fun, int client_id, ArgList *arglist);
static unsigned char *_findregex(regex_t * re, unsigned char *str, int len);
static char *_getregex_buf(Buffer b, regex_t * re);
static bool _command_needs_device(Device *dev, hostlist_t hl);
static void _process_ping(Device *dev, struct timeval *timeout);

static List dev_devices = NULL;


#define MAX_MATCH 20 


static void _dbg_actions(Device *dev)
{
    char tmpstr[1024];
    Action *act;
    ListIterator itr = list_iterator_create(dev->acts);

    tmpstr[0] = '\0';
    while ((act = list_next(itr))) {
	snprintf(tmpstr + strlen(tmpstr), sizeof(tmpstr) - strlen(tmpstr),
			"%d,", act->com);
    }
    tmpstr[strlen(tmpstr) - 1] = '\0'; /* zap trailing comma */
    dbg(DBG_ACTION, "%s: %s", dev->name, tmpstr);
}

/* 
 * Find regular expression in string.
 *  re (IN)	regular expression
 *  str (IN)	where to look for regular expression
 *  len (IN)	number of chars in str to search
 *  RETURN	pointer to char following the last char of the match,
 *		or NULL if no match
 */
static unsigned char *_findregex(regex_t * re, unsigned char *str, int len)
{
    int n;
    size_t nmatch = MAX_MATCH;
    regmatch_t pmatch[MAX_MATCH];
    int eflags = 0;

    n = Regexec(re, str, nmatch, pmatch, eflags);
    if (n != REG_NOERROR)
	return NULL;
    if (pmatch[0].rm_so == -1 || pmatch[0].rm_eo == -1)
	return NULL;
    assert(pmatch[0].rm_eo <= len);
    return (str + pmatch[0].rm_eo);
}

/*
 * Apply regular expression to the contents of a Buffer.
 * If there is a match, return (and consume) from the beginning
 * of the buffer to the last character of the match.
 * NOTE: embedded \0 chars are converted to \377 by buf_getstr/buf_getline and
 * buf_peekstr/buf_getstr because libc regex functions would treat these as 
 * string terminators.  As a result, \0 chars cannot be matched explicitly.
 *  b (IN)	buffer to apply regex to
 *  re (IN)	regular expression
 *  RETURN	String match (caller must free) or NULL if no match
 */
static char *_getregex_buf(Buffer b, regex_t * re)
{
    unsigned char str[MAX_BUF];
    int bytes_peeked = buf_peekstr(b, str, MAX_BUF);
    unsigned char *match_end;

    if (bytes_peeked == 0)
	return NULL;
    match_end = _findregex(re, str, bytes_peeked);
    if (match_end == NULL)
	return NULL;
    assert(match_end - str <= strlen(str));
    *match_end = '\0';
    buf_eat(b, match_end - str);	/* only consume up to what matched */

    return Strdup(str);
}

static Action *_create_action(Device *dev, int com, char *target,
		ActionCB fun, int client_id, ArgList *arglist)
{
    Action *act;

    dbg(DBG_ACTION, "_create_action: %d", com);
    act = (Action *) Malloc(sizeof(Action));
    INIT_MAGIC(act);
    act->com = com;
    act->cb_fun = fun;
    act->client_id = client_id;
    act->itr = list_iterator_create(dev->prot->scripts[act->com]);
    act->cur = NULL;
    act->target = target ? Strdup(target) : NULL;
    act->error = FALSE;
    act->arglist = arglist ? dev_link_arglist(arglist) : NULL;
    timerclear(&act->time_stamp);
    return act;
}

static void _destroy_action(Action * act)
{
    CHECK_MAGIC(act);

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
    CLEAR_MAGIC(act);
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
void dev_add(Device *dev)
{
    list_append(dev_devices, dev);
}

/*
 * Telemetry logging callback for Buffer outgoing to device.
 */
static void _buflogfun_to(unsigned char *mem, int len, void *arg)
{
    Device *dev = (Device *) arg;
    char *str = dbg_memstr(mem, len);

    dbg(DBG_DEV_TELEMETRY, "S(%s): %s", dev->name, str);
    Free(str);
}

/*
 * Telemetry logging callback for Buffer incoming from device.
 */
static void _buflogfun_from(unsigned char *mem, int len, void *arg)
{
    Device *dev = (Device *) arg;
    char *str = dbg_memstr(mem, len);

    dbg(DBG_DEV_TELEMETRY, "D(%s): %s", dev->name, str);
    Free(str);
}

/*
 * Test whether timeout has occurred
 *  time_stamp (IN) 
 *  timeout (IN)
 *  timeleft (OUT)	if timeout has not occurred, put time left here
 *  RETURN		TRUE if (time_stamp + timeout > now)
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

    if (timercmp(&now, &limit, >))	    /* if now > limit */
	result = TRUE;

    if (result == FALSE)
	timersub(&limit, &now, timeleft);   /* timeleft = limit - now */
    else
	timerclear(timeleft);

    return result;
}

/* 
 * If tv is less than timeout, or timeout is zero, set timeout = tv.
 */
void _update_timeout(struct timeval *timeout, struct timeval *tv)
{
    if (timercmp(tv, timeout, <) || (!timeout->tv_sec && !timeout->tv_sec))
	*timeout = *tv;
}

/* 
 * Return TRUE if OK to attempt reconnect.  If FALSE, put the time left 
 * in timeout if it is less than timeout or if timeout is zero.
 */
bool _time_to_reconnect(Device *dev, struct timeval *timeout)
{
    static int rtab[] = { 1, 2, 4, 8, 15, 30, 60 };
    int max_rtab_index = sizeof(rtab)/sizeof(int) - 1;
    int rix = dev->reconnect_count - 1;
    struct timeval timeleft, retry;
    bool reconnect = TRUE;

    if (dev->reconnect_count > 0) {

	    timerclear(&retry);
	    retry.tv_sec = rtab[rix > max_rtab_index ? max_rtab_index : rix];

	    if (!_timeout(&dev->last_reconnect, &retry, &timeleft))
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

    CHECK_MAGIC(dev);
    assert(dev->type == TCP_DEV);
    assert(dev->connect_status == DEV_NOT_CONNECTED);
    assert(dev->fd == -1);

    Gettimeofday(&dev->last_reconnect, NULL);
    dev->reconnect_count++;

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

    assert(dev->to == NULL);
    dev->to = buf_create(dev->fd, MAX_BUF, _buflogfun_to, dev);
    assert(dev->from == NULL);
    dev->from = buf_create(dev->fd, MAX_BUF, _buflogfun_from, dev);

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
	    dbg(DBG_ACTION, "_reconnect: next action on queue: %d", act->com);
	else
	    dbg(DBG_ACTION, "_reconnect: no action on queue");
    }

    return (dev->connect_status == DEV_CONNECTED);
}

/* helper for dev_check_actions/dev_enqueue_actions */
static bool _command_needs_device(Device *dev, hostlist_t hl)
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
		if (dev->prot->scripts[com] == NULL) { /* unimplemented */
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
int dev_enqueue_actions(int com, hostlist_t hl, ActionCB fun, int client_id,
		ArgList *arglist)
{
    Device *dev;
    ListIterator itr;
    int count = 0;

    itr = list_iterator_create(dev_devices);
    while ((dev = list_next(itr))) {
	if (!dev->prot->scripts[com]) /* unimplemented script */
	    continue;
	if (hl && !_command_needs_device(dev, hl)) /* uninvolved device */
	    continue;
	count += _enqueue_actions(dev, com, hl, fun, client_id, arglist);
    }
    list_iterator_destroy(itr);

    return count;
}

static int _enqueue_actions(Device *dev, int com, hostlist_t hl, 
		ActionCB fun, int client_id, ArgList *arglist)
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
            act = _create_action(dev, com, NULL, fun, client_id, arglist);
            list_prepend(dev->acts, act);
	    count++;
            break;
        case PM_POWER_ON:
        case PM_POWER_OFF:
        case PM_POWER_CYCLE:
        case PM_RESET:
	    if (hl) {
		count += _enqueue_targetted_actions(dev, com, hl, fun, 
			    client_id, arglist);
		break;
	    }
	    /*FALLTHROUGH*/
        case PM_UPDATE_PLUGS:
        case PM_UPDATE_NODES:
        case PM_LOG_OUT:
	case PM_PING:
            act = _create_action(dev, com, NULL, fun, client_id, arglist);
            list_append(dev->acts, act);
	    count++;
            break;
        default:
            assert(FALSE);
    }

    return count;
}

/* return "all" version of script if defined else -1 */
static int _get_all_script(Device *dev, int com)
{
    int new = -1;

    switch (com) {
        case PM_POWER_ON:
	    if (dev->prot->scripts[PM_POWER_ON_ALL])
		new = PM_POWER_ON_ALL;
	    break;
        case PM_POWER_OFF:
	    if (dev->prot->scripts[PM_POWER_OFF_ALL])
		new = PM_POWER_OFF_ALL;
	    break;
        case PM_POWER_CYCLE:
	    if (dev->prot->scripts[PM_POWER_CYCLE_ALL])
		new = PM_POWER_CYCLE_ALL;
	    break;
        case PM_RESET:
	    if (dev->prot->scripts[PM_RESET_ALL])
		new = PM_RESET_ALL;
	    break;
	default:
	    break;
    }
    return new;
}


static int _enqueue_targetted_actions(Device *dev, int com, hostlist_t hl,
		ActionCB fun, int client_id, ArgList *arglist)
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
	act = _create_action(dev, com, plug->name, fun, client_id, arglist);
	list_append(new_acts, act);
    }

    if (all) {
	if (dev->all != NULL) {	/* normal script, "*" plug */
	    act = _create_action(dev, com, dev->all, fun, client_id, arglist);
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
static bool _finish_connect(Device *dev, struct timeval *timeout)
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
	dev->reconnect_count = 0;
        _enqueue_actions(dev, PM_LOG_IN, NULL, NULL, 0, NULL);
    }

    return (dev->connect_status == DEV_CONNECTED);
}

/*
 * Select says device is ready for reading.
 */
static void _handle_read(Device * dev)
{
    int n;

    CHECK_MAGIC(dev);
    n = buf_read(dev->from);
    assert(n >= 0 || errno == ECONNRESET || errno == EWOULDBLOCK);
    if (n < 0) {
	err(TRUE, "read error on %s", dev->name);
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

    CHECK_MAGIC(dev);

    n = buf_write(dev->to);
    assert (n >= 0 || errno == EAGAIN || errno == ECONNRESET || errno == EPIPE);
    if (n < 0) {
	err(TRUE, "write error on %s", dev->name);
	_disconnect(dev);
	_reconnect(dev);
    }
    if (n == 0) {
	err(TRUE, "write sent no data on %s", dev->name);
	_disconnect(dev);
	_reconnect(dev);
	/* XXX: is this even possible? */
    }
}

static void _disconnect(Device *dev)
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

    /* destroy buffers */
    buf_destroy(dev->from);
    dev->from = NULL;
    buf_destroy(dev->to);
    dev->to = NULL;

    /* update state */
    dev->connect_status = DEV_NOT_CONNECTED;
    dev->script_status = 0;

    /* delete PM_LOG_IN action queued for this device, if any */
    if (((act = list_peek(dev->acts)) != NULL) && act->com == PM_LOG_IN)
	_destroy_action(list_dequeue(dev->acts));
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

	if (act == NULL)		/* no actions in queue */
	    break;
	dbg(DBG_ACTION, "_process_action: processing action %d", act->com);
	_dbg_actions(dev);

	/* Start the action if it is a new one.
	 * act->cur points to the current script element.
	 * act->time_stamp will cause action timeout if elapsed time for
	 * entire action exceeds dev->timeout.
	 */
	assert(act->itr != NULL);
	if (!_act_started(act)) {
	    act->cur = list_next(act->itr); /* point to first script elem */
	    Gettimeofday(&act->time_stamp, NULL);
	}
	assert(act->cur != NULL);

	/* process actions */
	if (_timeout(&act->time_stamp, &dev->timeout, &timeleft)) {
	    dbg(DBG_SCRIPT, "_process_action(%s): %s timeout - aborting", 
			    dev->name, act->cur->type == EL_EXPECT ? "expect"
			    : act->cur->type == EL_SEND ? "send"
			    : "delay");
	    act->error = TRUE;			    /* timed out */
	} else if (!(dev->connect_status & DEV_CONNECTED)) {
	    stalled = TRUE;			    /* not connnected */
	} else if (act->cur->type == EL_EXPECT) {
	    stalled = !_process_expect(dev);	    /* do expect */
	} else if (act->cur->type == EL_SEND) {
	    stalled = !_process_send(dev);	    /* do send */
	} else {
	    assert(act->cur->type == EL_DELAY);
	    stalled = !_process_delay(dev, timeout); /* do delay */
	}

	if (stalled) {	/* if stalled, set timeout for select */
	    if (act->cur->type != EL_DELAY) /* FIXME: why test this? */
		_update_timeout(timeout, &timeleft);
	}
	else {
	    bool error = act->error;

	    assert(act->itr != NULL);
	    /* completed action - notify client and dequeue action */
	    if (act->error || (act->cur = list_next(act->itr)) == NULL) {
		if (act->com == PM_LOG_IN) {
		    if (!act->error)
			dev->script_status |= DEV_LOGGED_IN;
		} 
		if (act->cb_fun)			/* notify client */
		    act->cb_fun(act->client_id, 
				    act->error ? CP_ERR_TIMEOUT : NULL,
				    dev->name);
		_destroy_action(list_dequeue(dev->acts));
	    }

	    /* reconnect/login if expect timed out */
	    if (error && (dev->connect_status & DEV_CONNECTED)) {
		dbg(DBG_DEVICE, "_process_action: disconnecting due to error");
		_disconnect(dev);
		_reconnect(dev);
		break;
	    }
	}
    }
}

/* return TRUE if expect is finished */
static bool _process_expect(Device *dev)
{
    regex_t *re;
    Action *act = list_peek(dev->acts);
    char *expect;
    bool finished = FALSE;

    assert(act != NULL);
    assert(act->cur != NULL);
    assert(act->cur->type == EL_EXPECT);

    re = &act->cur->s_or_e.expect.exp;

    if ((expect = _getregex_buf(dev->from, re))) {	/* match */
	dbg(DBG_SCRIPT, "_process_expect(%s): match", dev->name);

	/* process values of parenthesized subexpressions, if any */
	_match_subexpressions(dev, act, expect);
	Free(expect);
	finished = TRUE;
    } else {						/* no match */
	unsigned char mem[MAX_BUF];
	int len = buf_peekstr(dev->from, mem, MAX_BUF);
	char *str = dbg_memstr(mem, len);

	dbg(DBG_SCRIPT, "_process_expect(%s): no match: '%s'", 
			dev->name, str);
	Free(str);
    }

    return finished;
}

static bool _process_send(Device * dev)
{
    Action *act = list_peek(dev->acts);
    char *fmt;
    bool finished = FALSE;

    assert(act != NULL);
    CHECK_MAGIC(act);
    assert(act->cur != NULL);
    assert(act->cur->type == EL_SEND);

    fmt = act->cur->s_or_e.send.fmt;

    /* first time through? */
    if (!(dev->script_status & DEV_SENDING)) {
	dev->script_status |= DEV_SENDING;
	if (act->target == NULL)
	    buf_printf(dev->to, fmt); 
	else
	    buf_printf(dev->to, fmt, act->target);
    }

    if (buf_isempty(dev->to)) {				/* finished! */
	dbg(DBG_SCRIPT, "_process_send(%s): send complete", dev->name);
	dev->script_status &= ~DEV_SENDING;
	finished = TRUE;
    } else						/* more to write */
	dbg(DBG_SCRIPT, "_process_send(%s): incomplete send", dev->name);

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
    assert(act->cur->type == EL_DELAY);

    delay = act->cur->s_or_e.delay.tv;

    /* first time */
    if (!(dev->script_status & DEV_DELAYING)) {
	dbg(DBG_SCRIPT, "_process_delay(%s): %ld.%-6.6ld", dev->name, 
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

    assert(m.rm_so < MAX_BUF && m.rm_so >= 0);
    assert(m.rm_eo < MAX_BUF && m.rm_eo >= 0);
    assert(m.rm_eo - m.rm_so > 0);

    new = Malloc(m.rm_eo - m.rm_so + 1);
    memcpy(new, str + m.rm_so, m.rm_eo - m.rm_so);
    new[m.rm_eo - m.rm_so] = '\0';
    /* XXX andrew zapped trailing spaces in old code - needed here? */
    return new;
}

static void _match_subexpressions(Device *dev, Action *act, char *expect)
{
    Interpretation *interp;
    ListIterator itr;
    size_t nmatch = MAX_MATCH;
    regmatch_t pmatch[MAX_MATCH];
    int eflags = 0;
    int n;

    if (act->com != PM_UPDATE_PLUGS)
	return;
    if (act->cur->s_or_e.expect.map == NULL)
	return;

    /* match regex against expect - must succede, we checked before */
    n = Regexec(&act->cur->s_or_e.expect.exp, expect, nmatch, pmatch, eflags);
    assert(n == REG_NOERROR); 
    assert(pmatch[0].rm_so != -1 && pmatch[0].rm_eo != -1);
    assert(pmatch[0].rm_so <= strlen(expect));

    itr = list_iterator_create(act->cur->s_or_e.expect.map);
    while ((interp = list_next(itr))) {
	char *str;

	if (interp->node == NULL) /* unused plug? */
	    continue;
	assert(interp->match_pos >= 0 && interp->match_pos < MAX_MATCH);

	str = _copy_pmatch(expect, pmatch[interp->match_pos]);

	if (_findregex(&dev->on_re, str, strlen(str)))
	    _set_argval_onoff(act->arglist, interp->node, str, ST_ON);
	if (_findregex(&dev->off_re, str, strlen(str)))
	    _set_argval_onoff(act->arglist, interp->node, str, ST_OFF);

	Free(str);
    }
    list_iterator_destroy(itr);
}

Device *dev_create(const char *name)
{
    Device *dev;

    dev = (Device *) Malloc(sizeof(Device));
    INIT_MAGIC(dev);
    dev->name = Strdup(name);
    dev->type = NO_DEV;
    dev->connect_status = DEV_NOT_CONNECTED;
    dev->script_status = 0;
    dev->fd = NO_FD;
    dev->acts = list_create((ListDelF) _destroy_action);
    timerclear(&dev->timeout);
    timerclear(&dev->last_reconnect);
    timerclear(&dev->last_ping);
    timerclear(&dev->ping_period);
    dev->to = NULL;
    dev->from = NULL;
    dev->prot = NULL;
    dev->num_plugs = 0;
    dev->plugs = list_create((ListDelF) dev_plug_destroy);
    dev->reconnect_count = 0;
    return dev;
}

/* helper for dev_findbyname */
static int _match_name(Device * dev, void *key)
{
    return (strcmp(dev->name, (char *)key) == 0);
}

Device *dev_findbyname(char *name)
{
    return list_find_first(dev_devices, (ListFindF) _match_name, name);
}

void dev_destroy(Device * dev)
{
    int i;
    CHECK_MAGIC(dev);

    Free(dev->name);
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

    if (dev->prot != NULL)
        for (i = 0; i < NUM_SCRIPTS; i++)
            if (dev->prot->scripts[i] != NULL)
                list_destroy(dev->prot->scripts[i]);

    CLEAR_MAGIC(dev);
    if (dev->to != NULL)
	buf_destroy(dev->to);
    if (dev->from != NULL)
	buf_destroy(dev->from);
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
int dev_plug_match(Plug * plug, void *key)
{
    return (strcmp(((Plug *)plug)->name, (char *)key) == 0);
}

/* helper for dev_create */
void dev_plug_destroy(Plug * plug)
{
    Free(plug->name);
    Free(plug->node);
    Free(plug);
}

static void _destroy_arg(Arg *arg)
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
    ArgList *new = (ArgList *)Malloc(sizeof(ArgList));
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
	arg = (Arg *)Malloc(sizeof(Arg));
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
void dev_unlink_arglist(ArgList *arglist)
{
    if (--arglist->refcount == 0) {
	list_destroy(arglist->argv);
	Free(arglist);
    }
}

/*
 * Increment the ref count for arglist.
 */
ArgList *dev_link_arglist(ArgList *arglist)
{
    arglist->refcount++;
    return arglist;
}

/* helper for _set_argval_onoff */
static int _arg_match(Arg *arg, void *key)
{
    return (strcmp(arg->node, (char *)key) == 0);
}

/*
 * Set the value of the argument with key = node.
 */
static void _set_argval_onoff(ArgList *arglist, char *node, char *val, 
		ArgState state)
{
    Arg *arg;

    if ((arg = list_find_first(arglist->argv, (ListFindF) _arg_match, node))) {
	arg->state = state;
	arg->val = Strdup(val);
    }
}

static void _process_ping(Device *dev, struct timeval *timeout)
{
    struct timeval timeleft;

    if (dev->prot->scripts[PM_PING] != NULL && timerisset(&dev->ping_period)) {
	if (_timeout(&dev->last_ping, &dev->ping_period, &timeleft)) {
	    _enqueue_actions(dev, PM_PING, NULL, NULL, 0, NULL);
	    Gettimeofday(&dev->last_ping, NULL);
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
void dev_pre_select(fd_set *rset, fd_set *wset, int *maxfd)
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
			    fdsetstr, sizeof(fdsetstr)));
}

/* 
 * Called after select to process ready file descriptors, timeouts, etc.
 */
void dev_post_select(fd_set *rset, fd_set *wset, struct timeval *timeout)
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
		    FD_CLR(dev->fd, wset); /* avoid _handle_write error */
	    }
	}

	/* read/write from/to buffer */
	if (dev->fd != NO_FD && (dev->connect_status & DEV_CONNECTED)) {
	    if (FD_ISSET(dev->fd, rset))
		_handle_read(dev); /* also handles ECONNRESET */
	    if (FD_ISSET(dev->fd, wset))
		_handle_write(dev);
	}

	/* if this device needs a ping, take care of it */
	if ((dev->connect_status & DEV_CONNECTED)) {
	    _process_ping(dev, timeout);
	}

	/* process actions */
	if (list_peek(dev->acts)) {
	    _process_action(dev, timeout);
	}

    }
    list_iterator_destroy(itr);
}

/*
 * vi:softtabstop=4
 */
