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

#ifndef _H8_PROTO_H
#define _H8_PROTO_H

#define H8_BCAST_ADDR       0x1f
#define H8_INVAL_ADDR       0
#define H8_MASTER_ADDR      1

#define H8_MIN_PKTLEN       (H8_DATA_OFFSET + 2)
#define H8_MAX_PKTLEN       256 /* XXX a guess - hb packets are 156 */

#define H8PKT_MAGIC 0xfeebdaed
typedef struct {
    /* metadata - not in packet */
    int     magic;
    int     tot_len;
    int     trail_len;

    /* header - fixed offsets from beginning */
    uint8_t *start_sentinel;
    uint8_t *dst;
    uint8_t *src;
    uint8_t *len;               /* specifies data length */

    /* data - variable length */
    uint8_t *data;
    uint8_t *trail;             /* XXX purpose of these bytes not understood */

    /* trailer - fixed offsets from end */
    uint8_t *crc;
    uint8_t *end_sentinel;

    /* pointers above reference into this */
    uint8_t raw[H8_MAX_PKTLEN];
} h8pkt_t;

/* fixed offsets */
#define H8_DST_OFFSET       9
#define H8_SRC_OFFSET       10
#define H8_LEN_OFFSET       12
#define H8_DATA_OFFSET	    13
#define H8_CRC_OFFSET       (-1)

/* sentinel values */
#define H8_END_SENTINEL	    0xff
#define H8_START_SENTINEL   "\375\375\375\375\375\375\375\375\374"

/* Some characters (esp those that conflict with sentinels) must be escaped,
 * e.g. transformed according to H8_ESCAPE then prefixed with H8_ESCAPE_BYTE.
 * All bytes are subject to escape processing except start/end sentinels.
 */
#define H8_ESCAPE_BYTE      0xfb
#define H8_MUST_ESCAPE(c)   ((c) >= 0xfa)
#define H8_ESCAPE(c)        (0xff - (c))
#define H8_ESCAPE_START_OFFSET H8_DST_OFFSET
#define H8_ESCAPE_END_OFFSET(pkt) ((pkt)->tot_len - 2)

/* To calculated the CRC, take the sum of each byte in packet except start/end 
 * sentinels, crc byte, and "trail" bytes, and process with this macro.
 */
#define H8_MAKE_CRC(s)      (0xff - (s) + 2)
#define H8_CRC_START_OFFSET H8_DST_OFFSET
#define H8_CRC_END_OFFSET(pkt) ((pkt)->tot_len - (pkt)->trail_len - 3)

/*
 * Data - major guesswork here.
 */
/* byte 2 (always 0x00 here) is a request sequence number */
#define H8_MATCH_DATA(p,k)  (*(p)->len >= sizeof(k) && sizeof(k) >= 4 \
        && !memcmp((p)->data,k,2) && !memcmp((p)->data+3,k+3,sizeof(k)-3))

#define H8_POWER_STAT_REQ     { 0xf3, 0x50, 0x00, 0x00, 0x02, 0xf4, 0x0c }
#define H8_POWER_STAT_RSP_OFF { 0xf3, 0x50, 0x00, 0x00, 0x03, 0xf4, 0x00, 0x0c }
#define H8_POWER_STAT_RSP_ON  { 0xf3, 0x50, 0x00, 0x00, 0x03, 0xf4, 0x01, 0x0b }
#define H8_POWER_ON_REQ       { 0xf3, 0x50, 0x00, 0x00, 0x03, 0xf0, 0x01, 0x0f }
#define H8_POWER_OFF_REQ      { 0xf3, 0x50, 0x00, 0x00, 0x03, 0xf0, 0x00, 0x10 }
#define H8_POWER_TRAILER      { 0xfc }

#define H8_HEARTBEAT          { 0xfa, 0x02, 0, 0}

/* sp name cache stuff */
#define H8_SPNAME_OFFSET    0x35 /* offset in heartbeat data */
#define H8_MAX_DATALEN      (H8_MAX_PKTLEN - H8_DATA_OFFSET - 2)
#define H8_MAX_SPNAME_LEN   32
#define H8_HEARTBEAT_PERIOD 32  /* heartbeat every H8_HEARTBEAT_PERIOD sec */
#define H8_SPNAME_CACHE_TTL 8*H8_HEARTBEAT_PERIOD /* cached heartbeat TTL (sec) */

/* Initialize module, passing in RS-485 input and output cbufs.
 */
void    h8_init(cbuf_t cb_in, cbuf_t cb_out);
void    h8_fini(void);

/* Create/destroy/copy an RS-485 packet.  Exit on malloc error.
 */
h8pkt_t *h8_create_packet(void);
void    h8_destroy_packet(h8pkt_t *pkt);
h8pkt_t *h8_copy_packet(h8pkt_t *pkt);

/* Interpret an incoming packet: decode escapes, assign variable header 
 * pointers into pkt->raw, verify checksum.  Return 1 on success, 0 on failure.
 */
int     h8_decode_packet(h8pkt_t *pkt);

/* Construct an outgoing packet: assign variable header pointers into pkt->raw,
 * assign values from parameter list into pkt->raw, calculate checksum and
 * store in packet, encode escapes.
 */ 
void h8_encode_packet(h8pkt_t *pkt, uint8_t dst, uint8_t src, uint8_t *data, 
        uint8_t data_len, uint8_t *trail, uint8_t trail_len);

/* Construct outgoing power on/off/stat packets directed at address 'dst'.
 */
void    h8_encode_power_stat_req(h8pkt_t *pkt, uint8_t dst);
void    h8_encode_power_off_req(h8pkt_t *pkt, uint8_t dst);
void    h8_encode_power_on_req(h8pkt_t *pkt, uint8_t dst);

/* Accessor functions for 'master' address.
 */
void    h8_setaddr(uint8_t addr);
uint8_t h8_getaddr(void);

/* Read a raw packet from RS-485 or return NULL if none are available.
 * Result should be processed through h8_decode_packet(). 
 * It must also be freed with h8_destroy_packet().
 */
h8pkt_t *h8_read_packet(void);

/* Write a raw packet to RS-485.  Packet should have been processed through
 * h8_encode_packet().
 */
void    h8_write_packet(h8pkt_t *pkt);

#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
