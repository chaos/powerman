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
 * Somewhat like the sys_err() of Stevens, "UNIX Network Programming."
 */

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>

#include "powerman.h"
#include "list.h"
#include "exit_error.h"

static char *program = NULL;	/* basename of calling program */
static bool syslog_on = FALSE;	/* use stderr until told otherwise */
static ErrCallback error_callback = NULL; /* called just before exit */

#define ERROR_BUFLEN 1024

static void
_make_error_callback(void)
{
	ErrCallback cb = error_callback;

	error_callback = NULL; /* prevent recursion */
	if (cb)
		cb();
}

/*
 * Initialize this module with the name of the program.
 */
void
init_error(char *prog, ErrCallback cb)
{
	char *p = strrchr(prog, '/'); /* store only the basename */

	program = p ? p + 1 : prog;

	error_callback = cb;
}

/*
 * Accessor function for syslog_enable.
 */
void
syslog_on_error(bool syslog_enable)
{
	syslog_on = syslog_enable;
}

/*
 * Report message on either stderr or syslog, then exit.
 */
void
exit_msg(const char *fmt, ...)
{
	va_list ap;
	char buf[ERROR_BUFLEN];

	assert(program != NULL);

	va_start(ap, fmt);
	vsnprintf(buf, ERROR_BUFLEN, fmt, ap);
	buf[sizeof(buf) - 1] = '\0'; /* ensure termination after overflow */
	va_end(ap);

	if ( syslog_on == TRUE )  
		syslog(LOG_ERR, "%s", buf);
	else
		fprintf(stderr, "%s: %s\n", program, buf);

	_make_error_callback();
	exit(1);
}

/*
 * Report error message on either stderr or syslog, then exit.
 * This should only be called in situations where errno is known to be valid.
 */
void
exit_error(const char *fmt, ...)
{
	va_list ap;
	char buf[ERROR_BUFLEN];
	int er = errno;

	assert(program != NULL);

	va_start(ap, fmt);
	vsnprintf(buf, ERROR_BUFLEN, fmt, ap);
	buf[sizeof(buf) - 1] = '\0'; /* ensure termination after overflow */
	va_end(ap);

	if ( syslog_on == TRUE )  
		syslog(LOG_ERR, "%s: %s", buf, strerror(er));
	else
		fprintf(stderr, "%s: %s: %s\n", program, buf, strerror(er));

	_make_error_callback();
	exit(1);
}
