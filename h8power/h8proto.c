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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "list.h"
#include "cbuf.h"
#include "hexdump.h"
#include "wrappers.h"
#include "xtypes.h"
#include "error.h"
#include "h8proto.h"
#include "h8heartbeat.h"
#include "h8sniff.h"

#ifndef MIN
#define MIN(a,b)            ((a) < (b) ? (a) : (b))
#endif

static cbuf_t rs485in = NULL;
static cbuf_t rs485out = NULL;

static const uint8_t power_stat_req[] =       H8_POWER_STAT_REQ;
static const uint8_t power_stat_rsp_off[] =   H8_POWER_STAT_RSP_OFF;
static const uint8_t power_stat_rsp_on[] =    H8_POWER_STAT_RSP_ON;
static const uint8_t power_off_req[] =        H8_POWER_OFF_REQ;
static const uint8_t power_on_req[] =         H8_POWER_ON_REQ;
static const uint8_t heartbeat[] =            H8_HEARTBEAT;

static uint8_t myaddr = H8_MASTER_ADDR;

/* Calculate the checksum for the packet and return it.
 */
static uint8_t _calculate_crc(h8pkt_t *pkt)
{
    uint8_t sum = 0;
    int i;

    for (i = H8_CRC_START_OFFSET; i <= H8_CRC_END_OFFSET(pkt); i++)
        sum += pkt->raw[i];
    return H8_MAKE_CRC(sum);
}


/* Convert escaped bytes (2 byte sequence) to their initial (1 byte) values.
 */
static void _decode_escapes(h8pkt_t *pkt)
{
    int i;

    assert(pkt->magic == H8PKT_MAGIC);

    /* work backwards so we don't trip over decoded escapes that 
     * happen to be equal to H8_ESCAPE_BYTE.
     */
    for (i = H8_ESCAPE_END_OFFSET(pkt) - 1; i >= H8_ESCAPE_START_OFFSET; i--) { 
        if (pkt->raw[i] == H8_ESCAPE_BYTE) {
            pkt->raw[i] = H8_ESCAPE(pkt->raw[i + 1]);
            memmove(&pkt->raw[i + 1], &pkt->raw[i + 2], pkt->tot_len - i - 2);
            pkt->tot_len--;
        } 
    }
    assert(pkt->tot_len >= H8_DATA_OFFSET + 2);
}

/* Convert escapable bytes (1 byte) to escaped representation (2 byte sequence)
 */
static void _encode_escapes(h8pkt_t *pkt)
{
    int i;

    assert(pkt->magic == H8PKT_MAGIC);

    for (i = H8_ESCAPE_START_OFFSET; i <= H8_ESCAPE_END_OFFSET(pkt); i++) {
        if (H8_MUST_ESCAPE(pkt->raw[i])) {
            assert(pkt->tot_len + 1 <= H8_MAX_PKTLEN);
            memmove(&pkt->raw[i + 1], &pkt->raw[i], pkt->tot_len - i);
            pkt->raw[i++] = H8_ESCAPE_BYTE;
            pkt->raw[i] = H8_ESCAPE(pkt->raw[i]);
            pkt->tot_len++;
        }
    }
    assert(pkt->tot_len < H8_MAX_PKTLEN);
}

int h8_decode_packet(h8pkt_t *pkt)
{
    assert(pkt->magic == H8PKT_MAGIC);

    if (pkt->tot_len < H8_MIN_PKTLEN) {
        dbg("h8_decode_packet: ignoring runt");
        return 0;
    }
    if (pkt->tot_len > H8_MAX_PKTLEN) {
        dbg("h8_decode_packet: ignoring giant");
        return 0;
    }

    _decode_escapes(pkt);

    if (*pkt->len + H8_DATA_OFFSET > (pkt->tot_len - 2)) {
        dbg("h8_decode_packet: packet length is wrong");
        return 0;
    }
    pkt->trail = &pkt->raw[H8_DATA_OFFSET + *pkt->len];
    pkt->trail_len = pkt->tot_len - H8_DATA_OFFSET - *pkt->len - 2;
    pkt->crc = &pkt->raw[pkt->tot_len - 1 + H8_CRC_OFFSET];
    pkt->end_sentinel = &pkt->raw[pkt->tot_len - 1];

    if (memcmp(pkt->start_sentinel, H8_START_SENTINEL, H8_DST_OFFSET) != 0) {
        dbg("h8_decode_packet: start sentinel not intact");
        return 0;
    }
    if (*pkt->end_sentinel != H8_END_SENTINEL) {
        dbg("h8_decode_packet: end sentinel not intact");
        return 0;
    }
    if (_calculate_crc(pkt) != *pkt->crc) {
        dbg("h8_decode_packet: CRC error");
        return 0;
    }

    return 1;
}

