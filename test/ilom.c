/*****************************************************************************
 *  Copyright (C) 2004 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
 *  UCRL-CODE-2002-008.
 *
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see http://code.google.com/p/powerman/
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

#define ILOM_LOGIN_PROMPT "SUNSP00144FEE320F login: "
#define ILOM_PASSWD_PROMPT "Password: "

#define ILOM_PROMPT " -> "

#define ILOM_BANNER "\n\
Sun(TM) Integrated Lights Out Manager\n\
\n\
Version 2.0.2.3\n\
\n\
Copyright 2007 Sun Microsystems, Inc. All rights reserved.\n\
Use is subject to license terms.\n\
\n\
Warning: password is set to factory default.\n\n"

#define ILOM_PROP_SYS "\
  /SYS\n\
    Properties:\n\
        type = Host System\n\
        chassis_name = SUN FIRE X4140\n\
        chassis_part_number = 540-7618-XX\n\
        chassis_serial_number = 0226LHF-0823A600HM\n\
        chassis_manufacturer = SUN MICROSYSTEMS\n\
        product_name = SUN FIRE X4140\n\
        product_part_number = 602-4125-01\n\
        product_serial_number = 0826QAD075\n\
        product_manufacturer = SUN MICROSYSTEMS\n\
        power_state = %s\n\n\n"

#define ILOM_STOP_RESP "Stopping /SYS immediately\n\n"
#define ILOM_STOP_RESP2 "stop: Target already stopped\n\n"
#define ILOM_START_RESP "Starting /SYS\n\n"
#define ILOM_START_RESP2 "start: Target already started\n\n"
#define ILOM_RESET_RESP "Performing hard reset on /SYS\n\n"
#define ILOM_RESET_RESP2 "Performing hard reset on /SYS failed\n\
reset: Target already stopped\n\n"

#define ILOM_CMD_INVAL "\
Invalid command '%s' - type help for a list of commands.\n\n"

#define ILOM_HELP "\
The help command is used to view information about commands and targets\n\
\n\
Usage: help [-o|-output terse|verbose] [<command>|legal|targets]\n\
\n\
Special characters used in the help command are\n\
[]   encloses optional keywords or options\n\
<>   encloses a description of the keyword\n\
     (If <> is not present, an actual keyword is indicated)\n\
|    indicates a choice of keywords or options\n\
\n\
help targets  displays a list of targets\n\
help legal    displays the product legal notice\n\
\n\
Commands are:\n\
cd\n\
create\n\
delete\n\
exit\n\
help\n\
load\n\
reset\n\
set\n\
show\n\
start\n\
stop\n\
version\n\n"

#define ILOM_VERS "\
SP firmware 2.0.2.3\n\
SP firmware build number: 29049\n\
SP firmware date: Thu Feb 21 19:42:30 PST 2008\n\
SP filesystem version: 0.1.16\n\n"

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
    int authenticated;

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
            authenticated = 0;
    }
    for (;;) {
        switch (authenticated) {
            case 0:
                if (xreadline(ILOM_LOGIN_PROMPT, buf, sizeof(buf)) == NULL)
                    goto done;
                if (!strcmp(buf, "root"))
                    authenticated = 1;
                break;
            case 1:
                if (xreadline(ILOM_PASSWD_PROMPT, buf, sizeof(buf)) == NULL)
                    goto done;
                if (!strcmp(buf, "changeme")) {
                    authenticated = 2;
                    printf(ILOM_BANNER);
                }
                break;
            case 2:
                if (xreadline(ILOM_PROMPT, buf, sizeof(buf)) == NULL)
                    goto done;
                if (!strcmp(buf, "exit")) {
                    goto done;
                } else if (!strcmp(buf, "help")) {
                    printf(ILOM_HELP);
                } else if (!strcmp(buf, "version")) {
                    printf(ILOM_VERS);
                } else if (!strcmp(buf, "show -d properties /SYS")) {
                    printf(ILOM_PROP_SYS, plug);
                } else if (!strcmp(buf, "start -script /SYS")) {
                    if (!strcmp(plug, "Off")) {
                        strcpy(plug, "On");
                        printf(ILOM_START_RESP);
                    } else
                        printf(ILOM_START_RESP2);
                } else if (!strcmp(buf, "stop -script -force /SYS")) {
                    if (!strcmp(plug, "On")) {
                        strcpy(plug, "Off");
                        printf(ILOM_STOP_RESP);
                    } else
                        printf(ILOM_STOP_RESP2);
                } else if (!strcmp(buf, "reset -script /SYS")) {
                    if (!strcmp(plug, "On")) {
                        printf(ILOM_RESET_RESP);
                    } else
                        printf(ILOM_RESET_RESP2);
                } else {
                    printf(ILOM_CMD_INVAL, buf);
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
