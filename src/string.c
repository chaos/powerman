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
#include "string.h"
#include "wrappers.h"
#include "error.h"

#define STRING_MAGIC 0xabbaabba

struct string_implementation {
    int magic;			/* COOKIE!!! */
    unsigned char *string;	/* string, NULL terminated */
};


/* 
 * Create a String object from character string 'cs'. 
 * Result must be freed with str_destroy().
 */
String str_create(const unsigned char *cs)
{
    String s;

    s = (String) Malloc(sizeof(struct string_implementation));
    s->magic = STRING_MAGIC;
    s->string = cs ? Strdup(cs) : NULL;
    return s;
}

/*
 * Free a String object.
 */
void str_destroy(String s)
{
    assert(s);
    assert(s->magic == STRING_MAGIC);
    if (s->string)
	Free(s->string);
    Free(s);
}

/* 
 * Copy a String object.  Result must be freed with str_destroy().
 */
String str_copy(String s)
{
    assert(s);
    assert(s->magic == STRING_MAGIC);
    return str_create(s->string);
}

/*
 * Return a pointer to the string portion of the String.  Result may be NULL.
 */
unsigned char *str_get(String s)
{
    assert(s);
    assert(s->magic == STRING_MAGIC);
    return s->string;
}

/*
 * Return the nth byte of String.
 */
unsigned char str_byte(String s, int n)
{
    assert(s);
    assert(s->magic == STRING_MAGIC);
    assert(s->string);
    assert(n >= 0 && n < strlen(s->string));
    return s->string[n];
}

/*
 * Return the length of String (not including NULL).
 */
int str_length(String s)
{
    assert(s);
    assert(s->magic == STRING_MAGIC);
    return s->string ? strlen(s->string) : 0;
}

/*
 * Return TRUE if string is empty.
 */
bool str_isempty(String s)
{
    assert(s);
    assert(s->magic == STRING_MAGIC);
    return (str_length(s) == 0) ? TRUE : FALSE;
}

/*
 * Return TRUE if String 's' is equal to character string 'cs'.
 */
bool str_match(String s, unsigned char *cs)
{
    assert(s);
    assert(s->magic == STRING_MAGIC);
    assert(s->string);
    assert(cs);
    return (strcmp(s->string, cs) == 0) ? TRUE : FALSE;
}


/*
 * vi:softtabstop=4
 */
