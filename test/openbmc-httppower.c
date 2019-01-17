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

/* openbmc-httppower.c - mimic httppower talking to a openbmc server */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "xread.h"

static void
print_unauthenticated (void)
{
  printf("{\n"
         "\"data\": {\n"
         "\"description\": \"Login required\"\n"
         "},\n"
         "\"message\": \"401 Unauthorized\",\n"
         "\"status\": \"error\"\n"
         "}\n");
}

static void
prompt_loop(void)
{
    char buf[1024], urltmp[1024], datatmp[1024];
    int hoststatus = 0;         /* 0 = off, 1 = on */
    int authenticated = 0;

    for (;;) {
        if (xreadline("httppower> ", buf, sizeof(buf)) == NULL)
            break;
        if (!strcmp(buf, "help")) {
            printf("Commands are:\n");
            printf(" get url data\n");
            printf(" post url data\n");
            printf(" put url data\n");
        } else if (sscanf(buf, "post login %s", datatmp) == 1) {
            if (!authenticated)
                authenticated = 1;
            printf("{\n"
                   "\"data\": \"User 'root' logged in\",\n"
                   "\"message\": \"200 OK\",\n"
                   "\"status\": \"ok\"\n"
                   "}\n");
        } else if (sscanf(buf, "post logout %s", datatmp) == 1) {
            if (authenticated) {
                authenticated = 0;
                printf("{\n"
                       "\"data\": \"User 'root' logged in\",\n"
                       "\"message\": \"200 OK\",\n"
                       "\"status\": \"ok\"\n"
                       "}\n");
            }
            else
                printf("{\n"
                       "\"data\": \"No user logged in\",\n"
                       "\"message\": \"200 OK\",\n"
                       "\"status\": \"ok\"\n"
                       "}\n");
        } else if (sscanf(buf, "get %s", urltmp) == 1) {
            if (!authenticated) {
                print_unauthenticated ();
                continue;
            }
            if (strstr (urltmp, "CurrentPowerState") == NULL)
                goto err;
            printf("{\n"
                   "\"data\": \"xyz.openbmc_project.State.Chassis.PowerState.%s\",\n"
                   "\"message\": \"200 OK\",\n"
                   "\"status\": \"ok\"\n"
                   "}\n",
                   hoststatus ? "On" : "Off");
        } else if (sscanf(buf, "put xyz/openbmc_project/state/host0/attr/RequestedHostTransition %s",
                          datatmp) == 1) {
            if (!authenticated) {
                print_unauthenticated ();
                continue;
            }
            if (strstr (datatmp, "On")) {
                hoststatus = 1;
            }
            else if (strstr (datatmp, "Off")) {
                hoststatus = 0;
            }
            else if (strstr (datatmp, "Reboot")) {
                if (!hoststatus)
                    hoststatus = 1;
            }
            else
                goto err;
            /* Doesn't matter what operation, output success */
            printf("{\n"
                   "\"data\": null,\n"
                   "\"message\": \"200 OK\",\n"
                   "\"status\": \"ok\"\n"
                   "}\n");
        } else
            goto err;

        continue;
err:
        printf("Error\n");
    }
}

int
main(int argc, char *argv[])
{
    prompt_loop();
    exit (0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
