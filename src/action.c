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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <syslog.h>

#include "powerman.h"

#include "pm_string.h"
#include "wrappers.h"
#include "exit_error.h"
#include "buffer.h"
#include "config.h"
#include "device.h"
#include "powermand.h"
#include "action.h"
#include "server.h"

/* Each of these represents a script that must be completed.
 * An error message and two of the debug "dump" routines rely on
 * this array for the names of the client protocol operations.
 * The header file has corresponding #define values.
 */
char *pm_coms[] = {
	"PM_ERROR",
	"PM_LOG_IN",
	"PM_CHECK_LOGIN",
	"PM_LOG_OUT",
	"PM_UPDATE_PLUGS",
	"PM_UPDATE_NODES",
	"PM_POWER_ON",
	"PM_POWER_OFF",
	"PM_POWER_CYCLE",
	"PM_RESET",
	"PM_NAMES"
};

/*
 *   The main select() loop will generate this function call 
 * periodically.  The frequency can be set in the config file.
 */
void
update_Action(Cluster *cluster, List acts)
{
	Action *act;

	Gettimeofday( &(cluster->time_stamp), NULL);
	syslog(LOG_INFO, "Updating plugs and nodes");

	act = make_Action(PM_UPDATE_PLUGS);
	list_append(acts, act);

	act = make_Action(PM_UPDATE_NODES);
	list_append(acts, act);
}

/*
 *   This function doesn't get called in powermand, but is usefull
 * in generating a "torture" client that will interact with the 
 * daemon at extremely high loads.  The patch for this modification
 * is out of date, but not hard to reconstruct.
 */
#define ACTION_BUF 1024
void
random_Action(Cluster *cluster, List acts)
{
	Action *act;
	char buf[ACTION_BUF];
	char *bp = buf;
	int len;
	bool done = FALSE;
	int com; 
	int NUM_NODES = cluster->num;
/* random between 4 (PM_UPDATE_PLUGS) and 9 (PM_RESET), inclusive */
	int all;
/* random choice between 0 (not all) and 1 (all nodes targetted) */
	int node;
/* random choice between 0 and NUM_NODES - 1, inclusive */
	int multi;
/* random choice between 0 (no more) and 1 (more nodes targetted) */

	com = 4 + (int) (6.0*rand()/(RAND_MAX+1.0));
	act = make_Action(com);
	if (act->com >= PM_POWER_ON)/* we have to set a target */
	{
		memset(buf, 0, sizeof(buf));
		all = (int) (2.0*rand()/(RAND_MAX+1.0));
		if (all) sprintf(bp, ".*");
		else
		{
			node = (int) (1.0*NUM_NODES*rand()/(RAND_MAX+1.0));
			multi = (int) (2.0*rand()/(RAND_MAX+1.0));
			if(multi == 0)
			{
				sprintf(bp, "%d", node);
				done = TRUE;
			}
			else
			{
				sprintf(bp, "(%d", node);
				len = strlen(bp);
				bp += len;
				while ( !done )
				{
					node = (int) (1.0*NUM_NODES*rand()/(RAND_MAX+1.0));
					multi = (int) (2.0*rand()/(RAND_MAX+1.0));
					if(multi == 0)
					{
						sprintf(bp, "|%d)", node);
						done = TRUE;
					}
					else
					{
						sprintf(bp, "|%d", node);
					}
					len = strlen(bp);
					bp += len;
				}
			}
		}
		act->target = make_String(buf);
	}
	list_append(acts, act);
}


/*
 *   This function retrieves the Action at the head of the queue.
 * It must verify that the retrieved action is for a client who 
 * still exists.  Internally generated Actions (from udate_Action())
 * do not have this concern, so they are not checked.  If the
 * client has disconnected the Action is discarded.
 */
Action *
find_Action(List acts, List clients)
{
	Client *client;
	Action *act = NULL;

	while ( (! list_is_empty(acts)) && (act == NULL) ) 
	{
		act = list_peek(acts);
/* act->client == NULL is a special internally generated action */
		if ( act->client == NULL ) continue;

/* What if the client has disappeared since enqueuing the action? */
		client = list_find_first(clients, (ListFindF) match_Client, act->client );
		if (client != NULL) continue;

/* I could log an event here: "client abort prior to action completion"  */
		del_Action(acts);
		act = NULL;
	}
	return act;
}
	

