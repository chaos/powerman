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

#include <assert.h>

#include "powerman.h"
#include "wrappers.h"
#include "exit_error.h"
#include "pm_string.h"
#include "buffer.h"
#include "log.h"

#define LOG_MAGIC 0xabc123

typedef struct {
	int magic;
	String name;
	int fd;
	bool writeable;
	Buffer to;
	int level;
} Log;

static Log *log = NULL;

/*
 *   Constructor
 *
 * Produces:  Log
 */
void
make_log(void)
{
	log = (Log *)Malloc(sizeof(Log));
	log->magic = LOG_MAGIC;
	log->name = NULL;
	log->fd = NO_FD;
	log->writeable = FALSE;
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

	assert(log != NULL);
	assert(log->magic == LOG_MAGIC);
	if (log->name != NULL)
		exit_msg("log can only be initialized once");
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

	assert(log != NULL);
	assert(log->magic == LOG_MAGIC);
	if (level > log->level) return;

	va_start(ap, fmt);
	vsnprintf(str, MAX_BUF, fmt, ap);
	va_end(ap);

	send_Buffer(log->to, "PowerManD:  %s\n", str);

	log->writeable = TRUE;
}

/*
 *   When the select indicates that some data may be written
 * do the writing here.  Just call the write_Buffer routine.
 */
void
handle_log(void)
{
	int n;

	assert(log != NULL);
	assert(log->magic == LOG_MAGIC);
	n = write_Buffer(log->to);

}
/*
 *   Destructor
 *
 * Destroys:  Log
 */
void
free_Log(void)
{
	assert(log != NULL);
	assert(log->magic == LOG_MAGIC);
	Free(log, sizeof(Log));
	log = NULL;
}

/* Needed to detect recursion in buffer package */
bool
is_log_buffer(Buffer b)
{
	if (log != NULL && log->to == b)
		return TRUE;
	else
		return FALSE;
}

/* Needed to test file descriptor in main select loop */
int
fd_log(void)
{
	assert(log != NULL);
	assert(log->magic == LOG_MAGIC);
	return log->fd;
}

bool
writeable_log(void)
{
	assert(log != NULL);
	assert(log->magic == LOG_MAGIC);
	return log->writeable;
}
