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

#include "powerman.h"

#include "exit_error.h"
#include "wrappers.h"
#include "buffer.h"
#include "log.h"


#define BUF_MAGIC  0x4244052

/* 
 * Each file descriptor is associated with two of these Buffer objects.  
 * They are for the kind of processing presented in Stevens, "UNIX 
 * Network Programming," (ch 15) where he discusses non-blocking reads 
 * and writes.
 */ 
/* Review: add comments to structure members documenting purpose */
struct buffer_implementation {
	int magic;		/* magic cookie */
	int fd;
	char buf[MAX_BUF + 1];
	char *start;
	char *end;
	char *in;
	char *out;
	char *high;
	char *low;
#ifndef NDEBUG
	/* Review: document high water mark purpose */
	char *hwm;   		/* High Water Mark */
#endif
};

static void check_Buffer(Buffer b, int len);

Buffer
make_Buffer(int fd)
{
	Buffer b;

	assert(fd >= 0);
	b = (Buffer)Malloc(sizeof(struct buffer_implementation));
	b->magic = BUF_MAGIC;
	b->fd = fd;
	memset( &(b->buf), 0, MAX_BUF + 1 );
	b->start = b->in = b->out = b->buf;
	b->end = b->buf + MAX_BUF;
	b->low  = b->buf + LOW_WATER;
	b->high = b->end - HIGH_WATER;
#ifndef NDEBUG
	b->hwm = b->start;
#endif
	return b;
}



void
send_Buffer(Buffer b, const char *fmt, ...)
{
/*
 *   All incoming material is deposited at b->in.  The data 
 * waiting to be written out begins at b->out.  As soon as 
 * b->out == b->in the buffer is empty and both may be reset 
 * to b->start.  If the request asks for more than MAX_BUF 
 * space then everything after MAX_BUF bytes is ignored.  If 
 * the input writes above the buffer's high water mark then
 * shift the content down to the base of the buffer.  If the 
 * shift cannot occur or doesn't make enough space, then 
 * (and only then) the buffered write will block waiting for 
 * space to become available.    
 */
	char str[MAX_BUF + 1];
	va_list ap;
	int len;
	fd_set rset, wset;
	struct timeval tv;
	int maxfd;
	int res;
      
      	assert(b != NULL);	
      	assert(b->magic == BUF_MAGIC);
	maxfd = b->fd + 1;

/* get at most MAX_BUF of what the caller wanted to send */
	memset( str, 0, MAX_BUF + 1 );
	va_start(ap, fmt);
	res = vsnprintf(str, MAX_BUF, fmt, ap);
	assert(res != -1 && res <= MAX_BUF);
	va_end(ap);
	len = strlen(str);

/* Make room if needed and if possible */
	check_Buffer(b, len);

/* 
 *   If there's still no room at the inn block for a little while 
 * trying to make room.  Properly the 1 second delay below should
 * be tunable in the config file, but this is an utterly unused
 * code path (I think), so I've neglected it.  If for some reason
 * writes do back up then the program will hang in this loop 
 * indefinitely.  
 */

	/* Review: add loop control varaile to ensure loop will complete */
	/* Review: consider adding check_Buffer() to beginning of loop   */
	while( b->in + len > b->end )
	{
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(b->fd, &wset);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		Select(maxfd, &rset, &wset, NULL, &tv);
		if ( FD_ISSET(b->fd, &wset) )
			write_Buffer(b);
	}
	/* Review: consider assert(have space in buffer) */

/* We have space (the usual case from the start) */
	strncpy(b->in, str, len + 1);
	if( ! is_buffer_Log(b) ) /* avoid recursion, log uses buffer */
		send_Log(0, "send \"%s\" to descriptor %d", b->in, b->fd);
	b->in += len;
	*(b->in) = '\0';
}