/*
 *   The devices have gone idle and the cluster quiescent.  It's
 * time to start a new action.  The first five ACTION types 
 * may be handled immediately, and yet another action pulled 
 * from the queue.  do_Action is called recursively until 
 * some Action comes to the head of the list that requires device
 * communication (or the list runs out and the function returns).
 *   When an Action is encountered thatrequires device 
 * communication the cluster is marked "Occupied" and corresponding
 * Actions are dispatched to each appropriate device.
 */
void
do_Action(Globals *g, Action *act)
{
	Device *dev;
	ListIterator dev_i;

	assert(g != NULL);
	CHECK_MAGIC(g);
	assert(act != NULL);
	CHECK_MAGIC(act);

	switch (act->com)
	{
	case PM_ERROR :
	case PM_LOG_IN :
	case PM_CHECK_LOGIN :
	case PM_LOG_OUT :
	case PM_NAMES :
		finish_Action(g, act);
		if ( (act = find_Action(g->acts, g->clients)) != NULL ) 
			do_Action(g, act);
		return;
	case PM_UPDATE_PLUGS :
	case PM_UPDATE_NODES :
	case PM_POWER_ON :
	case PM_POWER_OFF :
	case PM_POWER_CYCLE :
	case PM_RESET :
		break;
	default :
		assert(FALSE);
	}
	g->status = Occupied;
	dev_i = list_iterator_create(g->devs);
	while( (dev = list_next(dev_i)) )
		map_Action_to_Device(dev, act);
	list_iterator_destroy(dev_i);
}

/*
 *    From either do_Action or the main select() loop this function
 * replies to a client if appropriate, marks the timestamp (suppressing
 * an update_Action() for update_interval), and destroys the Action
 * strucutre.
 *
 * Destroys:  Action
 */ 
void
finish_Action(Globals *g, Action *act)
{
/* 
 *   act->client == NULL means that there is no client 
 * expecting a reply.
*/
	if (act->client != NULL)
		client_reply(g->cluster, act);
	Gettimeofday( &(g->cluster->time_stamp), NULL);
	del_Action(g->acts);
	g->status = Quiescent;
}

/*
 *   Constructor
 *
 * Produces:  Action
 */
Action *
make_Action(int com)
{
	Action *act;

	act = (Action *)Malloc(sizeof(Action));
	INIT_MAGIC(act);
	act->com = com;
	act->client = NULL;
	act->seq = 0;
	act->itr = NULL;
	act->cur = NULL;
	act->target = NULL;
	return act;
}

#ifndef NDUMP

/*
 *    Debug printout of structure contents.
 */
void
dump_Action(List acts)
{
	Action *act = list_peek(acts);

	fprintf(stderr, "\tAction: %x\n", (unsigned int)act);
	if( act->client == NULL )
		fprintf(stderr, "\t\tInternal action (no Client)\n");
	else
		fprintf(stderr, "\t\tAction for client fd: %d\n", act->client->fd);
	fprintf(stderr, "\t\tSequence number: %d\n", act->seq);
	fprintf(stderr, "\t\tcommand: %s\n", pm_coms[act->com]);
	fprintf(stderr, "\t\t\tcurrent script : %d\n", (int)act->cur);
	if(act->target == NULL)
		fprintf(stderr, "\t\tTarget: Null\n");
	else
	{
		fprintf(stderr, "\t\tTarget: %s\n", get_String(act->target));
	}
}

#endif


/*
 *   Destructor
 *
 * Destroys:  Action
 */
void
free_Action(Action *act)
{
	CHECK_MAGIC(act);

	if(act->target != NULL)
		free_String(act->target);
	act->target = NULL;
	if( act->itr != NULL )
		list_iterator_destroy(act->itr);
	act->itr = NULL;
	act->cur = NULL;
	CLEAR_MAGIC(act);
	Free(act);
}


/*
 *  Get rid of the Action at the head of the queue.
 *
 * Destroys:  Action
 */
void
del_Action(List acts)
{
	Action *act;

	act = list_pop(acts);
	free_Action(act);
}
