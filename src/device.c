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

#include "powerman.h"
#include "list.h"
#include "config.h"
#include "device.h"
#include "powermand.h"
#include "action.h"
#include "exit_error.h"
#include "wrappers.h"
#include "pm_string.h"
#include "buffer.h"
#include "log.h"
#include "util.h"

/* prototypes */
static void set_targets(Device *dev, Action *sact);
static Action *do_target_copy(Device *dev, Action *sact, String target);
static void do_target_some(Device *dev, Action *sact);
static void do_Device_reconnect(Device *dev);
static bool process_expect(Device *dev);
static bool process_send(Device *dev);
static bool process_delay(Device *dev);
static void do_Device_semantics(Device *dev, List map);
static void do_PMD_semantics(Device *dev, List map);
static bool match_RegEx(Device *dev, String expect);


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
void 
init_Device(Device *dev)
{
	switch (dev->type)
	{
	case TCP_DEV :
	case PMD_DEV :
	case TELNET_DEV :
		initiate_nonblocking_connect(dev);
		break;
	case TTY_DEV :
	case SNMP_DEV :
		exit_msg("powerman device not yet implemented");
		break;
	case NO_DEV :
	default :
		exit_msg("no such powerman device");
	}
}

/*
 *   The dev struct is initialized with all the needed host info.
 * This is my copying of Stevens' nonblocking connect.  After we 
 * have a file descriptor we create buffers for sending and 
 * receiving.  At the bottom, in the unlikely event the the 
 * connect completes immediately we launch the log in script.
 */