/*
 *   If the ->in point gets too high we'd like to try to push it 
 * down by resetting ->out to ->start and moving the data.  It
 * isn't worth doing this unless we're going to see more than
 * minimum of benefit.  The main header sets "too high" to 
 * ->end - 200, and "minimum of benefit" to 200.  It would take
 * an extrordinary series of coincidences for this to ever be
 * helpfull, but it is a possibility so I've accounted for it.
 */
void
check_Buffer(Buffer b, int len)
{
	int del;
	int num;
	int i;

      	assert(b != NULL);	
      	assert(b->magic == BUF_MAGIC);

	del = b->out - b->start;
	num = b->in - b->out;

	assert(b->in >= b->out);
	if( (b->in + len >  b->high) &&
	    (b->out > b->low) )
	{
		if( ! is_buffer_Log(b) ) /* avoid recursion, log uses buffer  */
			send_Log(0, "Buffer shift required on descriptr %d", b->fd);
		/* Review: try memmove */
		for (i = 0; i <  num; i++)
			b->buf[i] = b->buf[i + del];
		b->out -= del;
		b->in  -= del;
		memset(b->in, 0, del);
	}
}


int
write_Buffer(Buffer b)
{
/* 
 *   Dead simple.  The only interasting piece is that 
 * the b->out pointer is being advanced.  It never gets
 * beyond b->in, though.  send_Buffer ensures that 
 * the buffer is never over run.
 */
	int n;

      	assert(b != NULL);	
      	assert(b->magic == BUF_MAGIC);

	n = Write(b->fd, b->out, (int) (b->in - b->out));
	if ( n < 0 ) return n; /* EWOULDBLOCK */

	memset(b->out, 0, n);
	b->out += n;
	assert(b->out <= b->in);
	if ( b->out == b->in )
		b->out = b->in = b->start;
	return n;
}

int
read_Buffer(Buffer b)
{
/*
 *   The caller will need to check the return code to see if the
 * connection got lost.  This would send handle_Client_read
 * into 
 *		c->read_status = CLI_DONE;
 * The Read return value is set to zero if it was just EWOULDBLOCK
 */
	int n;
	int i;
	int len;
	char str[MAX_BUF + 1];

      	assert(b != NULL);	
      	assert(b->magic == BUF_MAGIC);

	/* Review: try to read directly into Buffer buf */

        /* initial check of buffer, can we reset ->in and ->out? */
	assert(b->out <= b->in);
	if (b->out == b->in)
	{
		memset(b->start, 0, b->in - b->start);
		b->out = b->in = b->start;
	}

	/* get new bytes if there are any */
	/* Review: explicitly terminate string instead of zeroing memory? maybe? */
	memset( str, 0, MAX_BUF + 1 );
	n = Read(b->fd, str, MAX_BUF);
	if ( n <= 0) return n;

	/*
	 *  "If it is desired to transmit an actual carriage return 
	 * this is transmitted as a carriage return followed by a NUL 
	 * (all bits zero) character."
	 * http://www.scit.wlv.ac.uk/~jphb/comms/telnet.html
	 */
	/* Review: consider moving this check for NULL up */
	for (i = 0; i < n - 1; i++)
	  if(b->in[i] == '\0') b->in[i] = '\n';

	/* Make room if needed and possible */
	check_Buffer(b, n);

	/* If there still isn't room enough we'll just have to drop some */
	len = ( n > b->end - b->in ) ? b->end - b->in : n;
	strncpy(b->in, str, len + 1);
	b->in += len;
	*(b->in) = '\0';
	return n;
}

bool
is_empty_Buffer(Buffer b)
{
      	assert(b != NULL);	
      	assert(b->magic == BUF_MAGIC);

	return (b->in == b->out);
}


