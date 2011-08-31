/*****************************************************************************
 *  Copyright (C) 2003-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
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

#ifndef PM_HPRINTF_H
#define PM_HPRINTF_H

/* A vsprintf-like function that allocates the string on the heap and ensures
 * that no truncation occurs.  The caller must Free() the resulting string.
 */
char *hvsprintf(const char *fmt, va_list ap);

/* An sprintf-like function that allocates the string on the heap.
 * The caller must Free() the resulting string.
 */
char *hsprintf(const char *fmt, ...);

int hfdprintf(int fd, const char *format, ...);

#endif /* PM_HPRINTF_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
