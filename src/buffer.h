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

#include <regex.h>

#define MAX_BUF    64000          /* 2560 nodes with 16 chars per name   */

typedef struct buffer_implementation *Buffer;

Buffer make_Buffer(int fd, int length);
void free_Buffer(void *vb);
bool send_Buffer(Buffer b, const char *fmt, ...);
int write_Buffer(Buffer b);
int read_Buffer(Buffer b);
bool is_empty_Buffer(Buffer b);

int get_line_Buffer(Buffer b, unsigned char *str, int len);
int peek_line_Buffer(Buffer b, unsigned char *str, int len);
int get_string_Buffer(Buffer b, unsigned char *str, int len);
int peek_string_Buffer(Buffer b, unsigned char *str, int len);
void eat_Buffer(Buffer b, int len);

#ifndef NDEBUG
void dump_Buffer(Buffer b);
#endif

#endif /* BUFFER_H */
