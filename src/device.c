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

static bool _reconnect(Device * dev);
static bool _finish_connect(Device * dev, struct timeval *timeout);
static bool _time_to_reconnect(Device *dev, struct timeval *timeout);
static void _disconnect(Device * dev);
static bool _process_expect(Device * dev, struct timeval *timeout);
static bool _process_send(Device * dev);
static bool _process_delay(Device * dev, struct timeval *timeout);
static void _do_device_semantics(Device * dev, List map);
static bool _match_regex(Device * dev, char *expect);
static int _match_name(Device * dev, void *key);
static void _handle_read(Device * dev);
static void _handle_write(Device * dev);
static void _process_script(Device * dev, struct timeval *timeout);
static bool _timeout(struct timeval * timestamp, struct timeval * timeout,
		struct timeval *timeleft);
static int _enqueue_actions(Device * dev, int com, hostlist_t hl, 
		ActionCB fun, void *arg);
static Action *_create_action(Device *dev, int com, char *target,
		ActionCB fun, void *arg);
static int _enqueue_targetted_actions(Device *dev, int com, hostlist_t hl,
		ActionCB fun, void *arg);
static unsigned char *_findregex(regex_t * re, unsigned char *str, int len);
static char *_getregex_buf(Buffer b, regex_t * re);

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
		ActionCB fun, void *arg)
{
    Action *act;

    dbg(DBG_ACTION, "_create_action: %d", com);
    act = (Action *) Malloc(sizeof(Action));
    INIT_MAGIC(act);
    act->com = com;
    act->cb_fun = fun;
    act->cb_arg = arg;
    act->itr = list_iterator_create(dev->prot->scripts[act->com]);
    act->cur = NULL;
    act->target = target ? Strdup(target) : NULL;
    act->error = FALSE;
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

	    if (!_timeout(&dev->time_stamp, &retry, &timeleft))
		reconnect = FALSE;
	    if (timeout && !reconnect)
		_update_timeout(timeout, &timeleft);
    }
#if 0
    dbg(DBG_DEVICE, "_time_to_reconnect(%d): %s", dev->reconnect_count,
			reconnect ? "yes" : "no");
#endif
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

    Gettimeofday(&dev->time_stamp, NULL);
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

    /* 0 = connected; -1 implies EINPROGRESS */
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

/*
 * Translate a command from a client into actions for devices.
 * Return an action count so the client be notified when all the
 * actions "check in".
 */
int dev_enqueue_actions(int com, hostlist_t hl, ActionCB fun, void *arg)
{
    Device *dev;
    ListIterator itr;
    int count = 0;

    itr = list_iterator_create(dev_devices);
    while ((dev = list_next(itr))) {
	/* not logged in to device */
	if (!(dev->script_status & DEV_LOGGED_IN) && com != PM_LOG_IN) {
	    if (fun)
		fun(arg, TRUE);
	    continue;
	    /* FIXME: just enqueue a login in front of this one
	     * (if there isn't already one in the queue) instead of error.
	     */
	}
	/* command not implemented on this device */
	if (dev->prot->scripts[com] == NULL) {
	    if (fun)
		fun(arg, TRUE);	/* FIXME: timeout message inappropriate */
	    continue;
	}
	/* do the real work */
	count += _enqueue_actions(dev, com, hl, fun, arg);
    }
    list_iterator_destroy(itr);

    return count;
}

static int _enqueue_actions(Device *dev, int com, hostlist_t hl, 
		ActionCB fun, void *arg)
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
            act = _create_action(dev, com, NULL, fun, arg);
            list_prepend(dev->acts, act);
	    count++;
            break;
        case PM_LOG_OUT:
            act = _create_action(dev, com, NULL, fun, arg);
            list_append(dev->acts, act);
	    count++;
            break;
        case PM_UPDATE_PLUGS:
        case PM_UPDATE_NODES:
        case PM_POWER_ON:
        case PM_POWER_OFF:
        case PM_POWER_CYCLE:
        case PM_RESET:
	    if (hl != NULL) {
		count += _enqueue_targetted_actions(dev, com, hl, fun, arg);
	    } else {
		act = _create_action(dev, com, NULL, fun, arg); 
		list_append(dev->acts, act);
		count++;
	    }
            break;
        default:
            assert(FALSE);
    }

    return count;
}


