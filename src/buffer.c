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

#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>

#include "powerman.h"

#include "error.h"
#include "wrappers.h"
#include "util.h"
#include "buffer.h"
#include "debug.h"


#define BUF_MAGIC  0x4244052


struct buffer_implementation {
    int magic;			/* magic cookie */
    int fd;			/* file descriptor */
    BufferLogFun *logfun;	/* log function (NULL OK) */
    void *logfunarg;		/* argument to log function */
    int len;			/* size allocated to buffer */
    unsigned char *buf;		/* buffer */
    unsigned char *in;		/* incoming goes here (points into buffer) */
    unsigned char *out;		/* outgoing starts here (points into buffer)  */
};

/* length of various segments of buffer */
#define _buf_length(b)		((b)->in - (b)->out)
#define _buf_length_pre(b)	((b)->out - (b)->buf)
#define _buf_length_post(b)	((b)->len - _buf_length(b) - _buf_length_pre(b))

/* invariants to be checked on every call */
#define _buf_check(b)	do { \
	assert(b); \
	assert((b)->magic == BUF_MAGIC); \
	assert((b)->in >= (b)->out); \
} while(0)

#define _buf_isopen(b)	do { assert((b)->fd >= 0); } while(0)

/*
 * Construct a Buffer of specified length.
 *  fd (IN)	file descriptor associated with buffer
 *  logfun (IN) if non-null, logfun(b->out, nbytes, b->logfunarg)
 *		will be called on every read/write to file descriptor
 *  logfunarg (IN)  see above
 *  RETURN	new Buffer object (malloc failure terminates program)
 */
Buffer buf_create(int fd, int length, BufferLogFun * logfun, void *logfunarg)
{
    Buffer b;

    b = (Buffer) Malloc(sizeof(struct buffer_implementation));
    b->magic = BUF_MAGIC;
    b->fd = fd;
    b->logfun = logfun;
    b->logfunarg = logfunarg;
    b->len = length;
    b->in = b->out = b->buf = Malloc(b->len);
    return b;
}

/*
 * Free a Buffer.
 *  b (IN)	Buffer object to free.
 */
void buf_destroy(Buffer b)
{
    _buf_check(b);
    Free(b->buf);
    Free(b);
}

/*
 * Printf into buffer.  Do a memmove to get rid of unused space at
 * beginning of buffer.  
 *  b (IN)	Buffer that is written to
 *  fmt (IN)	Printf style format string
 *  args... (IN) Printf style Variable args
 *  RETURN	TRUE if data was s uccessfully written, FALSE if insufficient
 *		room in buffer.
 */
bool buf_printf(Buffer b, const char *fmt, ...)
{
    va_list ap;
    int len;

    _buf_check(b);

    /* compact the buffer */
    if (b->out > b->buf) {
	int used = _buf_length(b);

	memmove(b->buf, b->out, used);
	b->out = b->buf;
	b->in = b->buf + used;
    }

    /* write the "string" */
    va_start(ap, fmt);
    len = vsnprintf(b->in, _buf_length_post(b), fmt, ap);
    va_end(ap);
    if (len == -1 || len > _buf_length_post(b))
	return FALSE;
    b->in += len;		/* does not include NULL byte */
    return TRUE;
}

/*
 * Write out any data in buffer.  
 *  b (IN)	Buffer to flush.
 *  RETURN	number of bytes written or -1 on error (e.g. EWOULDBLOCK)
 */
int buf_write(Buffer b)
{
    int nbytes;

    _buf_check(b);
    _buf_isopen(b);
    nbytes = Write(b->fd, b->out, _buf_length(b));
    if (nbytes > 0) {
	if (b->logfun != NULL)
	    b->logfun(b->out, nbytes, b->logfunarg);
	b->out += nbytes;
	_buf_check(b);
	dbg(DBG_BUFFER, "wrote %d bytes to fd %d", nbytes, b->fd);
    } else {
	dbg(DBG_BUFFER, "write error on fd %d", b->fd);
    }
	
    return nbytes;
}

/*
 * Read in any available to buffer.  
 *  b (IN)	Buffer to append 
 *  RETURN	number of bytes read or * -1 on error (e.g. EAGAIN).
 */
int buf_read(Buffer b)
{
    int nbytes;

    _buf_check(b);
    _buf_isopen(b);
    nbytes = Read(b->fd, b->in, _buf_length_post(b));
    if (nbytes > 0) {
	if (b->logfun != NULL)
	    b->logfun(b->in, nbytes, b->logfunarg);
	b->in += nbytes;
	_buf_check(b);
	dbg(DBG_BUFFER, "read %d bytes from fd %d", nbytes, b->fd);
    } else {
	dbg(DBG_BUFFER, "read error on fd %d: %s", b->fd, strerror(errno));
    }
    return nbytes;
}

/*
 * Is buffer empty?
 *  b (IN)	Buffer to check
 *  RETURN	TRUE if buffer is empty.
 */
