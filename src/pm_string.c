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

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "powerman.h"
#include "pm_string.h"
#include "wrappers.h"
#include "exit_error.h"

#define MAX_STR_BUF 64000

#define STRING_MAGIC 0xabbaabba

/* 
 *   This structure is for holding strings mostly, but also can capture
 * indexed items like tux0, tux1, ... .  If there is a single index then
 * the code below for prefix is correct.  A range may have a '[', though.
 * In that case prefix points to the '['.  This structure can only
 * accomodate a contiguous range.  It would require a list of these to 
 * hold tux[1-3, 5-7]. 
 */
struct string_implementation {
	int magic;
	int length; /* length = strlen(string) */
	int width;  /* width of index field, i.e. width(tux02) = 2 */
	int prefix; /* prefix=length-1;while(isdigit(string[prefix]))prefix--; */
	int index;  /* n = sscanf(string + prefix, "%d", &index); */
	int count;  /* if ( n == 0 ) try to get a range index..index+count */
	char *string;
};


String
make_String(const char *cs)
{
/*
 * This needs to be modified to read canonical ranges as well as indexed
 * strings.  The only recognized form of canonical range is 
 * "base[index1-index2]" where base is a nonempty string that does not
 * end in a digit, and index1 and index2 are integers with index1 < index2.
 * There is no whitespace inside the "[...]".   Finaly, the width of the
 * two integers must be the same, as in tux[000-256].
 */

	String s;
	char buf[MAX_STR_BUF];
	int n;
	int index2;
	char *leftb  = NULL;
	char *dash   = NULL;
	char *rightb = NULL;

	s = (String)Malloc(sizeof(struct string_implementation));
	s->magic = STRING_MAGIC;
	if( cs == NULL )
	{
		s->string = NULL;
		s->length = 0;
		s->prefix = NO_INDEX;
		s->index = NO_INDEX;
		s->width = 0;
		s->count = 0;
		return s;
	}
	s->length = strlen(cs);
	leftb  = (char *)memchr(cs, '[', s->length);
	if( leftb != NULL )
	    dash   = (char *)memchr(leftb, '-', s->length - (leftb - cs));
	if( dash !=  NULL )
		rightb = (char *)memchr(dash, ']', s->length - (dash - cs));
	s->string = Malloc(s->length + 1);
	strncpy(s->string, cs, s->length);
	s->string[s->length] = '\0';
	if( rightb != NULL )
	{
		s->prefix = leftb - cs;
		s->width = dash - leftb - 1;
		strncpy(buf, leftb + 1, s->width);
		buf[s->width] = '\0';
		n = sscanf(buf, "%d", &(s->index));
		if( n != 1 )
			exit_msg("Failure interpreting first index of \"%s\"", cs);
		ASSERT( s->width == rightb - dash - 1 );
		strncpy(buf, dash + 1, s->width);
		buf[s->width] = '\0';
		n = sscanf(buf, "%d", &(index2));
		if( n != 1 )
			exit_msg("Failure interpreting second index of \"%s\"", cs);
		ASSERT(index2 > s->index);
		s->count = index2 - s->index;
	}
	else
	{
		s->prefix = s->length;
		while( (s->prefix > 0) && isdigit(s->string[s->prefix - 1]) )
			s->prefix--;
		s->width = s->length - s->prefix;
		if( s->width > 0 )
		{
			n = sscanf(s->string + s->prefix, "%d", &(s->index));
			if( n != 1)
				exit_msg("Failure interpreting %s", cs);
		}
		else
			s->index = NO_INDEX;
		s->count = 1;
	}
	return s;
}

String
copy_String(String s)
{
	String new;

	if( s == NULL ) return NULL;

	ASSERT(s->magic == STRING_MAGIC);

	new = (String)Malloc(sizeof(struct string_implementation));
	*new = *s;
	if (s->length > 0) {
		ASSERT(s->string != NULL);
		ASSERT(strlen(s->string) == s->length);
		new->string = Malloc(s->length + 1);
		strncpy(new->string, s->string, s->length + 1);
	}
	return new;
}


char *
get_String(String s)
{
	ASSERT(s != NULL);
	ASSERT(s->magic == STRING_MAGIC);
	return s->string;
}

unsigned char 
byte_String(String s, int offset)
{
	ASSERT( s != NULL );
	ASSERT(s->magic == STRING_MAGIC);
	ASSERT( (offset >= 0) && (offset < s->length) );
	return s->string[offset];
}

int 
length_String(String s)
{
	ASSERT( s != NULL );
	ASSERT(s->magic == STRING_MAGIC);
	return s->length;
}


int 
prefix_String(String s)
{
	ASSERT( s != NULL );
	ASSERT(s->magic == STRING_MAGIC);
	return s->prefix;
}


int 
index_String(String s)
{
	ASSERT( s != NULL );
	ASSERT(s->magic == STRING_MAGIC);
	return s->index;
}


int 
cmp_String(String s1, String s2)
{
	int result = 0;
	int len;

	ASSERT( (s1 != NULL) && (s2 != NULL) );
	ASSERT(s1->magic == STRING_MAGIC);
	ASSERT(s2->magic == STRING_MAGIC);
	if( (s1->length == 0) && (s2->length == 0) )
		return 0;
	if( s1->length == 0 )
		return -1;
	if( s2->length == 0 )
		return 1;
	ASSERT( (s1->string != NULL) && (s2->string != NULL) );
	len = MIN(s1->prefix, s2->prefix);
	result = strncmp(s1->string, s2->string, len);
	if( result != 0 ) return result;
	if( s1->prefix < s2->prefix ) return -1;
	if( s1->prefix > s2->prefix ) return  1;
	if( s1->index == NO_INDEX )   return -1;
	if( (s1->index != NO_INDEX) && (s2->index == NO_INDEX) ) return 1;
	return s1->index - s2->index;
}

bool
prefix_match(String s1, String s2)
{
	int n;

	ASSERT( (s1 != NULL) && (s2 != NULL) );
	ASSERT(s1->magic == STRING_MAGIC);
	ASSERT(s2->magic == STRING_MAGIC);

	if( s1->prefix != s2->prefix ) return FALSE;
	n = strncmp(s1->string, s2->string, s1->prefix);
	if( n == 0 ) return TRUE;
	else return FALSE;
}


bool
empty_String(String s)
{
	ASSERT( s != NULL );
	ASSERT(s->magic == STRING_MAGIC);
	return (s->string == NULL );
}

void
free_String(void *str)
{
	String s = str;

	if( s != NULL )
	{
		ASSERT(s->magic == STRING_MAGIC);
		if( s->string != NULL )
		{
			Free(s->string, s->length + 1);
			s->string = NULL;
		}
		Free(s, sizeof(struct string_implementation));	
	}
}

bool
match_String(String s, char *cs)
{
	int len;
	int n;

	ASSERT(s != NULL);
	ASSERT(s->magic == STRING_MAGIC);

	len = strlen(cs);
	if(len != s->length) return FALSE;
	if( (n = strncmp(s->string, cs, len)) == 0 ) return TRUE;
	return FALSE;
}