/*
 * When a buffer has received some sasynchronous data it
 * must be checked if there is anything to be interpreted 
 * yet.  If "re" is passed in as a NULL then the function
 * is looking for a newline terminated string.  If there
 * is one it is returned as a NUL terminated string copied 
 * into a character array.  If a RegEX "re" is passed in 
 * then it is used to determine where a "valid" input ends, 
 * usually it's some sort of prompt string like "prompt>".  
 * In either case if the function doesn't find valid input 
 * then it returns NULL, which just signifies that perhaps 
 * we haven't seen enough input yet.  As a special case
 * an empty line (just "\n" or "\r\n") is not returned to 
 * the caller as a line of input.
 */
int
get_str_from_Buffer(Buffer b, regex_t *re, char *str, int length)
{
	char *pos;
	int len;

      	assert(b != NULL);	
      	assert(b->magic == BUF_MAGIC);

	len = b->in - b->out;

	memset(str, 0, length);
	/* Review: len+1 should be less than MAX_BUF -- add assertion  */
	assert(len + 1 < length);
	strncpy(str, b->out, len + 1);
	if( re == NULL )
		/* get a line */
		pos = (char *)memchr(str, '\n', len);
	   	/* Review: consider advancing pos here to match 
	    	 * pos semantics of find_RegEx */
	else
		/* get a region - multiple lines or fraction there of */
		pos = find_RegEx(re, str, len);
	/* nothing found? */
	if( pos == NULL ) return 0;

/*
 * something was found, mark it as consumed from the buffer and possibly
 * strip trialing '\r' and/or '\n' from the end of a line.
*/
	/* Review: refactor following code */
	if( re == NULL )
	{
		/* memchr points at the '\n' */
		b->out += pos + 1 - str;
		if ( (pos > str) && (*(pos - 1) == '\r') ) pos--;
		/* was it an empty line? */
		if( pos == str ) return 0;
	}
	else
	{
		/* find_RegEx points to just past the match */
		b->out += pos - str;
	}
	*pos = '\0';
	send_Log(0, "Received \"%s\" from descriptor %d", str, b->fd);
	return strlen(str);
}


#ifndef NDEBUG

void
dump_Buffer(Buffer b)
{
/*
 *   This overcomplicated business ensures that pieces of the
 * buffer separated by NUL characters all get printed.
 */

/* Review: consider making this more robust if printing buffer 
 * with binary data */
	int i;
	int top;
	char *str;

      	assert(b != NULL);	
      	assert(b->magic == BUF_MAGIC);

	fprintf(stderr, "\t\tBuffer: %0x\n\t\t\t", (unsigned int)b);
	for(i = MAX_BUF; i > 0; i--)
		if(b->buf[i]) break;
	top = i;
	str = b->buf;
	for(i = 0; i < top; i++)
		if (b->buf[i] == '\0')
		{
			fprintf(stderr, "%s\\000", str);
			str = &(b->buf[i]);
		}
	fprintf(stderr, "\n");
	fprintf(stderr, "\t\t\tstart, out, in, end: %d, %d, %d, %d\n", 
		b->start - b->buf, b->out - b->buf, b->in - b->buf, 
		b->end - b->buf);
}

#endif

void
free_Buffer(void *b)
{
      	assert(b != NULL);	
      	assert(((Buffer)b)->magic == BUF_MAGIC);	
	Free(b);
}



/* Review: in general, don't name function in function comment */
/* Review: function comment is wrong */

/* 
 *   char *find_Reg_Ex(regex_t *re, char *str, in len) looks for "re" in 
 * the first "len" bytes of "str" and returns it's position if found 
 * or NULL if not found.
 */
char *
find_RegEx(regex_t *re, char *str, int len)
{
	int n;
	size_t nmatch = MAX_MATCH;
	regmatch_t pmatch[MAX_MATCH];
	int eflags = 0;

	send_Log(1, "find_RegEx of %s", str);
	/* Review: re_syntax options also set in Regexec() */
	n = Regexec(re, str, nmatch, pmatch, eflags);
	if (n != REG_NOERROR) return NULL;
	if ((pmatch[0].rm_so < 0) || (pmatch[0].rm_eo > len))
		return NULL;
	else
		return str + pmatch[0].rm_eo;
}
