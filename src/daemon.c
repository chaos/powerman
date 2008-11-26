/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
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

#include "xtypes.h"
#include "xsignal.h"
#include "error.h"
#include "daemon.h"

static int in_fdlist(int fd, int *fds, int len)
{
    while (--len >= 0)
        if (fds[len] == fd)
            return 1;
    return 0;
}

/* Review: if NDEBUG turn off core generation */
void daemon_init(int *skipfds, int skipfdslen, char *name)
{
    int i;

    switch (fork()) {
        case -1:
            err_exit(TRUE, "fork");
        case 0: /* child */
            break;
        default: /* parent */
            exit(0);           
    }
    /* 1st child continues */

    /* become session leader */
    if (setsid() < 0)
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

    /* clear our file mode creation mask */
    umask(0);

    /* close fd's */
    for (i = 0; i < 256; i++) {
        if (!in_fdlist(i, skipfds, skipfdslen))
            close(i);               /* ignore errors */
    }

    /* Init syslog */
    openlog(name, LOG_NDELAY | LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "started");
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