bool buf_isempty(Buffer b)
{
    _buf_check(b);
    return (_buf_length(b) == 0);
}

/* 
 * Get a copy of a line from the buffer.
 * A line is terminated with a \n character
 * Optionally Call buf_eat with the returned length to "consume" this.
 * Note: converts embedded \0 bytes to \377.
 *  b (IN)	Buffer to get a line from
 *  str (OUT)	where the line will be stored (will be \0 terminated)
 *  len (IN)	size of str.
 *  RETURN	number of bytes copied NOT including terminating \0.
 */
int buf_peekline(Buffer b, unsigned char *str, int len)
{
    unsigned char *p;
    int cpy_len;
    int i, j;

    _buf_check(b);
    if (buf_isempty(b))
	return 0;
    p = memchr(b->out, '\n', _buf_length(b));
    if (p == NULL)
	return 0;
    cpy_len = p - b->out + 1;
    assert(cpy_len < len);
    /* Was: memcpy(str, b->out, cpy_len) */
    for (j = i = 0; i < cpy_len; i++)
	str[j++] = b->out[i] == 0 ? 0377 : b->out[i];
    str[j] = '\0';

    return cpy_len;
}

/* 
 * Get a line from the buffer.
 * A line is terminated with a \n character
 * Note: converts embedded \0 bytes to \377.
 *  b (IN)	Buffer to get a line from
 *  str (OUT)	where the line will be stored (will be \0 terminated)
 *  len (IN)	size of str.
 *  RETURN	number of bytes copied NOT including terminating \0.
 */
int buf_getline(Buffer b, unsigned char *str, int len)
{
    int cpy_len;

    cpy_len = buf_peekline(b, str, len);
    buf_eat(b, cpy_len);
    return cpy_len;
}

/*
 * Get a copy of the ENTIRE contents of the buffer in string form.
 * Optionally Call buf_eat with the returned length to "consume" this.
 * Note: converts embedded \0 bytes to \377.
 *  b (IN)	Buffer to get a str from
 *  str (OUT)	where the str will be stored (will be \0 terminated)
 *  len (IN)	size of str.
 *  RETURN	number of bytes copied NOT including terminating \0.
 */
int buf_peekstr(Buffer b, unsigned char *str, int len)
{
    int cpy_len;
    int i, j;

    _buf_check(b);
    cpy_len = _buf_length(b);
    if (cpy_len == 0)
	return 0;
    assert(cpy_len < len);
    /* Was: memcpy(str, b->out, cpy_len) */
    for (j = i = 0; i < cpy_len; i++)
	str[j++] = b->out[i] == 0 ? 0377 : b->out[i];
    str[j] = '\0';
    return cpy_len;
}

/*
 * Get a copy of the ENTIRE contents of the buffer in string form.
 * Note: converts embedded \0 bytes to \377.
 *  b (IN)	Buffer to get a str from
 *  str (OUT)	where the str will be stored (will be \0 terminated)
 *  len (IN)	size of str.
 *  RETURN	number of bytes copied NOT including terminating \0.
 */
int buf_getstr(Buffer b, unsigned char *str, int len)
{
    int cpy_len;

    cpy_len = buf_peekstr(b, str, len);
    buf_eat(b, cpy_len);
    return cpy_len;
}

/*
 * Apply regular expression to the contents of a Buffer.
 * If there is a match, return (and consume) from the beginning
 * of the buffer to the last character of the match.
 * NOTE: embedded \0 chars are converted to \377 by buf_getstr/buf_getline and
 * buf_peekstr/buf_getstr because libc regex functions would treat these as 
 * string terminators.  As a result, \0 chars cannot be matched explicitly.
 *  b (IN)	buffer to apply regex to
 *  re (IN)	regular expression
 *  RETURN	String match (caller must free) or NULL if no match
 */
char *buf_getregex(Buffer b, regex_t * re)
{
    unsigned char str[MAX_BUF];
    int bytes_peeked = buf_peekstr(b, str, MAX_BUF);
    unsigned char *match_end;

    if (bytes_peeked == 0)
	return NULL;
    match_end = util_findregex(re, str, bytes_peeked);
    if (match_end == NULL)
	return NULL;
    assert(match_end - str <= strlen(str));
    *match_end = '\0';
    buf_eat(b, match_end - str);	/* only consume up to what matched */

    return Strdup(str);
}

/* 
 * Call after buf_peekstr or buf_peekline to remove the peeked 
 * data from Buffer 
 *  b (IN)	buffer to consume
 *  len (IN)	number of bytes to consume
 */
void buf_eat(Buffer b, int len)
{
    _buf_check(b);
    assert(len <= _buf_length(b));
    b->out += len;
}

/* 
 * Emtpy the Buffer.
 *  b (IN)	buffer to consume
 */
void buf_clear(Buffer b)
{
    _buf_check(b);
    b->in = b->out = b->buf;
}

/*
 * vi:softtabstop=4
 */
