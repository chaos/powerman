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

#ifndef BUFFER_H
#define BUFFER_H


#define MAX_BUF    64000          /* 2560 nodes with 16 chars per name   */
#define HIGH_WATER 200            /* wti and baytech have some sreen     */
#define LOW_WATER  200            /*            oriented display modes   */
#define MAX_MATCH        20

/* 
 * Each file descriptor is associated with two of these Buffer objects.  
 * They are for the kind of processing presented in Stevens, "UNIX 
 * Network Programming," (ch 15) where he discusses non-blocking reads 
 * and writes.
 */ 
struct buffer_struct {
	int fd;
	char buf[MAX_BUF + 1];
	char *start;
	char *end;
	char *in;
	char *out;
	char *high;
	char *low;
#ifndef NDEBUG
	char *hwm;   /* High Water Mark */
#endif
};


/* buffer.c extern prototypes */
extern Buffer *make_Buffer(int fd);
extern void send_Buffer(Buffer *b, const char *fmt, ...);
extern int write_Buffer(Buffer *b);
extern int read_Buffer(Buffer *b);
extern bool empty_Buffer(Buffer *b);
extern String *get_String_from_Buffer(Buffer *b, regex_t *re);
#ifndef NDEBUG
extern void dump_Buffer(Buffer *b);
#endif
extern void free_Buffer(void *b);
extern char *find_RegEx(regex_t *re, char *str, int len);


#endif
