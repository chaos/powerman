/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2003 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
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

/*
 * Implement connect/disconnect device methods for pipes.
 * Well it started out as a pipe, now actually it's a "coprocess" on a pty.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "powerman.h"
#include "parse_util.h"
#include "xmalloc.h"
#include "xpoll.h"
#include "pluglist.h"
#include "device.h"
#include "device_pipe.h"
#include "error.h"
#include "debug.h"
#include "argv.h"
#include "xpty.h"

typedef struct {
    char **argv;
    pid_t cpid;
} PipeDev;

/* Create "pipe device" data struct.
 * cmdline would normally look something like "/usr/bin/conman -j -Q bay0 |&"
 * (Korn shell style "coprocess" syntax)
 */
void *pipe_create(char *cmdline, char *flags)
{
    PipeDev *pd = (PipeDev *)xmalloc(sizeof(PipeDev));

    pd->argv = argv_create(cmdline, "|&");
    pd->cpid = -1;

    return (void *)pd;
}

/* Destroy pipe device data struct.
 */
void pipe_destroy(void *data)
{
    PipeDev *pd = (PipeDev *)data;

    argv_destroy(pd->argv);
    xfree(pd);
}

/* Start the coprocess using forkpty(3).  */
bool pipe_connect(Device * dev)
{
    int fd;
    pid_t pid;
    PipeDev *pd = (PipeDev *)dev->data;
    char ptyname[64];

    assert(dev->connect_state == DEV_NOT_CONNECTED);
    assert(dev->fd == NO_FD);

    pid = xforkpty(&fd, ptyname, sizeof(ptyname));
    if (pid < 0) {
        err(FALSE, "_pipe_connect(%s): forkpty error", dev->name);
    } else if (pid == 0) {      /* child */
        execv(pd->argv[0], pd->argv);
        err_exit(TRUE, "exec %s", pd->argv[0]);
    } else {                    /* parent */
        int flags;

        flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0)
            err_exit(TRUE, "fcntl F_GETFL");
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
            err_exit(TRUE, "fcntl F_SETFL");

        dev->fd = fd;

        dev->connect_state = DEV_CONNECTED;
        dev->stat_successful_connects++;

        pd->cpid = pid;

        err(FALSE, "_pipe_connect(%s): opened on %s", dev->name, ptyname);
    }

    return (dev->connect_state == DEV_CONNECTED);
}

/*
 * Close down the pipes/pty.
 */
void pipe_disconnect(Device * dev)
{
    PipeDev *pd = (PipeDev *)dev->data;

    assert(dev->connect_state == DEV_CONNECTED);

    dbg(DBG_DEVICE, "_pipe_disconnect: %s on fd %d", dev->name, dev->fd);

    if (dev->fd >= 0) {
        if (close(dev->fd) < 0)
            err(TRUE, "_pipe_disconnect: %s close fd %d", dev->name, dev->fd);
        dev->fd = NO_FD;
    }

    /* reap child */
    if (pd->cpid > 0) {
        int wstat;

        kill(pd->cpid, SIGTERM); /* ignore errors */
        if (waitpid(pd->cpid, &wstat, 0) < 0)
            err(TRUE, "_pipe_disconnect(%s): wait", dev->name);
        if (WIFEXITED(wstat)) {
            err(FALSE, "_pipe_disconnect(%s): %s exited with status %d", 
                    dev->name, pd->argv[0], WEXITSTATUS(wstat));
        } else if (WIFSIGNALED(wstat)) {
            err(FALSE, "_pipe_disconnect(%s): %s terminated with signal %d", 
                    dev->name, pd->argv[0], WTERMSIG(wstat));
        } else {
            err(FALSE, "_pipe_disconnect(%s): %s terminated",
                    dev->name, pd->argv[0]);
        }
        pd->cpid = -1;
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
