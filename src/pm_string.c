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

#define STRING_MAGIC 0xabbaabba

/* length == 0 iff string == NULL */
struct string_implementation {
	int magic;		/* COOKIE!!! */
	int length;		/* string length (not including NULL) */
	char *string;		/* string, NULL terminated */
};


/* 
 * Create a String object from character string 'cs'.  If cs is NULL, 
 * Result must be freed with free_String().
 */
String
make_String(const char *cs)
{
	String s;

	s = (String)Malloc(sizeof(struct string_implementation));
	s->magic = STRING_MAGIC;
	s->string = NULL;
	s->length = 0;
	if (cs != NULL)
	{
		s->length = strlen(cs);
		s->string = Malloc(s->length + 1);
		strncpy(s->string, cs, s->length + 1); /* copies NULL */
	}
	return s;
}

/*
 * Free a String object.  The argument is (void *) for compat with list.c.
 */
void
free_String(void *str)
{
	String s = str;

	assert(s != NULL);
	assert(s->magic == STRING_MAGIC);
	s->magic = 0;
	if(s->string)
	{
		assert(strlen(s->string) == s->length);
		Free(s->string);
	}
	Free(s);
}

/* 
 * Copy a String object.  Result must be freed with free_String().
 */
String
copy_String(String s)
{
	String new;

	assert (s != NULL) ;
	assert(s->magic == STRING_MAGIC);
	new = (String)Malloc(sizeof(struct string_implementation));
	*new = *s;
	if (s->string) 
	{
		assert(strlen(s->string) == s->length);
		new->string = Malloc(s->length + 1);
		strncpy(new->string, s->string, s->length + 1); /* copies NULL*/
	}
	return new;
}

/*
 * Return a pointer to the string portion of the String.  Result may be NULL.
 */
char *
get_String(String s)
{
	assert(s != NULL);
	assert(s->magic == STRING_MAGIC);
	return s->string;
}

/*
 * Return the nth byte of String.
 */
unsigned char 
byte_String(String s, int n)
{
	assert(s != NULL);
	assert(s->magic == STRING_MAGIC);
	assert(n >= 0 && n < s->length);
	return s->string[n];
}

/*
 * Return the length of String (not including NULL).
 */
int 
length_String(String s)
{
	assert(s != NULL);
	assert(s->magic == STRING_MAGIC);
	return s->length;
}

/*
 * Return TRUE if string is empty.
 */
bool
empty_String(String s)
{
	assert(s != NULL);
	assert(s->magic == STRING_MAGIC);
	return (s->string == NULL);
}

/*
 * Return TRUE if String 's' is equal to character string 'cs'.
 */
bool
match_String(String s, char *cs)
{
	int len;

	assert(s != NULL);
	assert(s->magic == STRING_MAGIC);
	assert(cs != NULL);

	len = strlen(cs);
	if (len != s->length) 
		return FALSE;
	if (strncmp(s->string, cs, len) == 0) 
		return TRUE;
	return FALSE;
}
