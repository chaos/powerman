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
#include "h8proto.h"
#include "xtypes.h"
#include "error.h"
#include "h8heartbeat.h"
#include "h8sniff.h"

static const uint8_t power_stat_req[] =       H8_POWER_STAT_REQ;
static const uint8_t power_stat_rsp_off[] =   H8_POWER_STAT_RSP_OFF;
static const uint8_t power_stat_rsp_on[] =    H8_POWER_STAT_RSP_ON;
static const uint8_t power_off_req[] =        H8_POWER_OFF_REQ;
static const uint8_t power_on_req[] =         H8_POWER_ON_REQ;
static const uint8_t heartbeat[] =            H8_HEARTBEAT;

static long sniff_level = 0L;
static cbuf_t sniff_cbuf = NULL;

/* configuration block for Dave's hex dumper */
static const hexdump_format_t hdfmt = {
    bytes_per_line:     16,
    addrprint_width:    1,
    indent:             " ",
    sep1:               ": ",
    sep2:               " ",
    sep3:               " ",
    nonprintable:       '.',
    is_printable_fn:    NULL,
};

/* helper for _dump_packet:
 * Return a pointer to null-terminated SP name in heartbeat packet,
 * or NULL if it is not null-terminated.
 */
static char *_heartbeat_spname(h8pkt_t *pkt)
{
    int i;

    assert(pkt->magic == H8PKT_MAGIC);
    for (i = H8_SPNAME_OFFSET; i < *pkt->len; i++) {
        if (pkt->data[i] == '\0')
            break;
    }
    return (i >= *pkt->len ? NULL : &pkt->data[H8_SPNAME_OFFSET]);
}

/* helper for _dump_packet */
static char *_h8addrfmt(uint8_t addr)
{
    static char buf[H8_MAX_SPNAME_LEN];

    if (addr == H8_BCAST_ADDR)
        strcpy(buf, "*");
    else if (addr == h8_getaddr()) {
        strcpy(buf, "me");
    } else {
        h8hb_t *sp = h8hb_lookup_byaddr(addr);

        if (sp != NULL)
            sprintf(buf, "[%s]", sp->name);
        else
            sprintf(buf, "%-2.2x", (int)addr);
    }
    return Strdup(buf);
}

/* helper for _dump_packet */
static char *_timestr(void)
{
    static struct timeval start_tv = { 0L, 0L };
    struct timeval now_tv, tv;
    static char buf[32];

    if (start_tv.tv_sec == 0L && start_tv.tv_usec == 0L)
        Gettimeofday(&start_tv, NULL);
    Gettimeofday(&now_tv, NULL);
    timersub(&now_tv, &start_tv, &tv);
    snprintf(buf, sizeof(buf), "%-4.4ld.%2.2ld", tv.tv_sec, tv.tv_usec/10000L);
    buf[sizeof(buf) - 1] = '\0'; /* be sure */
    return buf;
}

static void _dump_pkt(h8pkt_t *pkt)
{
    char *desc = NULL, *spname = NULL;
    char *src_name = _h8addrfmt(*pkt->src);
    char *dst_name = _h8addrfmt(*pkt->dst);

    assert(sniff_cbuf != NULL);

    if (H8_MATCH_DATA(pkt, heartbeat)) {
        spname = _heartbeat_spname(pkt);
        desc = "heartbeat";
    } else if (H8_MATCH_DATA(pkt, power_stat_req)) {
        desc = "power stat request";
    } else if (H8_MATCH_DATA(pkt, power_stat_rsp_off)) {
        desc = "power stat response - off";
    } else if (H8_MATCH_DATA(pkt, power_stat_rsp_on)) {
        desc = "power stat response - on";
    } else if (H8_MATCH_DATA(pkt, power_off_req)) {
        desc = "power off request";
    } else if (H8_MATCH_DATA(pkt, power_on_req)) {
        desc = "power on request";
    } 

    if (sniff_level >= SNIFF_HEADER) {
        cbuf_printf(sniff_cbuf, "%s: %s->%s len %-2.2x %s %s\n", 
                _timestr(), src_name, dst_name, (int)*pkt->len, 
                desc ? desc : "", 
                spname ? spname : "");
    }
    if (sniff_level >= SNIFF_ALL)   /* hex dump of header */
        hexdump (pkt->dst, H8_DATA_OFFSET - H8_DST_OFFSET, 0LL, 
                sniff_cbuf, &hdfmt);
    if (sniff_level >= SNIFF_DATA)  /* hex dump of data */
        hexdump (pkt->data, *pkt->len, 0LL, sniff_cbuf, &hdfmt);
    if (sniff_level >= SNIFF_ALL)   /* hex dump of trailer */
        hexdump (pkt->trail, pkt->trail_len, 0LL, sniff_cbuf, &hdfmt);

    Free(src_name);
    Free(dst_name);
}

void h8sniff_pkt(h8pkt_t *pkt)
{
    if (sniff_level > SNIFF_NONE) {
        h8pkt_t *cpy = h8_copy_packet(pkt);

        if (h8_decode_packet(cpy))
            _dump_pkt(cpy);
        else
            cbuf_printf(sniff_cbuf, "h8sniff_pkt: ignoring mangled packet\n");
        h8_destroy_packet(cpy);
    }
}

void h8sniff_init(cbuf_t cb, sniff_level_t lev)
{
    sniff_level = lev;
    sniff_cbuf = cb;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
