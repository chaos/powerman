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

#define MAX_BUF    8192

typedef struct buffer_implementation *Buffer;

typedef void (BufferLogFun) (unsigned char *mem, int len, void *arg);
Buffer buf_create(int fd, int length, BufferLogFun logfun, void *logfunarg);
void buf_destroy(Buffer b);
bool buf_printf(Buffer b, const char *fmt, ...);
int buf_write(Buffer b);
int buf_read(Buffer b);
bool buf_isempty(Buffer b);
void buf_clear(Buffer b);

int buf_getline(Buffer b, unsigned char *str, int len);
int buf_peekline(Buffer b, unsigned char *str, int len);
int buf_getstr(Buffer b, unsigned char *str, int len);
int buf_peekstr(Buffer b, unsigned char *str, int len);
char *buf_getregex(Buffer b, regex_t * re);
void buf_eat(Buffer b, int len);

#endif				/* BUFFER_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
