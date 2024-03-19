/************************************************************\
 * Copyright (C) 2024 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

/*
 * Test driver for plugs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tap.h"
#include "plugs.h"

static void basic_tests(void)
{
    plugs_t *p;
    struct plug_data *pd;
    hostlist_t *hl;
    int count;

    p = plugs_create();
    if (!p)
        BAIL_OUT("plugs_create");

    count = plugs_count(p);
    ok(count == 0,
       "plugs_count returns 0 plugs before we added any");

    plugs_add(p, "blade0", "node0");
    plugs_add(p, "blade1", "node1");

    count = plugs_count(p);
    ok(count == 2,
       "plugs_count returns correct number of plugs");

    ok(plugs_name_valid(p, "blade0") == 1,
       "plug blade0 is valid");
    ok(plugs_name_valid(p, "blade1") == 1,
       "plug blade1 is valid");
    ok(plugs_name_valid(p, "foof") == 0,
       "plug foof is not valid");

    hl = plugs_hostlist(p);

    ok(hostlist_count(*hl) == 2,
       "hostlist contains 2 plugs");
    ok(hostlist_find(*hl, "blade0") >= 0,
       "hostlist contains blade0");
    ok(hostlist_find(*hl, "blade1") >= 0,
       "hostlist contains blade1");

    pd = plugs_get_data(p, "blade0");
    ok(pd != NULL,
       "plugs_get_data returns data for blade0");
    pd = plugs_get_data(p, "blade0");
    ok(strcmp(pd->plugname, "blade0") == 0
       && strcmp(pd->hostname, "node0") == 0,
       "plugs_get_data returns correct plug");

    pd = plugs_get_data(p, "foof");
    ok(pd == NULL,
       "plugs_get_data returns NULL for invalid plug");

    /* remove works */

    plugs_remove(p, "blade1");

    count = plugs_count(p);
    ok(count == 1,
       "plugs_count returns correct number of plugs after removal");
    ok(plugs_name_valid(p, "blade0") == 1,
       "plug blade0 is valid");
    ok(plugs_name_valid(p, "blade1") == 0,
       "plug blade1 is not valid");

    /* overwrite works */

    plugs_add(p, "blade0", "nodeX");

    ok(hostlist_count(*hl) == 1,
       "hostlist contains 1 plugs");

    ok(plugs_name_valid(p, "blade0") == 1,
       "plug blade0 is valid");

    pd = plugs_get_data(p, "blade0");
    ok(strcmp(pd->hostname, "nodeX") == 0,
       "plugs_get_data returns correct overwrite host");

    plugs_destroy(p);
}

static void path_tests(void)
{
    plugs_t *p;
    struct plug_data *pd;
    int ret;

    p = plugs_create();
    if (!p)
        BAIL_OUT("plugs_create");

    plugs_add(p, "blade0", "node0");
    plugs_add(p, "blade1", "node1");

    ret = plugs_update_path(p, "blade0", "stat", "statpath0", NULL);
    ok(ret == 0,
       "plugs_update_path of stat path works on blade0");
    ret = plugs_update_path(p, "blade0", "on", "onpath0", "onpostdata0");
    ok(ret == 0,
       "plugs_update_path of on path works on blade0");
    /* intionally leave no postdata for off for testing */
    ret = plugs_update_path(p, "blade0", "off", "offpath0", NULL);
    ok(ret == 0,
       "plugs_update_path of off path works on blade0");

    ret = plugs_update_path(p, "blade1", "stat", "statpath1", NULL);
    ok(ret == 0,
       "plugs_update_path of stat path works on blade1");
    ret = plugs_update_path(p, "blade1", "on", "onpath1", "onpostdata1");
    ok(ret == 0,
       "plugs_update_path of on path works on blade1");
    /* intionally leave no postdata for off for testing */
    ret = plugs_update_path(p, "blade1", "off", "offpath1", NULL);
    ok(ret == 0,
       "plugs_update_path of off path works on blade1");

    ret = plugs_update_path(p, "foof", "stat", "statpath", NULL);
    ok(ret == -1,
       "plugs_update_path of invalid plug fails");
    ret = plugs_update_path(p, "blade0", "foof", "statpath", NULL);
    ok(ret == -1,
       "plugs_update_path of invalid cmd fails");

    pd = plugs_get_data(p, "blade0");
    ok(strcmp(pd->stat, "statpath0") == 0,
       "blade0 has correct stat path");
    ok(strcmp(pd->on, "onpath0") == 0,
       "blade0 has correct on path");
    ok(strcmp(pd->onpostdata, "onpostdata0") == 0,
       "blade0 has correct on postdata");
    ok(strcmp(pd->off, "offpath0") == 0,
       "blade0 has correct off path");
    ok(pd->offpostdata == NULL,
       "blade0 has no off postdata");

    pd = plugs_get_data(p, "blade1");
    ok(strcmp(pd->stat, "statpath1") == 0,
       "blade1 has correct stat path");
    ok(strcmp(pd->on, "onpath1") == 0,
       "blade1 has correct on path");
    ok(strcmp(pd->onpostdata, "onpostdata1") == 0,
       "blade1 has correct on postdata");
    ok(strcmp(pd->off, "offpath1") == 0,
       "blade1 has correct off path");
    ok(pd->offpostdata == NULL,
       "blade1 has no off postdata");

    plugs_destroy(p);
}

int main(int argc, char *argv[])
{
    plan(NO_PLAN);

    basic_tests();
    path_tests();

    done_testing();
}
