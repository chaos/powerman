/************************************************************\
 * Copyright (C) 2024 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef REDFISHPOWER_PLUGS_H
#define REDFISHPOWER_PLUGS_H

#include "hostlist.h"

struct plug_data {
    char *plugname;
    char *hostname;

    /* paths */
    char *stat;
    char *on;
    char *onpostdata;
    char *off;
    char *offpostdata;
    char *cycle;
    char *cyclepostdata;
};

typedef struct plugs plugs_t;

plugs_t *plugs_create(void);

void plugs_destroy(plugs_t *p);

void plugs_add(plugs_t *p, const char *plugname, const char *hostname);

void plugs_remove(plugs_t *p, const char *plugname);

int plugs_count(plugs_t *p);

struct plug_data *plugs_get_data(plugs_t *p, const char *plugname);

hostlist_t *plugs_hostlist(plugs_t *p);

int plugs_update_path(plugs_t *p,
                      const char *plugname,
                      const char *cmd,
                      const char *path,
                      const char *postdata);

int plugs_name_valid(plugs_t *p, const char *plugname);

#endif /* REDFISHPOWER_PLUGS_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
