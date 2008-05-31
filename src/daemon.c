/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <syslog.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include "powerman.h"
#include "xpoll.h"
#include "xsignal.h"
#include "error.h"
#include "daemon.h"
#include "client.h"
#include "debug.h"

#define TMPSTR_LEN 80

/* Review: if NDEBUG turn off core generation */
void daemon_init(void)
{
    int i;
    char buf[TMPSTR_LEN];
    time_t t = time(NULL);
    int res;

    switch (fork()) {
        case -1:
            err_exit(TRUE, "fork");
        case 0: /* child */
            break;
        default: /* parent */
            exit(0);           
    }

    /* 1st child continues */
    /* Review: setsid may fail with -1, EPERM */
    if (setsid() < 0)           /* become session leader */
        err_exit(TRUE, "setsid");

    xsignal(SIGHUP, SIG_IGN);

    switch(fork()) {
        case -1:
            err_exit(TRUE, "fork");
        case 0: /* child */
            break;
        default: /* parent */
            exit(0);
    }

    /* 2nd child continues */

    /* change working directory */
    if (chdir(ROOT_DIR) < 0)
        err_exit(TRUE, "chdir %s", ROOT_DIR);

    /* clear our file mode creation mask */
    umask(0);

    /* close fd's */
    for (i = 0; i < 256; i++) {
        if (i != cli_listen_fd())
            close(i);               /* ignore errors */
    }

    /* Init syslog */
    /* Review: check for truncation */
    res = snprintf(buf, sizeof(buf), "Started %s", ctime(&t));
    assert(res != -1 && res <= sizeof(buf));
    openlog(DAEMON_NAME, LOG_NDELAY | LOG_PID, LOG_DAEMON);
    err_notty();                /* tell err_exit that stderr is no good */
    dbg_notty();                /* tell dbg that stderr is no good */
    syslog(LOG_NOTICE, buf);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
