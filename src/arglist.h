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

/*
 * The ArgList is used to pass the list of "target nodes" into a device
 * script, and then to pass the result back to the client.
 */

#ifndef ARGLIST_H
#define ARGLIST_H

#include "powerman.h"

typedef struct {
    char *node;                 /* node name */
    char *val;                  /* value as returned by the device */
    InterpState state;          /* interpreted value, if appropriate */
} Arg;

typedef struct arglist_iterator *ArgListIterator;
typedef struct arglist *ArgList;

/* create/destroy */
ArgList arglist_create(hostlist_t hl); /* refcount = 1 */
ArgList arglist_link(ArgList arglist); /* ++refcount */
void arglist_unlink(ArgList arglist);  /* --refcount (destroy if 0) */

/* accessor */
void arglist_set(ArgList arglist, char *node, char *val, InterpState state);
bool arglist_get(ArgList arglist, char *node, char **val, InterpState *state);

/* iteration */
ArgListIterator arglist_iterator_create(ArgList arglist);
void arglist_iterator_destroy(ArgListIterator itr);
Arg *arglist_next(ArgListIterator itr);

#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