void h8_encode_packet(h8pkt_t *pkt, uint8_t dst, uint8_t src, 
        uint8_t *data, uint8_t data_len, uint8_t *trail, uint8_t trail_len)
{
    assert(pkt->magic == H8PKT_MAGIC);

    pkt->tot_len = H8_DATA_OFFSET + data_len + trail_len + 2;
    pkt->trail_len = trail_len;
    assert(pkt->tot_len <= H8_MAX_PKTLEN);

    pkt->trail = &pkt->raw[H8_DATA_OFFSET + data_len];
    pkt->crc = &pkt->raw[pkt->tot_len - 1 + H8_CRC_OFFSET];
    pkt->end_sentinel = &pkt->raw[pkt->tot_len - 1];

    memcpy(pkt->start_sentinel, H8_START_SENTINEL, H8_DST_OFFSET);
    *pkt->dst = dst;
    *pkt->src = src;
    *pkt->len = data_len;
    memcpy(pkt->data, data, data_len);
    memcpy(pkt->trail, trail, trail_len);
    *pkt->end_sentinel = H8_END_SENTINEL;

    *pkt->crc = _calculate_crc(pkt);
    _encode_escapes(pkt);
}

h8pkt_t *h8_create_packet(void)
{
    h8pkt_t *pkt = (h8pkt_t *)Malloc(sizeof(h8pkt_t));
    memset(pkt , 0L, sizeof(h8pkt_t));
    pkt->magic = H8PKT_MAGIC;

    /* set fixed offset pointers */
    pkt->start_sentinel = &pkt->raw[0];
    pkt->dst = &pkt->raw[H8_DST_OFFSET];
    pkt->src = &pkt->raw[H8_SRC_OFFSET];
    pkt->len = &pkt->raw[H8_LEN_OFFSET];
    pkt->data = &pkt->raw[H8_DATA_OFFSET];

    return pkt;
}

void h8_destroy_packet(h8pkt_t *pkt)
{
    assert(pkt->magic == H8PKT_MAGIC);
    pkt->magic = 0;
    Free(pkt);
}

h8pkt_t *h8_copy_packet(h8pkt_t *pkt)
{
    h8pkt_t *cpy = h8_create_packet();

    assert(pkt->magic == H8PKT_MAGIC);
    memcpy(cpy->raw, pkt->raw, H8_MAX_PKTLEN);
    cpy->tot_len = pkt->tot_len;
    cpy->trail_len = pkt->trail_len;
    cpy->trail = &cpy->raw[cpy->tot_len - 1 + H8_CRC_OFFSET - cpy->trail_len];
    cpy->crc = &cpy->raw[cpy->tot_len - 1 + H8_CRC_OFFSET];
    cpy->end_sentinel = &cpy->raw[cpy->tot_len - 1];
    return cpy;
}

/* Return the next value in a 0...0x1f cycling sequence.
 */
static uint8_t _next_req_seq(void)
{
    static uint8_t seq = 0;

    /* XXX using the full 8-bit range, requests stop getting answered
     * somewhere up in the 0x20's.
     */
    return seq++ % 0x20;
}

void h8_encode_power_stat_req(h8pkt_t *pkt, uint8_t dst)
{
    uint8_t d[] = H8_POWER_STAT_REQ;
    uint8_t t[] = H8_POWER_TRAILER;

    assert(pkt->magic == H8PKT_MAGIC);
    d[2] = _next_req_seq();
    h8_encode_packet(pkt, dst, myaddr, d, sizeof(d), t, sizeof(t));
}

void h8_encode_power_on_req(h8pkt_t *pkt, uint8_t dst)
{
    uint8_t d[] = H8_POWER_ON_REQ;
    uint8_t t[] = H8_POWER_TRAILER;

    assert(pkt->magic == H8PKT_MAGIC);
    d[2] = _next_req_seq();
    h8_encode_packet(pkt, dst, myaddr, d, sizeof(d), t, sizeof(t));
}

void h8_encode_power_off_req(h8pkt_t *pkt, uint8_t dst)
{
    uint8_t d[] = H8_POWER_OFF_REQ;
    uint8_t t[] = H8_POWER_TRAILER;

    assert(pkt->magic == H8PKT_MAGIC);
    d[2] = _next_req_seq();
    h8_encode_packet(pkt, dst, myaddr, d, sizeof(d), t, sizeof(t));
}

