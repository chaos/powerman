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

#ifndef POWERMAND_H
#define POWERMAND_H

#include <stdio.h>

#define PROJECT "powerman"
#define VERSION "1.0.0"

#define TIMEOUT_SECONDS     1	/* default value for select(,,,tv) */
#define INTER_DEV_USECONDS 200000	/* default Delay() after device write */

/* Only when all device are devices are idle is the cluster Quiescent */
/* and only then can a new action be initiated from the queue.        */
typedef enum { Quiescent, Occupied } Server_Status;


/*
 *   Upper level functions in the calling hierarchy need enough of
 * the following to make it worth grouping together.  
 *
 *   FIXME:  The state information should be separate from the
 * configuration information.
 */
typedef struct {
    struct timeval timeout_interval;	/* select(,,,tv) value */
    struct timeval interDev;	/* Delay() after a device write */
    bool daemonize;		/* Can suppress for debugging */
    bool TCP_wrappers;		/* Config: use TCP wrappers? */
    Listener *listener;		/* listener module info */
    List clients;		/* server module info */
    Server_Status status;	/* (Quiescent, Occupied) */
    Cluster *cluster;		/* Cluster state info */
    List acts;			/* Action queue */
    Protocol *client_prot;	/* protocol for server module */
    List specs;			/* device specifications */
    List devs;			/* device module info */
    char *config_file;
    FILE *cf;
     MAGIC;
} Globals;

extern Globals *cheat;
extern bool debug_telemetry;

#endif				/* POWERMAND_H */
