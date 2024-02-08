/************************************************************\
 * Copyright (C) 2004 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "xmalloc.h"
#include "hostlist.h"
#include "pluglist.h"

struct pluglist_iterator {
    ListIterator    itr;
};

struct pluglist {
    List	        pluglist;
    bool            hardwired;
};

static Plug *_create_plug(char *name)
{
    Plug *plug = (Plug *) xmalloc(sizeof(Plug));

    assert(name != NULL);

    plug->name = xstrdup(name);
    plug->node = NULL;

    return plug;
}

static void _destroy_plug(Plug *plug)
{
    assert(plug != NULL);

    xfree(plug->name);
    if (plug->node)
        xfree(plug->node);
    xfree(plug);
}

PlugList pluglist_create(List plugnames)
{
    PlugList pl = (PlugList) xmalloc(sizeof(struct pluglist));

    pl->pluglist = list_create((ListDelF)_destroy_plug);
    pl->hardwired = false;

    /* create plug for each element of plugnames list */
    if (plugnames) {
        ListIterator itr;
        char *name;

        itr = list_iterator_create(plugnames);
        while ((name = list_next(itr)))
            list_append(pl->pluglist, _create_plug(name));
        list_iterator_destroy(itr);
        pl->hardwired = true;
    }

    return pl;
}

void pluglist_destroy(PlugList pl)
{
    assert(pl != NULL);

    list_destroy(pl->pluglist);
    xfree(pl);
}

static Plug *_pluglist_find_any(PlugList pl, char *name)
{
    PlugListIterator itr = pluglist_iterator_create(pl);
    Plug *plug;

    while ((plug = pluglist_next(itr))) {
        if (strcmp(plug->name, name) == 0)
            break;
    }
    pluglist_iterator_destroy(itr);

    return plug;
}

/* Assign a node name to an existing Plug.
 * Create Plug if Plug with name doesn't exist and Plugs are not hardwired.
 */
static pl_err_t _pluglist_map_one(PlugList pl, char *node, char *name)
{
    Plug *plug = _pluglist_find_any(pl, name);
    pl_err_t res = EPL_SUCCESS;

    if (plug == NULL) {
        if (pl->hardwired) {
            res = EPL_UNKPLUG;
            goto err;
        }
        plug = _create_plug(name);
        list_push(pl->pluglist, plug);
    }
    if (plug->node) {
        res = EPL_DUPPLUG;
        goto err;
    }
    plug->node = xstrdup(node);
err:
    return res;
}

/* Assign a node name to the next available plug.
 */
static pl_err_t _pluglist_map_next(PlugList pl, char *node)
{
    Plug *plug;
    PlugListIterator pitr;
    pl_err_t res = EPL_NOPLUGS;

    pitr = pluglist_iterator_create(pl);
    while ((plug = pluglist_next(pitr))) {
        if (plug->node == NULL) {
            plug->node = xstrdup(node);
            res = EPL_SUCCESS;
            break;
        }
    }
    pluglist_iterator_destroy(pitr);

    return res;
}

pl_err_t pluglist_map(PlugList pl, char *nodelist, char *pluglist)
{
    pl_err_t res = EPL_SUCCESS;

    assert(pl != NULL);
    assert(nodelist != NULL);

    /* If pluglist is omitted we have one of two cases:
     * 1) hardwired plugs, and we assign node names to existing plugs in order.
     * 2) no plugs, we create plugs with same names as node names.
     */
    if (pluglist == NULL) {
        hostlist_t nhl = hostlist_create(nodelist);
        hostlist_iterator_t nitr = hostlist_iterator_create(nhl);
        char *node;

        while ((node = hostlist_next(nitr))) {
            if (pl->hardwired)
                res = _pluglist_map_next(pl, node);
            else
                res = _pluglist_map_one(pl, node, node);
            free(node);
            if (res != EPL_SUCCESS)
                    break;
        }

        hostlist_iterator_destroy(nitr);
        hostlist_destroy(nhl);

    /* We have a pluglist so we just map plugs to nodes.
     */
    } else {
        hostlist_t nhl = hostlist_create(nodelist);
        hostlist_iterator_t nitr = hostlist_iterator_create(nhl);
        hostlist_t phl = hostlist_create(pluglist);
        hostlist_iterator_t pitr = hostlist_iterator_create(phl);
        char *node, *name;

        while ((node = hostlist_next(nitr))) {
            name = hostlist_next(pitr);
            if (name)
                res = _pluglist_map_one(pl, node, name);
            else
                res = EPL_NOPLUGS;
            free(node);
            free(name);
            if (res != EPL_SUCCESS)
                break;
        }
        if (res == EPL_SUCCESS) {
            char *tmp = hostlist_next(pitr);
            if (tmp != NULL)
                res = EPL_NONODES;
            free(tmp);
        }

        hostlist_iterator_destroy(pitr);
        hostlist_iterator_destroy(nitr);
        hostlist_destroy(phl);
        hostlist_destroy(nhl);
    }

    return res;
}

PlugListIterator pluglist_iterator_create(PlugList pl)
{
    PlugListIterator itr = (PlugListIterator)xmalloc(sizeof(struct pluglist_iterator));

    itr->itr = list_iterator_create(pl->pluglist);

    return itr;
}

void pluglist_iterator_destroy(PlugListIterator itr)
{
    assert(itr != NULL);
    list_iterator_destroy(itr->itr);
    xfree(itr);
}

Plug *pluglist_next(PlugListIterator itr)
{
    assert(itr != NULL);

    return (Plug *)list_next(itr->itr);
}

Plug *pluglist_find(PlugList pl, char *name)
{
    Plug *plug;

    assert(pl != NULL);
    assert(name != NULL);

    plug = _pluglist_find_any(pl, name);
    if (plug && plug->node == NULL)
        plug = NULL;

    return plug;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
