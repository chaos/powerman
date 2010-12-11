/*****************************************************************************\
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>.
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

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>

#include "cbuf.h"
#include "hprintf.h"
#include "error.h"
#include "wrappers.h"

/* basename of calling program */
static char             *err_prog = NULL;  
static char             *err_name = NULL;

/* error reporting destination */
static errdest_type_t   err_dest_type = ERR_NONE;
static void             *err_dest = NULL;

#define ERROR_BUFLEN 1024


/* Initialize error module.  'prog' is the name of the calling program
 * and will be the prefix of each error message.  Start logging to stderr.
 */
void err_init(char *prog)
{
    char *p = strrchr(prog, '/');       /* store only the basename */

    err_prog = p ? p + 1 : prog;
    err_redirect(ERR_FILE, stderr);

    openlog(err_prog, LOG_NDELAY | LOG_PID, LOG_DAEMON);
}

/* Set name prefix for syslog.  This is used to distinguish one instance of 
 * h8power from another in the logs.
 */
void err_setname(char *name)
{
    err_name = name;
}

/* Redirect error messages.
 */
void err_redirect(errdest_type_t dest_type, void *dest)
{
    err_dest_type = dest_type;
    err_dest = dest;
}

/* Common error/debug handing function for err(), err_exit(), and dbg().
 */
static void _verr(int syslog_level, const char *fmt, va_list ap)
{
    char buf[ERROR_BUFLEN];

    vsnprintf(buf, ERROR_BUFLEN, fmt, ap);  /* overflow ignored */
    switch (err_dest_type) {
        case ERR_FILE:
            assert(err_prog != NULL);
            assert(err_dest != NULL);
            fprintf((FILE *)err_dest, "%s: %s\n", err_prog, buf);
            break;
        case ERR_CBUF:
            assert(err_prog != NULL);
            assert(err_dest != NULL);
            /* may call malloc - inappropriate for out of mem errs */
            cbuf_printf((cbuf_t)err_dest, "%s: %s\n", err_prog, buf);
            break;
        case ERR_SYSLOG:
            if (err_name)
                syslog(syslog_level, "%s: %s", err_name, buf);
            else
                syslog(syslog_level, "%s", buf);
            break;
        case ERR_NONE:
            break;
    }
}

/* Report error message and exit.
 */
void err_exit(const char *fmt, ...)
{
    va_list ap;

    /* NOTE: If exiting, we can't use cbuf for output since that would require 
     * another trip thru select loop.  Switch to stderr.
     * This also makes it OK to call err_exit() on out of memory errors, as
     * we do from lsd_nomem_error() below.
     */
    if (err_dest_type == ERR_CBUF)
        err_redirect(ERR_FILE, stderr);

    va_start(ap, fmt);
    _verr(LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

/* Report error message.
 */
void err(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    _verr(LOG_ERR, fmt, ap);
    va_end(ap);
}

/* Report debug message.
 */
void dbg(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    _verr(LOG_DEBUG, fmt, ap);
    va_end(ap);
}

/* Error routines for cbuf.c, list.c, wrappers.c
 */
void lsd_fatal_error(char *file, int line, char *mesg)
{
    err_exit("ERROR: [%s::%d] %s: %s", file, line, mesg, strerror(errno));
}

void *lsd_nomem_error(char *file, int line, char *mesg)
{
    err_exit("OUT OF MEMORY: [%s::%d] %s", file, line, mesg);
    /*NOTREACHED*/
    return NULL;
}

/* Printf functions for cbufs.
 */
void cbuf_vprintf(cbuf_t cbuf, const char *fmt, va_list ap)
{
    char *str;
    int written, dropped;

    str = hvsprintf(fmt, ap);
    if (str == NULL)
        err_exit("cbuf_printf: out of memory");

    written = cbuf_write(cbuf, str, strlen(str), &dropped);
    if (written < 0)
        err_exit("cbuf_printf: cbuf write: %m");

    Free(str);
}

void cbuf_printf(cbuf_t cbuf, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    cbuf_vprintf(cbuf, fmt, ap);
    va_end(ap);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
