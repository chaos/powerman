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
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "fdutil.h"
#include "error.h"

void nonblock_set(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        err_exit(true, "fcntl F_GETFL");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        err_exit(true, "fcntl F_SETFL");
}

void nonblock_clr(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        err_exit(true, "fcntl F_GETFL");
    if (fcntl(fd, F_SETFL, flags & ~(O_NONBLOCK)) < 0)
        err_exit(true, "fcntl F_SETFL");
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