static int _enqueue_targetted_actions(Device *dev, int com, hostlist_t hl,
		ActionCB fun, void *arg)
{
    List new_acts = list_create((ListDelF) _destroy_action);
    Action *act;
    bool all = TRUE;
    Plug *plug;
    ListIterator itr;
    int count = 0;

    itr = list_iterator_create(dev->plugs);
    while ((plug = list_next(itr))) {
	/* antisocial to gratuitously turn on/off unused plug */
        if (plug->node == NULL) { 
            all = FALSE;
            continue;
        }
        /* check if node name for plug matches the target */
        if (hostlist_find(hl, plug->node->name) == -1) {
	    all = FALSE;
	    continue;
	}
	/* match! */
	act = _create_action(dev, com, plug->name, fun, arg);
	list_append(new_acts, act);
    }

    if (all) {
	act = _create_action(dev, com, dev->all, fun, arg);
        list_append(dev->acts, act);
	count++;
    } else {
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
        _enqueue_actions(dev, PM_LOG_IN, NULL, NULL, NULL);
    }

    return (dev->connect_status == DEV_CONNECTED);
}

/*
 *   The Read gets the string.  If it is EOF then we've lost 
 * our connection with the device and need to reconnect.  
 * buf_read() does the real work.  If something arrives,
 * that is handled in _process_script().
 */
static void _handle_read(Device * dev)
{
    int n;

    CHECK_MAGIC(dev);
    n = buf_read(dev->from);
    if (n < 0 && errno == EWOULDBLOCK) {
	err(TRUE, "read error on %s", dev->name);
	return;
    }
    if (n == 0 || (n < 0 && errno == ECONNRESET)) {
	err(n < 0, "read error on %s", dev->name);
	_disconnect(dev);
	/*dev->reconnect_count = 0;*/
	_reconnect(dev);
	return;
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
 * Update timeout and return if we are stalled in an expect or a delay.
 */
static void _process_script(Device * dev, struct timeval *timeout)
{
    bool stalled = FALSE;

    while (!stalled) {
	Action *act = list_peek(dev->acts);

	if (act == NULL)		/* no actions in queue */
	    break;
	dbg(DBG_ACTION, "_process_script: processing action %d", act->com);
	_dbg_actions(dev);

	assert(act->itr != NULL);
	if (act->cur == NULL)		/* point at first script element */
	    act->cur = list_next(act->itr);
	assert(act->cur != NULL);
	assert(act->error == FALSE);

	switch (act->cur->type) {
	    case EL_EXPECT:
		stalled = !_process_expect(dev, timeout);
		break;
	    case EL_DELAY:
		stalled = !_process_delay(dev, timeout);
		break;
	    case EL_SEND:
		_process_send(dev); /* can't stall */
		break;
	    default:
		err_exit(FALSE, "_process_script is very confused");
	}

	if (!stalled) {
	    bool error = act->error;

	    assert(act->itr != NULL);
	    /* completed action - notify client and dequeue action */
	    if (act->error || (act->cur = list_next(act->itr)) == NULL) {
		if (act->com == PM_LOG_IN) {
		    if (!act->error)
			dev->script_status |= DEV_LOGGED_IN;
		} 
		if (act->cb_fun)			/* notify client */
		    act->cb_fun(act->cb_arg, act->error);
		_destroy_action(list_dequeue(dev->acts));
	    }

	    /* reconnect/login if expect timed out */
	    if (error && (dev->connect_status & DEV_CONNECTED)) {
		dbg(DBG_DEVICE, "_process_script: disconnecting due to error");
		_disconnect(dev);
		/*dev->reconnect_count = 0;*/
		_reconnect(dev);
		break;
	    }
	}
    }
}

/*
 *   The next Script_El is an EXPECT.  Indeed we know exactly what to
 * be expecting, so we go to the buffer and look to see if there's a
 * possible new input.  If the regex matches, extract the expect string.
 * If there is an Interpretation map for the expect then send that info to
 * the semantic processor.  
 */
/* return TRUE if expect is finished */
bool _process_expect(Device * dev, struct timeval *timeout)
{
    regex_t *re;
    Action *act = list_peek(dev->acts);
    char *expect;
    bool finished = FALSE;
    struct timeval timeleft;

    assert(act != NULL);
    assert(act->cur != NULL);
    assert(act->cur->type == EL_EXPECT);

    /* first time through? */
    if (!(dev->script_status & DEV_EXPECTING)) {
	dev->script_status |= DEV_EXPECTING;
	Gettimeofday(&act->time_stamp, NULL);
    }

    re = &act->cur->s_or_e.expect.exp;

    if ((expect = _getregex_buf(dev->from, re))) {	/* match */
	dbg(DBG_SCRIPT, "_process_expect(%s): match", dev->name);

	/* process values of parenthesized subexpressions */
	if (act->cur->s_or_e.expect.map != NULL) {
	    bool res = _match_regex(dev, expect);

	    assert(res == TRUE); /* since the first regexec worked ... */
	    _do_device_semantics(dev, act->cur->s_or_e.expect.map);
	}
	Free(expect);
	finished = TRUE;
    } else if (_timeout(&act->time_stamp, &dev->timeout, &timeleft)) {
							/* timeout? */
	dbg(DBG_SCRIPT, "_process_expect(%s): timeout - aborting", dev->name);
	act->error = TRUE;
	finished = TRUE;
    } else {						/* keep trying */
	_update_timeout(timeout, &timeleft);
    }

    if (!finished) {
	unsigned char mem[MAX_BUF];
	int len = buf_peekstr(dev->from, mem, MAX_BUF);
	char *str = dbg_memstr(mem, len);

	dbg(DBG_SCRIPT, "_process_expect(%s): no match: '%s'", 
			dev->name, str);
	Free(str);
    }

    if (finished)
	dev->script_status &= ~DEV_EXPECTING;

    return finished;
}

/*
 *   buf_printf() does al the real work.  send.fmt has a string
 * printf(fmt, ...) style.  If it has a "%s" then call buf_printf
 * with the target as its last argument.  Otherwise just send the
 * string and update the device's status.  
 */
bool _process_send(Device * dev)
{
    bool finished = TRUE;
    Action *act;
    char *fmt;

    CHECK_MAGIC(dev);

    act = list_peek(dev->acts);
    assert(act != NULL);
    CHECK_MAGIC(act);
    assert(act->cur != NULL);
    assert(act->cur->type == EL_SEND);

    fmt = act->cur->s_or_e.send.fmt;

    if (act->target == NULL)
	buf_printf(dev->to, fmt);
    else
	buf_printf(dev->to, fmt, act->target);

    dev->script_status |= DEV_SENDING;

    return finished;
}

/* return TRUE if delay is finished */
bool _process_delay(Device * dev, struct timeval *timeout)
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
	Gettimeofday(&act->time_stamp, NULL);
    }

    /* timeout expired? */
    if (_timeout(&act->time_stamp, &delay, &timeleft)) {
	dev->script_status &= ~DEV_DELAYING;
	finished = TRUE;
    } else
	_update_timeout(timeout, &timeleft);
    
    return finished;
}

