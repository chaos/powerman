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
 */

#include "powerman.h"

String *make_String(const char *cs)
{
/*
 * This needs to be modified to read canonical ranges as well as indexed
 * strings.  The only recognized form of canonical range is 
 * "base[index1-index2]" where base is a nonempty string that does not
 * end in a digit, and index1 and index2 are integers with index1 < index2.
 * There is no whitespace inside the "[...]".   Finaly, the width of the
 * two integers must be the same, as in tux[000-256].
 */

	String *s;
	char buf[MAX_BUF];
	int n;
	int index2;
	char *leftb  = NULL;
	char *dash   = NULL;
	char *rightb = NULL;

	s = (String *)Malloc(sizeof(String));
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
		errno = 0;
		n = sscanf(buf, "%d", &(s->index));
		if( n != 1 )
			exit_error("Failure interpreting first index of \"%s\"", cs);
		ASSERT( s->width == rightb - dash - 1 );
		strncpy(buf, dash + 1, s->width);
		buf[s->width] = '\0';
		errno = 0;
		n = sscanf(buf, "%d", &(index2));
		if( n != 1 )
			exit_error("Failure interpreting second index of \"%s\"", cs);
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
			errno = 0;
			n = sscanf(s->string + s->prefix, "%d", &(s->index));
			if( n != 1)
				exit_error("Failure interpreting %s", cs);
		}
		else
			s->index = NO_INDEX;
		s->count = 1;
	}
	return s;
}

String *
copy_String(String *s)
{
	String *new;
	
	if( s == NULL ) return NULL;
	new = (String *)Malloc(sizeof(String));
	if( s->length == 0 ) 
	{
		new->string = NULL;
		new->length = 0;
		new->prefix = NO_INDEX;
		new->index = NO_INDEX;
		new->width = 0;
		new->count = 0;
		return new;
	}
	ASSERT( s->string != NULL );
	new->string = Malloc(s->length + 1);
	new->length = s->length;
	strncpy(new->string, s->string, s->length);
	new->string[new->length] = '\0';
	new->prefix = s->prefix;
	new->index = s->index;
	new->width = s->width;
	new->count = s->count;
	return new;
}


char *get_String(String *s)
{
	ASSERT( s != NULL );
	return s->string;
}

unsigned char 
byte_String(String *s, int offset)
{
	ASSERT( s != NULL );
	ASSERT( (offset >= 0) && (offset < s->length) );
	return s->string[offset];
}

int 
length_String(String *s)
{
	ASSERT( s != NULL );
	return s->length;
}


int 
prefix_String(String *s)
{
	ASSERT( s != NULL );
	return s->prefix;
}


int 
index_String(String *s)
{
	ASSERT( s != NULL );
	return s->index;
}


int 
cmp_String(String *s1, String *s2)
{
	int result = 0;
	int len;

	ASSERT( (s1 != NULL) && (s2 != NULL) );
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
prefix_match(String *s1, String *s2)
{
	int n;

	ASSERT( (s1 != NULL) && (s2 != NULL) );

	if( s1->prefix != s2->prefix ) return FALSE;
	n = strncmp(s1->string, s2->string, s1->prefix);
	if( n == 0 ) return TRUE;
	else return FALSE;
}


bool
empty_String(String *s)
{
	ASSERT( s != NULL );
	return (s->string == NULL );
}

void
free_String(void *s)
{
	if( (String *)s != NULL )
	{
		if(((String *)s)->string != NULL )
		{
			Free(((String *)s)->string, ((String *)s)->length + 1);
			((String *)s)->string = NULL;
		}
		Free((String *)s, sizeof(String));	
	}
}

bool
match_String(String *s, char *cs)
{
	int len;
	int n;

	len = strlen(cs);
	if(len != s->length) return FALSE;
	if( (n = strncmp(s->string, cs, len)) == 0 ) return TRUE;
	return FALSE;
}


