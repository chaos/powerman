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
#ifndef _H8_POWER_H
#define _H8_POWER_H

typedef enum { POWER_OFF, POWER_ON, POWER_STAT } powercmd_t;

/* Initialize / finalize module 
 */
void h8power_init(void);
void h8power_fini(void);

/* Return true if there are commands in process.  This is used by the command
 * line parser to test whether to reissue the prompt.
 */
int h8power_pending(void);

/* Initiate a power command 'cmd' to node 'sp'.
 */
void h8power_cmd(h8hb_t *sp, powercmd_t cmd);

/* Call at the bottom of the select loop to process any received power packets.
 * Return true if packet was a power packet.
 */
int h8power_process_packet(h8pkt_t *pkt, cbuf_t ttyout);

/* Call at the top of the select loop to process any pending requests.
 */
void h8power_process_pending(struct timeval *tmout, cbuf_t ttyout);

#endif
