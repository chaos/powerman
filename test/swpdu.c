/*****************************************************************************
 *  Copyright (C) 2004-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
 *  UCRL-CODE-2002-008.
 *  
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see <http://www.llnl.gov/linux/powerman/>.
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

/* swpdu.c - appro swpdu simulator */

/* FIXME: created from swpdu.dev, thus needs a sync with reality */

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

static char *prog;

#define OPTIONS "p:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
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
            default:
                usage();
        }
    }
    if (optind < argc)
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

static void
_prompt_loop(void)
{
    int i;
    char buf[128];
    int num_plugs = 48;
    int plug[48];
    int plug_origin = 1;
 
    for (i = 0; i < num_plugs; i++) {
        plug[i] = 0;
    }

    /* Process comands.
     */
    while (1) {
        if (xreadline("", buf, sizeof(buf)) == NULL)
            break;
        if (strlen(buf) == 0) {
            /* */
        } else if (!strcmp(buf, "exprange on")) {
            /* */
        } else if (!strcmp(buf, "exprange off")) {
            /* */
        } else if (!strcmp(buf, "status")) {
            for (i = 0; i < num_plugs; i++)
                printf ("plug %d: %s\n", i + plug_origin,
                        plug[i] ? "on" : "off");
        } else if (sscanf(buf, "status %d", &i) == 1) {
            i -= plug_origin;
            if (i < 0 || i >= num_plugs)
                break;
            printf ("plug %d: %s\n", i + plug_origin,
                     plug[i] ? "on" : "off");
        } else if (sscanf(buf, "on %d", &i) == 1) {
            i -= plug_origin;
            if (i < 0 || i >= num_plugs)
                break;
            plug[i] = 1;
        } else if (sscanf(buf, "off %d", &i) == 1) {
            i -= plug_origin;
            if (i < 0 || i >= num_plugs)
                break;
            plug[i] = 0;
        } else if (sscanf(buf, "cycle %d", &i) == 1) {
            i -= plug_origin;
            if (i < 0 || i >= num_plugs)
                break;
            plug[i] = 0;
            sleep(2);
            plug[i] = 1;
        } else
            break;
        printf("swpdu> ");
    }
    printf("Error, exiting");
    exit (1);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
