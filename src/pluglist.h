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
 * Pluglists are used to map node names to plug names within a given device
 * context.
 */

#ifndef PLUGLIST_H
#define PLUGLIST_H

#include "powerman.h"

typedef struct {
    char *name;                 /* how the plug is known to the device */
    char *node;                 /* node name */
} Plug;

typedef struct pluglist_iterator *PlugListIterator;
typedef struct pluglist *PlugList;

typedef enum { EPL_SUCCESS, EPL_DUPNODE, EPL_UNKPLUG, EPL_DUPPLUG,
               EPL_NOPLUGS, EPL_NONODES } pl_err_t;

/* Create a PlugList.  plugnames may be NULL (indicating plug creation is
 * deferred until pluglist_map() time), or a hardwired list of plug names.
 * ('hardwired' means the list of plugnames is fixed at creation time).
 */
PlugList          pluglist_create(List plugnames);

/* Destroy a PlugList and its Plugs
 */
void              pluglist_destroy(PlugList pl);

/* Assign node names to plugs.  nodelist and pluglist are strings in compressed
 * hostlist format (the degenerate case of which is a single name).  pluglist 
 * can be NULL, indicating 
 * - if hardwired plugs, available plugs should be assigned to nodes in order
 * - else (not hardwired), assume plug names are the same as node names.
 */
pl_err_t          pluglist_map(PlugList pl, char *nodelist, char *pluglist);

/* Search PlugList for a Plug with matching plug name.  Return pointer to Plug 
 * on success (points to actual list entry), or NULL on search failure.
 * Only plugs with non-NULL node fields are returned.
 */
Plug *            pluglist_find(PlugList pl, char *name);

/* An iterator interface for PlugLists, similar to the iterators in list.h.
 */
PlugListIterator  pluglist_iterator_create(PlugList pl);
void              pluglist_iterator_destroy(PlugListIterator itr);
Plug *            pluglist_next(PlugListIterator itr);

#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
