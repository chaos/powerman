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
#include "buffer.h"


Log *log;

/*
 *   Constructor
 *
 * Produces:  Log
 */
void
make_log()
{
	log = (Log *)Malloc(sizeof(Log));
	log->name = NULL;
	log->fd = NO_FD;
	log->write = FALSE;
	log->to = NULL;
	log->level = -1;
}

/*
 *   It's imprtant to avoid openning this until after daemonization
 * is complete.  The log uses the buffer.c interface, and since that
 * interface can log errors, we have to dance a little carefully
 * there or we end up in infinet recursion.
 */ 
void
init_log(const char *name, int level)
{
	time_t t;
	int flags;

	log->name = make_String( name );
	flags = O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK;
	log->fd = Open(get_String(log->name), flags, S_IRUSR | S_IWUSR);
	log->to = make_Buffer(log->fd);
	t = time(NULL);
	log->level = level;
	log_it(0, "Started %s", ctime(&t));
}

/*
 *   Send message to log.  It has a nice vararg sturcture, but
 * just passes things along to the send_Buffer() routine.
 */ 
void
log_it(int level, const char *fmt, ...)
{
	va_list ap;
	char str[MAX_BUF];

	if (level > log->level) return;

	va_start(ap, fmt);
	vsnprintf(str, MAX_BUF, fmt, ap);
	va_end(ap);

	send_Buffer(log->to, "PowerManD:  %s\n", str);

	log->write = TRUE;
}

/*
 *   When the select indicates that some data may be written
 * do the writing here.  Just call the write_Buffer routine.
 */
void
handle_log()
{
	int n;

	n = write_Buffer(log->to);

}
/*
 *   Destructor
 *
 * Destroys:  Log
 */
void
free_Log()
{
	Free(log, sizeof(Log));
}
