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
#include <syslog.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#include "powerman.h"
#include "list.h"
#include "config.h"
#include "device.h"
#include "action.h"
#include "error.h"
#include "wrappers.h"
#include "buffer.h"
#include "util.h"
#include "hostlist.h"

static void _set_targets(Device * dev, Action * sact);
static Action *_do_target_copy(Device * dev, Action * sact, char *target);
static void _do_target_some(Device * dev, Action * sact);
static bool _reconnect(Device * dev);
static bool _finish_connect(Device * dev, struct timeval *timeout);
bool _time_to_reconnect(Device *dev, struct timeval *timeout);
static void _disconnect(Device * dev);
static bool _process_expect(Device * dev, struct timeval *timeout);
static bool _process_send(Device * dev);
static bool _process_delay(Device * dev, struct timeval *timeout);
static void _do_device_semantics(Device * dev, List map);
static bool _match_regex(Device * dev, char *expect);
static void _acttodev(Device * dev, Action * sact);
static int _match_name(Device * dev, void *key);
static void _handle_read(Device * dev);
static void _handle_write(Device * dev);
static void _process_script(Device * dev, struct timeval *timeout);
static void _recover(Device * dev);
static bool _timeout(Device *dev, struct timeval * timeout,
		struct timeval *timeleft);

static List dev_devices = NULL;

/* NOTE: array positions correspond to values of PM_* in action.h */
static char *command_str[] = {
    "PM_LOG_IN", "PM_LOG_OUT", "PM_UPDATE_PLUGS", "PM_UPDATE_NODES", 
    "PM_POWER_ON", "PM_POWER_OFF", "PM_POWER_CYCLE", "PM_RESET"
};

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
    char *str = util_memstr(mem, len);

    printf("S(%s): %s\n", dev->name, str);
    Free(str);
}

/*
 * Telemetry logging callback for Buffer incoming from device.
 */
static void _buflogfun_from(unsigned char *mem, int len, void *arg)
{
    Device *dev = (Device *) arg;
    char *str = util_memstr(mem, len);

    printf("D(%s): %s\n", dev->name, str);
    Free(str);
}

/*
 * Test whether timeout has occurred
 *  time_stamp (IN) 
 *  timeout (IN)
 *  timeleft (OUT)	if timeout has not occurred, put time left here
 *  RETURN		TRUE if (time_stamp + timeout > now)
 */
static bool _timeout(Device *dev, struct timeval *timeout,
		struct timeval *timeleft)
{
    struct timeval now;
    struct timeval limit;
    bool result = FALSE;
					    /* limit = time_stamp + timeout */
    timeradd(&dev->time_stamp, timeout, &limit); 

    Gettimeofday(&now, NULL);

    if (timercmp(&now, &limit, >))	    /* if now > limit */
	result = TRUE;

    if (result == FALSE)
	timersub(&limit, &now, timeleft);   /* timeleft = limit - now */
    else
	timerclear(timeleft);
	
    if (dev->logit) {
	if (result)
	    printf("_timeout(%s): now!\n", dev->name);
	else
	    printf("_timeout(%s): in %ld.%-6.6ld seconds\n", dev->name,
			    timeleft->tv_sec, timeleft->tv_usec);
    }
	
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

	    if (!_timeout(dev, &retry, &timeleft))
		reconnect = FALSE;
	    if (timeout && !reconnect)
		_update_timeout(timeout, &timeleft);
    }
#if 0
    if (dev->logit)
	printf("_time_to_reconnect(%d): %s\n", dev->reconnect_count,
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

    if (dev->logit)
	printf("_reconnect: %s\n", dev->name);

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

    if (!dev->to) {
	dev->to = buf_create(dev->fd, MAX_BUF,
			      dev->logit ? _buflogfun_to : NULL, dev);
    }
    if (!dev->from) {
	dev->from = buf_create(dev->fd, MAX_BUF,
				dev->logit ? _buflogfun_from : NULL, dev);
    }

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

    return (dev->connect_status == DEV_CONNECTED);
}


