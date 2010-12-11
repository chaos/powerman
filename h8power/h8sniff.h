/*****************************************************************************\
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick (garlick@llnl.gov)
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
#ifndef _H8_SNIFF_H
#define _H8_SNIFF_H

/* Hook to display a packet in textual form.
 */
void h8sniff_pkt(h8pkt_t *pkt);

typedef enum { SNIFF_NONE=0, SNIFF_HEADER=1, SNIFF_DATA=2, SNIFF_ALL=3 } 
               sniff_level_t;

/* Configure the level of verbosity for packet sniffing and provide a cbuf
 * to send the packet descriptions to.
 */
void h8sniff_init(cbuf_t cb, sniff_level_t lev);

#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
