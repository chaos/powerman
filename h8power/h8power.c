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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#define _GNU_SOURCE
#include <getopt.h>

#include "serial.h"
#include "hexdump.h"
#include "list.h"
#include "cbuf.h"
#include "wrappers.h"
#include "h8proto.h"
#include "h8heartbeat.h"
#include "argv.h"
#include "xtypes.h"
#include "error.h"
#include "h8power.h"

#define POWER_CMD_RETRIES   5 /* if 5s power off delay, we lose 2 retries */

#define POWER_STAT_RETRIES  10

#define TMIN(a,b)       (timercmp((a),(b),<) ? *(a) : *(b))

/* Data type for queue of pending commands.  We have the following command
 * sequences:
 * 1. status request, status reply.
 * 2. on request + delay + status request, status reply indicating on.
 * 3. off request + delay + status request, status reply indicating off.
 */
typedef struct {
    h8hb_t        *sp;              /* target H8 */
    powercmd_t      cmd;              /* on/off/status */

    struct timeval  cmd_time_sent;    /* time stamp for on/off request xmit */
    int             cmd_retries_left; /* times to retry cmd req */
    h8pkt_t         *cmd_pkt;         /* on/off packet (for retrans) */

    struct timeval  stat_time_sent;   /* time stamp for stat request transmit */
    int             stat_retries_left;/* times to retry stat req */
    h8pkt_t         *stat_pkt;        /* stat packet (for retrans) */
} pending_t;

static void _power_stat_req(h8hb_t *sp);
static void _power_stat_rsp(h8pkt_t *pkt, int on, cbuf_t ttyout);

/* NOTE: nodes can be configured with about a 4.5s power on delay, so try
 * twice with 500ms and if that fails, try remaining retries with  5s.  That
 * should work well with no delay and the 4.5s delay, but be sure to configure
 * ample retries since in the 4.5s case, two tries will immediately be used up.
 */
static const struct timeval power_off_delay1 = { tv_sec: 0L, tv_usec: 500000L };
static const struct timeval power_off_delay2 = { tv_sec: 5L, tv_usec: 0L };
static const struct timeval power_on_delay =   { tv_sec: 0L, tv_usec: 500000L };
static const struct timeval power_stat_tmout = { tv_sec: 0L, tv_usec: 300000L };


static const uint8_t heartbeat[] =            H8_HEARTBEAT;
static const uint8_t power_stat_rsp_off[] =   H8_POWER_STAT_RSP_OFF;
static const uint8_t power_stat_rsp_on[] =    H8_POWER_STAT_RSP_ON;

static List pending;                /* queue of pending_t stat requests */


/* Create a 'pending' queue entry.
 */
static pending_t *_create_pending(h8hb_t *sp, powercmd_t cmd)
{
    pending_t *p = (pending_t *)Malloc(sizeof(pending_t));

    p->sp = sp;
    p->cmd = cmd;

    p->stat_retries_left = POWER_STAT_RETRIES;
    p->stat_pkt = h8_create_packet();
    timerclear(&p->stat_time_sent);

    if (p->cmd == POWER_ON || p->cmd == POWER_OFF) {
        p->cmd_retries_left = POWER_CMD_RETRIES;
        p->cmd_pkt = h8_create_packet();
        timerclear(&p->cmd_time_sent);
    }
    return p;
}

/* Destroy a 'pending' queue entry.
 */
static void _destroy_pending(pending_t *p)
{
    h8_destroy_packet(p->stat_pkt);
    if (p->cmd == POWER_ON || p->cmd == POWER_OFF) {
        h8_destroy_packet(p->cmd_pkt);
    }
    Free(p);
}

int h8power_process_packet(h8pkt_t *pkt, cbuf_t ttyout)
{
    int consumed = 0;

    if (H8_MATCH_DATA(pkt, power_stat_rsp_off)) {
        _power_stat_rsp(pkt, 0, ttyout);
        consumed = 1;
    } else if (H8_MATCH_DATA(pkt, power_stat_rsp_on)) {
        _power_stat_rsp(pkt, 1, ttyout);
        consumed = 1;
    }
    return consumed;
}

/* Send a power on command to the named node.
 */
static void _power_on_req(h8hb_t *sp)
{
    pending_t *p = _create_pending(sp, POWER_ON);

    /* send power on request immediately (no response expected) */
    h8_encode_power_on_req(p->cmd_pkt, p->sp->addr);
    h8_write_packet(p->cmd_pkt);
    Gettimeofday(&p->cmd_time_sent, NULL); /* time stamp sent request */

    /* prepare stat request */
    h8_encode_power_stat_req(p->stat_pkt, p->sp->addr);

    list_enqueue(pending, p);       /* add to tail */
}

/* Send a power off command to the named node.
 */
static void _power_off_req(h8hb_t *sp)
{
    pending_t *p = _create_pending(sp, POWER_OFF);

    /* send power off request immediately (no response expected) */
    h8_encode_power_off_req(p->cmd_pkt, p->sp->addr);
    h8_write_packet(p->cmd_pkt);
    Gettimeofday(&p->cmd_time_sent, NULL); /* time stamp sent request */

    /* prepare stat request */
    h8_encode_power_stat_req(p->stat_pkt, p->sp->addr);

    list_enqueue(pending, p);       /* add to tail */
}

/* Send a power status request to the named node.
 */
static void _power_stat_req(h8hb_t *sp)
{
    pending_t *p = _create_pending(sp, POWER_STAT);

    /* prepare stat request */
    h8_encode_power_stat_req(p->stat_pkt, p->sp->addr);

    list_enqueue(pending, p);       /* add to tail */
}