/*
 *   dev arrives here with "targets" filled in to let us know to which 
 * device components to apply the action.  
 *   We need to create the action and initiate the first step in
 * the send/expect script.
 */
static void _acttodev(Device * dev, Action * sact)
{
    Action *act;

    assert(dev != NULL);
    CHECK_MAGIC(dev);
    assert(sact != NULL);
    CHECK_MAGIC(sact);

    if (!(dev->script_status & DEV_LOGGED_IN) && (sact->com != PM_LOG_IN)) {
        syslog(LOG_ERR, "Attempt to initiate Action %s while not logged in", 
			         command_str[sact->com]);
        return;
    }
    /* Some devices do not implemnt some actions - ignore */
    if (dev->prot->scripts[sact->com] == NULL)
        return;

    /* This actually creates the one or more Actions for the Device */
    _set_targets(dev, sact);

    /* Look at action at the head of the Device's queue */
    /* and start its script */
    act = list_peek(dev->acts);
    if (act == NULL)
        return;
    assert(act->itr != NULL);
    /* act->itr is always pointing at the next unconsumed Script_El
     * act->cur is always the one being worked on
     * We may process one or more, but probably not the whole list
     * before returning to the select() loop.
     */
    /*
     * FIXME:  Review this and explain why we can move this 
     * startup processing down to _process_script() where it 
     * belongs. 
     */
    while ((act->cur = list_next(act->itr))) {
	switch (act->cur->type) {
	case EL_SEND:
	    syslog(LOG_DEBUG, "start script");
	    dev->script_status |= DEV_SENDING;
	    _process_send(dev);
	    break;
	case EL_EXPECT:
	    Gettimeofday(&dev->time_stamp, NULL);
	    dev->script_status |= DEV_EXPECTING;
	    return;
	case EL_DELAY:
	    Gettimeofday(&dev->time_stamp, NULL);
	    dev->script_status |= DEV_DELAYING;
	    return;
	case EL_NONE:
	default:
	}
    }
    /* mostly we'll return from inside the while loop, but if
     * we get to the end of the script after a SEND then we can
     * free the action now rather than waiting for an expect to
     * complete. 
     */
    act_del_queuehead(dev->acts);
}

void dev_apply_action(Action *act)
{
    Device *dev;
    ListIterator itr;

    itr = list_iterator_create(dev_devices);
    while ((dev = list_next(itr)))
        _acttodev(dev, act);
    list_iterator_destroy(itr);
}

/* 
 *   There are several cases of what the desired end result is. 
 * Only PM_LOG_IN pushes its action onto the front of the queue.
 * Nothing can happen until we get logged in, anyway.  If there 
 * were some other action onging when a connection was lost,
 * then it needs its script restarted.
 *   The next five cases always target all nodes, so just create
 * a new Action for the device and queue it up.
 *   The last five take specific targets for their Action.  In
 * LITERL mode you have to go through the Plugs one by one
 * looking for regex matches with the target.  IN REGEX mode
 * you can copy the regex right into the target device's 
 * Action and then queu it up.
 */
void _set_targets(Device * dev, Action * sact)
{
    Action *act;

    switch (sact->com) {
        case PM_LOG_IN:
            /* reset script of preempted action so it starts over */
            if (!list_is_empty(dev->acts)) {
                act = list_peek(dev->acts);
                list_iterator_reset(act->itr);
            }

            act = _do_target_copy(dev, sact, NULL);
            list_push(dev->acts, act);
            break;
        case PM_LOG_OUT:
        case PM_UPDATE_PLUGS:
        case PM_UPDATE_NODES:
            act = _do_target_copy(dev, sact, NULL);
            list_append(dev->acts, act);
            break;
        case PM_POWER_ON:
        case PM_POWER_OFF:
        case PM_POWER_CYCLE:
        case PM_RESET:
            assert(sact->target != NULL);
	    _do_target_some(dev, sact);
            break;
        default:
            assert(FALSE);
    }

    return;
}


