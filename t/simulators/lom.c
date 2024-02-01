/************************************************************\
 * Copyright (C) 2004 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <getopt.h>
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
static const struct option longopts[] = {
    { "personality", required_argument, 0, 'p' },
    {0, 0, 0, 0},
};

int
main(int argc, char *argv[])
{
    int c;

    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != -1) {
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