void h8_setaddr(uint8_t addr)
{
    myaddr = addr;
}

uint8_t h8_getaddr(void)
{
    return myaddr;
}

/* Return offset of first start sentinel in buffer, 
 * or return -1 if not found.
 * (helper for h8_read_packet)
 */
static int _find_start_sentinel(uint8_t *buf, int len)
{
    int n = strlen(H8_START_SENTINEL);
    int i;

    for (i = 0; i <= len - n; i++) {
        if (memcmp(&buf[i], H8_START_SENTINEL, n) == 0)
            return i;
    }
    return -1;
}

/* Return offset of the first end sentinel in buffer, 
 * or return -1 if not found.
 * (helper for h8_read_packet)
 */
static int _find_end_sentinel(uint8_t *buf, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        if (buf[i] == H8_END_SENTINEL)
            return i;
    }
    return -1;
}

/* Drop end chars from input cbuf. 
 * (helper for h8_read_packet)
 */
static void _drop_inbuf(int n)
{
    int dropped = cbuf_drop(rs485in, n);

    if (dropped != n)
        dbg("h8_read_packet: cbuf_drop returned %d (!= %d)", dropped, n);
}

h8pkt_t *h8_read_packet(void)
{
    h8pkt_t *pkt = h8_create_packet();
    int bytes_peeked;
    int start_offset, end_offset;

    assert(rs485in != NULL);

    /* Read raw data into pkt->raw.
     * Drop data preceeding start sentinel.
     */
    do {
        bytes_peeked = cbuf_peek(rs485in, pkt->raw, H8_MAX_PKTLEN);
        if (bytes_peeked < 0)
            err_exit("h8_read_packet: cbuf_peek error");

        start_offset = _find_start_sentinel(pkt->raw, bytes_peeked);
        /* If no start sentinel, prepare to drop all data in the packet
         * except room for partial start sentinel at the end.
         */
        if (start_offset == -1)
            start_offset = bytes_peeked - strlen(H8_START_SENTINEL) - 1;
        if (start_offset < 0)
            goto eof;

        if (start_offset == 0) {
            end_offset = _find_end_sentinel(pkt->raw, bytes_peeked);
            if (end_offset == -1 && bytes_peeked >= H8_MAX_PKTLEN) {
                start_offset = _find_start_sentinel(pkt->raw+1, bytes_peeked-1);
                if (start_offset == -1)
                    start_offset = bytes_peeked - strlen(H8_START_SENTINEL) - 1;
                else
                    start_offset++;
                assert(start_offset >= 0);
            }
        }

        /* Drop unframed data and try again on the next iteration. */
        if (start_offset > 0) {
            _drop_inbuf(start_offset);
            dbg("h8_read_packet: dropped %d unframed bytes", start_offset);
        }
    } while (start_offset > 0);
    assert(start_offset == 0);

    if (end_offset == -1)
        pkt->tot_len = bytes_peeked;
    else
        pkt->tot_len = end_offset + 1;

    /* start but no end - unfinished reading packet, return EOF */
    if (end_offset == -1)
        goto eof;

    /* a complete packet has been parsed - drop it from the cbuf */
    _drop_inbuf(pkt->tot_len);

    h8sniff_pkt(pkt); /* sniffer hook */

    return pkt;
eof:
    /* XXX big hammer to avoid deadlock */
    if (cbuf_used(rs485in) >= H8_MAX_PKTLEN)  {
        cbuf_flush(rs485in);
        dbg("h8_read_packet: flushed input buffer (this should never happen)");
    }
    h8_destroy_packet(pkt);
    return NULL;
}

void h8_write_packet(h8pkt_t *pkt)
{
    int written, dropped;

    assert(rs485out != NULL);
    written = cbuf_write(rs485out, pkt->raw, pkt->tot_len, &dropped);
    if (written < 0)
        err_exit("h8_write_packet: cbuf_write: %m");
    if (dropped > 0)
        dbg("h8_write_packet: cbuf_write dropped %d chars", dropped);

    /* sleep a little while heartbeats expected */
    h8hb_sleep_through_heartbeats(); 

    h8sniff_pkt(pkt); /* sniffer hook */
}

void h8_init(cbuf_t cb_in, cbuf_t cb_out)
{
    rs485in = cb_in;
    rs485out = cb_out;
}

void h8_fini(void)
{
    rs485in = NULL;
    rs485out = NULL;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
