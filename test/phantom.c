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

/* phantom.c - rackable phantom simulator */

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

#include "xtypes.h"
#include "error.h"
#include "xread.h"
#define _BOOL /* prevent solaris curses.h from defining bool */
#include <curses.h> /* after xtypes.h */

static void usage(void);
static void _noop_handler(int signum);
static void _prompt_loop(void);

typedef enum { NONE, V3, V4 } phantype_t;
static phantype_t personality = NONE;

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
                if (strcmp(optarg, "v3") == 0)
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
    fprintf(stderr, "Usage: %s -p v3|v4\n", prog);
    exit(1);
}

static void
_noop_handler(int signum)
{
    fprintf(stderr, "%s: received signal %d\n", prog, signum);
}

#define PHANTOM_MENU_V3 "\
+---------------------------------------------+\n\
|               Rackable Systems              |\n\
|                   Phantom                   |\n\
|              Copyright (c)2002              |\n\
|                Firmware v3.02               |\n\
|                                             |\n\
| D - Enter a new power on delay              |\n\
| R - Reset the Server                        |\n\
| P - Turn the Server Off                     |\n\
| L - Turn the LED On                         |\n\
| B - Start Blinking                          |\n\
| E - Erase the LCD                           |\n\
| M - Enter LCD message                       |\n\
| S - Save LCD message                        |\n\
| C - Adjust LCD contrast                     |\n\
| W - Show Saved LCD message: No              |\n\
| T - Read Temperature                        |\n\
| I - Power Sense: Other                      |\n\
| ESC - Quit                                  |\n\
|                                             |\n\
| Choose a command                            |\n\
+---------------------------------------------+\n"

#define PHANTOM_MENU_V4 "\
+---------------------------------------------+\n\
|               Rackable Systems              |\n\
|                   Phantom                   |\n\
|                Copyright 2003               |\n\
|                Firmware  v4.0               |\n\
|                                             |\n\
| D - Enter a new power on delay              |\n\
| R - Reset the Server                        |\n\
| P - Turn the Server On                      |\n\
| L - Turn the LED On                         |\n\
| B - Start Blinking                          |\n\
| E - Erase the LCD                           |\n\
| M - Enter LCD message                       |\n\
| S - Save LCD message                        |\n\
| C - Adjust LCD contrast                     |\n\
| V - View LCD message                        |\n\
| W - Show Saved LCD message: Yes             |\n\
| T - Read Temperature                        |\n\
| I - Power Sense: Reset                      |\n\
| H - Change Hotkey                           |\n\
| A - Change Baud Rate                        |\n\
| X - Reset the Phantom                       |\n\
| U - Upgrade Firmware                        |\n\
| ESC - Quit                                  |\n\
|                                             |\n\
| Choose a command                            |\n\
+---------------------------------------------+\n"

#define PHANTOM_PROMPT "Phantom [? for Help]:\r"
#define PHANTOM_STATUS "Server is %s\r"

static void
_prompt_loop(void)
{
    int led = 0;
    int blink = 0;
    int temp = 32;
    int plug = 0;
    int c;

    initscr();
    cbreak();
    noecho();
    nonl();

menu:
    while (1) {
        clear();
        mvprintw(0, 0, PHANTOM_PROMPT);
        mvprintw(1, 0, PHANTOM_STATUS, plug ? "on" : "off");
        refresh();
    menu_reprompt:
        switch ((c = getch())) {
            case ERR:
                goto done;
            case '\036':    /* ctrl-6 reset */
                break;
            case '\035':    /* ctrl-5 command mode */
                goto cmd;
            case '?':
                clear();
                if (personality == V3)
                    mvprintw(0, 0, PHANTOM_MENU_V3);
                else
                    mvprintw(0, 0, PHANTOM_MENU_V4);
                refresh();
                goto menu_reprompt;
            default:
                clear();
                mvprintw(0, 0, "Not implemented, hit return\r");
                refresh();
                if (getch() == ERR)
                    goto done;
                break;
        }
    }

cmd:
    move(0,0);
    clear();
    refresh();
    printf("ok\r");
    fflush(stdout);

    /* Process comands.
     */
    while (1) {
        switch ((c = getch())) {
            case ERR:
                goto done;
            case '\036':    /* ctrl-6 reset */
                goto menu;
            case '\035':    /* ctrl-5 command mode */
                goto cmd;
            case 'P':
                switch ((c = getch())) {
                    case ERR:
                        goto done;
                    case '?':
                        printf("%d\r", plug);
                        break;
                    case 'T':
                        plug = plug ? 0 : 1;
                        printf("ok\r");
                        break;
                    default:
                        printf("err\r");
                        break;
                }
                break;
            case 'L':
                switch ((c = getch())) {
                    case ERR:
                        goto done;
                    case '?':
                        if (!led)
                            printf("0\r");
                        else if (led && !blink)
                            printf("1\r");
                        else if (led && blink)
                            printf("B\r");
                        break;
                    case '0':
                        led = 0;
                        printf("ok\r");
                        break;
                    case '1':
                        led = 1;
                        printf("ok\r");
                        break;
                    default:
                        printf("err\r");
                        break;
                }
                break;
            case 'B':   /* blink */
                switch ((c = getch())) {
                    case ERR:
                        goto done;
                    case '0':
                        blink = 0;
                        printf("ok\r");
                        break;
                    case '1':
                        blink = 1;
                        printf("ok\r");
                        break;
                    default:
                        printf("err\r");
                        break;
                }
                break;
            case 'T':
                if (personality == V4)
                    printf("%.3d\r", temp);
                else {
                    switch ((c = getch())) { /* V3 has mult temp sensors */
                        case ERR:
                            goto done;
                        case '0':
                            printf("%.3d\r", temp);
                            break;
                        default:
                            printf("err\r");
                            break;
                    }
                }
                break;
        }
        fflush(stdout);
    }
done:
    endwin();
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
