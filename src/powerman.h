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

#ifndef POWERMAN_H
#define POWERMAN_H

typedef enum { FALSE = 0, TRUE = 1 } bool;

typedef enum { NO_DEV, TCP_DEV } Dev_Type;

#define NO_FD           (-1)
#define	MAXFD	         64

/* #define NDEBUG 1 Don't produce debug code */
#ifndef NDEBUG
/* Use debugging macros */

#  define MAGIC            int magic
#  define INIT_MAGIC(x)    (x)->magic = (MAGIC_VAL)
#  define CHECK_MAGIC(x)   assert((x)->magic == MAGIC_VAL)
#  define CLEAR_MAGIC(x)   (x)->magic = (0)

#  define MAGIC_VAL          0xdeadbee0
#else
/* Don't use debugging macros */
#  define MAGIC
#  define INIT_MAGIC(x)
#  define CHECK_MAGIC(x)
#  define CLEAR_MAGIC(x)
#endif

#ifndef MAX
#  define MAX(x, y) (((x) > (y))? (x) : (y))
#endif                          /* MAX */
#ifndef MIN
#  define MIN(x, y) (((x) < (y))? (x) : (y))
#endif                          /* MIN */

#define DAEMON_NAME   		"powermand"
#define PID_FILE_NAME 		"/var/run/powerman/powerman.pid"
#define PID_DIR       		"/var/run/powerman"
#define ROOT_DIR      		"/etc/powerman"
#define DFLT_CONFIG_FILE 	"/etc/powerman/powerman.conf"

#endif                          /* POWERMAN_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
