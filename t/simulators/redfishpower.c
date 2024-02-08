/************************************************************\
 * Copyright (C) 2021 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

/* redfishpower.c - simulate redfishpower
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <libgen.h>
#include <assert.h>

#include "hostlist.h"
#include "xread.h"
#include "xmalloc.h"
#include "czmq.h"

struct plug {
    char *name;
    bool state; // true=on, false=off
};

static void usage(void);
static void _noop_handler(int signum);
static void _prompt_loop(const char *hosts);

static char *prog;

/* we only support -h here, assuming user will configure all
 * paths/postdata via prompt */
#define OPTIONS "h:"
static const struct option longopts[] = {
    { "hostname", required_argument, 0, 'h' },
    {0, 0, 0, 0},
};

int
main(int argc, char *argv[])
{
    int c;
    const char *hosts = NULL;

    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != -1) {
        switch (c) {
            case 'h':
                hosts = optarg;
                break;
            default:
                usage();
        }
    }
    if (optind < argc)
        usage();
    if (hosts == NULL)
        usage();

    if (signal(SIGPIPE, _noop_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    _prompt_loop(hosts);
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

static void
_die(const char *txt)
{
    fprintf(stderr, "%s: %s: %s\n", prog, txt, strerror (errno));
    exit(1);
}

void _plug_destroy(void **item)
{
    if (item) {
        struct plug *plug = *item;
        xfree(plug->name);
        xfree(plug);
        *item = NULL;
    }
}

struct plug *_plug_create(const char *name)
{
    struct plug *plug = (struct plug *)xmalloc(sizeof(*plug));
    plug->name = xstrdup(name);
    plug->state = false;
    return plug;
}

void _plugs_add (zhashx_t *plugs, const char *hostlist)
{
    hostlist_t hl;
    hostlist_iterator_t itr;
    char *entry;
    struct plug *plug;

    if (!(hl = hostlist_create(hostlist))
        || !(itr = hostlist_iterator_create (hl)))
        _die("could not parse setplugs arguments");
    while ((entry = hostlist_next(itr))) {
        plug = _plug_create (entry);
        zhashx_update (plugs, entry, plug);
        free (entry);
    }
    hostlist_iterator_destroy(itr);
    hostlist_destroy(hl);
}

void _plugs_set (zhashx_t *plugs, const char *hostlist, bool state)
{
    if (hostlist) {
        hostlist_t hl;
        hostlist_iterator_t itr;
        char *entry;
        struct plug *plug;

        if (!(hl = hostlist_create(hostlist))
            || !(itr = hostlist_iterator_create (hl)))
            _die("could not parse power command argument");
        while ((entry = hostlist_next(itr))) {
            if (!(plug = zhashx_lookup(plugs, entry)))
                printf("%s: invalid hostname\n", entry);
            else {
                plug->state = state;
                printf("%s: ok\n", plug->name);
            }
            free (entry);
        }
        hostlist_iterator_destroy(itr);
        hostlist_destroy(hl);
    }
    else {
        struct plug *plug;
        plug = zhashx_first(plugs);
        while (plug) {
            plug->state = state;
            plug = zhashx_next(plugs);
        }
    }
}

void _plugs_stat (zhashx_t *plugs, const char *hostlist)
{
    if (hostlist) {
        hostlist_t hl;
        hostlist_iterator_t itr;
        char *entry;
        struct plug *plug;

        if (!(hl = hostlist_create(hostlist))
            || !(itr = hostlist_iterator_create (hl)))
            _die("could not parse power command argument");
        while ((entry = hostlist_next(itr))) {
            if (!(plug = zhashx_lookup(plugs, entry)))
                printf("%s: invalid hostname\n", entry);
            else
                printf("%s: %s\n", plug->name, plug->state ? "on" : "off");
            free (entry);
        }
        hostlist_iterator_destroy(itr);
        hostlist_destroy(hl);
    }
    else {
        struct plug *plug;
        plug = zhashx_first(plugs);
        while (plug) {
            printf("%s: %s\n", plug->name, plug->state ? "on" : "off");
            plug = zhashx_next(plugs);
        }
    }
}

static void
_prompt_loop(const char *hosts)
{
    static zhashx_t *plugs;
    char buf[4096];
    char argbuf[4096];

    plugs = zhashx_new();
    zhashx_set_destructor(plugs, _plug_destroy);

    while (1) {
        if (xreadline("redfishpower> ", buf, sizeof(buf)) == NULL) {
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
                   || !strcmp(buf, "setcyclepath")
                   || !strcmp(buf, "settimeout")
                   || !strcmp(buf, "setpath")) {
            /* do nothing with paths, just accept */
            continue;
        // setplugs <pluglist> <hostindices> [<parentplug>]
        } else if (sscanf(buf, "setplugs %s %*s %*s", argbuf) == 1
            || sscanf(buf, "setplugs %s %*s %*s %*s", argbuf) == 1) {
            _plugs_add(plugs, argbuf);
            continue;
        }

        const char *command = NULL;
        const char *arg = NULL;

        if (!strcmp(buf, "stat")) {
            command = "stat";
        } else if (sscanf(buf, "stat %s", argbuf) == 1) {
            command = "stat";
            arg = argbuf;
        } else if (!strcmp(buf, "on")) {
            command = "on";
        } else if (sscanf(buf, "on %s", argbuf) == 1) {
            command = "on";
            arg = argbuf;
        } else if (!strcmp(buf, "off")) {
            command = "off";
        } else if (sscanf(buf, "off %s", argbuf) == 1) {
            command = "off";
            arg = argbuf;
        } else if (!strcmp(buf, "cycle")) {
            command = "cycle";
        } else if (sscanf(buf, "cycle %s", argbuf) == 1) {
            command = "cycle";
            arg = argbuf;
        } else {
            printf("unknown command - type \"help\"\n");
            continue;
        }

        /* We have a valid power command, now ensure that plugs are
         * instantiated.  If "setplugs" was not called assume the device
         * script is not defining plugs and we must build plugs from
         * '-h hostlist'.
         */
        if (zhashx_size (plugs) == 0) {
            _plugs_add(plugs, hosts);
        }

        if (!strcmp (command, "stat"))
            _plugs_stat(plugs, arg);
        else if (!strcmp (command, "on") || !strcmp (command, "cycle"))
            _plugs_set (plugs, arg, true);
        else if (!strcmp (command, "off"))
            _plugs_set (plugs, arg, false);
    }

    zhashx_destroy (&plugs);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
