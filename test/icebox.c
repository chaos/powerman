/*****************************************************************************
 *  Copyright (C) 2004 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
 *  UCRL-CODE-2002-008.
 *
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see http://github.com/chaos/powerman/
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

/* icebox.c - icebox simulator */

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

#include "xread.h"

static void usage(void);
static void _noop_handler(int signum);
static void _prompt_loop(void);

typedef enum { NONE, V2, V3, V4 } icetype_t;
static icetype_t personality = NONE;

static char *prog;

#define OPTIONS "p:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    { "personality", required_argument, 0, 'p' },
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
            case 'p':
                if (strcmp(optarg, "v2") == 0)
                    personality = V2;
                else if (strcmp(optarg, "v3") == 0)
                    personality = V3;
                else if (strcmp(optarg, "v4") == 0)
                    personality = V4;
                else
                    usage();
                break;
            default:
                usage();
        }
    }
    if (optind < argc)
        usage();
    if (personality == NONE)
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
    fprintf(stderr, "Usage: %s -p v2|v3|v4\n", prog);
    exit(1);
}

static void
_noop_handler(int signum)
{
    fprintf(stderr, "%s: received signal %d\n", prog, signum);
}

#define V2_BANNER       "V2.1\r\n"
#define V3_BANNER       "V3.0\r\n"
#define V4_BANNER       "V4.0\r\n"

#define V3_AUTHCMD      "auth icebox\r\n"
#define V3_RESP_OK      "OK\r\n"
#define V3_ERROR_CMD    "ERROR 0\r\n"
#define V3_ERROR_AUTH   "ERROR 4\r\n"

#define V3_POWER_STATUS "\
N1:%d N2:%d N3:%d N4:%d N5:%d N6:%d N7:%d N8:%d N9:%d N10:%d\r\n"

#define V3_BEACON_STATUS "\
N1:%s N2:%s N3:%s N4:%s N5:%s N6:%s N7:%s N8:%s N9:%s N10:%s N11:%s N12:%s\r\n"

#define V2_TEMP "\
N1:%d,0,0,0 N2:%d,0,0,0 N3:%d,0,0,0 N4:%d,0,0,0 N5:%d,0,0,0 N6:%d,0,0,0 N7:%d,0,0,0 N8:%d,0,0,0 N9:%d,0,0,0 N10:%d,0,0,0 N11:%d,0,0,0 N12:%d,0,0,0\r\n"

#define V3_TEMP "\
N1:%d: N2:%d: N3:%d: N4:%d: N5:%d: N6:%d: N7:%d: N8:%d: N9:%d: N10:%d: N11:%d: N12:%d:\r\n"

static void
_prompt_loop(void)
{
    int i;
    char buf[128];
    int num_plugs = 10;
    int num_plugs_ext = 12;
    int plug[12];
    char beacon[12][4];
    int temp[12];
    int plug_origin = 1;
    int logged_in = 0;
    char arg[80];

    for (i = 0; i < num_plugs_ext; i++) {
        plug[i] = 0;
        temp[i] = 73 + i;
        strcpy(beacon[i], "OFF");
    }

    /* User must first authenticate.
     */
    switch (personality) {
        case V2:
            printf(V2_BANNER);
            break;
        case V3:
            printf(V3_BANNER);
            break;
        case V4:
            printf(V4_BANNER);
            break;
        case NONE:
            break;
    }
    while (!logged_in) {
        if (xreadline("", buf, sizeof(buf)) == NULL)
            return;
        if (strcmp(buf, "auth icebox") == 0) {
            logged_in = 1;
            printf(V3_RESP_OK);
        } else
            printf(V3_ERROR_AUTH);
    }

    /* Process commands.
     */
    while (logged_in) {
        if (xreadline("", buf, sizeof(buf)) == NULL)
            break;
        if (strlen(buf) == 0) {
            goto err;
        } else if (!strcmp(buf, "q")) {
            logged_in = 0;
        } else if (!strcmp(buf, "ps *") || !strcmp(buf, "ns *")) {
            printf(V3_POWER_STATUS, plug[0], plug[1], plug[2], plug[3],
                                    plug[4], plug[5], plug[6], plug[7],
                                    plug[8], plug[9]);
        } else if (personality != V2 && !strcmp(buf, "be *")) {
            printf(V3_BEACON_STATUS, beacon[0], beacon[1], beacon[2],
                                     beacon[3], beacon[4], beacon[5],
                                     beacon[6], beacon[7], beacon[8],
                                     beacon[9], beacon[10], beacon[11]);
        } else if (personality != V2 && !strcmp(buf, "is *")) {
            printf(V3_TEMP, temp[0], temp[1], temp[2], temp[3],
                            temp[4], temp[5], temp[6], temp[7],
                            temp[8], temp[9], temp[10], temp[11]);
        } else if (personality == V2 && !strcmp(buf, "ts *")) {
            printf(V2_TEMP, temp[0], temp[1], temp[2], temp[3],
                            temp[4], temp[5], temp[6], temp[7],
                            temp[8], temp[9], temp[10], temp[11]);
        } else if (!strcmp(buf, "ph *")) {
            for (i = 0; i < num_plugs; i++)
                plug[i] = 1;
        } else if (sscanf(buf, "ph %d", &i) == 1) {
            if (i >= plug_origin && i < num_plugs + plug_origin)
                plug[i - plug_origin] = 1;
            else
                goto err;
        } else if (!strcmp(buf, "pl *")) {
            for (i = 0; i < num_plugs; i++)
                plug[i] = 0;
        } else if (sscanf(buf, "pl %d", &i) == 1) {
            if (i >= plug_origin && i < num_plugs + plug_origin)
                plug[i - plug_origin] = 0;
            else
                goto err;
        } else if (!strcmp(buf, "rp *")) {
        } else if (sscanf(buf, "rp %d", &i) == 1) {
            if (!(i >= plug_origin && i < num_plugs + plug_origin))
                goto err;
        } else if (personality != V2 && sscanf(buf, "be %d %s", &i, arg) == 2) {
            if (i >= plug_origin && i < num_plugs_ext + plug_origin) {
                if (!strcmp(arg, "off"))
                    strcpy(beacon[i - plug_origin], "OFF");
                else if (!strcmp(arg, "on"))
                    strcpy(beacon[i - plug_origin], "ON");
                else
                    goto err;
            } else
                goto err;
        } else
            goto err;
        if (logged_in)
            printf(V3_RESP_OK);
        continue;
err:
        printf(V3_ERROR_CMD);
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
