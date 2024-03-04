/************************************************************\
 * Copyright (C) 2024 The Regents of the University of California.
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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "redfishpower_defs.h"
#include "plugs.h"

#include "xmalloc.h"
#include "czmq.h"
#include "hostlist.h"
#include "error.h"

struct plugs {
    hostlist_t plugs;
    /* map plug names to plug_data */
    zhashx_t *plug_map;
};

static struct plug_data *plug_data_create(const char *plugname,
                                          const char *hostname)
{
    struct plug_data *pd = (struct plug_data *)xmalloc(sizeof(*pd));
    pd->plugname = xstrdup(plugname);
    pd->hostname = xstrdup(hostname);
    return pd;
}

static void plug_data_destroy(struct plug_data *pd)
{
    if (pd) {
        xfree(pd->plugname);
        xfree(pd->hostname);
        xfree(pd);
    }
}

/* zhashx_destructor_fn */
static void plug_data_destroy_wrapper(void **data)
{
    struct plug_data *pd = *data;
    plug_data_destroy(pd);
}

plugs_t *plugs_create(void)
{
    plugs_t *p = (plugs_t *)xmalloc(sizeof(*p));

    if (!(p->plugs = hostlist_create(NULL)))
        goto cleanup;

    if (!(p->plug_map = zhashx_new()))
        goto cleanup;
    zhashx_set_destructor(p->plug_map, plug_data_destroy_wrapper);

    return p;

cleanup:
    plugs_destroy(p);
    return NULL;
}

void plugs_destroy(plugs_t *p)
{
    if (p) {
        hostlist_destroy(p->plugs);
        zhashx_destroy(&p->plug_map);
        xfree(p);
    }
}

void plugs_add(plugs_t *p, const char *plugname, const char *hostname)
{
    struct plug_data *pd;
    if (hostlist_find(p->plugs, plugname) < 0) {
        if (hostlist_push(p->plugs, plugname) == 0)
            err_exit(false, "hostlist_push failed");
    }
    pd = plug_data_create(plugname, hostname);
    zhashx_update(p->plug_map, plugname, pd);
}

void plugs_remove(plugs_t *p, const char *plugname)
{
    zhashx_delete(p->plug_map, plugname);
    hostlist_delete(p->plugs, plugname);
}

int plugs_count(plugs_t *p)
{
    return hostlist_count(p->plugs);
}

struct plug_data *plugs_get_data(plugs_t *p, const char *plugname)
{
    return zhashx_lookup(p->plug_map, plugname);
}

hostlist_t *plugs_hostlist(plugs_t *p)
{
    return &p->plugs;
}

int plugs_name_valid(plugs_t *p, const char *plugname)
{
    if (hostlist_find(p->plugs, plugname) < 0)
        return 0;
    return 1;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
