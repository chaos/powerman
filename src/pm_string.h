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

#ifndef PM_STRING_H
#define PM_STRING_H


#define NO_INDEX        (-1)

/* 
 *   This structure is for holding strings mostly, but also can capture
 * indexed items like tux0, tux1, ... .  If there is a single index then
 * the code below for prefix is correct.  A range may have a '[', though.
 * In that case prefix points to the '['.  This structure can only
 * accomodate a contiguous range.  It would require a list of these to 
 * hold tux[1-3, 5-7]. 
 */
struct string_struct {
	int length; /* length = strlen(string) */
	int width;  /* width of index field, i.e. width(tux02) = 2 */
	int prefix; /* prefix=length-1;while(isdigit(string[prefix]))prefix--; */
	int index;  /* n = sscanf(string + prefix, "%d", &index); */
	int count;  /* if ( n == 0 ) try to get a range index..index+count */
	char *string;
} ;


/* pm_string.c extern prototypes */
extern String *make_String(const char *cs);
extern String *copy_String(String *s);
extern char *get_String(String *s);
extern unsigned char byte_String(String *s, int offset);
extern int length_String(String *s);
extern int prefix_String(String *s);
extern int index_String(String *s);
extern int cmp_String(String *s1, String *s2);
extern bool prefix_match(String *s1, String *s2);
extern bool empty_String(String *s);
extern void free_String(void *s);
extern bool match_String(String *s, char *cs);




#endif