/* 
 *   The new Device Action is going to get its info directly from 
 * the Server Action, except we've stated it's target explicitly in 
 * the parameters list.  A NULL target means leave the Action's
 * target NULL as well (it comes that way from act_create()).
 */
Action *_do_target_copy(Device * dev, Action * sact, char *target)
{
    Action *act;

    act = act_create(sact->com);
    act->client = sact->client;
    act->seq = sact->seq;
    act->itr = list_iterator_create(dev->prot->scripts[act->com]);
    if (target != NULL)
	act->target = Strdup(target);
    return act;
}

/* 
 *   This is the tricky case for _set_targets().  We have a target in
 * the Server Action, and it is a hostlist.  We have to sequence through
 * each plug and see if the node connected to the plug has a name
 * that matches that RegEx.  If it does we add it to a tentative list
 * of new Actions for the Device, but we're not through.  If all of
 * the plugs' nodes match then we want to send the Device's special
 * "all" signal (every device has one).  Conversely, no plugs match
 * then no actions are added.  
 */
void _do_target_some(Device * dev, Action * sact)
{
    Action *act;
    List new_acts;
    bool all = TRUE, any = FALSE;
    Plug *plug;
    ListIterator plug_i;

    new_acts = list_create((ListDelF) act_destroy);
    plug_i = list_iterator_create(dev->plugs);
    while ((plug = list_next(plug_i))) {
        /* If plug->node == NULL it means that there is no node pluged
         * into that plug, not not managed by powerman.  Never use the
         * all in this case
         */
        if (plug->node == NULL) {
            all = FALSE;
            continue;
        }

        /* check if plug->node->name matches the target */
        if (hostlist_find(sact->hl, plug->node->name) != -1) {
            any = TRUE;
            act = _do_target_copy(dev, sact, plug->name);
            list_append(new_acts, act);
        } else {
            all = FALSE;
        }
    }

    if (all) {
        act = _do_target_copy(dev, sact, dev->all);
        list_append(dev->acts, act);
    } else {
        if (any)
            while ((act = list_pop(new_acts)))
                list_append(dev->acts, act);
    }

    list_iterator_destroy(plug_i);
    list_destroy(new_acts);
}


/*
 *   We've supposedly reconnected, so check if we really are.
 * If not go into reconnect mode.  If this is a succeeding
 * reconnect, log that fact.  When all is well initiate the
 * logon script.
 */
