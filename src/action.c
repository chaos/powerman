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

#include "wrappers.h"
#include "error.h"
#include "buffer.h"
#include "config.h"
#include "device.h"
#include "client.h"
#include "action.h"

Action *act_create(int com)
{
    Action *act;

    act = (Action *) Malloc(sizeof(Action));
    INIT_MAGIC(act);
    act->com = com;
    act->client = NULL;
    act->seq = 0;
    act->itr = NULL;
    act->cur = NULL;
    act->target = NULL;
    act->hl = NULL;
    act->error = FALSE;
    timerclear(&act->time_stamp);
    return act;
}

void act_destroy(Action * act)
{
    CHECK_MAGIC(act);

    if (act->target != NULL)
        Free(act->target);
    act->target = NULL;
    if (act->itr != NULL)
        list_iterator_destroy(act->itr);

    if (act->hl != NULL)
        hostlist_destroy(act->hl);

    act->itr = NULL;
    act->cur = NULL;
    CLEAR_MAGIC(act);
    Free(act);
}


/*
 *  Get rid of the Action at the head of the queue.
 */
void act_del_queuehead(List acts)
{
    Action *act;

    act = list_pop(acts);
    act_destroy(act);
}

/*
 * vi:softtabstop=4
 */
