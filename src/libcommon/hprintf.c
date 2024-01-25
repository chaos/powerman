/************************************************************\
 * Copyright (C) 2003 The Regents of the University of California.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "xmalloc.h"
#include "hprintf.h"
#include "xread.h"

#define CHUNKSIZE 80
char *hvsprintf(const char *fmt, va_list ap)
{
    int len, size = 0;
    char *str = NULL;

    do {
        va_list vacpy;

        str = (size == 0) ? xmalloc(CHUNKSIZE) : xrealloc(str, size+CHUNKSIZE);
        size += CHUNKSIZE;

        va_copy(vacpy, ap);
        len = vsnprintf(str, size, fmt, vacpy); /* always null terminates */
        va_end(vacpy);
    } while (len == -1 || len >= size);
    assert(len == strlen(str));
    return str;
}

char *hsprintf(const char *fmt, ...)
{
    char *str;
    va_list ap;

    va_start(ap, fmt);
    str = hvsprintf(fmt, ap);
    va_end(ap);

    return str;
}

int hfdprintf(int fd, const char *format, ...)
{
    char *str, *p;
    va_list ap;
    int n, rc;

    va_start(ap, format);
    str = hvsprintf(format, ap);
    va_end(ap);

    p = str;
    n = strlen(p);
    rc = 0;
    do {
        rc = xwrite(fd, str, n);
        if (rc < 0)
            return rc;
        n -= rc;
        p += rc;
    } while (n > 0);
    xfree(str);

    return n;
}


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
