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

#include "powerman.h"

#include "exit_error.h"
#include "wrappers.h"
#include "buffer.h"
#include "log.h"


#define BUF_MAGIC  0x4244052


struct buffer_implementation {
	int magic;		/* magic cookie */
	int fd;			/* file descriptor */
	int len;		/* size allocated to buffer */
	char *buf;		/* buffer */
	char *in;		/* incoming goes here (points into buffer) */
	char *out;		/* outgoing starts here (points into buffer)  */
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
	assert((b)->fd >= 0); \
} while(0)

/*
 * Construct a Buffer of specified length.
 */
Buffer
make_Buffer(int fd, int length)
{
	Buffer b;

	assert(fd >= 0);
	b = (Buffer)Malloc(sizeof(struct buffer_implementation));
	b->magic = BUF_MAGIC;
	b->fd = fd;
	b->len = length;
	b->in = b->out = b->buf = Malloc(b->len);
	return b;
}

/*
 * Free a Buffer.
 */
void
free_Buffer(void *vb)
{
	Buffer b = (Buffer)vb;

	_buf_check(b);
	Free(b->buf);
	Free(b);
}

/*
 * Drop data specified in printf style into Buffer.
 * Compact Buffer if space is low and data could then be appended.
 * If there is still insufficient space, block up to 1 second until 
 * there is space.  Return TRUE if data was successfully added to buffer.
 */
bool
send_Buffer(Buffer b, const char *fmt, ...)
{
	va_list ap;
	int len;
     
       	_buf_check(b);	

	/* compact the buffer */
	if (b->out > b->buf)
	{
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
	b->in += len; /* does not include NULL byte */
	return TRUE;
}

/*
 * Write out any data in buffer.  Return the number of bytes written or 
 * -1 on error (e.g. EWOULDBLOCK).
 */
int
write_Buffer(Buffer b)
{
	int nbytes;

	_buf_check(b);
	nbytes = Write(b->fd, b->out, _buf_length(b));
	if (nbytes > 0) 
		b->out += nbytes;
	_buf_check(b);
	return nbytes;
}

/*
 * Read in any available to buffer.  Return the number of bytes read or 
 * -1 on error (e.g. EAGAIN).
 */
int
read_Buffer(Buffer b)
{
	int nbytes;

	_buf_check(b);
	nbytes = Read(b->fd, b->in, _buf_length_post(b));
	if (nbytes > 0)
		b->in += nbytes;
	_buf_check(b);
	return nbytes;
}

/*
 * Return TRUE if buffer is empty.
 */
bool
is_empty_Buffer(Buffer b)
{
	_buf_check(b);
	return (_buf_length(b) == 0);
}

static void
_zap_trailing_whitespace_str(char *str)
{
	char *p = str + strlen(str) - 1;
	while (strlen(str) > 0 && isspace(*p))
		*p-- = '\0';
}

static void
_zap_leading_whitespace_buf(Buffer b)
{
	while (_buf_length(b) > 0 && isspace(*b->out))
		b->out++;
}

/* 
 * Get a line from the buffer.
 * A line is terminated with a '\n' character.
 * This function presumes that concepts like "whitespace" apply to the buffer.
 */
int
get_line_Buffer(Buffer b, char *str, int len)
{
	char *p;
	int cpy_len;

	_buf_check(b);
	if (is_empty_Buffer(b))
		return 0;
	p = memchr(b->out, '\n', _buf_length(b));
	if (p == NULL)
		return 0;
	cpy_len = p - b->out + 1;
	assert(cpy_len < len);
	memcpy(str, b->out, cpy_len);
	str[cpy_len] = '\0';

	b->out += cpy_len;

	/*_zap_trailing_whitespace_str(str);
	_zap_leading_whitespace_buf(b);*/

	return cpy_len;
}

/*
 * Get the contents of the buffer.
 * Optionally Call eat_Buffer with the returned length to "consume" this.
 */
int
peek_string_Buffer(Buffer b, char *str, int len)
{
	int cpy_len;

	_buf_check(b);
	cpy_len = _buf_length(b);
	if (cpy_len == 0)
		return 0;
	assert(cpy_len < len);
	memcpy(str, b->out, cpy_len);
	str[cpy_len] = '\0';
	/*_zap_trailing_whitespace_str(str);*/

	return cpy_len;
}

/* Call after peek_string_Buffer to remove the peeked data from Buffer */
void
eat_Buffer(Buffer b, int len)
{
	_buf_check(b);
	assert(len <= _buf_length(b));
	b->out += len;
	/*_zap_leading_whitespace_buf(b);*/
}

#ifndef NDEBUG
void
dump_Buffer(Buffer b)
{
	fprintf(stderr, "dump_Buffer not implemented\n");
}
#endif /* !NDEBUG */
