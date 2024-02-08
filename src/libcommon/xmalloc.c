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
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include "xmalloc.h"
#include "error.h"

char *xmalloc(int size)
{
    char *new;

    if (!(new = calloc(1, size)))
        err_exit(false, "out of memory");
    return new;
}

char *xrealloc(char *item , int newsize)
{
    char *new;

    if (!(new = realloc(item, newsize)))
        err_exit(false, "out of memory");
    return new;
}

void xfree(void *ptr)
{
    free(ptr);
}

char *xstrdup(const char *str)
{
    char *cpy;

    if (!(cpy = strdup(str)))
        err_exit(false, "out of memory");
    return cpy;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
