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
#include <assert.h>

#include "powerman.h"
#include "pm_string.h"
#include "wrappers.h"
#include "exit_error.h"

#define MAX_STR_BUF 64000

#define STRING_MAGIC 0xabbaabba

struct string_implementation {
	int magic;
	int length;
	char *string;
};


String
make_String(const char *cs)
{
	String s;

	s = (String)Malloc(sizeof(struct string_implementation));
	s->magic = STRING_MAGIC;
	if( cs == NULL )
	{
		s->string = NULL;
		s->length = 0;
		return s;
	}
	s->length = strlen(cs);
	s->string = Malloc(s->length + 1);
	strncpy(s->string, cs, s->length);
	s->string[s->length] = '\0';

	return s;
}

String
copy_String(String s)
{
	String new;

	if( s == NULL ) return NULL;

	assert(s->magic == STRING_MAGIC);

	new = (String)Malloc(sizeof(struct string_implementation));
	*new = *s;
	if (s->length > 0) {
		assert(s->string != NULL);
		assert(strlen(s->string) == s->length);
		new->string = Malloc(s->length + 1);
		strncpy(new->string, s->string, s->length + 1);
	}
	return new;
}


char *
get_String(String s)
{
	assert(s != NULL);
	assert(s->magic == STRING_MAGIC);
	return s->string;
}

unsigned char 
byte_String(String s, int offset)
{
	assert( s != NULL );
	assert(s->magic == STRING_MAGIC);
	assert( (offset >= 0) && (offset < s->length) );
	return s->string[offset];
}

int 
length_String(String s)
{
	assert( s != NULL );
	assert(s->magic == STRING_MAGIC);
	return s->length;
}

bool
empty_String(String s)
{
	assert( s != NULL );
	assert(s->magic == STRING_MAGIC);
	return (s->string == NULL );
}

void
free_String(void *str)
{
	String s = str;

	if( s != NULL )
	{
		assert(s->magic == STRING_MAGIC);
		if( s->string != NULL )
		{
			Free(s->string);
			s->string = NULL;
		}
		Free(s);
	}
}

bool
match_String(String s, char *cs)
{
	int len;
	int n;

	assert(s != NULL);
	assert(s->magic == STRING_MAGIC);

	len = strlen(cs);
	if(len != s->length) 
		return FALSE;
	if( (n = strncmp(s->string, cs, len)) == 0 ) 
		return TRUE;
	return FALSE;
}
