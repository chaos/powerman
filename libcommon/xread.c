/*****************************************************************************
 *  Copyright (C) 2001 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  UCRL-CODE-2002-008.
 *
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see http://code.google.com/p/powerman/
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
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include "xread.h"
#include "xtypes.h"
#include "xmalloc.h"
#include "error.h"

int xread(int fd, char *p, int max)
{
    int n;

    do {
        n = read(fd, p, max);
    } while (n < 0 && errno == EINTR);
    if (n < 0 && errno != EWOULDBLOCK && errno != ECONNRESET)
        err_exit(TRUE, "read");
    return n;
}

#define CHUNKSIZE 80
char *xreadstr(int fd)
{
    int size = 0;
    int len = 0;
    char *str = NULL;
    int n;

    do {
        if (size - len - 1 <= 0) {
            str = (size == 0) ? xmalloc(CHUNKSIZE)
                              : xrealloc(str, size + CHUNKSIZE);
            size += CHUNKSIZE;
        }
        //n = xread(fd, str + len, size - len - 1);
        n = xread(fd, str + len, 1);
        if (n < 0)
            err_exit (TRUE, "read");
        if (n == 0)
            err_exit (FALSE, "EOF on read");
        len += n;
        str[len] = '\0';
    } while (len < 2 || strcmp(&str[len - 2], "\r\n") != 0);

    str[len - 2] = '\0';
    return str;
}

int xwrite(int fd, char *p, int max)
{
    int n;

    do {
        n = write(fd, p, max);
    } while (n < 0 && errno == EINTR);
    if (n < 0 && errno != EAGAIN && errno != ECONNRESET && errno != EPIPE)
        err_exit(TRUE, "write");
    return n;
}

void xwrite_all(int fd, char *p, int count)
{
    int n;
    int done = 0;

    while (done < count) {
        n = xwrite(fd, p + done, count - done);
        if (n < 0)
            err_exit(TRUE, "write");
        done += n;
    }
}

void xread_all(int fd, char *p, int count)
{
    int n;
    int done = 0;

    while (done < count) {
        n = xread(fd, p + done, count - done);
        if (n < 0)
            err_exit(TRUE, "read");
        if (n == 0)
            err_exit(FALSE, "EOF on read");
         done += n;
    }
}

static void _zap_trailing_whitespace(char *s)
{
    char *p = s + strlen(s) - 1;

    while (p >= s && isspace(*p))
        *p-- = '\0';
}

char *xreadline(char *prompt, char *buf, int buflen)
{
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, buflen, stdin) == NULL)
        return NULL;
    _zap_trailing_whitespace(buf);
    return buf;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
