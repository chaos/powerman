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

    plugs_add(p, "blade0", "node0", NULL);
    plugs_add(p, "blade1", "node1", NULL);

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

    plugs_add(p, "blade0", "nodeX", NULL);

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

    plugs_add(p, "blade0", "node0", NULL);
    plugs_add(p, "blade1", "node1", NULL);

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

static void hierarchy_tests(void)
{
    plugs_t *p;
    struct plug_data *pd;
    char *plug;
    int ret;

    p = plugs_create();
    if (!p)
        BAIL_OUT("plugs_create");

    /*
     * simple 3 level binary tree with 7 nodes.
     *
     * note that for these tests the hostname is basically irrelevant
     */
    plugs_add(p, "node0", "foo0", NULL);
    plugs_add(p, "node1", "foo1", "node0");
    plugs_add(p, "node2", "foo2", "node0");
    plugs_add(p, "node3", "foo3", "node1");
    plugs_add(p, "node4", "foo4", "node1");
    plugs_add(p, "node5", "foo5", "node2");
    plugs_add(p, "node6", "foo6", "node2");

    pd = plugs_get_data(p, "node0");
    ok(pd->parent == NULL,
       "no parent for node0 configured");
    pd = plugs_get_data(p, "node1");
    ok(strcmp(pd->parent, "node0") == 0,
       "parent of node1 is node0");
    pd = plugs_get_data(p, "node3");
    ok(strcmp(pd->parent, "node1") == 0,
       "parent of node3 is node1");

    /* plugs_find_root_parent tests */

    plug = plugs_find_root_parent(p, "node0");
    ok(strcmp(plug, "node0") == 0,
       "plugs_find_root_parent finds correct root for node0");
    plug = plugs_find_root_parent(p, "node1");
    ok(strcmp(plug, "node0") == 0,
       "plugs_find_root_parent finds correct root for node1");
    plug = plugs_find_root_parent(p, "node2");
    ok(strcmp(plug, "node0") == 0,
       "plugs_find_root_parent finds correct root for node2");
    plug = plugs_find_root_parent(p, "node3");
    ok(strcmp(plug, "node0") == 0,
       "plugs_find_root_parent finds correct root for node3");
    plug = plugs_find_root_parent(p, "node4");
    ok(strcmp(plug, "node0") == 0,
       "plugs_find_root_parent finds correct root for node4");
    plug = plugs_find_root_parent(p, "node5");
    ok(strcmp(plug, "node0") == 0,
       "plugs_find_root_parent finds correct root for node5");
    plug = plugs_find_root_parent(p, "node6");
    ok(strcmp(plug, "node0") == 0,
       "plugs_find_root_parent finds correct root for node6");

    /* plugs_is_descendant tests */

    ret = plugs_is_descendant(p, "node0", "node0");
    ok(ret == 0,
       "plugs_is_descendant, node0 is not descendant of node0 ");
    ret = plugs_is_descendant(p, "node0", "node1");
    ok(ret == 0,
       "plugs_is_descendant, node1 is not descendant of node0 ");
    ret = plugs_is_descendant(p, "node0", "node3");
    ok(ret == 0,
       "plugs_is_descendant, node1 is not descendant of node0 ");

    ret = plugs_is_descendant(p, "node1", "node0");
    ok(ret == 1,
       "plugs_is_descendant, node1 is descendant of node0 ");
    ret = plugs_is_descendant(p, "node3", "node0");
    ok(ret == 1,
       "plugs_is_descendant, node3 is descendant of node0 ");
    ret = plugs_is_descendant(p, "node5", "node0");
    ok(ret == 1,
       "plugs_is_descendant, node5 is descendant of node0 ");

    ret = plugs_is_descendant(p, "node2", "node1");
    ok(ret == 0,
       "plugs_is_descendant, node2 is not descendant of node1 ");
    ret = plugs_is_descendant(p, "node3", "node1");
    ok(ret == 1,
       "plugs_is_descendant, node3 is descendant of node1 ");
    ret = plugs_is_descendant(p, "node4", "node1");
    ok(ret == 1,
       "plugs_is_descendant, node4 is descendant of node1 ");
    ret = plugs_is_descendant(p, "node5", "node1");
    ok(ret == 0,
       "plugs_is_descendant, node5 is not descendant of node1 ");

    ret = plugs_is_descendant(p, "node2", "node3");
    ok(ret == 0,
       "plugs_is_descendant, node2 is not descendant of node3 ");
    ret = plugs_is_descendant(p, "node3", "node3");
    ok(ret == 0,
       "plugs_is_descendant, node3 is not descendant of node3 ");
    ret = plugs_is_descendant(p, "node5", "node3");
    ok(ret == 0,
       "plugs_is_descendant, node5 is not descendant of node3 ");

    /* plugs_child_of_ancestor tests */

    plug = plugs_child_of_ancestor(p, "node0", "node0");
    ok(plug == NULL,
       "plugs_child_of_ancestor no child of node0 from node0");
    plug = plugs_child_of_ancestor(p, "node1", "node1");
    ok(plug == NULL,
       "plugs_child_of_ancestor no child of node1 from node1");
    plug = plugs_child_of_ancestor(p, "node2", "node3");
    ok(plug == NULL,
       "plugs_child_of_ancestor no child of node3 from node2");

    plug = plugs_child_of_ancestor(p, "node1", "node0");
    ok(strcmp(plug, "node1") == 0,
       "plugs_child_of_ancestor child of node0 starting from node1 is node1");
    plug = plugs_child_of_ancestor(p, "node2", "node0");
    ok(strcmp(plug, "node2") == 0,
       "plugs_child_of_ancestor child of node0 starting from node2 is node2");
    plug = plugs_child_of_ancestor(p, "node3", "node0");
    ok(strcmp(plug, "node1") == 0,
       "plugs_child_of_ancestor child of node0 starting from node3 is node1");
    plug = plugs_child_of_ancestor(p, "node5", "node0");
    ok(strcmp(plug, "node2") == 0,
       "plugs_child_of_ancestor child of node0 starting from node5 is node2");

    plugs_destroy(p);
}

int main(int argc, char *argv[])
{
    plan(NO_PLAN);

    basic_tests();
    path_tests();
    hierarchy_tests();

    done_testing();
}