static bool _finish_connect(Device *dev, struct timeval *tv)
{
    int rc;
    int err = 0;
    int len = sizeof(err);
    Action *act;

    assert(dev->connect_status == DEV_CONNECTING);
    rc = getsockopt(dev->fd, SOL_SOCKET, SO_ERROR, &err, &len);
    /*
     *  If an error occurred, Berkeley-derived implementations
     *    return 0 with the pending error in 'err'.  But Solaris
     *    returns -1 with the pending error in 'errno'.  -dun
     */
    if (rc < 0)
	err = errno;
    if (err) {
	syslog(LOG_INFO, "connect to %s: %m\n", dev->name);
	if (dev->logit)
	    printf("_finish_connect: %s: %s\n", dev->name, strerror(errno));
	_disconnect(dev);
	if (_time_to_reconnect(dev, tv))
	    _reconnect(dev);
    } else {
	syslog(LOG_INFO, "_finish_connect: %s connected\n", dev->name);
	dev->connect_status = DEV_CONNECTED;
	act = act_create(PM_LOG_IN);
	_acttodev(dev, act);
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
    if ((n < 0) && (errno == EWOULDBLOCK))
	return;
    if ((n == 0) || ((n < 0) && (errno == ECONNRESET))) {
	syslog(LOG_ERR, "read error on %s", dev->name);
	if (dev->logit)
	    printf("read error on %s\n", dev->name);
	_disconnect(dev);
	dev->reconnect_count = 0;
	_reconnect(dev);
	return;
    }
}

static void _disconnect(Device *dev)
{
    Action *act;

    assert(dev->connect_status == DEV_CONNECTING 
		    || dev->connect_status == DEV_CONNECTED);

    if (dev->logit)
	printf("_disconnect: %s\n", dev->name);

    /* close socket if open */
    if (dev->fd != NO_FD) {
	Close(dev->fd);
	dev->fd = NO_FD;
    }

    /* flush buffers */
    if (dev->from != NULL)
	buf_clear(dev->from);
    if (dev->to != NULL)
	buf_clear(dev->to);

    /* update state */
    dev->connect_status = DEV_NOT_CONNECTED;
    dev->script_status = 0;

    /* delete PM_LOG_IN action queued for this device, if any */
    if (((act = list_peek(dev->acts)) != NULL) && (act->com == PM_LOG_IN))
	act_del_queuehead(dev->acts);
}

/*
 *   If there's stuff in the incoming buffer then EXPECTs can 
 * be processed.  SENDs and DELAYs can always be processed.  
 * The processing is done when there's nothing on the current 
 * script left to do, and there's not more actions.  Or the
 * processing is done if we're stuck at an EXPECT with no 
 * matching stuff in the input buffer.
 */
static void _process_script(Device * dev, struct timeval *timeout)
{
    Action *act = list_peek(dev->acts);
    bool script_incomplete = FALSE;

    if (act == NULL)
	return;
    assert(act->cur != NULL);
    if ((act->cur->type == EL_EXPECT) && buf_isempty(dev->from))
	return;

    /* 
     * The condition for "script_incomplete" is:
     * 1) There is nothing I can interpret in dev->from
     *   a) buf_isempty(dev->from), or
     *   b) find_Reg_Ex() fails
     * and
     * 2) There's no sends to process
     *   a)  (act = top_Action(dev->acts)) == NULL, or
     *   b)  script_el->type == EL_EXPECT
     */
    while (!script_incomplete) {
	switch (act->cur->type) {
	case EL_EXPECT:
	    script_incomplete = _process_expect(dev, timeout);
	    if (script_incomplete)
		return;
	    break;
	case EL_DELAY:
	    script_incomplete = _process_delay(dev, timeout);
	    if (script_incomplete)
		return;
	    break;
	case EL_SEND:
	    script_incomplete = _process_send(dev);
	    break;
	default:
	    err_exit(FALSE, "unrecognized Script_El type %d", act->cur->type);
	}
	assert(act->itr != NULL);
	if ((act->cur = list_next(act->itr)) == NULL) {
	    if (act->com == PM_LOG_IN)
		dev->script_status |= DEV_LOGGED_IN;
	    act_del_queuehead(dev->acts);
	} else {
	    if (act->cur->type == EL_EXPECT) {
		Gettimeofday(&dev->time_stamp, NULL);
		dev->script_status |= DEV_EXPECTING;
	    }
	}
	act = list_peek(dev->acts);
	if (act == NULL)
	    return;
	else if (act->cur == NULL)
	    act->cur = list_next(act->itr);
    }
}

/*
 *   The next Script_El is an EXPECT.  Indeed we know exactly what to
 * be expecting, so we go to the buffer and look to see if there's a
 * possible new input.  If the regex matches, extract the expect string.
 * If there is an Interpretation map for the expect then send that info to
 * the semantic processor.  
 */
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
	Gettimeofday(&dev->time_stamp, NULL);
    }

    re = &act->cur->s_or_e.expect.exp;

    if ((expect = buf_getregex(dev->from, re))) {	/* match */

	/* process values of parenthesized subexpressions */
	if (act->cur->s_or_e.expect.map != NULL) {
	    bool res = _match_regex(dev, expect);

	    assert(res == TRUE); /* since the first regexec worked ... */
	    _do_device_semantics(dev, act->cur->s_or_e.expect.map);
	}
	Free(expect);
	finished = TRUE;
    } else if (_timeout(dev, &dev->timeout, &timeleft)) { /* timeout? */
	/*_recover(dev);*/	/* FIXME: need to record failure for client!!! */
	finished = TRUE;
    } else {						/* keep trying */
	_update_timeout(timeout, &timeleft);
    }

    if (dev->logit) {
	if (finished) {
	    printf("_process_expect(%s): match\n", dev->name);
	} else {
	    unsigned char mem[MAX_BUF];
	    int len = buf_peekstr(dev->from, mem, MAX_BUF);
	    char *str = util_memstr(mem, len);

	    printf("_process_expect(%s): no match: '%s'\n", dev->name, str);

	    Free(str);
	}
    }

    if (finished)
	dev->script_status &= ~DEV_EXPECTING;

    return !finished;
}