/*
 *   The reply for a device may be in one or 
 * several EXPECT's Intterpretation maps.  This function goes
 * through all the Interpretations for this EXPECT and sets the
 * plug or node states for whatever plugs it finds.
 */
void _do_device_semantics(Device * dev, List map)
{
    Action *act;
    Interpretation *interp;
    ListIterator map_i;
    regex_t *re;
    char *str;
    char *end;
    char tmp;
    char *pos;
    int len;

    CHECK_MAGIC(dev);

    act = list_peek(dev->acts);
    assert(act != NULL);
    assert(map != NULL);
    map_i = list_iterator_create(map);

    switch (act->com) {
    case PM_UPDATE_PLUGS:
	while (((Interpretation *) interp = list_next(map_i))) {
	    if (interp->node == NULL)
		continue;
	    interp->node->p_state = ST_UNKNOWN;
	    re = &(dev->on_re);
	    str = interp->val;
	    len = strlen(str);
	    end = str;
	    while (*end && !isspace(*end) && ((end - str) < len))
		end++;
	    tmp = *end;
	    len = end - str;
	    *end = '\0';
	    if ((pos = _findregex(re, str, len)) != NULL)
		interp->node->p_state = ST_ON;
	    re = &(dev->off_re);
	    if ((pos = _findregex(re, str, len)) != NULL)
		interp->node->p_state = ST_OFF;
	    *end = tmp;
	}
	break;
    case PM_UPDATE_NODES:
	while (((Interpretation *) interp = list_next(map_i))) {
	    if (interp->node == NULL)
		continue;
	    interp->node->n_state = ST_UNKNOWN;
	    re = &(dev->on_re);
	    str = interp->val;
	    len = strlen(str);
	    end = str;
	    while (*end && !isspace(*end) && ((end - str) < len))
		end++;
	    tmp = *end;
	    len = end - str;
	    *end = '\0';
	    if ((pos = _findregex(re, str, len)) != NULL)
		interp->node->n_state = ST_ON;
	    re = &(dev->off_re);
	    if ((pos = _findregex(re, str, len)) != NULL)
		interp->node->n_state = ST_OFF;
	    *end = tmp;
	}
	break;
    default:
    }
    list_iterator_destroy(map_i);
}

/*
 *   buf_write does all the work here except for changing the 
 * device state.
 */