/* Process a power stat response packet.
 * The request at the head of the pending queue is the active one.
 */
static void _power_stat_rsp(h8pkt_t *pkt, int on, cbuf_t ttyout)
{
    pending_t *p = list_peek(pending);
    int success = 0;

    if (p && *pkt->src == p->sp->addr && *pkt->dst == h8_getaddr()) {
        switch (p->cmd) {
            case POWER_STAT:
                success = 1;
                break;
            case POWER_ON:
                if (on)
                    success = 1;
                break;
            case POWER_OFF:
                if (!on)
                    success = 1;
                break;
        }

        if (success) {
            cbuf_printf(ttyout, "%s: %s\n", p->sp->name, on ? "on" : "off");
            _destroy_pending(list_dequeue(pending));

        /* If we get here, we have a power on or off command whose stat
         * has returned the wrong answer.  We retry the command if we have
         * any retries left.
         */
        } else {
            if (p->cmd_retries_left-- > 0) {    /* (re-)transmit */
                if (p->cmd == POWER_ON)
                    h8_encode_power_on_req(p->cmd_pkt, p->sp->addr);
                else /* p->cmd == POWER_OFF */
                    h8_encode_power_off_req(p->cmd_pkt, p->sp->addr);
                h8_write_packet(p->cmd_pkt);
                Gettimeofday(&p->cmd_time_sent, NULL);

                h8_encode_power_stat_req(p->stat_pkt, p->sp->addr);
                timerclear(&p->stat_time_sent);
                p->stat_retries_left = POWER_STAT_RETRIES;
            } else {                            /* several tries failed */
                cbuf_printf(ttyout, "%s: failed\n", p->sp->name);
                err("%s: power command failed", p->sp->name);
                _destroy_pending(list_dequeue(pending));
            }
        }
    }
}


/* Helper for _process_pending:
 * If event occuring at time 'stamp' has exceeded time limit 'lim',
 * return true.  If limit has not been exceeded, set tmout to the time
 * remaining if it is less than its current value and return false.
 */
static int _timeout(struct timeval *stamp, const struct timeval *lim, 
                    struct timeval *tmout)
{
    struct timeval now, elapsed, left;
    int expired = 1;

    Gettimeofday(&now, NULL);
    timersub(&now, stamp, &elapsed);    /* elapsed = now - stamp */
    if (timercmp(&elapsed, lim, <)) {   /* if not expired... */
        timersub(lim, &elapsed, &left); /*     left = lim - elapsed */
        *tmout = timerisset(tmout) ? TMIN(tmout, &left) : left;
        expired = 0;
    }

    return expired;
}

/* Process the queue of pending requests, setting tmout for subsequent
 * use by select.  Only the head of the queue is active.  If the head
 * request times out here, we remove it from the queue and recurse 
 * on the next request.
 */
void h8power_process_pending(struct timeval *tmout, cbuf_t ttyout)
{
    pending_t *p = list_peek(pending);

    if (p == NULL)
        return;

    timerclear(tmout);

    /* nothing queued */
    switch (p->cmd) {
        /* Power on command - if power on delay has timed out, send the
         * stat request; otherwise fix up the timeout for select and return.
         */
        case POWER_ON:
            assert(timerisset(&p->cmd_time_sent));
            if (!_timeout(&p->cmd_time_sent, &power_on_delay, tmout))
                return;
            break;
        /* Power off command - if power off delay has timed out, send the
         * stat request; otherwise fix up the timeout for select and return.
         */
        case POWER_OFF:
            assert(timerisset(&p->cmd_time_sent));
            /* try delay1 two times first, then try delay2 */
            if (p->cmd_retries_left >= POWER_CMD_RETRIES - 1) {
                if (!_timeout(&p->cmd_time_sent, &power_off_delay1, tmout))
                    return;
            } else {
                if (!_timeout(&p->cmd_time_sent, &power_off_delay2, tmout))
                    return;
            }
            break;
        /* Power stat command - if request not sent, send it.
         */
        case POWER_STAT:
            break;
    } 

    /* send stat request if not sent */
    if (!timerisset(&p->stat_time_sent)) {
        h8_write_packet(p->stat_pkt);
        Gettimeofday(&p->stat_time_sent, NULL);
    }

    /* check for timeout on stat req */
    if (_timeout(&p->stat_time_sent, &power_stat_tmout, tmout)) {
        if (p->stat_retries_left-- > 0) {   /* retry */
            h8_encode_power_stat_req(p->stat_pkt, p->sp->addr); /* seq++ */
            err("%s: status query retry (%d left)", 
                    p->sp->name, p->stat_retries_left);
            h8_write_packet(p->stat_pkt);
            Gettimeofday(&p->stat_time_sent, NULL);
        } else {                            /* timeout */
            cbuf_printf(ttyout, "%s: timed out\n", p->sp->name);
            err("%s: status query timed out", p->sp->name);
            _destroy_pending(list_dequeue(pending));
                                            /* we popped the queue...recurse */
            h8power_process_pending(tmout, ttyout); 
        }
    }
}

/* Issue the specified power on/off/stat command.
 */
void h8power_cmd(h8hb_t *sp, powercmd_t cmd)
{
    switch (cmd) {
        case POWER_ON:
            _power_on_req(sp);
            break;
        case POWER_OFF:
            _power_off_req(sp);
            break;
        case POWER_STAT:
            _power_stat_req(sp);
            break;
    }
}

void h8power_init(void)
{
    pending = list_create((ListDelF)_destroy_pending);
}

void h8power_fini(void)
{
    list_destroy(pending);
}

int h8power_pending(void)
{
    return !list_is_empty(pending);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