void
initiate_nonblocking_connect(Device *dev)
{
	int n;
	struct addrinfo hints, *addrinfo;
	int sock_opt;
	int fd_settings;

	CHECK_MAGIC(dev);
	assert( (dev->type == TCP_DEV) || (dev->type == PMD_DEV) || 
		(dev->type == TELNET_DEV) );

	memset(&hints, 0, sizeof(struct addrinfo));
	addrinfo = &hints;
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (dev->type == TCP_DEV)
		Getaddrinfo(get_String(dev->devu.tcpd.host), get_String(dev->devu.tcpd.service), 
			    &hints, &addrinfo);
	else if (dev->type == PMD_DEV)
		Getaddrinfo(get_String(dev->devu.pmd.host), get_String(dev->devu.pmd.service), 
			    &hints, &addrinfo);
	else
		exit_msg("That device type is not implemented yet");

	dev->fd = Socket(addrinfo->ai_family, addrinfo->ai_socktype,
			      addrinfo->ai_protocol);
	dev->to = make_Buffer(dev->fd);
	dev->from = make_Buffer(dev->fd);

/* set up and initiate a non-blocking connect */

	sock_opt = 1;
	Setsockopt(dev->fd, SOL_SOCKET, SO_REUSEADDR,
		   &(sock_opt), sizeof(sock_opt));
	fd_settings = Fcntl(dev->fd, F_GETFL, 0);
	Fcntl(dev->fd, F_SETFL, fd_settings | O_NONBLOCK);

	n = Connect(dev->fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
	if ( n >= 0 ) 
	{
		Action *act;

		dev->status = DEV_CONNECTED;
		log_it(0, "Early device connect");
		act = make_Action(PM_LOG_IN);
		map_Action_to_Device(dev, act);
	}
	freeaddrinfo(addrinfo);
}

/*
 * Called from do_Device_connect() (or possibly 
 * initiate_nonblocking_connect()) with a PM_LOG_IN when connection 
 * is first established.  Otherwise, called from  do_Action() for 
 * other actions.  
 *   dev arrives here with "targets" filled in to let us know to which 
 * device components to apply the action.  
 *   We need to create the action and initiate the first step in
 * the send/expect script.
 */
void
map_Action_to_Device(Device *dev, Action *sact)
{
	Action *act;

	assert(dev != NULL);
	CHECK_MAGIC(dev);
	assert(sact != NULL);
	CHECK_MAGIC(sact);

	if ( !dev->loggedin && (sact->com != PM_LOG_IN) )
	{
		log_it(0, "Attempt to initiate Action %s while not logged in", 
		       pm_coms[sact->com]);
		return;
	}
	/* Some devices do not implemnt some actions - ignore */
	if( dev->prot->scripts[sact->com] == NULL ) return;

	/* This actually creates the one or more Actions for hte Device */
	set_targets(dev, sact);

	/* Look at action at the head of the Device's queue */
	/* and start its script */ 
	act = (Action *)list_peek(dev->acts);
	if (act == NULL) return;
	assert( act->itr != NULL );
	/* act->itr is always pointing at the next unconsumed Script_El */
	/* act->cur is always the one being worked on */
	/* We may process one or more, but probably not the whole list */
	/* before returning to the select() loop. */
	/* */
	/* FIXME:  Review this and explain why we can move this */
	/* startup processing down to process_script() where it */
	/* belongs. */
	while( (act->cur = (Script_El *)list_next(act->itr)) )
	{
		switch ( act->cur->type )
		{
		case SEND :
			log_it(0, "start script:");
			dev->status |= DEV_SENDING;
			process_send(dev);
			break;
		case EXPECT :
			Gettimeofday( &(dev->time_stamp), NULL ); 
			dev->status |= DEV_EXPECTING;
			return;
		case DELAY :
			return;
		case SND_EXP_UNKNOWN :
		default :
		}
	}
/* mostly we'll return from inside the while loop, but if */
/* we get to the end of the script after a SEND then we can */
/* free the action now rather than waiting for an expect to */
/* complete. */
	del_Action(dev->acts);
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
void
set_targets(Device *dev, Action *sact)
{
	Action *act;

	switch ( sact->com )
	{
	case PM_LOG_IN :
/* reset script of preempted action so it starts over */
		if (!list_is_empty(dev->acts))
		{
			act = (Action *)list_peek(dev->acts);
			list_iterator_reset(act->itr);
		}
		act = do_target_copy(dev, sact, NULL);
		list_push(dev->acts, (void *)act);
		break;
	case PM_ERROR :
	case PM_CHECK_LOGIN :
	case PM_LOG_OUT :
	case PM_UPDATE_PLUGS :
	case PM_UPDATE_NODES :
		act = do_target_copy(dev, sact, NULL);
		list_append(dev->acts, (void *)act);
		break;
	case PM_POWER_ON :
	case PM_POWER_OFF :
	case PM_POWER_CYCLE :
	case PM_RESET :
	case PM_NAMES :
		assert(sact->target != NULL);
		if ( dev->prot->mode == LITERAL)
		{
			do_target_some(dev, sact);
		}
		else /* dev->prot->mode == REGEX */
		{
			act = do_target_copy(dev, sact, sact->target);
			list_append(dev->acts, (void *)act);
		}
		break;
	default :
		assert( FALSE );
	}
	return;
}


/* 
 *   The new Device Action is going to get its info directly from 
 * the Server Action, except we've stated it's target explicitly in 
 * the parameters list.  A NULL target means leave the Action's
 * target NULL as well (it comes that way from make_Action()).
 *
 * Produces:  Action
 */
Action *
do_target_copy(Device *dev, Action *sact, String target)
{
	Action *act;

	act = make_Action(sact->com);
	act->client = sact->client;
	act->seq = sact->seq;
	act->itr = list_iterator_create(dev->prot->scripts[act->com]);
	if( target != NULL )
		act->target = copy_String(target);
	return act;
}

/* 
 *   This is the tricky case for set_targets().  We have a target in
 * the Server Action, and it is a RegEx.  We have to sequence through
 * each plug and see if the node connected to the plug has a name
 * that matches that RegEx.  If it does we add it to a tentative list
 * of new Actions for the Device, but we're no through.  If all of
 * the plugs' nodes match then we want to send the Device's special
 * "all" signal (every device has one).  Conversely, no plugs match
 * then no actions are added.  
 */
void
do_target_some(Device *dev, Action *sact)
{
	Action *act;
	List new_acts;
	int n;
	bool all;
	bool any;
	Plug *plug;
	ListIterator plug_i;
        size_t nm = MAX_MATCH;
        regmatch_t pm[MAX_MATCH];
        int eflags = 0;
        regex_t nre;
        reg_syntax_t cflags = REG_EXTENDED;

	all = TRUE;
	any = FALSE;
	new_acts = list_create(free_Action);
	Regcomp( &nre, get_String(sact->target), cflags );
	plug_i = list_iterator_create(dev->plugs);
	while( (plug = (Plug *)list_next(plug_i)) )
	{
/* check if plug->node->name matches the target re */
/* if it does make a new act for it and set any to TRUE */
/* if it doesn't, set all to FALSE */
		n = Regexec( &nre, get_String(plug->node->name), nm, pm, eflags );
		if( (n == REG_NOMATCH) || ((pm[0].rm_eo - pm[0].rm_so) != length_String(plug->node->name)) )
		{
			all = FALSE;
		}
		else
		{
			any = TRUE;
			act = do_target_copy(dev, sact, plug->name);
			list_append(new_acts, (void *)act);
		}
	}
	list_iterator_destroy(plug_i);
	if ( all )
	{
/* list of new_acts is destroyed at the end of this function */
		act = do_target_copy(dev, sact, dev->all);
		list_append(dev->acts, (void *)act);
	}
	else if ( any )
	{
/* we have some but not all so we need to stick them all on the queue */
		while( (act = (Action *)list_pop(new_acts)) )
		{
			list_append(dev->acts, (void *)act);
		}
	}
	list_destroy(new_acts);
}


/*
 *   We've supposedly reconnected, so check if we really are.
 * If not go into reconnect mode.  If this is a succeeding
 * reconnect, log that fact.  When all is well initiate the
 * logon script.
 */
void
do_Device_connect(Device *dev)
{
	int error;
	int s = sizeof(error);
	Action *act;
	
	if ( (getsockopt(dev->fd, SOL_SOCKET, SO_ERROR, &error, &s) < 0) 
	     || (error != 0) )
	{
/* 
 * Here is my effort to be resilient to device failure.  The device will
 * start (in the main select loop) another initiate_nonblocking_connect()
 * after the update interval.
 */
		if( dev->error )
			syslog(LOG_WARNING, "Failure attempting to connect to %s: %m\n", get_String(dev->name));
		else
		{
			dev->error = TRUE;
			syslog(LOG_ERR, "Failure attempting to connect to %s: %m\n", get_String(dev->name));
		}
		return;
	}
	if( dev->error )
	{
		syslog(LOG_WARNING, "Connection to %s reestablished\n", get_String(dev->name));
		dev->error = FALSE;
	}
	dev->status = DEV_CONNECTED;
	act = make_Action(PM_LOG_IN);
	map_Action_to_Device(dev, act);
}

/*
 *   The Read gets the string.  If it is EOF then we've lost 
 * our connection with the device and need to reconnect.  
 * Read_Buffer() does the real work.  If something arrives,
 * that is handled in process_script().
 */ 
void
handle_Device_read(Device *dev)
{
	int n;

	CHECK_MAGIC(dev);
	n = read_Buffer(dev->from);
	if ( (n < 0) && (errno == EWOULDBLOCK) ) return;
	if ( (n == 0) || ((n < 0) && (errno == ECONNRESET)) ) 
	{
		do_Device_reconnect(dev);
		return;
	}
}

/*
 *   Reset the Device's state and restart the whole nonblocking
 * connect interction.
 */
void
do_Device_reconnect(Device *dev)
{
/* 
 * If EOF on a device connection means we lost the connection then I 
 * need to reconnect.  Once reconnected a LOG_IN will automatically
 * be generated.  If the current action was a LOG_IN then just delete
 * it, otherwise leave it for completion after we're reconnected.
 */
	Action *act;

	Close(dev->fd);
	log_it(0, "Lost connection to device");
	dev->status   = DEV_NOT_CONNECTED;
	dev->loggedin = FALSE;
	if ( ((act = (Action *)list_peek(dev->acts)) != NULL) && 
	     (act->com == PM_LOG_IN) )
	{
		del_Action(dev->acts);
	}
	initiate_nonblocking_connect(dev);
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
void
process_script(Device *dev)
{
	Action *act = (Action *)list_peek(dev->acts);
	bool done = FALSE;

	if (act == NULL) return;
	assert(act->cur != NULL);
	if ( ( act->cur->type == EXPECT ) &&
		 is_empty_Buffer(dev->from) ) return;
	/* 
	 * The condition for "done" is:
	 * 1) There is nothing I can interpret in dev->from
	 *   a) is_empty_Buffer(dev->from), or
	 *   b) find_Reg_Ex() fails
	 * and
	 * 2) There's no sends to process
	 *   a)  (act = top_Action(dev->acts)) == NULL, or
	 *   b)  script_el->type == EXPECT
	 */
	while ( ! done )
	{
		switch( act->cur->type )
		{
		case EXPECT :
			done = process_expect(dev);
			break;
		case SEND :
			done = process_send(dev);
			break;
		case DELAY :
			done = process_delay(dev);
			break;
		default :
			exit_msg("unrecognized Script_El type %d", 
					act->cur->type);
		}
		if( (act->cur = (Script_El *)list_next(act->itr)) == NULL )
		{
			if (act->com == PM_LOG_IN) dev->loggedin = TRUE;
			del_Action(dev->acts);
		}
		else
		{
			if (act->cur->type == EXPECT)
			{
				gettimeofday( &(dev->time_stamp), NULL ); 
				dev->status |= DEV_EXPECTING;
			}
		}
		act = (Action *)list_peek(dev->acts);
		if(act == NULL) done = TRUE;
		else if( act->cur == NULL ) 
			act->cur = (Script_El *)list_next(act->itr);
	}
}
/*
 *   The next Script_El is an EXPECT.  Indeed we know exactly what to
 * be expecting, so we go to the buffer and look to see if there's a
 * pssible new input.  The expect.completion RegEx identifies if/when
 * there's enough in the buffer to try processing it.  If not just 
 * give up.  If there is extract the expect string, see if it matches
 * what we really wanted (throwing away bad matches), and if there
 * is an Interpretation map for the expect then send that info to
 * the semantic processors.  There are two because PMD_DEV devices
 * are handled differently (a PMD_DEV is an instance of powermand
 * acting as intermediary for another powermand in a distributed 
 * control arrangement).   
 */
bool
process_expect(Device *dev)
{
	regex_t *re;
	Action *act = (Action *)list_peek(dev->acts);
	String expect;

	assert(act != NULL);
	assert(act->cur != NULL);
	assert(act->cur->type == EXPECT);
	
	re = &(act->cur->s_or_e.expect.completion);
	
	if ( (expect = get_String_from_Buffer(dev->from, re)) == NULL ) 
		return TRUE;

	dev->status &= ~DEV_EXPECTING;
	if( ! match_RegEx(dev, expect) )
	{
		del_Action(dev->acts);
		free_String((void *)expect);
		return FALSE;
	}
	if(act->cur->s_or_e.expect.map != NULL) 
	{
		if( dev->type == PMD_DEV )
			do_PMD_semantics(dev, act->cur->s_or_e.expect.map);
		else
			do_Device_semantics(dev, act->cur->s_or_e.expect.map);
	}
	free_String((void *)expect);

	return FALSE;
}

/*
 *   send_Buffer() does al the real work.  send.fmt has a string
 * printf(fmt, ...) style.  If it has a "%s" then call send_Buffer
 * with the target as its last argument.  Otherwise just send the
 * string and update the device's status.  The TRUE return value
 * is for the while( ! done ) loop in process_scripts().
 */ 
bool
process_send(Device *dev)
{
	Action *act; 
	char *fmt;

	CHECK_MAGIC(dev);

	act  = (Action *)list_peek(dev->acts);
	assert(act != NULL);
	CHECK_MAGIC(act);
	assert(act->cur != NULL);
	assert(act->cur->type == SEND);

	fmt  = get_String(act->cur->s_or_e.send.fmt);
	
	if (act->target == NULL)
		send_Buffer(dev->to, fmt);
	else
		send_Buffer(dev->to, fmt, get_String(act->target));

	dev->status |= DEV_SENDING;

	return TRUE;
}


/*
 *   This just mediates a call to Delay() which uses and empty
 * Select() to cause struct timeval tv worth of delay.
 */
bool
process_delay(Device *dev)
{
	Action *act = (Action *)list_peek(dev->acts);
	
	assert (act != NULL);
	assert (act->cur != NULL);
	assert(act->cur->type == DELAY);
	
	Delay( &(act->cur->s_or_e.delay.tv) );

	return TRUE;
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
void
do_PMD_semantics(Device *dev, List map)
{
	Action *act;
	char *ch;
	Plug *plug;
	ListIterator plug_i;
	Interpretation *interp = (Interpretation *)list_peek(map);

	CHECK_MAGIC(dev);

	act = (Action *)list_peek(dev->acts);
	assert(act != NULL);

	ch = interp->val;
	plug_i = list_iterator_create(dev->plugs);
	switch(act->com)
	{
	case PM_UPDATE_PLUGS :
		while( (plug = (Plug *)list_next(plug_i)) )
		{
			plug->node->p_state = ST_UNKNOWN;
			if( *(ch) == '1' )
				plug->node->p_state = ON;
			if( *(ch) == '0' )
				plug->node->p_state = OFF;
			ch++;
		}
		break;
	case PM_UPDATE_NODES :
		while( (plug = (Plug *)list_next(plug_i)) )
		{
			plug->node->n_state = ST_UNKNOWN;
			if( *(ch) == '1' )
				plug->node->n_state = ON;
			if( *(ch) == '0' )
				plug->node->n_state = OFF;
			ch++;
		}
		break;
	default :
	}
	list_iterator_destroy(plug_i);
}

/*
 *   For non-PMD devices the reply for a device may be in one or 
 * several EXPECT's Intterpretation maps.  This function goes
 * through all the Interpretations for this EXPECT and sets the
 * plug or node states for whatever plugs it finds.
 */
void
do_Device_semantics(Device *dev, List map)
{
	Action *act;
	Interpretation *interp;
	ListIterator map_i;
	regex_t *re;
	char *str;
	char *end;
	char  tmp;
	char *pos;
	int len;

	CHECK_MAGIC(dev);

	act = (Action *)list_peek(dev->acts);
	assert(act != NULL);
	assert( map != NULL );
	map_i = list_iterator_create(map);

	switch(act->com)
	{
	case PM_UPDATE_PLUGS :
		while( ((Interpretation *)interp = list_next(map_i)) )
		{
			assert(interp->node != NULL);
			interp->node->p_state = ST_UNKNOWN;
			re = &(dev->on_re);
			str = interp->val;
			len = strlen(str);
			end = str;
			while ( *end && !isspace(*end) && ((end - str) < len) )
				end++;
			tmp = *end;
			len = end - str;
			*end = '\0';
			if ((pos = find_RegEx(re, str, len)) != NULL)
				interp->node->p_state = ON;
			re = &(dev->off_re);
			if ((pos = find_RegEx(re, str, len)) != NULL)
				interp->node->p_state = OFF;
			*end = tmp;
		}
		break;
	case PM_UPDATE_NODES :
		while( ((Interpretation *)interp = list_next(map_i)) )
		{
			assert(interp->node != NULL);
			interp->node->n_state = ST_UNKNOWN;
			re = &(dev->on_re);
			str = interp->val;
			len = strlen(str);
			end = str;
			while ( *end && !isspace(*end) && ((end - str) < len) )
				end++;
			tmp = *end;
			len = end - str;
			*end = '\0';
			if ((pos = find_RegEx(re, str, len)) != NULL)
				interp->node->n_state = ON;
			re = &(dev->off_re);
			if ((pos = find_RegEx(re, str, len)) != NULL)
				interp->node->n_state = OFF;
			*end = tmp;
		}
		break;
	default :
	}
	list_iterator_destroy(map_i);
}

/*
 *   write_Buffer(0 does all the work here except for changing the 
 * device state.
 */ 
void
handle_Device_write(Device *dev)
{
	int n;

	CHECK_MAGIC(dev);

	n = write_Buffer(dev->to);
	if( n < 0 ) return;
	if( is_empty_Buffer(dev->to) ) dev->status &= ~DEV_SENDING;
}
/*
 *   A device can only be stalled while it's inthe  EXPECTING 
 * state.  The dev->timeout value can be set in the config file.
 */
bool
stalled_Device(Device *dev)
{
	return ( (dev->status & DEV_EXPECTING) && 
		 overdue(&(dev->time_stamp), &(dev->timeout)) );
}

/*
 *   When a Device drops its connection you have to clear out its
 * Actions, and set its nodes and plugs to an UNKNOWN state.
 */
void
recover_Device(Device *dev)
{
	Plug *plug;
	ListIterator plug_i;


	assert( dev != NULL );
	while( !list_is_empty(dev->acts) )
		del_Action(dev->acts);
	plug_i = list_iterator_create(dev->plugs);
	while( (plug = (Plug *)list_next(plug_i)) )
	{
		if( plug->node != NULL )
		{
			plug->node->p_state = ST_UNKNOWN;
			plug->node->n_state = ST_UNKNOWN;
		}
	}
	do_Device_reconnect(dev);
}

/*
 *   Constructor
 *
 * Produces:  Device
 */
Device *
make_Device(const char *name)
{
	Device *dev;

	dev = (Device *)Malloc(sizeof(Device));
	INIT_MAGIC(dev);
	dev->name = make_String(name);
	dev->type = NO_DEV;
	dev->loggedin = FALSE;
	dev->error = FALSE;
	dev->status = DEV_NOT_CONNECTED;
	dev->fd = NO_FD;
	dev->acts = list_create(free_Action);
	gettimeofday( &(dev->time_stamp), NULL ); 
	dev->timeout.tv_sec = 0;
	dev->timeout.tv_usec = 0;
	dev->to = NULL;
	dev->from = NULL;
	dev->prot = NULL;
	dev->num_plugs = 0;
	dev->plugs = list_create(free_Plug);
	return dev;
}

/*
 *   This match utility is compatible with the list API's ListFindF
 * prototype for searching a list of Device * structs.  The match
 * criterion is a string match on their names.  This comes into use 
 * in the parser when the devices have been parsed into a list and 
 * a node line referes to a device by its name.  
 */
int
match_Device(void *dev, void *key)
{
	return ( match_String(((Device *)dev)->name, (char *)key) );
}

#ifndef NDUMP

/*
 *  Debug structure dump routine 
 */
void
dump_Device(Device *dev)
{
	ListIterator    act_iter;
	List act;
	Plug *plug;
	ListIterator plug_i;
	int i;

	fprintf(stderr, "\tDevice: %0x\n", (unsigned int)dev);
	fprintf(stderr, "\t\tname: %s\n", get_String(dev->name));
	fprintf(stderr, "\t\tall nodes symbol: %s\n", get_String(dev->all));
	fprintf(stderr, "\t\tdevice type: ");
	if(dev->type == NO_DEV) fprintf(stderr, "NO_DEV\n");
	else if(dev->type == TTY_DEV)
	{
		fprintf(stderr, "TTY_DEV\n");
		fprintf(stderr, "\t\t\tdevice: %s\n", get_String(dev->devu.ttyd.device));
	}
	else if(dev->type == TCP_DEV)
	{
		fprintf(stderr, "TCP_DEV\n");
		fprintf(stderr, "\t\t\thost: %s\n", get_String(dev->devu.tcpd.host));
		fprintf(stderr, "\t\t\tservice: %s\n", get_String(dev->devu.tcpd.service));
	}
	else if(dev->type == PMD_DEV)
	{
		fprintf(stderr, "PMD_DEV\n");
		fprintf(stderr, "\t\t\thost: %s\n", get_String(dev->devu.pmd.host));
		fprintf(stderr, "\t\t\tservice: %s\n", get_String(dev->devu.pmd.service));
	}
	else if(dev->type == TELNET_DEV)
	{
		fprintf(stderr, "TELNET_DEV\n");
		fprintf(stderr, "\t\t\tunimplemented: %s\n", get_String(dev->devu.telnetd.unimplemented));
	}
	else if(dev->type == SNMP_DEV)
	{
		fprintf(stderr, "SNMP_DEV\n");
		fprintf(stderr, "\t\t\tunimplemented: %s\n", get_String(dev->devu.snmpd.unimplemented));
	}
	else fprintf(stderr, "unknown\n");
	fprintf(stderr, "\t\tloggedin: ");
	if(dev->loggedin) fprintf(stderr, "TRUE\n");
	else fprintf(stderr, "FALSE\n");
	fprintf(stderr, "\t\terror: ");
	if(dev->error) fprintf(stderr, "TRUE\n");
	else fprintf(stderr, "FALSE\n");
	fprintf(stderr, "\t\tstatus: \n");
	if(dev->status == DEV_NOT_CONNECTED)
		fprintf(stderr, "\t\t\tDEV_NOT_CONNECTED\n");
	if(dev->status & DEV_CONNECTED)
		fprintf(stderr, "\t\t\tDEV_CONNECTED\n");
	if(dev->status & DEV_LOGGED_IN)
		fprintf(stderr, "\t\t\tDEV_LOGGED_IN\n");
	if(dev->status & DEV_SENDING)
		fprintf(stderr, "\t\t\tDEV_SENDING\n");
	if(dev->status & DEV_EXPECTING)
		fprintf(stderr, "\t\t\tDEV_EXPECTING\n");
	fprintf(stderr, "\t\tfd: %d\n", dev->fd);
	fprintf(stderr, "\t\tActions:\n");
	act_iter = list_iterator_create(dev->acts);
	while( (act = list_next(act_iter)) )
		dump_Action(act);
	list_iterator_destroy(act_iter);
	/* FIXME: time_stamp type has changed jg */
	/*fprintf(stderr, "\t\ttime_stamp: %d\n", 
		(int)dev->time_stamp);*/
	/* FIXME: timeout_interval does not exist jg */
	/*fprintf(stderr, "\t\ttimeout_interval: %d\n", 
		(int)dev->timeout_interval);*/
	dump_Buffer( dev->to );
	dump_Buffer( dev->from );
	fprintf(stderr, "\t\tnumber of nodes: %d\n", dev->num_plugs);
	fprintf(stderr, "\t\tPlugs:\n");
	plug_i = list_iterator_create(dev->plugs);
	while( (plug = (Plug *)list_next(plug_i)) )
	{
		dump_Plug(plug);
	}
	list_iterator_destroy(plug_i);
	fprintf(stderr, "\t\tDevice Scripts:\n");
	for (i = 0; i < dev->prot->num_scripts; i++)
		dump_Script(dev->prot->scripts[i], i);
}

#endif


/*
 *  Should I free protocol elements and targets here?
 */
void
free_Device(void *vdev)
{
	Device *dev = (Device *)vdev;

	CHECK_MAGIC(dev);

	free_String( (void *)dev->name );
	free_String( (void *)dev->all );
	if(dev->type == TCP_DEV)
	{
		free_String( (void *)dev->devu.tcpd.host );
		free_String( (void *)dev->devu.tcpd.service );
	}
	else if(dev->type == PMD_DEV)
	{
		free_String( (void *)dev->devu.pmd.host );
		free_String( (void *)dev->devu.pmd.service );
	}
	list_destroy(dev->acts);
	list_destroy(dev->plugs);
	CLEAR_MAGIC(dev);
	if( dev->to != NULL ) free_Buffer(dev->to);
	if( dev->from != NULL ) free_Buffer(dev->from);
	Free(dev);
}


/*
 *   Constructor
 *
 * Produces:  Plug
 */
Plug *
make_Plug(const char *name)
{
	Plug *plug;
	reg_syntax_t cflags = REG_EXTENDED;
	
	plug = (Plug *)Malloc(sizeof(Plug));
	plug->name = make_String( name );
	re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
	Regcomp(&(plug->name_re), name, cflags);
	plug->node = NULL;
	return plug;
}

/*
 *   This comes into use in the parser when the devices have been 
 * parsed into a list and a node line referes to a plug by its name.   
 */
int
match_Plug(void *plug, void *key)
{
	if( match_String(((Plug *)plug)->name, (char *)key) )
		return TRUE;
	return FALSE;	
}

#ifndef NDUMP

/*
 *  Debug structure dump routine 
 */
void
dump_Plug(Plug *plug)
{
	fprintf(stderr, "\t\t\tPlug: %0x\n", (unsigned int)plug);
	fprintf(stderr, "\t\t\t\tName: %s\n", get_String(plug->name));
	fprintf(stderr, "\t\t\t\tNode: %s\n", get_String(plug->node->name));
}

#endif



void
free_Plug(void *plug)
{
	free_String( (void *)((Plug *)plug)->name );
	Free(plug);
}




#define OK            0

/*
 *   This is the function that actually matches an EXPECT against
 * a candidate expect string.  If the match succeeds then it either
 * returns TRUE, or if there's an interpretation to be done it
 * goes through and sets all the Interpretation "val" fields, for
 * the semantics functions to later find.
 */
bool
match_RegEx(Device *dev, String expect)
{
	Action *act;
	regex_t *re;
	int n;
	size_t nmatch = MAX_MATCH;
	regmatch_t pmatch[MAX_MATCH];
	int eflags = 0;
	Interpretation *interp;
	ListIterator map_i;
	char *str = get_String(expect);
	int len = length_String(expect);

	CHECK_MAGIC(dev);
	act = (Action *)list_peek(dev->acts);
	assert(act != NULL);
	CHECK_MAGIC(act);
	assert(act->cur != NULL);
	assert(act->cur->type == EXPECT);

	re     = &(act->cur->s_or_e.expect.exp);
	re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
	n = Regexec(re, str, nmatch, pmatch, eflags);
	if (n != REG_NOERROR) return FALSE;
	if ((pmatch[0].rm_so < 0) || (pmatch[0].rm_so > len))
		return FALSE;
	
/* 
 *  The "foreach" construct assumes that the initializer is never NULL
 * but instead points to a header record to be skipped.  In this case
 * the initializer can be NULL, though.  That is how we signal there
 * is no subexpression to be interpreted.  If it is non-NULL then it
 * is the header record to an interpretation for the device.
 */
	if ( act->cur->s_or_e.expect.map == NULL )
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
 * ones and zeros.  Let do_PMD_semantics() pull it apart.
 */

	if (dev->type == PMD_DEV)
	{
		interp = (Interpretation *)list_peek(act->cur->s_or_e.expect.map);
		interp->val = str;
		return TRUE;
	}
	map_i = list_iterator_create(act->cur->s_or_e.expect.map);
	while( (interp = (Interpretation *)list_next(map_i)) )
	{
		assert( (pmatch[interp->match_pos].rm_so < MAX_BUF) &&
			(pmatch[interp->match_pos].rm_so >= 0) );
		assert( (pmatch[interp->match_pos].rm_eo < MAX_BUF) &&
			(pmatch[interp->match_pos].rm_eo >= 0) );
		interp->val = str + pmatch[interp->match_pos].rm_so;
	}
	list_iterator_destroy(map_i);
	return TRUE;
}	


/*
 * This is just time_stamp + timeout > now?
 */
bool
overdue(struct timeval *time_stamp, struct timeval *timeout)
{
	struct timeval now;
	struct timeval limit;
	bool result = FALSE;

	limit.tv_usec = time_stamp->tv_usec + timeout->tv_usec;
	limit.tv_sec  = time_stamp->tv_sec + timeout->tv_sec;
	if(limit.tv_usec > 1000000)
	{
		limit.tv_sec ++;
		limit.tv_usec -= 1000000;
	}
	gettimeofday(&now, NULL);
	if( (now.tv_sec > limit.tv_sec) ||
	    ((now.tv_sec == limit.tv_sec) && (now.tv_usec > limit.tv_usec)) )
		result = TRUE;
	return result;
}
