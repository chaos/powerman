/************************************************************\
 * Copyright (C) 2004 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

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
