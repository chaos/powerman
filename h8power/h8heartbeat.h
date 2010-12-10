/*****************************************************************************\
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

#ifndef _H8_HEARTBEAT_H
#define _H8_HEARTBEAT_H

/* heartbeat cache entry */
typedef struct {
    struct timeval  when;       /* time stamp of most recent heartbeat */
    struct timeval  last;       /* time stamp of heartbeat before that one */
    uint8_t         addr;       /* address */
    char            *name;      /* sp name */
    int             logged_dead;/* true if missing heartbeat logged */
} h8hb_t;

typedef ListIterator h8hb_iterator_t; 

/* Look up a heartbeat cache entry given address or sp name, returning NULL 
 * on search failure.  Entries may have expired - this can be tested with 
 * h8hb_expired().
 */
h8hb_t            *h8hb_lookup_byaddr(uint8_t addr);
h8hb_t            *h8hb_lookup_byname(char *name);

/* Iterator interface similar to those described in list.c for access to
 * the h8hb cache.
 */
h8hb_iterator_t     h8hb_iterator_create(void);
void                h8hb_iterator_destroy(h8hb_iterator_t itr);
h8hb_t              *h8hb_next(h8hb_iterator_t itr);

/* Return true if the cache entry has expired.
 */
int                 h8hb_expired(h8hb_t *sp);

/* Display the h8hb cache on the provided cbuf.
 */
void                h8hb_display_cache(cbuf_t outbuf);

/* Initialize/finalize module.  If non-NULL, 'spnames' is a comma-separated
 * list of sp names (possibly containing host ranges) that limits the names 
 * that will be cached.  All others will generate errors.  If non-NULL, 
 * 'ignore' is a hostlist that lists names to ignore, e.g. do not cache, and 
 * do not generate errors.
 */
void                h8hb_init(char *spnames, char *ignore);
void                h8hb_fini(void);

/* Interface for receiving inbound packets.  If a heartbeat packet, process it 
 * and update the cache, returning 1.  If not a heartbeat, return 0 so someone
 * else can process the packet.
 */
int                 h8hb_process_packet(h8pkt_t *pkt);

/* This is a hook for reducing collisions on the RS-485 wire.  Call it
 * before transmitting and it should cause the caller to sleep through
 * any anticipated heartbeat packets if transmitting would collide with one.
 */
void                h8hb_sleep_through_heartbeats(void);

/* Log missed or restored heartbeats.
 */
void                h8hb_process_late_heartbeats(struct timeval *tmout);

#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

