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

/*
 * Implement connect/disconnect device methods for pipes.
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

#include "powerman.h"
#include "parse_util.h"
#include "device.h"
#include "device_pipe.h"
#include "error.h"
#include "wrappers.h"
#include "debug.h"

typedef struct {
    int argc;
    char **argv;
    pid_t cpid;
} PipeDev;

/* make a copy of the first word in str and advance str past it */
static char *_nextargv(char **strp)
{
    char *str = *strp;
    char *word; 
    int len;
    char *cpy = NULL;

    while (*str && (isspace(*str) || *str == '|')) /* skip '|' preceeding cmd */
        str++;
    word = str;
    while (*str && !isspace(*str))
        str++;
    len = str - word;

    if (len > 0) {
        cpy = (char *)Malloc(len + 1);
        memcpy(cpy, word, len);
        cpy[len] = '\0';
    }

    *strp = str;
    return cpy;
}

/* return number of space seperated words in str */
static int _sizeargv(char *str)
{
    int count = 0;

    do {
        while (*str && (isspace(*str) || *str == '|'))
            str++;
        if (*str)
            count++;
        while (*str && !isspace(*str))
            str++;
    } while (*str);

    return count;
}

void *pipe_create(char *cmdline, char *flags)
{
    PipeDev *pd = (PipeDev *)Malloc(sizeof(PipeDev));
    int i = 0; 

    pd->argc = _sizeargv(cmdline);
    pd->argv = (char **)Malloc(sizeof(char *) * (pd->argc + 1));
    for (i = 0; i < pd->argc; i++) {
        pd->argv[i] = _nextargv(&cmdline);
        assert(pd->argv[i] != NULL);
    }
    pd->argv[i] = NULL;
    
    pd->cpid = -1;

    return (void *)pd;
}

void pipe_destroy(void *data)
{
    PipeDev *pd = (PipeDev *)data;
    int i;

    for (i = 0; i < pd->argc; i++)
        Free(pd->argv[i]);
    Free(pd->argv);
    Free(pd);
}

/* Start the pipe command as a "coprocess", i.e. with both stdin and 
 * stdout/stderr under powerman control.
 */
bool pipe_connect(Device * dev)
{
    PipeDev *pd = (PipeDev *)dev->data;
    int rfd[2] = { -1, -1 }; /* parent(r) <- child(w) */
    int wfd[2] = { -1, -1 }; /* child(r) <- parent(w) */

    assert(dev->connect_state == DEV_NOT_CONNECTED);
    assert(dev->ifd == NO_FD);
    assert(dev->ofd == NO_FD);

    Pipe(rfd);
    Pipe(wfd);
    pd->cpid = Fork();

    if (pd->cpid == 0) {        /* child */
        Close(rfd[0]);          /* these ends belong to parent - close */
        Close(wfd[1]);   

        Dup2(rfd[1], 0);        /* dup stdin */
        Close(rfd[1]);

        Dup2(wfd[0], 1);        /* dup stdout/stderr */
        Dup2(wfd[0], 2);
        Close(wfd[0]);

        /* XXX: need to close other fd's */

        Execv(pd->argv[0], pd->argv);
        /*NOTREACHED*/

    } else {                    /* parent */
        int flags;

        Close(rfd[1]);          /* these ends belong to child - close */
        Close(wfd[0]);   

        dev->ifd = rfd[0];
        dev->ofd = wfd[1];

        flags = Fcntl(dev->ifd, F_GETFL, 0);
        Fcntl(dev->ifd, F_SETFL, flags | O_NONBLOCK);

        flags = Fcntl(dev->ofd, F_GETFL, 0);
        Fcntl(dev->ofd, F_SETFL, flags | O_NONBLOCK);

        dev->connect_state = DEV_CONNECTED;
        dev->stat_successful_connects++;
        /*dev->retry_count = 0;*/

        err(FALSE, "_pipe_connect: %s opened", dev->name);
    }
    return (dev->connect_state == DEV_CONNECTED);
}

/*
 * Close down the pipes.
 */
void pipe_disconnect(Device * dev)
{
    PipeDev *pd = (PipeDev *)dev->data;

    assert(dev->connect_state == DEV_CONNECTED);

    dbg(DBG_DEVICE, "_pipe_disconnect: %s on fds %d/%d", dev->name, 
            dev->ifd, dev->ofd);

    /* close devices if open */
    if (dev->ifd >= 0) {
        Close(dev->ifd);
        dev->ifd = NO_FD;
    }
    if (dev->ofd >= 0) {
        Close(dev->ofd);
        dev->ofd = NO_FD;
    }

    /* reap child */
    if (pd->cpid > 0) {
        int wstat;

        Waitpid(pd->cpid, &wstat, 0);
        if (WIFEXITED(wstat)) {
            err(FALSE, "pipe_disconnect: %s exited with status %d", 
                    pd->argv[0], WEXITSTATUS(wstat));
        } else if (WIFSIGNALED(wstat)) {
            err(FALSE, "pipe_disconnect: %s terminated with signal %d", 
                    pd->argv[0], WTERMSIG(wstat));
        }
        pd->cpid = -1;
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
