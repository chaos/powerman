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
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "list.h"
#include "error.h"

static char *err_prog = NULL;           /* basename of calling program */
static bool err_ttyvalid = true;        /* use stderr until told otherwise */

#define ERROR_BUFLEN 1024

/*
 * Initialize this module with the name of the program.
 */
void err_init(char *prog)
{
    char *p = strrchr(prog, '/');       /* store only the basename */

    err_prog = p ? p + 1 : prog;
}

/*
 * Accessor function for syslog_enable.
 */
void err_notty(void)
{
    err_ttyvalid = false;
}

/* helper for err, err_exit */
static void _verr(bool errno_valid, const char *fmt, va_list ap)
{
    char buf[ERROR_BUFLEN];
    int er = errno;

    assert(err_prog != NULL);

    vsnprintf(buf, ERROR_BUFLEN, fmt, ap);  /* overflow ignored on purpose */
    if (errno_valid) {
        if (err_ttyvalid)
            fprintf(stderr, "%s: %s: %s\n", err_prog, buf, strerror(er));
        else
            syslog(LOG_ERR, "%s: %s", buf, strerror(er));
    } else {
        if (err_ttyvalid)
            fprintf(stderr, "%s: %s\n", err_prog, buf);
        else
            syslog(LOG_ERR, "%s", buf);
    }
}

/*
 * Report error message on either stderr or syslog, then exit.
 */
void err_exit(bool errno_valid, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    _verr(errno_valid, fmt, ap);
    va_end(ap);
    exit(1);
}

/*
 * Report error message on either stderr or syslog, then return.
 */
void err(bool errno_valid, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    _verr(errno_valid, fmt, ap);
    va_end(ap);
}

void lsd_fatal_error(char *file, int line, char *mesg)
{
    err_exit(true, "%s", mesg);
}

void *lsd_nomem_error(char *file, int line, char *mesg)
{
    err_exit(false, "%s: out of memory", mesg);
    /*NOTREACHED*/
    return NULL;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
