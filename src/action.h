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

#ifndef ACTION_H
#define ACTION_H

#define PM_ERROR            0
#define PM_LOG_IN           1	/* actions: indexes into script arrays */
#define PM_CHECK_LOGIN      2
#define PM_LOG_OUT          3
#define PM_UPDATE_PLUGS     4
#define PM_UPDATE_NODES     5
#define PM_POWER_ON         6
#define PM_POWER_OFF        7
#define PM_POWER_CYCLE      8
#define PM_RESET            9
#define PM_NAMES           10

/*
 *   Actions are created and either 1) appended to a list that is 
 * Global->acts for actions coming from a client (or update_Action()
 * or random_Action()), 2) appended to a per device list in 
 * dev->acts, or 3) prepended (list_push(acts, act)) for the special 
 * case of a login action after a reconnection event.  Operation is 
 * always on the front of the Action queue (list_peek(acts) ).  The 
 * action is list_pop(acts) removed immediately prior to being 
 * deleted.  
 *   There is no occasion for iterating on a list of Action structs.
 */

struct action_struct {
    Client *client;
    int seq;			/* sequence number from client */
    int com;			/* one of the PM_* above */
    ListIterator itr;		/* next place in the script sequence */
    Script_El *cur;		/* current place in the script sequence */
    String target;		/* node name, regex, or device plug name */
     MAGIC;
};

extern char *pm_coms[];		/* string represetnations of the PM_* coms */

/* prototypes */

/* enque requests for hard- and soft-power state */
void act_update(void);

/* Get and validate the next action in the queue */
Action *act_find(void);

/* process Action and (possibly) dispatch to devices */
bool act_initiate(Action * act);

/* clean up and reply to client */
void act_finish(Action * act);

/* Constructor */
Action *act_create(int com);

/* Destructor */
void act_destroy(Action * act);

/* remove from queue and destroy */
void act_del_queuehead(List acts);

void act_add(Action *act);

void act_init(void);
void act_fini(void);

#endif				/* ACTION_H */
