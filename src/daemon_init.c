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
  These are Stevens'.  I put this stuff in powerman.h.
   #include	"unp.h" 
   #include	<syslog.h>
*/
#include "powerman.h"
#include "wrappers.h"
#include "exit_error.h"
#include "daemon_init.h"

#define TMPSTR_LEN 32

void
daemon_init()
{
	int i;
	int n;
	int len;
	int fd;
	pid_t pid;
	char pidstr[TMPSTR_LEN];
	char buf[TMPSTR_LEN];
	time_t t;
	


	if ( (pid = Fork()) != 0 )
		exit(0);			/* parent terminates */

	/* 1st child continues */
	setsid();				/* become session leader */

	Signal(SIGHUP, SIG_IGN);

	if ( (pid = Fork()) != 0 )
		exit(0);			/* 1st child terminates */

	/* 2nd child continues */

	/* change working directory */
	n = chdir(ROOT_DIR);
	if( n == -1 ) exit_error("Requires access to root directory");
	n = mkdir(PID_DIR, S_IRWXU);
	if( (n == -1) && (errno != EEXIST) )
		exit_error("Couldn't create dirsctory /var/run/powerman");
	fd = open(PID_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, S_IRWXU);
	if( fd == -1 ) exit_error("Failed to open %s", PID_FILE_NAME);
	sprintf(pidstr, "%d\n", pid = getpid());
	len = strlen(pidstr);
	n = write(fd, pidstr, len);
	if( n != len ) exit_msg("Failed to write out pid value");
	close(fd);

	/* clear our file mode creation mask */
	umask(0);

	for (i = 0; i < MAXFD; i++)
		close(i);


	sprintf(buf, "Started %s", ctime(&t));
	openlog("PowerManD", LOG_NDELAY |  LOG_PID, LOG_DAEMON);
	syslog_on_error(TRUE);
	syslog(LOG_NOTICE, buf);
/*
#ifndef NDEBUG
	sleep(30);
#endif
*/
}
