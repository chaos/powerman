/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton (uselton2@llnl.gov>
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

#include "powerman.h"

#include "wrappers.h"
#include "exit_error.h"
#include "daemon_init.h"

#define TMPSTR_LEN 80

/* Review: if NDEBUG turn off core generation */
void daemon_init(void)
{
    int i;
    char buf[TMPSTR_LEN];
    time_t t = time(NULL);
    int res;

    if (Fork() != 0)
	exit(0);		/* parent terminates */

    /* 1st child continues */
    /* Review: setsid may fail with -1, EPERM */
    if (setsid() < 0)		/* become session leader */
	exit_error("setsid");

    Signal(SIGHUP, SIG_IGN);

    if (Fork() != 0)
	exit(0);		/* 1st child terminates */

    /* 2nd child continues */

    /* change working directory */
    if (chdir(ROOT_DIR) < 0)
	exit_error("chdir %s", ROOT_DIR);

    /* clear our file mode creation mask */
    umask(0);

    /* Close fd's */
    for (i = 0; i < MAXFD; i++)
	close(i);		/* ignore errors */

    /* Init syslog */
    /* Review: check for truncation */
    res = snprintf(buf, sizeof(buf), "Started %s", ctime(&t));
    assert(res != -1 && res <= sizeof(buf));
    openlog(DAEMON_NAME, LOG_NDELAY | LOG_PID, LOG_DAEMON);
    syslog_on_error(TRUE);	/* tell exit_error that stderr is no good */
    /* Review: check for error from syslog */
    syslog(LOG_NOTICE, buf);
}

/*
 * vi:softtabstop=4
 */
