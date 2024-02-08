/************************************************************\
 * Copyright (C) 2003 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

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

#include "hostlist.h"
#include "list.h"
#include "cbuf.h"
#include "parse_util.h"
#include "xmalloc.h"
#include "xpoll.h"
#include "pluglist.h"
#include "arglist.h"
#include "xregex.h"
#include "device_private.h"
#include "device_pipe.h"
#include "error.h"
#include "debug.h"
#include "argv.h"
#include "fdutil.h"

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
        err_exit(true, "_pipe_connect(%s): forkpty error", dev->name);
    } else if (pid == 0) {      /* child */
        xcfmakeraw(STDIN_FILENO);
        execv(pd->argv[0], pd->argv);
        err_exit(true, "exec %s", pd->argv[0]);
    } else {                    /* parent */
        nonblock_set(fd);

        dev->fd = fd;

        dev->connect_state = DEV_CONNECTED;
        dev->stat_successful_connects++;

        pd->cpid = pid;

        err(false, "_pipe_connect(%s): opened on %s", dev->name, ptyname);
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
            err(true, "_pipe_disconnect: %s close fd %d", dev->name, dev->fd);
        dev->fd = NO_FD;
    }

    /* reap child */
    if (pd->cpid > 0) {
        int wstat;

        kill(pd->cpid, SIGTERM); /* ignore errors */
        if (waitpid(pd->cpid, &wstat, 0) < 0)
            err(true, "_pipe_disconnect(%s): wait", dev->name);
        if (WIFEXITED(wstat)) {
            err(false, "_pipe_disconnect(%s): %s exited with status %d",
                    dev->name, pd->argv[0], WEXITSTATUS(wstat));
        } else if (WIFSIGNALED(wstat)) {
            err(false, "_pipe_disconnect(%s): %s terminated with signal %d",
                    dev->name, pd->argv[0], WTERMSIG(wstat));
        } else {
            err(false, "_pipe_disconnect(%s): %s terminated",
                    dev->name, pd->argv[0]);
        }
        pd->cpid = -1;
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
