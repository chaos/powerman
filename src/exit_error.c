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
#include "powerman.h"
#include "main.h"


#ifndef NDUMP

bool Dump = FALSE;

#endif

/*
 * Somewhat like the sys_err() of Stevens, "UNIX Network Programming."
 */

void
exit_error(const char *fmt, ...)
{
	va_list ap;
	char buf[MAX_BUF];
	int er = errno;
	int i;

	va_start(ap, fmt);
	vsnprintf(buf, MAX_BUF, fmt, ap);
	va_end(ap);
	if( syslog_on == FALSE )  
	{
		if( er )
			fprintf(stderr, "PowerManD: %s: %s\n", buf, strerror(er));
		else
			fprintf(stderr, "PowerManD: %s\n", buf);
	}
	else
	{
		if( er )
			syslog(LOG_ERR, "%s: %m", buf);
		else
			syslog(LOG_ERR, "%s", buf);
	}
#ifndef NDUMP
	if ( !Dump )
	  {
	    Dump = TRUE;
	    dump_Globals(cheat);
	  }
	if( syslog_on == FALSE )  
		fflush(stderr);
#endif
	for (i = 0; i < MAXFD; i++)
		close(i);
	if( syslog_on == TRUE )
		closelog();
	exit(1);
}



#ifndef NDUMP

void
Report_Memory()
{
	fprintf(stderr, "Remaining allocated memory is: %d\n", 
		allocated_memory);
}



#endif

