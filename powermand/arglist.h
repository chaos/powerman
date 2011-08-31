/*****************************************************************************
 *  Copyright (C) 2004-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
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

#ifndef PM_ARGLIST_H
#define PM_ARGLIST_H

typedef enum { ST_UNKNOWN, ST_OFF, ST_ON } InterpState;

typedef struct {
    char *node;                 /* node name (in) */
    char *val;                  /* value as returned by the device (out) */
    InterpState state;          /* interpreted value, if appropriate (out) */
} Arg;

typedef struct arglist_iterator *ArgListIterator;
typedef struct arglist *ArgList;

/* Create an ArgList with an Arg entry for each node in hl (refcount == 1).
 */
ArgList          arglist_create(hostlist_t hl);

/* Do refcount++ in ArgList.
 */
ArgList          arglist_link(ArgList arglist);

/* Do refcount-- in ArgList.
 * If the refcount reaches zero, destroy the Arglist.
 */
void             arglist_unlink(ArgList arglist);

/* Search ArgList for an Arg entry that matches node.
 * Return pointer to Arg on success (points to actual list entry),
 * or NULL on search failure.
 */
Arg *            arglist_find(ArgList arglist, char *node);

/* An iterator interface for ArgLists, similar to the iterators in list.h.
 */
ArgListIterator  arglist_iterator_create(ArgList arglist);
void             arglist_iterator_destroy(ArgListIterator itr);
Arg *            arglist_next(ArgListIterator itr);

#endif /* PM_ARGLIST_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
