/*****************************************************************************
 *  Copyright (C) 2021 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Albert Chu <chu11@llnl.gov>
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

/* redfish-httppower.c - mimic httppower talking to a redfish server */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "xread.h"

static void
prompt_loop(void)
{
    char buf[1024], urltmp[1024], datatmp[1024];
    int hoststatus = 0;         /* 0 = off, 1 = on */

    for (;;) {
        if (xreadline("httppower> ", buf, sizeof(buf)) == NULL)
            break;
        if (!strcmp(buf, "help")) {
            printf("Commands are:\n");
            printf(" auth user:pass\n");
            printf(" get url data\n");
            printf(" post url data\n");
        } else if (sscanf(buf, "auth %s", datatmp) == 1) {
            /* we're not authenticating for real, so do nothing */
        } else if (sscanf(buf, "get %s", urltmp) == 1) {
            if (strstr (urltmp, "redfish") == NULL)
                goto err;
            printf("{\n"
                   "\"PowerState\":\"%s\",\n"
                   "}\n",
                   hoststatus ? "On" : "Off");
        } else if (sscanf(buf, "put redfish/v1/Systems/1/Actions/ComputerSystem.Reset %s",
                          datatmp) == 1) {
            if (strstr (datatmp, "On")) {
                hoststatus = 1;
            }
            else if (strstr (datatmp, "ForceOff")) {
                hoststatus = 0;
            }
            else if (strstr (datatmp, "ForceRestart")) {
                if (!hoststatus)
                    hoststatus = 1;
            }
            else
                goto err;
            /* Doesn't matter what operation, output success */
            printf("{\n"
                   "\"PowerState\":\"%s\",\n"
                   "}\n",
                   hoststatus ? "On" : "Off");
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
