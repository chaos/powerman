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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <stdio.h>
#include <libgen.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "xread.h"

#define LOM_LOGIN_PROMPT "SUNSP00144FEE320F login: "
#define LOM_PASSWD_PROMPT "admin@paradise-sp's password: "

#define LOM_PROMPT "paradise $ "

#define LOM_BANNER "\n\
Sun Microsystems\n\
IPMI v1.5 Service Processor\n\
\n\
Version:  V2.1.0.16\n"

#define LOM_ON_RESP "Scheduled platform on\n"
#define LOM_OFF_RESP "Scheduled platform off\n"

#define LOM_CMD_INVAL "\
%s: not found\n"

static void usage(void);
static void prompt_loop(void);

typedef enum { NONE, SSH, SER, SER_LOGIN } ilomtype_t;
static ilomtype_t personality = NONE;

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
                if (strcmp(optarg, "ssh") == 0)
                    personality = SSH;
                else if (strcmp(optarg, "serial") == 0)
                    personality = SER;
                else if (strcmp(optarg, "serial_loggedin") == 0)
                    personality = SER_LOGIN;
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
    prompt_loop();
    exit(0);
}

static void
usage(void)
{
    fprintf(stderr, "Usage: %s -p ssh|serial|serial_loggedin\n", prog);
    exit(1);
}

static void
prompt_loop(void)
{
    char buf[128];
    static char plug[4];
    int authenticated = 0;

    strcpy(plug, "Off");
    switch (personality) {
        case SER:
            authenticated = 0;
            break;
        case SSH:
            authenticated = 1;
            break;
        case SER_LOGIN:
            authenticated = 2;
            break;
        case NONE:
            break;
    }
    for (;;) {
        switch (authenticated) {
            case 0:
                if (xreadline(LOM_LOGIN_PROMPT, buf, sizeof(buf)) == NULL)
                    goto done;
                if (!strcmp(buf, "admin"))
                    authenticated = 1;
                break;
            case 1:
                if (xreadline(LOM_PASSWD_PROMPT, buf, sizeof(buf)) == NULL)
                    goto done;
                if (!strcmp(buf, "admin")) {
                    authenticated = 2;
                    printf(LOM_BANNER);
                }
                break;
            case 2:
                if (xreadline(LOM_PROMPT, buf, sizeof(buf)) == NULL)
                    goto done;
                if (!strcmp(buf, "exit")) {
                    goto done;
                } else if (!strcmp(buf, "platform get power state")) {
                        printf("%s\n", plug);
                } else if (!strcmp(buf, "platform set power state -W -f -q on")) {
                    if (!strcmp(plug, "Off")) {
                        strcpy(plug, "On");
                        printf(LOM_ON_RESP);
                    }
                } else if (!strcmp(buf, "platform set power state -W -f -q off")) {
                    if (!strcmp(plug, "On")) {
                        strcpy(plug, "Off");
                        printf(LOM_OFF_RESP);
                    }
                } else {
                    printf(LOM_CMD_INVAL, buf);
                }
        }
    continue;
done:
    break;
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
