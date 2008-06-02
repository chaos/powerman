/*****************************************************************************\
 *  $Id: wrappers.h 912 2008-05-31 01:28:46Z garlick $
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

#ifndef PM_XREAD_H
#define PM_XREAD_H

/* Read function that handles EINTR internally and terminates on all
 * other errors except EWOULDBLOCK and ECONNRESET.
 */
int xread(int fd, char *p, int max);

/* Write function that handles EINTR internally and terminates on all
 * other errors except EPIPE, EWOULDBLOCK, and ECONNRESET.
 */
int xwrite(int fd, char *p, int max);

#endif /* PM_XREAD_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
