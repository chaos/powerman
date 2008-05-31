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
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "xmalloc.h"
#include "hprintf.h"

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

int xdprintf(int fd, const char *format, ...)
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
        rc = Write(fd, str, n);
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
