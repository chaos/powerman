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
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "powerman.h"
#include "wrappers.h"
#include "exit_error.h"
#include "pm_string.h"
#include "buffer.h"
#include "log.h"

#define LOG_MAGIC 0xabc123

typedef struct {
	int magic;		/* magic cookie */
	char *filename;		/* name of log file */
	int fd;			/* file descriptor for log file */
	Buffer buffer;		/* buffer associated with log */
	int level;		/* log level: 0=no logging,1,2,...,N */
} Log;

static Log *log = NULL;

/*
 * Allocate Log structure, and set up everything so start_Log() can
 * start logging.  The reason for a separate start_Log() call is that
 * make_Log() is called during config file parsing before daemonization,
 * and daemonization aggressively closes open file descriptors.
 */
void
make_Log(const char *filename, int level)
{
	if (log != NULL) /* config file error? */
		exit_msg("log can only be initialized once");
	log = (Log *)Malloc(sizeof(Log)); 
	log->magic = LOG_MAGIC;
	log->filename = filename ? Strdup(filename) : NULL;
	log->level = level;

	/* these will be initialized by start_Log() */
	log->fd = NO_FD;
	log->buffer = NULL;

	/* verify that log file can be opened */
	if (log->filename)
		Close(Open(log->filename, O_WRONLY | O_CREAT | O_APPEND, 
					S_IRUSR | S_IWUSR));
}

/*
 * Start the logging subsystem.
 */
void
start_Log(void)
{
	assert(log != NULL);
	assert(log->magic == LOG_MAGIC);
	if (log->filename != NULL)
	{	
		int flags = O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK;

		log->fd = Open(log->filename, flags, S_IRUSR | S_IWUSR);
		log->buffer = make_Buffer(log->fd, MAX_BUF);
		send_Log(0, "Log started fd %d", log->fd);
	}
}

/* Return a time string of the form "Wed Jun 30 21:49:08: " */
static char *
_get_timestr(void)
{
	time_t t = Time(NULL);
	char *timestr = ctime(&t);
       
	if (timestr)
	{
		char *p = strrchr(timestr, ' '); /* ends in " 2002\n" */

		assert(p);
		strcpy(p, ": ");		/* overwrite year with ": " */
	}
	return timestr;
}

/* 
 * Copy timestamp into beginning of string pointer.
 * Advance string pointer and subtract string length from len.
 */
static void
_copy_timestamp(char *str, int len)
{
	char *timestr = _get_timestr();

	if (timestr)
	{
		strncpy(str, timestr, len);
		str[len - 1] = '\0';	/* ensure NULL-termination */
	}
}

/* Add newline to end of 'str' */
static void
_append_newline(char *str, int len)
{
	strncat(str, "\n", len);
	str[len - 1] = '\0';			/* ensure NULL termination */
}

/* Write truncation message on end of string */
#define TRUNC_MSG "[truncated]\n"
static void
_insert_trunc_msg(char *str, int len)
{
	int pos = len - strlen(TRUNC_MSG) - 2;

	assert(pos >= 0);
	strcpy(&str[pos], TRUNC_MSG);
	str[len - 1] = '\0';
}

/*
 * Log a message to the log file if level of message is <= current level.
 */ 
void
send_Log(int level, const char *fmt, ...)
{
	va_list ap;
	char str[MAX_BUF];
	const int len = sizeof(str);

	assert(log);
	assert(log->magic == LOG_MAGIC);
	if (log->filename && level <= log->level)  
	{
		int res;

		_copy_timestamp(str, len);
		va_start(ap, fmt);
		res = vsnprintf(str + strlen(str), len - strlen(str), fmt, ap);
		if (res == -1 || res > len - strlen(str)) 
			_insert_trunc_msg(str, len);
		va_end(ap);
		_append_newline(str, len);

		/* XXX this can block if the buffer fills up! */
		send_Buffer(log->buffer, str);
	}
}

/*
 * Free storage associated with the log and close the file.
 */
void
free_Log(void)
{
	assert(log);
	assert(log->magic == LOG_MAGIC);
	if (log->filename) 
	{
		Free(log->filename);
		free_Buffer(log->buffer);
		Close(log->fd);
	}
	Free(log);
	log = NULL;
}

/* 
 * Main select loop calls this to decide whether to watch the log fd.
 */
bool
write_pending_Log(void)
{
	assert(log != NULL);
	assert(log->magic == LOG_MAGIC);
	return (log->filename != NULL && !is_empty_Buffer(log->buffer));
}

/* 
 * Main select loop gets log fd from here if it is going to watch log fd.
 */
int
fd_Log(void)
{
	assert(log != NULL);
	assert(log->magic == LOG_MAGIC);
	return log->fd;
}

/*
 * Main select loop calls this if log fd is ready.
 */
void
handle_Log(void)
{
	assert(log != NULL);
	assert(log->magic == LOG_MAGIC);
	write_Buffer(log->buffer);
}
