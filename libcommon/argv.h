/*****************************************************************************
 *  Copyright (C) 2003 The Regents of the University of California.
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

#ifndef PM_ARGV_H
#define PM_ARGV_H

/* Create a NULL-terminated argv array suitable for passing to execv()
 * from 'cmdline' string.  Characters in the 'ignore' set are treated as white
 * space.  Caller must free with argv_destroy().
 */
char **argv_create(char *cmdline, char *ignore);

/* Destroy an argv array created by argv_create.
 */
void argv_destroy(char **argv);

/* Return the number of elements in the argv array (less the NULL terminator).
 */
int argv_length(char **argv);

/* Expand an argv array by one slot and add an entry.
 */
char **argv_append(char **argv, char *s);


#endif /* PM_ARGV_H */
