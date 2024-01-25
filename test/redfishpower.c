/************************************************************\
 * Copyright (C) 2021 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

/* redfishpower.c - simulate redfishpower */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <libgen.h>
#include <assert.h>

#include "hash.h"
#include "hostlist.h"
#include "xread.h"

static void usage(void);
static void _noop_handler(int signum);
static void _prompt_loop(void);

static char *prog;
static char *hostname = NULL;

/* we only support -h here, assuming user will configure all
 * paths/postdata via prompt */
#define OPTIONS "h:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    { "hostname", required_argument, 0, 'h' },
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int
main(int argc, char *argv[])
{
    int c;

    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
            case 'h':
                hostname = optarg;
                break;
            default:
                usage();
        }
    }
    if (optind < argc)
        usage();
    if (hostname == NULL)
        usage();

    if (signal(SIGPIPE, _noop_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    _prompt_loop();
    exit(0);
}

static void
usage(void)
{
    fprintf(stderr, "Usage: %s -h hostlist\n", prog);
    exit(1);
}

static void
_noop_handler(int signum)
{
    fprintf(stderr, "%s: received signal %d\n", prog, signum);
}

#define CMD_PROMPT "redfishpower> "

#define OFF_STATUS "off"
#define ON_STATUS  "on"
#define OK_STATUS  "ok"

#define HASH_SIZE 1024

static void
_stat(hash_t hstatus, const char *nodes)
{
    hostlist_iterator_t hlitr;
    hostlist_t hlnodes;
    char *node;
    char *str;

    assert(hstatus);

    if (!(hlnodes = hostlist_create(nodes))) {
        perror("hostlist_create");
        exit(1);
    }
    if (!(hlitr = hostlist_iterator_create(hlnodes))) {
        perror("hostlist_iterator_create");
        exit(1);
    }
    while ((node = hostlist_next(hlitr))) {
        if ((str = hash_find(hstatus, node)))
            printf("%s: %s\n", node, str);
        else
            printf("%s: %s\n", node, "invalid hostname");
        free(node);
    }
    hostlist_iterator_destroy(hlitr);
    hostlist_destroy(hlnodes);
}

static void
_powercmd(hash_t hstatus, const char *nodes, const char *state)
{
    hostlist_iterator_t hlitr;
    hostlist_t hlnodes;
    char *node;
    char *str;

    assert(hstatus);

    if (!(hlnodes = hostlist_create(nodes))) {
        perror("hostlist_create");
        exit(1);
    }
    if (!(hlitr = hostlist_iterator_create(hlnodes))) {
        perror("hostlist_iterator_create");
        exit(1);
    }
    while ((node = hostlist_next(hlitr))) {
        if ((str = hash_find(hstatus, node))) {
            printf("%s: %s\n", node, OK_STATUS);
            hash_remove(hstatus, node);
            if (!hash_insert(hstatus, (void *)node, (void *)state)) {
                perror("hash_insert");
                exit(1);
            }
            /* XXX: Don't free 'node' here, it needs to be alloc'd for
             * the hash key.  It's a mem-leak.  Fix later.
             */
        } else {
            printf("%s: %s\n", node, "invalid hostname");
            free(node);
        }
    }
    hostlist_iterator_destroy(hlitr);
    hostlist_destroy(hlnodes);
}

static void
_prompt_loop(void)
{
    char buf[128];
    char bufnode[128];
    hash_t hstatus = NULL;
    hostlist_t hl = NULL;
    hostlist_iterator_t hlitr = NULL;
    char *node;

    assert(hostname);

    if (!(hstatus = hash_create(HASH_SIZE,
                                (hash_key_f)hash_key_string,
                                (hash_cmp_f)strcmp,
                                (hash_del_f)NULL))) {
        perror("hash_create");
        exit(1);
    }
    if (!(hl = hostlist_create(hostname))) {
        perror("hostlist_create");
        exit(1);
    }
    if (!(hlitr = hostlist_iterator_create(hl))) {
        perror("hostlist_iterator");
        exit(1);
    }
    /* all nodes begin as off */
    while ((node = hostlist_next(hlitr))) {
        if (!hash_insert(hstatus, (void *)node, OFF_STATUS)) {
            perror("hash_insert");
            exit(1);
        }
        /* XXX: Don't free 'node' here, it needs to be alloc'd for
         * the hash key.  It's a mem-leak.  Fix later.
         */
    }
    hostlist_iterator_destroy(hlitr);
    hostlist_destroy(hl);

    while (1) {
        if (xreadline(CMD_PROMPT, buf, sizeof(buf)) == NULL) {
            break;
        } else if (strlen(buf) == 0) {
            continue;
        } else if (!strcmp(buf, "quit")) {
            break;
        } else if (!strcmp(buf, "auth")
                   || !strcmp(buf, "setheader")
                   || !strcmp(buf, "setstatpath")
                   || !strcmp(buf, "setonpath")
                   || !strcmp(buf, "setoffpath")
                   || !strcmp(buf, "setcyclepath")) {
            /* do nothing with config, just accept */
            ;
        } else if (!strcmp(buf, "stat")) {
            _stat(hstatus, hostname);
        } else if (sscanf(buf, "stat %s", bufnode) == 1) {
            _stat(hstatus, bufnode);
        } else if (!strcmp(buf, "on")) {
            _powercmd(hstatus, hostname, ON_STATUS);
        } else if (sscanf(buf, "on %s", bufnode) == 1) {
            _powercmd(hstatus, bufnode, ON_STATUS);
        } else if (!strcmp(buf, "off")) {
            _powercmd(hstatus, hostname, OFF_STATUS);
        } else if (sscanf(buf, "off %s", bufnode) == 1) {
            _powercmd(hstatus, bufnode, OFF_STATUS);
        } else if (!strcmp(buf, "cycle")) {
            _powercmd(hstatus, hostname, ON_STATUS);
        } else if (sscanf(buf, "cycle %s", bufnode) == 1) {
            _powercmd(hstatus, bufnode, ON_STATUS);
        } else
            printf("unknown command - type \"help\"\n");
    }

    hash_destroy(hstatus);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