static void _handle_write(Device * dev)
{
    int n;

    CHECK_MAGIC(dev);

    n = buf_write(dev->to);
    if (n < 0)
	return;
    if (buf_isempty(dev->to))
	dev->script_status &= ~DEV_SENDING;
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
    Gettimeofday(&dev->time_stamp, NULL);
    timerclear(&dev->timeout);
    dev->to = NULL;
    dev->from = NULL;
    dev->prot = NULL;
    dev->num_plugs = 0;
    dev->plugs = list_create((ListDelF) dev_plug_destroy);
    dev->reconnect_count = 0;
    return dev;
}

/*
 *   This match utility is compatible with the list API's ListFindF
 * prototype for searching a list of Device * structs.  The match
 * criterion is a string match on their names.  This comes into use 
 * in the parser when the devices have been parsed into a list and 
 * a node line referes to a device by its name.  
 */
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
    Free(plug);
}

/*
 *   This is the function that actually matches an EXPECT against
 * a candidate expect string.  If the match succeeds then it either
 * returns TRUE, or if there's an interpretation to be done it
 * goes through and sets all the Interpretation "val" fields, for
 * the semantics functions to later find.
 */
static bool _match_regex(Device * dev, char *expect)
{
    Action *act;
    regex_t *re;
    int n;
    size_t nmatch = MAX_MATCH;
    regmatch_t pmatch[MAX_MATCH];
    int eflags = 0;
    Interpretation *interp;
    ListIterator map_i;

    CHECK_MAGIC(dev);
    act = list_peek(dev->acts);
    assert(act != NULL);
    CHECK_MAGIC(act);
    assert(act->cur != NULL);
    assert(act->cur->type == EL_EXPECT);

    re = &(act->cur->s_or_e.expect.exp);
    n = Regexec(re, expect, nmatch, pmatch, eflags);

    if (n != REG_NOERROR)
	return FALSE;
    if (pmatch[0].rm_so == -1 || pmatch[0].rm_eo == -1)
	return FALSE;
    assert(pmatch[0].rm_so <= strlen(expect));

    /* 
     *  The "foreach" construct assumes that the initializer is never NULL
     * but instead points to a header record to be skipped.  In this case
     * the initializer can be NULL, though.  That is how we signal there
     * is no subexpression to be interpreted.  If it is non-NULL then it
     * is the header record to an interpretation for the device.
     */
    if (act->cur->s_or_e.expect.map == NULL)
	return TRUE;

    map_i = list_iterator_create(act->cur->s_or_e.expect.map);
    while ((interp = list_next(map_i))) {
	assert((pmatch[interp->match_pos].rm_so < MAX_BUF) &&
	       (pmatch[interp->match_pos].rm_so >= 0));
	assert((pmatch[interp->match_pos].rm_eo < MAX_BUF) &&
	       (pmatch[interp->match_pos].rm_eo >= 0));
	interp->val = expect + pmatch[interp->match_pos].rm_so;
    }
    list_iterator_destroy(map_i);
    return TRUE;
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

	/* 
	 * If we are not in the read set always, then select will
	 * not unblock if the connection is dropped.
	 */
	FD_SET(dev->fd, rset);
	*maxfd = MAX(*maxfd, dev->fd);

	/*
	 * Need to be in the write set if we are sending scripted text.
	 */
	if (dev->connect_status == DEV_CONNECTED) {
	    if ((dev->script_status & DEV_SENDING)) {
		FD_SET(dev->fd, wset);
		*maxfd = MAX(*maxfd, dev->fd);
	    }
	}

	/*
	 * Descriptor will become writable after a connect.
	 */
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
	    if (_time_to_reconnect(dev, timeout) && !_reconnect(dev))
		continue;
	}

	if (dev->fd == NO_FD)
	    continue;

	/* complete non-blocking connect + "log in"  to the device */
	if ((dev->connect_status == DEV_CONNECTING)) {
	    if (FD_ISSET(dev->fd, rset) || FD_ISSET(dev->fd, wset))
		if (!_finish_connect(dev, timeout))
		    continue;
	}
	
	/* read/write from/to buffer */
	if (FD_ISSET(dev->fd, rset))
	    _handle_read(dev);
	if (FD_ISSET(dev->fd, wset))
	    _handle_write(dev);

	/* in case of I/O or timeout, process scripts (expect/send/delay) */
	if ((dev->connect_status & DEV_CONNECTED) && list_peek(dev->acts))
	    _process_script(dev, timeout);
    }
    list_iterator_destroy(itr);
}

/*
 * vi:softtabstop=4
 */
