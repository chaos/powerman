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
static void _do_reconnect(Device * dev);
static bool _process_expect(Device * dev);
static bool _process_send(Device * dev);
static bool _process_delay(Device * dev);
static void _do_device_semantics(Device * dev, List map);
static void _do_pmd_semantics(Device * dev, List map);
static bool _match_regex(Device * dev, char *expect);
static void _acttodev(Device * dev, Action * sact);
static int _match_name(Device * dev, void *key);

static List dev_devices = NULL;

/* NOTE: array positions correspond to values of PM_* in action.h */
static char *command_str[] = {
    "PM_ERROR", "PM_LOG_IN", "PM_CHECK_LOGIN", "PM_LOG_OUT", "PM_UPDATE_PLUGS",
    "PM_UPDATE_NODES", "PM_POWER_ON", "PM_POWER_OFF", "PM_POWER_CYCLE",
    "PM_RESET", "PM_NAMES"
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
 *   This is called for each device to get it started on its
 * connection.  It probably will be after a few trips through the 
 * main select() loop before the devices actually connect.
 *   The first three device types connect in more or less the same 
 * way.  A TTY_DEV would be easily initiated by connecting a 
 * file descriptor to some /dev/tty entry.  I haven't implemented
 * this, and we don't plan on putting any such equipment into 
 * production soon, so I'm not worried about it.
 */
static void _start_dev(Device * dev, bool logit)
{
    dev->logit = logit;
    switch (dev->type) {
	case TCP_DEV:
	case PMD_DEV:
	case TELNET_DEV:
	    dev_nb_connect(dev);
	    break;
	case TTY_DEV:
	    err_exit(FALSE, "tty device is unimplemented");
	case SNMP_DEV:
	    err_exit(FALSE, "snmp device is unimplemented");
	case NO_DEV:
	default:
	    err_exit(FALSE, "attempt to start unimplemented device");
    }
}


/* initialize all the power control devices */
void dev_start_all(bool logit)
{
    Device *dev;
    ListIterator itr = list_iterator_create(dev_devices);

    while ((dev = list_next(itr)))
	_start_dev(dev, logit);
    list_iterator_destroy(itr);
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
 *   The dev struct is initialized with all the needed host info.
 * This is my copying of Stevens' nonblocking connect.  After we 
 * have a file descriptor we create buffers for sending and 
 * receiving.  At the bottom, in the unlikely event the the 
 * connect completes immediately we launch the log in script.
 */
void dev_nb_connect(Device * dev)
{
    int n;
    struct addrinfo hints, *addrinfo;
    int sock_opt;
    int fd_settings;

    CHECK_MAGIC(dev);
    assert((dev->type == TCP_DEV) || (dev->type == PMD_DEV) ||
	   (dev->type == TELNET_DEV));
    assert(dev->status == DEV_NOT_CONNECTED);
    assert(dev->fd == -1);

    memset(&hints, 0, sizeof(struct addrinfo));
    addrinfo = &hints;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    switch (dev->type) {
    case TCP_DEV:
	Getaddrinfo(dev->devu.tcpd.host, dev->devu.tcpd.service, &hints, &addrinfo);
	break;
    case PMD_DEV:
	Getaddrinfo(dev->devu.pmd.host, dev->devu.pmd.service, &hints, &addrinfo);
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
    n = Connect(dev->fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
    if (n >= 0) {
	Action *act;

	dev->error = FALSE;
	dev->status = DEV_CONNECTED;
	act = act_create(PM_LOG_IN);
	_acttodev(dev, act);
    } else {
	dev->status = DEV_CONNECTING;
    }
    freeaddrinfo(addrinfo);
}


/*
 * Called from dev_connect() (or possibly 
 * dev_nb_connect()) with a PM_LOG_IN when connection 
 * is first established.  Otherwise, called from  act_initiate() for 
 * other actions.  
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

    if (!dev->loggedin && (sact->com != PM_LOG_IN)) {
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
     * startup processing down to dev_process_script() where it 
     * belongs. 
     */
    while ((act->cur = list_next(act->itr))) {
	switch (act->cur->type) {
	case EL_SEND:
	    syslog(LOG_DEBUG, "start script");
	    dev->status |= DEV_SENDING;
	    _process_send(dev);
	    break;
	case EL_EXPECT:
	    Gettimeofday(&(dev->time_stamp), NULL);
	    dev->status |= DEV_EXPECTING;
	    return;
	case EL_DELAY:
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
        case PM_ERROR:
        case PM_CHECK_LOGIN:
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
        case PM_NAMES:
            assert(sact->target != NULL);

            if (dev->prot->mode == SM_LITERAL) {
                _do_target_some(dev, sact);
            } else {		/* dev->prot->mode == REGEX */
                act = _do_target_copy(dev, sact, sact->target);
                list_append(dev->acts, act);
            }
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
void dev_connect(Device * dev)
{
    int rc;
    int err = 0;
    int len = sizeof(err);
    Action *act;

    rc = getsockopt(dev->fd, SOL_SOCKET, SO_ERROR, &err, &len);
    /*
     *  If an error occurred, Berkeley-derived implementations
     *    return 0 with the pending error in 'err'.  But Solaris
     *    returns -1 with the pending error in 'errno'.  -dun
     */
    if (rc < 0)
	err = errno;
    if (err) {
	close(dev->fd);
	dev->fd = -1;
	dev->error = TRUE;
	dev->status = DEV_NOT_CONNECTED;
	buf_clear(dev->to);
	buf_clear(dev->from);
	syslog(LOG_INFO, "Failure attempting to connect to %s: %m\n", dev->name);
	/*
	 *  Back in the main loop after the update interval has passed,
	 *  device will attempt another dev_nb_connect().
	 */
	return;
    } else if (dev->error) {
	syslog(LOG_INFO, "Connection to %s re-established\n", dev->name);
    }
    dev->error = FALSE;
    dev->status = DEV_CONNECTED;
    act = act_create(PM_LOG_IN);
    _acttodev(dev, act);
}

/*
 *   The Read gets the string.  If it is EOF then we've lost 
 * our connection with the device and need to reconnect.  
 * buf_read() does the real work.  If something arrives,
 * that is handled in dev_process_script().
 */
void dev_handle_read(Device * dev)
{
    int n;

    CHECK_MAGIC(dev);
    n = buf_read(dev->from);
    if ((n < 0) && (errno == EWOULDBLOCK))
	return;
    if ((n == 0) || ((n < 0) && (errno == ECONNRESET))) {
	syslog(LOG_ERR, "Device read problem, reconnecting to %s", dev->name);
	if (dev->logit)
	    printf("Device read problem, reconnecting to: %s\n", dev->name);
	_do_reconnect(dev);
	return;
    }
}

/*
 *   Reset the Device's state and restart the whole nonblocking
 * connect interction.
 * If EOF on a device connection means we lost the connection then I 
 * need to reconnect.  Once reconnected a LOG_IN will automatically
 * be generated.  If the current action was a LOG_IN then just delete
 * it, otherwise leave it for completion after we're reconnected.
 */
void _do_reconnect(Device * dev)
{
    Action *act;


    Close(dev->fd);
    dev->fd = -1;

    assert(dev->from != NULL);
    buf_destroy(dev->from);
    dev->from = NULL;

    assert(dev->to != NULL);
    buf_destroy(dev->to);
    dev->to = NULL;

    dev->status = DEV_NOT_CONNECTED;
    dev->loggedin = FALSE;
    if (((act = list_peek(dev->acts)) != NULL) && (act->com == PM_LOG_IN)) {
	act_del_queuehead(dev->acts);
    }
    dev_nb_connect(dev);
    return;
}

/*
 *   If there's stuff in the incoming buffer then EXPECTs can 
 * be processed.  SENDs and DELAYs can always be processed.  
 * The processing is done when there's nothing on the current 
 * script left to do, and there's not more actions.  Or the
 * processing is done if we're stuck at an EXPECT with no 
 * matching stuff in the input buffer.
 */
void dev_process_script(Device * dev)
{
    Action *act = list_peek(dev->acts);
    bool done = FALSE;

    if (act == NULL)
	return;
    assert(act->cur != NULL);
    if ((act->cur->type == EL_EXPECT) && buf_isempty(dev->from))
	return;

    /* 
     * The condition for "done" is:
     * 1) There is nothing I can interpret in dev->from
     *   a) buf_isempty(dev->from), or
     *   b) find_Reg_Ex() fails
     * and
     * 2) There's no sends to process
     *   a)  (act = top_Action(dev->acts)) == NULL, or
     *   b)  script_el->type == EL_EXPECT
     */
    while (!done) {
	switch (act->cur->type) {
	case EL_EXPECT:
	    done = _process_expect(dev);
	    /* 
	     * XXX Band-aid (jg)
	     * Without this return, a short read causes the test 
	     * for act->cur != NULL at the top of this function
	     * to fail next time the fd is ready and we go here
	     * (e.g. when more of the buffer is filled).
	     * This needs to be revisited and fixed better...
	     */
	    if (done)
		return;
	    break;
	case EL_SEND:
	    done = _process_send(dev);
	    break;
	case EL_DELAY:
	    done = _process_delay(dev);
	    break;
	default:
	    err_exit(FALSE, "unrecognized Script_El type %d", act->cur->type);
	}
	if ((act->cur = list_next(act->itr)) == NULL) {
	    if (act->com == PM_LOG_IN)
		dev->loggedin = TRUE;
	    act_del_queuehead(dev->acts);
	} else {
	    if (act->cur->type == EL_EXPECT) {
		gettimeofday(&(dev->time_stamp), NULL);
		dev->status |= DEV_EXPECTING;
	    }
	}
	act = list_peek(dev->acts);
	if (act == NULL)
	    done = TRUE;
	else if (act->cur == NULL)
	    act->cur = list_next(act->itr);
    }
}

/*
 *   The next Script_El is an EXPECT.  Indeed we know exactly what to
 * be expecting, so we go to the buffer and look to see if there's a
 * possible new input.  If the regex matches, extract the expect string.
 * If there is an Interpretation map for the expect then send that info to
 * the semantic processors.  There are two because PMD_DEV devices
 * are handled differently (a PMD_DEV is an instance of powermand
 * acting as intermediary for another powermand in a distributed 
 * control arrangement).   
 */
bool _process_expect(Device * dev)
{
    regex_t *re;
    Action *act = list_peek(dev->acts);
    char *expect;
    bool res;

    assert(act != NULL);
    assert(act->cur != NULL);
    assert(act->cur->type == EL_EXPECT);

    re = &(act->cur->s_or_e.expect.exp);

    if ((expect = util_bufgetregex(dev->from, re)) == NULL) {
	if (dev->logit) {
	    unsigned char mem[MAX_BUF];
	    int len = buf_peekstr(dev->from, mem, MAX_BUF);
	    char *str = util_memstr(mem, len);

	    printf("_process_expect(%s): no match: '%s'\n", dev->name, str);

	    Free(str);
	}
	return TRUE;
    }

    /*
     * We already matched the regular expression in util_bufgetregex
     * but now we need to process values of parenthesized subexpressions.
     */
    dev->status &= ~DEV_EXPECTING;
    res = _match_regex(dev, expect);
    assert(res == TRUE); /* the first regexec worked, this one should too */

    if (act->cur->s_or_e.expect.map != NULL) {
	if (dev->type == PMD_DEV)
	    _do_pmd_semantics(dev, act->cur->s_or_e.expect.map);
	else
	    _do_device_semantics(dev, act->cur->s_or_e.expect.map);
    }
    Free(expect);

    if (dev->logit)
	printf("_process_expect(%s): match\n", dev->name);
    return FALSE;
}

/*
 *   buf_printf() does al the real work.  send.fmt has a string
 * printf(fmt, ...) style.  If it has a "%s" then call buf_printf
 * with the target as its last argument.  Otherwise just send the
 * string and update the device's status.  The TRUE return value
 * is for the while( ! done ) loop in dev_process_script().
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

    dev->status |= DEV_SENDING;

    return TRUE;
}


/*
 *   This just mediates a call to Delay() which uses and empty
 * Select() to cause struct timeval tv worth of delay.
 */
bool _process_delay(Device * dev)
{
    Action *act = list_peek(dev->acts);
    struct timeval tv = act->cur->s_or_e.delay.tv;

    assert(act != NULL);
    assert(act->cur != NULL);
    assert(act->cur->type == EL_DELAY);

    if (dev->logit)
        printf("_process_delay(%s): %ld.%-6.6ld \n",
               dev->name, tv.tv_sec, tv.tv_usec);

    Delay(&tv);

    return FALSE;
}


/*
 *   The two update commands are the only ones that change the 
 * internal state of the cluster data structure.  When an 
 * update script completes one sequences through the Plugs/Nodes
 * for the Device and reads the state value picked up by the 
 * interpretation for that node.
 *
 *  The problem is the same for PMD_DEV and other device types.
 * The data is just arranged differently.  For PMD it's always
 * vector of 1's and 0's withas many as the PMD device has nodes.
 */
void _do_pmd_semantics(Device * dev, List map)
{
    Action *act;
    char *ch;
    Plug *plug;
    ListIterator plug_i;
    Interpretation *interp = list_peek(map);

    CHECK_MAGIC(dev);

    act = list_peek(dev->acts);
    assert(act != NULL);

    ch = interp->val;
    plug_i = list_iterator_create(dev->plugs);
    switch (act->com) {
    case PM_UPDATE_PLUGS:
	while ((plug = list_next(plug_i))) {
	    if (plug->node == NULL)
		continue;
	    plug->node->p_state = ST_UNKNOWN;
	    if (*(ch) == '1')
		plug->node->p_state = ST_ON;
	    if (*(ch) == '0')
		plug->node->p_state = ST_OFF;
	    ch++;
	}
	break;
    case PM_UPDATE_NODES:
	while ((plug = list_next(plug_i))) {
	    if (plug->node == NULL)
		continue;
	    plug->node->n_state = ST_UNKNOWN;
	    if (*(ch) == '1')
		plug->node->n_state = ST_ON;
	    if (*(ch) == '0')
		plug->node->n_state = ST_OFF;
	    ch++;
	}
	break;
    default:
    }
    list_iterator_destroy(plug_i);
}

/*
 *   For non-PMD devices the reply for a device may be in one or 
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
void dev_handle_write(Device * dev)
{
    int n;

    CHECK_MAGIC(dev);

    n = buf_write(dev->to);
    if (n < 0)
	return;
    if (buf_isempty(dev->to))
	dev->status &= ~DEV_SENDING;
}

/*
 *   A device can only be stalled while it's inthe  EXPECTING 
 * state.  The dev->timeout value can be set in the config file.
 */
bool dev_stalled(Device * dev)
{
    return ((dev->status & DEV_EXPECTING) &&
	    util_overdue(&(dev->time_stamp), &(dev->timeout)));
}

/*
 *   When a Device drops its connection you have to clear out its
 * Actions, and set its nodes and plugs to an UNKNOWN state.
 */
void dev_recover(Device * dev)
{
    Plug *plug;
    ListIterator plug_i;

    assert(dev != NULL);

    syslog(LOG_ERR, "Expect timed out, reconnecting to %s", dev->name);
    if (dev->logit)
        printf("Expect timed out, reconnecting to %s\n", dev->name);
    while (!list_is_empty(dev->acts))
	act_del_queuehead(dev->acts);
    plug_i = list_iterator_create(dev->plugs);
    while ((plug = list_next(plug_i))) {
	if (plug->node != NULL) {
	    plug->node->p_state = ST_UNKNOWN;
	    plug->node->n_state = ST_UNKNOWN;
	}
    }
    _do_reconnect(dev);
}

Device *dev_create(const char *name)
{
    Device *dev;

    dev = (Device *) Malloc(sizeof(Device));
    INIT_MAGIC(dev);
    dev->name = Strdup(name);
    dev->type = NO_DEV;
    dev->loggedin = FALSE;
    dev->error = FALSE;
    dev->status = DEV_NOT_CONNECTED;
    dev->fd = NO_FD;
    dev->acts = list_create((ListDelF) act_destroy);
    gettimeofday(&(dev->time_stamp), NULL);
    dev->timeout.tv_sec = 0;
    dev->timeout.tv_usec = 0;
    dev->to = NULL;
    dev->from = NULL;
    dev->prot = NULL;
    dev->num_plugs = 0;
    dev->plugs = list_create((ListDelF) dev_plug_destroy);
    dev->logit = FALSE;
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

/*
 *  Should I free protocol elements and targets here?
 */
void dev_destroy(Device * dev)
{
    CHECK_MAGIC(dev);

    Free(dev->name);
    Free(dev->all);
    if (dev->type == TCP_DEV) {
        Free(dev->devu.tcpd.host);
        Free(dev->devu.tcpd.service);
    } else if (dev->type == PMD_DEV) {
        Free(dev->devu.pmd.host);
        Free(dev->devu.pmd.service);
    }
    list_destroy(dev->acts);
    list_destroy(dev->plugs);
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
    reg_syntax_t cflags = REG_EXTENDED;

    plug = (Plug *) Malloc(sizeof(Plug));
    plug->name = Strdup(name);
    re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
    Regcomp(&(plug->name_re), name, cflags);
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
    regfree(&(plug->name_re));
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
    int len = strlen(expect);

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
    assert(pmatch[0].rm_so <= len);

    /* 
     *  The "foreach" construct assumes that the initializer is never NULL
     * but instead points to a header record to be skipped.  In this case
     * the initializer can be NULL, though.  That is how we signal there
     * is no subexpression to be interpreted.  If it is non-NULL then it
     * is the header record to an interpretation for the device.
     */
    if (act->cur->s_or_e.expect.map == NULL)
	return TRUE;

    /*
     *   Here is where we have a problem with the powermand to pmd 
     * protocol.  PMD_DEV devices do not all have the exact same
     * number of nodes.  Thus we can't write an explicite regex
     * with a substring for each node.  We do, however, know just
     * how many nodes there are, and we know that a pmd replies 
     * with a list of ones and zeros.  So interpret PMD_DEV replies
     * directly, and without recourse to regex recognition.  The
     * rest of the structure should work.  The config file lists
     * exactly one "map" structure for the expects for UPDATE_*. 
     * Just set "val" to point to its corresponding vector of
     * ones and zeros.  Let _do_pmd_semantics() pull it apart.
     */

    if (dev->type == PMD_DEV) {
	interp = list_peek(act->cur->s_or_e.expect.map);
	interp->val = expect;
	return TRUE;
    }
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

void dev_prepfor_select(fd_set *rset, fd_set *wset, int *maxfd)
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
	if ((dev->status & DEV_CONNECTING) || (dev->status & DEV_SENDING))
	    FD_SET(dev->fd, wset);
    }
    list_iterator_destroy(itr);
}


bool dev_process_select(fd_set *rset, fd_set *wset, bool over_time)
{
    Device *dev;
    ListIterator itr;
    bool active_devs = FALSE;   /* active_devs == FALSE => Quiescent */
    bool activity;              /* activity == TRUE => scripts need service */

    itr = list_iterator_create(dev_devices);

    /* Device reading and writing? */
    while ((dev = list_next(itr))) {
	/* we only initiate device recover once per update period.  
	 * Otherwise we can get swamped with reconnect messages.
	 */
	if (over_time && (dev->status == DEV_NOT_CONNECTED))
	    dev_nb_connect(dev);

	if (dev->fd < 0)
	    continue;

	activity = FALSE;

	/* Any active device is sufficient to suppress starting next action */
	if (dev->status & (DEV_SENDING | DEV_EXPECTING))
	    active_devs = TRUE;

	/* The first activity is always the signal of a newly connected device.
	 * Run the log in script to get back into business as usual. */
	if ((dev->status == DEV_CONNECTING)) {
	    if (FD_ISSET(dev->fd, rset) || FD_ISSET(dev->fd, wset))
		dev_connect(dev);
	    continue;
	}
	if (FD_ISSET(dev->fd, rset)) {
	    dev_handle_read(dev);
	    activity = TRUE;
	}
	if (FD_ISSET(dev->fd, wset)) {
	    struct timeval tv_delay;

	    dev_handle_write(dev);
	    conf_get_write_pause(&tv_delay);
	    Delay(&tv_delay);		/* FIXME: blocks whole select loop! */
	    activity = TRUE;
	}
	/* Since I/O took place we need to see if the scripts should run */
	if (activity)
	    dev_process_script(dev);
	/* dev->timeout may be set in the config file */
	if (dev_stalled(dev))
	    dev_recover(dev);
    }

    list_iterator_destroy(itr);

    return active_devs;
}

/*
 * vi:softtabstop=4
 */
