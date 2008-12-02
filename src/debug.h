/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2004-2008 The Regents of the University of California.
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

#ifndef PM_DEBUG_H
#define PM_DEBUG_H

#define DBG_ALWAYS          0x0000
#define DBG_DEVICE          0x0001
#define DBG_POLL            0x0002
#define DBG_CLIENT          0x0004
#define DBG_ACTION          0x0008
#define DBG_MEMORY          0x0010
#define DBG_TELNET          0x0020

#define DBG_NAME_TAB {                      \
    { DBG_DEVICE,       "device" },         \
    { DBG_POLL,         "poll"   },         \
    { DBG_CLIENT,       "client" },         \
    { DBG_ACTION,       "action" },         \
    { DBG_MEMORY,       "memory" },         \
    { DBG_TELNET,       "telnet" },         \
    { 0, NULL }                             \
}

void dbg_notty(void);
void dbg_setmask(unsigned long mask);
void dbg_wrapped(unsigned long channel, const char *fmt, ...);
char *dbg_memstr(char *mem, int len);

#define dbg(channel, fmt...)    dbg_wrapped(channel, fmt)

#endif /* PM_DEBUG_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
