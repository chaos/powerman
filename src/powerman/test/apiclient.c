/************************************************************\
 * Copyright (C) 2008 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

/* cli.c - simple client to demo the libpowerman api */

#include <stdio.h>
#include <stdlib.h>

#include "libpowerman.h"

static pm_err_t list_nodes(pm_handle_t pm);
static void usage(void);

#define statstr(s) ((s) == PM_ON ? "on" : (s) == PM_OFF ? "off" : "unknown")

int
main(int argc, char *argv[])
{
    pm_err_t err = PM_ESUCCESS;
    pm_node_state_t ns;
    pm_handle_t pm;
    char ebuf[64];
    char *server, *node = NULL;
    char cmd;

    if (argc < 3 || argc > 4)
        usage();
    server = argv[1];
    cmd = argv[2][0];
    if (argc == 3 && cmd != 'l')
        usage();
    if (argc == 4 && cmd != '1' && cmd != '0' && cmd != 'c' && cmd != 'q')
        usage();
    if (argc == 4)
        node = argv[3];

    if ((err = pm_connect(server, NULL, &pm, 0)) != PM_ESUCCESS) {
        fprintf(stderr, "%s: %s\n", server,
                pm_strerror(err, ebuf, sizeof(ebuf)));
        exit(1);
    }

    switch (cmd) {
        case '1':
            err = pm_node_on(pm, node);
            break;
        case '0':
            err = pm_node_off(pm, node);
            break;
        case 'c':
            err = pm_node_cycle(pm, node);
            break;
        case 'l':
            err = list_nodes(pm);
            break;
        case 'q':
            ns = PM_UNKNOWN;
            if ((err = pm_node_status(pm, node, &ns)) == PM_ESUCCESS)
                printf("%s: %s\n", node, statstr(ns));
            break;
    }

    if (err != PM_ESUCCESS) {
        fprintf(stderr, "Error: %s\n", pm_strerror(err, ebuf, sizeof(ebuf)));
        pm_disconnect(pm);
        exit(1);
    }

    pm_disconnect(pm);

    exit(0);
}

static pm_err_t
list_nodes(pm_handle_t pm)
{
    pm_node_iterator_t pmi;
    pm_err_t err;
    char *s;

    if ((err = pm_node_iterator_create(pm, &pmi)) != PM_ESUCCESS)
        return err;
    while ((s = pm_node_next(pmi)))
        printf("%s\n", s);
    pm_node_iterator_destroy(pmi);
    return err;
}

static void
usage(void)
{
    fprintf(stderr, "Usage: cli host:port 0|1|q node\n");
    fprintf(stderr, "       cli host:port l\n");
    exit(1);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
