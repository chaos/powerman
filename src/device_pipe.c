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

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/*
 * Implement connect/disconnect device methods for pipes.
 * Well it started out as a pipe, now actually it's a "coprocess" on a pty.
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pty.h>


#include "powerman.h"
#include "parse_util.h"
#include "wrappers.h"
#include "pluglist.h"
#include "device.h"
#include "device_pipe.h"
#include "error.h"
#include "debug.h"
#include "argv.h"

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
    PipeDev *pd = (PipeDev *)Malloc(sizeof(PipeDev));

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
    Free(pd);
}

/* Start the coprocess using forkpty(3).  */
bool pipe_connect(Device * dev)
{
    int fd;
    pid_t pid;
    PipeDev *pd = (PipeDev *)dev->data;
    char ptyname[64]; /* XXX forkpty doesn't have a 'len' parameter */

    assert(dev->connect_state == DEV_NOT_CONNECTED);
    assert(dev->fd == NO_FD);

#if HAVE_FORKPTY
    pid = forkpty(&fd, ptyname, NULL, NULL);
#else
#error Need forkpty function
#endif
    if (pid > 0) {
        assert(strlen(ptyname) < sizeof(ptyname));
    }

    if (pid < 0) {
        err(FALSE, "_pipe_connect(%s): forkpty error", dev->name);
    } else if (pid == 0) {      /* child */
        Execv(pd->argv[0], pd->argv);
        /*NOTREACHED*/

    } else {                    /* parent */
        int flags;

        flags = Fcntl(fd, F_GETFL, 0);
        Fcntl(fd, F_SETFL, flags | O_NONBLOCK);

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
        Close(dev->fd);
        dev->fd = NO_FD;
    }

    /* reap child */
    if (pd->cpid > 0) {
        int wstat;

        kill(pd->cpid, SIGTERM); /* ignore errors */
        Waitpid(pd->cpid, &wstat, 0);
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
