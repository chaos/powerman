/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

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

void daemon_init(int *skipfds, int skipfdslen, char *rundir, char *pidfile,
                 char *logname)
{
    int i;
    FILE *fp;

    switch (fork()) {
        case -1:
            err_exit(true, "fork");
        case 0: /* child */
            break;
        default: /* parent */
            exit(0);
    }
    /* 1st child continues */

    /* become session leader */
    if (setsid() < 0)
        err_exit(true, "setsid");

    xsignal(SIGHUP, SIG_IGN);

    switch(fork()) {
        case -1:
            err_exit(true, "fork");
        case 0: /* child */
            break;
        default: /* parent */
            exit(0);
    }
    /* 2nd child continues */

    /* change working directory */
    if (chdir(rundir) < 0)
        err_exit(true, "chdir %s", rundir);

    /* clear our file mode creation mask */
    umask(0022);

    /* create pidfile */
    (void)unlink(pidfile);
    if (!(fp = fopen(pidfile, "w")))
        err_exit(true, "fopen %s", pidfile);
    if (fprintf(fp, "%d\n", (int)getpid()) == EOF) {
        (void)unlink(pidfile);
        err_exit(true, "fwrite %s", pidfile);
    }
    if (fclose(fp) == EOF) {
        (void)unlink(pidfile);
        err_exit(true, "fclose %s", pidfile);
    }

    /* close fd's */
    for (i = 0; i < 256; i++) {
        if (!in_fdlist(i, skipfds, skipfdslen))
            close(i);               /* ignore errors */
    }

    /* Init syslog */
    openlog(logname, LOG_NDELAY | LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "started");
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
