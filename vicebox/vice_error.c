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
#include "vicebox.h"

/*
 * Somewhat like the sys_err() of Stevens, "UNIX Network Programming."
 */

void
exit_error(const char *fmt, ...)
{
	va_list ap;
	char buf[MAX_BUF];
	int er;
	int len;

	er = errno;
	snprintf(buf, MAX_BUF, "Vicebox: ");
	len = strlen(buf);
	va_start(ap, fmt);
	vsnprintf(buf + len, MAX_BUF - len, fmt, ap);
	len = strlen(buf);
	snprintf(buf + len, MAX_BUF - len, ": %s\n", strerror(er));
	fflush(stdout);
	fputs(buf, stderr);
	fflush(stderr);
	va_end(ap);
	exit(1);
}