/*
 *   buf_printf() does al the real work.  send.fmt has a string
 * printf(fmt, ...) style.  If it has a "%s" then call buf_printf
 * with the target as its last argument.  Otherwise just send the
 * string and update the device's status.  The TRUE return value
 * is for the while( ! done ) loop in _process_script().
 */
bool _process_send(Device * dev)
{
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

    return TRUE;
}

/* return TRUE if delay is not finished yet */
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
	if (dev->logit)
	    printf("_process_delay(%s): %ld.%-6.6ld \n", dev->name, 
			    delay.tv_sec, delay.tv_usec);
	dev->script_status |= DEV_DELAYING;
	Gettimeofday(&dev->time_stamp, NULL);
    }

    /* timeout expired? */
    if (_timeout(dev, &delay, &timeleft)) {
	dev->script_status &= ~DEV_DELAYING;
	finished = TRUE;
    } else
	_update_timeout(timeout, &timeleft);
    
    return !finished;
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
	    if ((pos = util_findregex(re, str, len)) != NULL)
		interp->node->p_state = ST_ON;
	    re = &(dev->off_re);
	    if ((pos = util_findregex(re, str, len)) != NULL)
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
	    if ((pos = util_findregex(re, str, len)) != NULL)
		interp->node->n_state = ST_ON;
	    re = &(dev->off_re);
	    if ((pos = util_findregex(re, str, len)) != NULL)
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


/*
 *   When a Device drops its connection you have to clear out its
 * Actions, and set its nodes and plugs to an UNKNOWN state.
 */
