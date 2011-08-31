/*****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
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
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>

#include "xtypes.h"
#include "list.h"
#include "error.h"
#include "debug.h"
#include "xmalloc.h"

static char *err_prog = NULL;           /* basename of calling program */
static bool err_ttyvalid = TRUE;        /* use stderr until told otherwise */

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
    err_ttyvalid = FALSE;
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
    dbg(DBG_MEMORY, "err_exit: memory not reclaimed: %d\n", xmemory());
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
    err_exit(TRUE, "%s", mesg);
}

void *lsd_nomem_error(char *file, int line, char *mesg)
{
    err_exit(FALSE, "%s: out of memory", mesg);
    /*NOTREACHED*/
    return NULL;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
