/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
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

#ifndef PM_POWERMAN_H
#define PM_POWERMAN_H

#define DAEMON_NAME   		"powermand"
#define PID_FILE_NAME 		"/var/run/powerman/powerman.pid"
#define PID_DIR       		"/var/run/powerman"
#ifndef CONFIG_DIR
#define CONFIG_DIR          "/etc/powerman"
#endif
#define CONFIG_FILE 	    "powerman.conf"

#define DFLT_PORT           "10101"
#define DFLT_HOSTNAME       "localhost"

#endif /* PM_POWERMAN_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