static void _recover(Device * dev)
{
    Plug *plug;
    ListIterator itr;

    assert(dev != NULL);

    syslog(LOG_ERR, "expect timeout on %s", dev->name);
    if (dev->logit)
        printf("expect timeout %s\n", dev->name);

    /* dequeue all actions for this device */
    while (!list_is_empty(dev->acts))
	act_del_queuehead(dev->acts);

    itr = list_iterator_create(dev->plugs);
    while ((plug = list_next(itr))) {
	if (plug->node != NULL) {
	    plug->node->p_state = ST_UNKNOWN;
	    plug->node->n_state = ST_UNKNOWN;
	}
    }
    list_iterator_destroy(itr);

    _disconnect(dev);
    dev->reconnect_count = 0;
    _reconnect(dev);
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
    dev->acts = list_create((ListDelF) act_destroy);
    Gettimeofday(&dev->time_stamp, NULL);
    timerclear(&dev->timeout);
    dev->to = NULL;
    dev->from = NULL;
    dev->prot = NULL;
    dev->num_plugs = 0;
    dev->plugs = list_create((ListDelF) dev_plug_destroy);
    dev->logit = FALSE;
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
  /*
       Dev_Type
 */

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

/*
 *   This comes into use in the parser when the devices have been 
 * parsed into a list and a node line referes to a plug by its name.   
 */
int dev_plug_match(Plug * plug, void *key)
{
    return (strcmp(((Plug *)plug)->name, (char *)key) == 0);
}


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
void dev_initial_connect(bool logit)
{
    Device *dev;
    ListIterator itr;

    itr = list_iterator_create(dev_devices);
    while ((dev = list_next(itr))) {
	assert(dev->connect_status == DEV_NOT_CONNECTED);
	dev->logit = logit;
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

    itr = list_iterator_create(dev_devices);
    while ((dev = list_next(itr))) {
	if (dev->fd < 0)
	    continue;

	/* To handle telnet I'm having it always ready to read.  */
	FD_SET(dev->fd, rset);
	*maxfd = MAX(*maxfd, dev->fd);

	/* The descriptor becomes writable when a non-blocking connect 
	 * (ie, DEV_CONNECTING) completes. */
	if ((dev->connect_status == DEV_CONNECTING) 
			|| (dev->script_status & DEV_SENDING))
	    FD_SET(dev->fd, wset);
    }
    list_iterator_destroy(itr);
}

/* 
 * Called after select to process ready file descriptors, timeouts, etc.
 */
void dev_post_select(fd_set *rset, fd_set *wset, struct timeval *timeout)
{
    Device *dev;
    Action *act;
    ListIterator itr;
    bool scripts_complete = TRUE;
    /* Only when all device are idle is the cluster Quiescent */
    /* and only then can a new action be initiated from the queue. */
    static enum { STAT_QUIESCENT, STAT_OCCUPIED } status = STAT_QUIESCENT;


    itr = list_iterator_create(dev_devices);

    while ((dev = list_next(itr))) {
	bool scripts_need_service = FALSE;

	/* 
	 * (Re)connect if device not connected.
	 * If we are still waiting for a connect timeout, modify 'timeout' 
	 * so select can wake up when it expires and run us again.
	 */
	if (dev->connect_status == DEV_NOT_CONNECTED 
			&& _time_to_reconnect(dev, timeout)) {
	    if (!_reconnect(dev))
		continue;
	}
	if (dev->fd == NO_FD)
	    continue;

	/* The first activity is always the signal of a newly connected device.
	 * Run the log in script to get back into business as usual. */
	if ((dev->connect_status == DEV_CONNECTING)) {
	    if (FD_ISSET(dev->fd, rset) || FD_ISSET(dev->fd, wset))
		if (!_finish_connect(dev, timeout))
		    continue;
	}
	assert(dev->fd >= 0);
	if (FD_ISSET(dev->fd, rset)) {
	    _handle_read(dev);
	    scripts_need_service = TRUE;
	}
	if (FD_ISSET(dev->fd, wset)) {
	    _handle_write(dev);
	    scripts_need_service = TRUE;
	}
	/* 
	 * Since I/O took place we need to see if the scripts should run.
	 * If we are processing a delay or expect, we may modify 'timeout' 
	 * so select can wake up when timeout expires and run us again.
	 */
	if (scripts_need_service || (dev->script_status & DEV_DELAYING)
			|| (dev->script_status & DEV_EXPECTING))
	    _process_script(dev, timeout);

	/* XXX doing this in _process_expect now */
	/* stalled on an expect */
	if ((dev->script_status & DEV_EXPECTING)) {
	    if (_timeout(dev, &dev->timeout, timeout))
		_recover(dev);
	}

	if (dev->script_status & (DEV_SENDING | DEV_EXPECTING))
	    scripts_complete = FALSE;
    }

    list_iterator_destroy(itr);

    /* launch the next action in the queue */
    if (scripts_complete && ((act = act_find()) != NULL)) {
	/* a previous action may need a reply sent back to a client */
	if (status == STAT_OCCUPIED) {
	    act_finish(act);
	    status = STAT_QUIESCENT;
	}

	/* double check - if there was an action in the queue, launch it */
	if ((act = act_find()) != NULL) {
	    dev_apply_action(act);
	    status = STAT_OCCUPIED;
	}
    }
}

/*
 * vi:softtabstop=4
 */
